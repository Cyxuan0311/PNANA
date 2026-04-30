#include "core/document.h"
#include "core/buffer_factory.h"
#include "utils/logger.h"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

#ifdef __linux__
#include <fcntl.h>
#include <sys/mman.h>
#endif

namespace pnana {
namespace core {

Document::Document()
    : buffer_backend_(nullptr), backend_type_(BufferBackendType::PIECE_TABLE), filepath_(""),
      encoding_("UTF-8"), line_ending_(LineEnding::LF), modified_(false), read_only_(false),
      is_binary_(false) {
    // 默认使用 PieceTable 后端
    buffer_backend_ = std::make_unique<PieceTable>();
    lines_.push_back("");
    original_lines_.push_back("");
}

Document::Document(const std::string& filepath) : Document() {
    load(filepath);
}

const char* Document::getBufferBackendName() const {
    return SmartBufferFactory::getBackendName(backend_type_);
}

void Document::setBufferBackend(BufferBackendType type) {
    if (backend_type_ == type) {
        return; // 已经是该类型
    }

    backend_type_ = type;

    // 创建新的缓冲区后端
    auto new_backend = SmartBufferFactory::create(type);

    // 复制当前内容到新后端
    std::string content = getContent();
    new_backend->insert(0, content);

    // 替换后端
    buffer_backend_ = std::move(new_backend);

    // 更新缓存
    lines_.clear();
    for (size_t i = 0; i < buffer_backend_->lineCount(); ++i) {
        lines_.push_back(buffer_backend_->getLine(i));
    }
}

void Document::autoSelectBufferBackend() {
    // 根据文件大小和类型自动选择后端
    size_t file_size = 0;
    try {
        if (std::filesystem::exists(filepath_)) {
            file_size = std::filesystem::file_size(filepath_);
        }
    } catch (...) {
        // 如果文件不存在或无法访问，使用默认后端
    }

    BufferBackendType selected_type = SmartBufferFactory::selectBackend(filepath_, file_size);

    setBufferBackend(selected_type);
}

bool Document::load(const std::string& filepath) {
    // 检查路径是否是目录
    try {
        if (std::filesystem::exists(filepath) && std::filesystem::is_directory(filepath)) {
            last_error_ = "Cannot open directory as file: " + filepath;
            LOG_ERROR("[debug] Document::load FAILED is_directory");
            return false;
        }
    } catch (...) {
        // 如果检查失败，继续尝试打开（可能是新文件）
    }

    // 获取文件大小用于智能选择缓冲区后端
    size_t file_size = 0;
    try {
        if (std::filesystem::exists(filepath)) {
            file_size = std::filesystem::file_size(filepath);
        }
    } catch (...) {
        // 忽略错误
    }

    // 根据文件类型和大小自动选择缓冲区后端
    BufferBackendType selected_type = SmartBufferFactory::selectBackend(filepath, file_size);

    if (selected_type != backend_type_) {
        setBufferBackend(selected_type);
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        filepath_ = filepath;
        lines_.clear();
        lines_.push_back("");
        modified_ = false;
        lazy_loaded_ = false;
        line_offsets_.clear();
        line_cache_.clear();
        line_cache_lru_.clear();
        return true;
    }

    lines_.clear();
    is_binary_ = false;
    lazy_loaded_ = false;
    line_offsets_.clear();
    line_cache_.clear();
    line_cache_lru_.clear();

    // 只读前 8KB 做二进制检测与行尾检测，避免整文件读入内存（流式优化）
    const size_t check_size = 8192;
    std::string prefix(check_size, '\0');
    file.read(&prefix[0], check_size);
    prefix.resize(static_cast<size_t>(file.gcount()));

    if (!prefix.empty()) {
        size_t null_count = 0;
        size_t limit = std::min(prefix.size(), check_size);
        for (size_t i = 0; i < limit; ++i) {
            if (prefix[i] == '\0')
                null_count++;
        }
        if (null_count > limit / 100) {
            is_binary_ = true;
        } else {
            size_t non_printable = 0;
            for (size_t i = 0; i < limit; ++i) {
                unsigned char ch = static_cast<unsigned char>(prefix[i]);
                if (ch < 32 && ch != '\n' && ch != '\r' && ch != '\t')
                    non_printable++;
            }
            if (non_printable > limit / 20) {
                is_binary_ = true;
            }
        }
    }

    if (is_binary_) {
        lines_.push_back("");
        filepath_ = filepath;
        modified_ = false;
        return true;
    }

    detectLineEnding(prefix);

    file.clear();
    file.seekg(0);
    file.seekg(0, std::ios::end);
    std::streamsize stream_file_size = file.tellg();
    file.seekg(0);

    // 第一遍：只构建行偏移表（不存行内容），用于判断是否走懒加载
    const size_t lazy_line_threshold = 100000;
    const size_t parallel_threshold = 10 * 1024 * 1024; // 10MB 以上使用并行扫描
    line_offsets_.clear();
    line_offsets_.push_back(0);

    bool early_exit = false;
    size_t num_lines = 0;

#ifdef __linux__
    if (file_size > parallel_threshold && file_size < 4ULL * 1024 * 1024 * 1024) {
        // 大文件：使用 mmap + 并行扫描
        int fd = open(filepath.c_str(), O_RDONLY);
        if (fd >= 0) {
            void* mapped = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
            if (mapped != MAP_FAILED) {
                madvise(mapped, file_size, MADV_SEQUENTIAL);

                unsigned int hw_threads = std::thread::hardware_concurrency();
                unsigned int num_threads = std::min(hw_threads, 8u);
                if (num_threads == 0)
                    num_threads = 4;

                size_t chunk_size = file_size / num_threads;
                if (chunk_size == 0)
                    chunk_size = file_size;

                struct ChunkResult {
                    std::vector<uint64_t> offsets;
                    size_t newlines_in_chunk;
                    bool found_threshold;
                };

                std::vector<ChunkResult> results(num_threads);
                std::atomic<bool> threshold_reached(false);

                auto scan_chunk = [&](unsigned int thread_id, size_t start, size_t end) {
                    const char* data = static_cast<const char*>(mapped);
                    std::vector<uint64_t> local_offsets;
                    local_offsets.reserve(end - start > 0 ? (end - start) / 80 : 1024);
                    size_t newline_count = 0;

                    for (size_t i = start;
                         i < end && !threshold_reached.load(std::memory_order_relaxed); ++i) {
                        if (data[i] == '\n') {
                            newline_count++;
                            local_offsets.push_back(static_cast<uint64_t>(i + 1));
                            if (newline_count > lazy_line_threshold) {
                                threshold_reached.store(true, std::memory_order_release);
                                break;
                            }
                        }
                    }

                    results[thread_id] = {std::move(local_offsets), newline_count,
                                          threshold_reached.load()};
                };

                std::vector<std::thread> threads;
                for (unsigned int i = 0; i < num_threads; ++i) {
                    size_t chunk_start = i * chunk_size;
                    size_t chunk_end = (i == num_threads - 1) ? file_size : (i + 1) * chunk_size;

                    if (chunk_start >= file_size)
                        break;

                    threads.emplace_back(scan_chunk, i, chunk_start, chunk_end);
                }

                for (auto& t : threads) {
                    if (t.joinable())
                        t.join();
                }

                // 合并结果
                size_t global_line_count = 0;
                bool stop_merging = false;
                for (unsigned int i = 0; i < num_threads && !stop_merging; ++i) {
                    if (i >= results.size())
                        break;

                    for (uint64_t offset : results[i].offsets) {
                        line_offsets_.push_back(static_cast<uint64_t>(i * chunk_size) + offset);
                        global_line_count++;
                        if (global_line_count > lazy_line_threshold) {
                            early_exit = true;
                            stop_merging = true;
                            break;
                        }
                    }
                }

                num_lines = global_line_count;
                munmap(mapped, file_size);
            }
            close(fd);
        }
    }
#endif

    if (num_lines == 0 && line_offsets_.size() <= 1) {
        // 回退到串行扫描（小文件或 mmap 失败）
        const size_t chunk_size = 512 * 1024;
        std::string buffer(chunk_size, '\0');
        uint64_t offset = 0;
        while (true) {
            file.read(&buffer[0], chunk_size);
            const size_t n = static_cast<size_t>(file.gcount());
            if (n == 0)
                break;
            for (size_t i = 0; i < n; ++i) {
                if (buffer[i] == '\n') {
                    line_offsets_.push_back(offset + i + 1);
                }
            }
            offset += n;
            if (line_offsets_.size() - 1 > lazy_line_threshold) {
                early_exit = true;
                break;
            }
        }
        num_lines = line_offsets_.size() - 1;
    }

    if (!early_exit && stream_file_size > 0 &&
        line_offsets_.back() != static_cast<uint64_t>(stream_file_size)) {
        line_offsets_.push_back(static_cast<uint64_t>(stream_file_size));
    }
    if (num_lines == 0) {
        line_offsets_.push_back(0);
    }

    filepath_ = filepath;
    modified_ = false;

    if (num_lines > lazy_line_threshold) {
        // 懒加载：不读行内容，打开即完成
        lazy_loaded_ = true;
        lines_.clear();
        lines_.shrink_to_fit();
        original_lines_.clear();
        original_lines_.shrink_to_fit();
        large_file_skip_original_ = true;
        line_cache_.clear();
        line_cache_lru_.clear();
        return true;
    }

    // 小文件：第二遍按行加载到 lines_（复用当前逻辑）
    lazy_loaded_ = false;
    line_offsets_.clear();
    line_offsets_.shrink_to_fit();
    lines_.clear();
    std::string carry;
    carry.reserve(4096);
    file.clear();
    file.seekg(0);
    if (stream_file_size > 0) {
        size_t hint = static_cast<size_t>(stream_file_size) / 50;
        if (hint > 4000000u)
            hint = 4000000u;
        lines_.reserve(hint);
    }
    const size_t load_chunk_size = 512 * 1024;
    std::string load_buffer(load_chunk_size, '\0');
    while (true) {
        file.read(&load_buffer[0], load_chunk_size);
        const size_t n = static_cast<size_t>(file.gcount());
        if (n == 0)
            break;
        size_t start = 0;
        for (size_t i = 0; i < n; ++i) {
            if (load_buffer[i] == '\n') {
                carry.append(load_buffer.data() + start, i - start);
                if (!carry.empty() && carry.back() == '\r')
                    carry.pop_back();
                lines_.push_back(std::move(carry));
                carry.clear();
                carry.reserve(4096);
                start = i + 1;
            }
        }
        if (start < n)
            carry.append(load_buffer.data() + start, n - start);
    }
    if (!carry.empty()) {
        if (carry.back() == '\r')
            carry.pop_back();
        lines_.push_back(std::move(carry));
    }
    if (lines_.empty()) {
        lines_.push_back("");
    }

    const size_t large_file_line_threshold = 500000;
    if (lines_.size() > large_file_line_threshold) {
        large_file_skip_original_ = true;
        original_lines_.clear();
        original_lines_.shrink_to_fit();
    } else {
        large_file_skip_original_ = false;
        saveOriginalContent();
    }

    // 关键修复：加载文件时清空撤销历史
    // 防止撤销栈中包含之前编辑会话的旧操作
    clearHistory();

    // 将加载的内容同步到缓冲区后端
    if (buffer_backend_) {
        buffer_backend_->clear();
        std::string full_content = getContent();
        buffer_backend_->insert(0, full_content);
    }

    return true;
}

bool Document::save() {
    if (filepath_.empty()) {
        return false;
    }
    return saveAs(filepath_);
}

bool Document::saveAs(const std::string& filepath) {
    if (lazy_loaded_) {
        materialize();
    }
    // nano风格的安全保存：
    // 1. 获取原文件权限
    // 2. 写入临时文件
    // 3. 创建备份（可选）
    // 4. 原子性替换原文件

    struct stat file_stat;
    bool file_exists = (stat(filepath.c_str(), &file_stat) == 0);
    mode_t original_mode = file_exists ? file_stat.st_mode : 0644;

    // 创建临时文件名
    std::string temp_file = filepath + ".tmp~";

    // 写入临时文件
    {
        std::ofstream file(temp_file, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            last_error_ = "Cannot create temporary file: " + std::string(strerror(errno));
            return false;
        }

        // 写入所有行
        for (size_t i = 0; i < lines_.size(); ++i) {
            file << lines_[i];

            // 添加行尾（除了最后一行如果为空）
            if (i < lines_.size() - 1 || !lines_.back().empty()) {
                file << applyLineEnding(lines_[i]);
            }
        }

        // 检查写入是否成功
        if (!file.good()) {
            file.close();
            std::remove(temp_file.c_str());
            last_error_ = "Write error: " + std::string(strerror(errno));
            return false;
        }

        file.close();

        // 确保数据已写入磁盘
        if (file.fail()) {
            std::remove(temp_file.c_str());
            last_error_ = "Failed to close temporary file";
            return false;
        }
    }

    // 如果原文件存在，创建临时备份（用于安全保存）
    std::string backup_file = filepath + "~";
    bool backup_created = false;

    if (file_exists) {
        // 删除旧备份
        std::remove(backup_file.c_str());

        // 创建新备份
        if (std::rename(filepath.c_str(), backup_file.c_str()) == 0) {
            backup_created = true;
        }
        // 备份失败不是致命错误，继续保存
    }

    // 原子性替换：重命名临时文件为目标文件
    if (std::rename(temp_file.c_str(), filepath.c_str()) != 0) {
        std::remove(temp_file.c_str());
        // 如果备份存在，尝试恢复
        if (backup_created) {
            std::rename(backup_file.c_str(), filepath.c_str());
        }
        last_error_ = "Cannot rename temp file: " + std::string(strerror(errno));
        return false;
    }

    // 恢复原文件权限
    if (file_exists) {
        chmod(filepath.c_str(), original_mode);
    }

    // 保存成功后，删除备份文件（避免残留备份文件）
    if (backup_created) {
        std::remove(backup_file.c_str());
    }

    // 更新文档状态
    filepath_ = filepath;
    modified_ = false;
    if (!large_file_skip_original_) {
        saveOriginalContent();
    } else {
        original_file_hash_ = computeFileHash(filepath_);
        try {
            original_file_mtime_ = std::filesystem::last_write_time(filepath_);
        } catch (...) {
            original_file_mtime_ = std::filesystem::file_time_type::min();
        }
    }
    clearHistory(); // 清除撤销历史，因为已经保存了
    last_error_.clear();

    return true;
}

bool Document::reload() {
    if (filepath_.empty()) {
        return false;
    }
    return load(filepath_);
}

size_t Document::lineCount() const {
    if (lazy_loaded_) {
        return line_offsets_.empty() ? 0 : (line_offsets_.size() - 1);
    }
    return lines_.size();
}

const std::string& Document::getLine(size_t row) const {
    static const std::string empty;
    if (lazy_loaded_) {
        const size_t num_lines = line_offsets_.size() > 1 ? (line_offsets_.size() - 1) : 0;
        if (row >= num_lines) {
            return empty;
        }
        auto it = line_cache_.find(row);
        if (it != line_cache_.end()) {
            auto it_idx = line_cache_lru_index_.find(row);
            if (it_idx != line_cache_lru_index_.end()) {
                line_cache_lru_.erase(it_idx->second);
                line_cache_lru_index_.erase(it_idx);
            }
            line_cache_lru_.push_front(row);
            line_cache_lru_index_[row] = line_cache_lru_.begin();
            return it->second;
        }
        std::string line = loadLineFromFile(row);
        while (line_cache_.size() >= LINE_CACHE_MAX && !line_cache_lru_.empty()) {
            size_t evict = line_cache_lru_.back();
            line_cache_lru_.pop_back();
            line_cache_.erase(evict);
            line_cache_lru_index_.erase(evict);
        }
        line_cache_lru_.push_front(row);
        line_cache_lru_index_[row] = line_cache_lru_.begin();
        line_cache_[row] = std::move(line);
        return line_cache_[row];
    }
    if (row >= lines_.size()) {
        return empty;
    }
    return lines_[row];
}

const std::vector<std::string>& Document::getLines() const {
    if (lazy_loaded_) {
        const_cast<Document*>(this)->materialize();
    }
    return lines_;
}

std::vector<std::string>& Document::getLines() {
    if (lazy_loaded_) {
        materialize();
    }
    return lines_;
}

std::string Document::getContent() const {
    if (lazy_loaded_) {
        const_cast<Document*>(this)->materialize();
    }
    std::string content;
    for (size_t i = 0; i < lines_.size(); ++i) {
        content += lines_[i];
        if (i < lines_.size() - 1) {
            content += "\n";
        }
    }
    return content;
}

std::string Document::getFileName() const {
    if (filepath_.empty()) {
        return "[Untitled]";
    }

    size_t pos = filepath_.find_last_of("/\\");
    if (pos != std::string::npos) {
        return filepath_.substr(pos + 1);
    }
    return filepath_;
}

std::string Document::getFileExtension() const {
    std::string filename = getFileName();
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return "";
}

void Document::materialize() {
    if (!lazy_loaded_ || filepath_.empty() || line_offsets_.size() < 2) {
        if (lazy_loaded_) {
            lazy_loaded_ = false;
            line_offsets_.clear();
            line_cache_.clear();
            line_cache_lru_.clear();
            line_cache_lru_index_.clear();
        }
        return;
    }
    std::ifstream file(filepath_, std::ios::binary);
    if (!file.is_open()) {
        lazy_loaded_ = false;
        line_offsets_.clear();
        line_cache_.clear();
        line_cache_lru_.clear();
        line_cache_lru_index_.clear();
        lines_.clear();
        lines_.push_back("");
        return;
    }
    const size_t num_lines = line_offsets_.size() - 1;
    lines_.clear();
    lines_.reserve(num_lines);
    for (size_t i = 0; i < num_lines; ++i) {
        const uint64_t start = line_offsets_[i];
        const uint64_t end = line_offsets_[i + 1];
        if (start >= end) {
            lines_.push_back("");
            continue;
        }
        const size_t len = static_cast<size_t>(end - start);
        std::string line(len, '\0');
        file.seekg(static_cast<std::streamoff>(start));
        file.read(&line[0], static_cast<std::streamsize>(len));
        line.resize(static_cast<size_t>(file.gcount()));
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines_.push_back(std::move(line));
    }
    if (lines_.empty()) {
        lines_.push_back("");
    }
    line_offsets_.clear();
    line_offsets_.shrink_to_fit();
    line_cache_.clear();
    line_cache_lru_.clear();
    line_cache_lru_index_.clear();
    lazy_loaded_ = false;
    can_undo_ = true;
    clearHistory();
}

std::string Document::loadLineFromFile(size_t row) const {
    if (lazy_loaded_ && row + 1 < line_offsets_.size()) {
        const uint64_t start = line_offsets_[row];
        const uint64_t end = line_offsets_[row + 1];
        if (start >= end) {
            return "";
        }
        std::ifstream file(filepath_, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }
        file.seekg(static_cast<std::streamoff>(start));
        const size_t len = static_cast<size_t>(end - start);
        std::string line(len, '\0');
        file.read(&line[0], static_cast<std::streamsize>(len));
        line.resize(static_cast<size_t>(file.gcount()));
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        return line;
    }
    return "";
}

void Document::insertChar(size_t row, size_t col, char ch) {
    if (lazy_loaded_) {
        materialize();
    }
    if (row >= lines_.size()) {
        return;
    }

    if (col > lines_[row].length()) {
        col = lines_[row].length();
    }

    std::string old_line = lines_[row];
    lines_[row].insert(col, 1, ch);

    pushChange(DocumentChange(DocumentChange::Type::INSERT, row, col, "", std::string(1, ch)));
}

void Document::insertText(size_t row, size_t col, const std::string& text) {
    if (lazy_loaded_) {
        materialize();
    }
    if (row >= lines_.size() || text.empty()) {
        return;
    }

    if (col > lines_[row].length()) {
        col = lines_[row].length();
    }

    lines_[row].insert(col, text);

    pushChange(DocumentChange(DocumentChange::Type::INSERT, row, col, "", text));
}

void Document::insertLine(size_t row) {
    if (lazy_loaded_) {
        materialize();
    }
    if (row > lines_.size()) {
        row = lines_.size();
    }

    lines_.insert(lines_.begin() + row, "");
}

void Document::deleteLine(size_t row) {
    if (lazy_loaded_) {
        materialize();
    }
    if (row >= lines_.size()) {
        return;
    }

    const size_t old_size = lines_.size();
    std::string deleted = lines_[row];

    if (old_size == 1) {
        // 单行文档：退化为替换为空串，撤销可直接恢复原行
        lines_[0] = "";
        pushChange(DocumentChange(DocumentChange::Type::REPLACE, row, 0, deleted, ""));
        return;
    }

    if (row == old_size - 1) {
        // 删除最后一行：等价于删除上一行行尾处的 "\n + deleted"
        size_t prev_row = row - 1;
        size_t prev_col = lines_[prev_row].length();
        lines_.erase(lines_.begin() + row);
        pushChange(
            DocumentChange(DocumentChange::Type::DELETE, prev_row, prev_col, "\n" + deleted, ""));
    } else {
        // 删除中间/首行：等价于从本行行首删除 "deleted + \n"
        lines_.erase(lines_.begin() + row);
        pushChange(DocumentChange(DocumentChange::Type::DELETE, row, 0, deleted + "\n", ""));
    }
}

void Document::deleteChar(size_t row, size_t col) {
    if (lazy_loaded_) {
        materialize();
    }
    if (row >= lines_.size()) {
        return;
    }

    if (col < lines_[row].length()) {
        char deleted = lines_[row][col];
        lines_[row].erase(col, 1);

        pushChange(
            DocumentChange(DocumentChange::Type::DELETE, row, col, std::string(1, deleted), ""));
    } else if (row < lines_.size() - 1) {
        // 合并行
        std::string next_line = lines_[row + 1];
        std::string old_line = lines_[row];
        lines_[row] += next_line;
        lines_.erase(lines_.begin() + row + 1);
        // 记录行合并操作
        pushChange(DocumentChange(DocumentChange::Type::REPLACE, row, old_line.length(),
                                  old_line + "\n" + next_line, old_line + next_line));
    }
}

void Document::deleteRange(size_t start_row, size_t start_col, size_t end_row, size_t end_col) {
    if (lazy_loaded_) {
        materialize();
    }
    if (start_row >= lines_.size() || end_row >= lines_.size() || start_row > end_row) {
        return;
    }

    size_t lines_before = lines_.size();
    LOG_DEBUG("[DELETE_RANGE] start_row=" + std::to_string(start_row) +
              " start_col=" + std::to_string(start_col) + " end_row=" + std::to_string(end_row) +
              " end_col=" + std::to_string(end_col) +
              " lines_before=" + std::to_string(lines_before));

    // Clamp columns
    std::string old_content;

    if (start_row == end_row) {
        const std::string& line = lines_[start_row];
        size_t sc = std::min(start_col, line.length());
        size_t ec = std::min(end_col, line.length());
        if (sc >= ec) {
            return; // nothing to delete
        }
        old_content = line.substr(sc, ec - sc);
        lines_[start_row].erase(sc, ec - sc);
        pushChange(DocumentChange(DocumentChange::Type::DELETE, start_row, sc, old_content, ""));
        return;
    }

    // Multi-line deletion
    // first line: keep prefix before start_col
    size_t sc = std::min(start_col, lines_[start_row].length());
    size_t ec = std::min(end_col, lines_[end_row].length());

    // Build old_content = suffix of first line (from sc) + '\n' + middle lines + '\n' + prefix of
    // last line (0..ec)
    old_content = lines_[start_row].substr(sc);
    old_content += '\n';
    for (size_t r = start_row + 1; r < end_row; ++r) {
        old_content += lines_[r];
        old_content += '\n';
    }
    old_content += lines_[end_row].substr(0, ec);

    // Construct new first line = prefix before sc + suffix after ec of last line
    std::string new_first = lines_[start_row].substr(0, sc) + lines_[end_row].substr(ec);

    // Erase range of lines and replace first line with new_first
    lines_.erase(lines_.begin() + start_row, lines_.begin() + end_row + 1);
    lines_.insert(lines_.begin() + start_row, new_first);

    DocumentChange change(DocumentChange::Type::DELETE, start_row, sc, old_content, "");
    change.restored_lines.clear();

    std::istringstream iss(old_content);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        change.restored_lines.push_back(line);
    }
    if (!old_content.empty() && (old_content.back() == '\n' || old_content.back() == '\r')) {
        change.restored_lines.push_back("");
    }

    size_t lines_after = lines_.size();
    LOG_DEBUG("[DELETE_RANGE] END: lines_after=" + std::to_string(lines_after) +
              " lines_removed=" + std::to_string(lines_before - lines_after) +
              " restored_lines_count=" + std::to_string(change.restored_lines.size()) +
              " old_content_len=" + std::to_string(old_content.length()));

    pushChange(change);
}

void Document::replaceLine(size_t row, const std::string& content) {
    if (lazy_loaded_) {
        materialize();
    }
    if (row >= lines_.size()) {
        return;
    }

    std::string old_content = lines_[row];
    lines_[row] = content;

    pushChange(DocumentChange(DocumentChange::Type::REPLACE, row, 0, old_content, content));
}

bool Document::undo(size_t* out_row, size_t* out_col, DocumentChange::Type* out_type) {
    if (undo_stack_.empty()) {
        return false;
    }

    size_t lines_before = lines_.size();
    size_t undo_stack_size_before = undo_stack_.size();

    DocumentChange change = undo_stack_.back();
    undo_stack_.pop_back();

    // VSCode 风格的撤销逻辑：原子性操作，直接应用反向操作
    // 每个撤销点都是完整的、不可分割的操作
    bool success = false;
    switch (change.type) {
        case DocumentChange::Type::INSERT: {
            // 撤销插入操作：删除之前插入的内容
            if (change.row < lines_.size()) {
                std::string& current_line = lines_[change.row];
                size_t line_len = current_line.length();

                // 只有当 new_content 等于整行内容时，才认为是整行插入（如 duplicateLine）
                // 否则只是普通的行内插入，应该只删除插入的文本
                // 关键修复：增加额外条件避免误判在新行开头插入的情况
                // duplicateLine 的特征：old_content 为空，col=0，new_content 等于整行，
                // 且该行之前的内容应该为空（因为是新创建的行）
                bool is_whole_line_insert =
                    (change.old_content.empty() && change.col == 0 &&
                     current_line == change.new_content && change.new_content.length() > 0);

                // 额外检查：如果 new_content 只有一个字符，需要确认这真的是整行插入
                // 而不是在新行开头插入单个字符
                if (is_whole_line_insert && change.new_content.length() == 1) {
                    // 单字符整行插入很少见，更可能是普通插入
                    // 除非有明确的标识（如 restored_lines 中有内容）
                    is_whole_line_insert = false;
                }

                if (is_whole_line_insert) {
                    // 整行插入：撤销时删除整行
                    lines_.erase(lines_.begin() + change.row);
                    if (lines_.empty()) {
                        lines_.push_back("");
                    }
                    success = true;
                } else {
                    size_t insert_len = change.new_content.length();
                    size_t max_erase = line_len - change.col;
                    size_t erase_len = std::min(insert_len, max_erase);

                    if (erase_len > 0) {
                        // 验证要删除的内容是否匹配（额外的安全检查）
                        if (current_line.substr(change.col, erase_len) ==
                            change.new_content.substr(0, erase_len)) {
                            current_line.erase(change.col, erase_len);
                            success = true;
                        }
                    } else {
                        success = true; // 空操作也算成功
                    }
                }
            }

            // 撤销后的光标位置：回到插入开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::DELETE: {
            if (change.row >= lines_.size()) {
                break;
            }

            std::string& current_line = lines_[change.row];
            size_t line_len = current_line.length();

            if (change.col > line_len) {
                break;
            }

            if (change.old_content.find('\n') != std::string::npos) {
                std::vector<std::string> restored_lines;
                if (!change.restored_lines.empty()) {
                    restored_lines = change.restored_lines;
                } else {
                    std::istringstream iss(change.old_content);
                    std::string line;
                    while (std::getline(iss, line)) {
                        if (!line.empty() && line.back() == '\r') {
                            line.pop_back();
                        }
                        restored_lines.push_back(line);
                    }
                    if (!change.old_content.empty() &&
                        (change.old_content.back() == '\n' || change.old_content.back() == '\r')) {
                        restored_lines.push_back("");
                    }
                }

                if (!restored_lines.empty()) {
                    std::string remaining_part = current_line.substr(change.col);
                    current_line = current_line.substr(0, change.col) + restored_lines[0];

                    if (restored_lines.size() > 1) {
                        for (size_t i = 1; i < restored_lines.size() - 1; ++i) {
                            lines_.insert(lines_.begin() + change.row + i, restored_lines[i]);
                        }

                        std::string last_line = restored_lines.back() + remaining_part;
                        lines_.insert(lines_.begin() + change.row + restored_lines.size() - 1,
                                      last_line);
                    } else {
                        current_line += remaining_part;
                    }

                    success = true;
                }
            } else {
                current_line.insert(change.col, change.old_content);
                success = true;
            }

            if (out_row)
                *out_row = change.row;
            if (out_col) {
                *out_col = (change.old_content.find('\n') != std::string::npos) ? 0 : change.col;
            }
            break;
        }

        case DocumentChange::Type::REPLACE: {
            // 撤销替换操作：恢复原始内容
            if (change.row < lines_.size()) {
                // 验证当前内容是否与预期的新内容匹配（安全检查）
                if (lines_[change.row] == change.new_content) {
                    lines_[change.row] = change.old_content;
                    success = true;
                }
            }

            // 撤销后的光标位置：回到替换开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::NEWLINE: {
            // 撤销换行操作：合并两行，恢复原始行
            // 由于撤销是逆序执行，change.row 始终准确，不需要内容匹配
            if (change.row < lines_.size() && change.row + 1 < lines_.size()) {
                // 直接恢复原始完整行，删除分裂出的第二行
                lines_[change.row] = change.old_content;
                lines_.erase(lines_.begin() + change.row + 1);
                success = true;
            }

            // 撤销后的光标位置：回到换行前的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::COMPLETION: {
            // 撤销补全操作：恢复被补全替换的原始文本
            if (change.row < lines_.size()) {
                std::string& current_line = lines_[change.row];
                size_t replace_start = change.col;

                if (replace_start <= current_line.length()) {
                    const std::string& completion_text = change.new_content;

                    // 精确匹配：检查指定位置是否正好是补全文本
                    if (replace_start + completion_text.length() <= current_line.length() &&
                        current_line.substr(replace_start, completion_text.length()) ==
                            completion_text) {
                        current_line.replace(replace_start, completion_text.length(),
                                             change.old_content);
                        success = true;
                    } else {
                        size_t found_pos = current_line.find(completion_text, replace_start);
                        if (found_pos != std::string::npos) {
                            current_line.replace(found_pos, completion_text.length(),
                                                 change.old_content);
                            success = true;
                        }
                    }
                }
            }

            // 撤销后的光标位置：回到补全开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::MOVE_LINE: {
            size_t target = change.target_row;
            if (change.row < lines_.size() && target < lines_.size()) {
                std::swap(lines_[change.row], lines_[target]);
                success = true;
            }
            if (out_row)
                *out_row = target;
            if (out_col)
                *out_col = 0;
            break;
        }

        case DocumentChange::Type::COMMENT_TOGGLE: {
            if (!change.restored_lines.empty()) {
                for (size_t i = 0;
                     i < change.restored_lines.size() && (change.row + i) < lines_.size(); ++i) {
                    lines_[change.row + i] = change.restored_lines[i];
                }
            } else {
                std::istringstream old_stream(change.old_content);
                std::string old_line;
                size_t r = change.row;
                while (std::getline(old_stream, old_line) && r < lines_.size()) {
                    if (!old_line.empty() && old_line.back() == '\r')
                        old_line.pop_back();
                    lines_[r] = old_line;
                    r++;
                }
            }
            success = true;
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = 0;
            break;
        }
    }

    // 确保文档至少有一行（边界情况处理）
    if (lines_.empty()) {
        lines_.push_back("");
    }

    bool is_same = isContentSameAsOriginal();

    // 将操作移到重做栈（用于重做功能）
    redo_stack_.push_back(change);

    if (out_type) {
        *out_type = change.type;
    }

    // 修复：仅通过 isContentSameAsOriginal() 自然判定 modified_ 状态
    // 移除强制还原 original_lines_ 的危险逻辑
    modified_ = !is_same;

    return success;
}

bool Document::redo(size_t* out_row, size_t* out_col) {
    if (redo_stack_.empty()) {
        LOG_DEBUG("[REDO] redo_stack is empty, cannot redo");
        return false;
    }

    size_t lines_before = lines_.size();
    size_t redo_stack_size_before = redo_stack_.size();

    DocumentChange change = redo_stack_.back();
    redo_stack_.pop_back();

    LOG_DEBUG("[REDO] START: type=" + std::to_string(static_cast<int>(change.type)) +
              " row=" + std::to_string(change.row) + " col=" + std::to_string(change.col) +
              " lines_before=" + std::to_string(lines_before) +
              " redo_stack_size=" + std::to_string(redo_stack_size_before));

    bool success = false;

    switch (change.type) {
        case DocumentChange::Type::INSERT:
            if (change.old_content.empty() && change.col == 0) {
                if (change.row <= lines_.size()) {
                    lines_.insert(lines_.begin() + change.row, change.new_content);
                    success = true;
                }
            } else {
                if (change.row < lines_.size()) {
                    size_t col = std::min(change.col, lines_[change.row].length());
                    lines_[change.row].insert(col, change.new_content);
                    success = true;
                }
            }
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col + change.new_content.length();
            break;

        case DocumentChange::Type::DELETE:
            if (change.row < lines_.size()) {
                size_t col = std::min(change.col, lines_[change.row].length());
                size_t len =
                    std::min(change.old_content.length(), lines_[change.row].length() - col);
                if (len > 0) {
                    lines_[change.row].erase(col, len);
                    success = true;
                }
            }
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;

        case DocumentChange::Type::REPLACE:
            if (change.row < lines_.size()) {
                lines_[change.row] = change.new_content;
                success = true;
            }
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col =
                    std::min(change.col + change.new_content.length(), change.new_content.length());
            break;

        case DocumentChange::Type::NEWLINE:
            if (change.row < lines_.size()) {
                lines_[change.row] = change.new_content;
                if (change.row + 1 <= lines_.size()) {
                    lines_.insert(lines_.begin() + change.row + 1, change.after_cursor);
                }
                success = true;
            }
            if (out_row)
                *out_row = change.row + 1;
            if (out_col)
                *out_col = 0;
            break;

        case DocumentChange::Type::COMPLETION:
            if (change.row < lines_.size()) {
                std::string& current_line = lines_[change.row];
                size_t replace_start = std::min(change.col, current_line.length());

                if (replace_start + change.old_content.length() <= current_line.length() &&
                    current_line.substr(replace_start, change.old_content.length()) ==
                        change.old_content) {
                    current_line.replace(replace_start, change.old_content.length(),
                                         change.new_content);
                    success = true;
                } else {
                    size_t found = current_line.find(change.old_content, replace_start);
                    if (found != std::string::npos) {
                        current_line.replace(found, change.old_content.length(),
                                             change.new_content);
                        success = true;
                    }
                }
            }
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col + change.new_content.length();
            break;

        case DocumentChange::Type::MOVE_LINE: {
            size_t target = change.target_row;
            if (change.row < lines_.size() && target < lines_.size()) {
                std::swap(lines_[change.row], lines_[target]);
                success = true;
            }
            if (out_row)
                *out_row = target;
            if (out_col)
                *out_col = 0;
            break;
        }

        case DocumentChange::Type::COMMENT_TOGGLE: {
            if (!change.restored_lines.empty()) {
                for (size_t i = 0;
                     i < change.restored_lines.size() && (change.row + i) < lines_.size(); ++i) {
                    lines_[change.row + i] = change.restored_lines[i];
                }
                success = true;
            } else {
                std::istringstream new_stream(change.new_content);
                std::string new_line;
                size_t r = change.row;
                while (std::getline(new_stream, new_line) && r < lines_.size()) {
                    if (!new_line.empty() && new_line.back() == '\r')
                        new_line.pop_back();
                    lines_[r] = new_line;
                    r++;
                }
                success = true;
            }
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = 0;
            break;
        }
    }

    if (lines_.empty()) {
        lines_.push_back("");
    }

    size_t lines_after = lines_.size();
    bool is_same = isContentSameAsOriginal();

    LOG_DEBUG("[REDO] END: success=" + std::to_string(success) +
              " lines_after=" + std::to_string(lines_after) + " lines_diff=" +
              std::to_string(static_cast<int>(lines_after) - static_cast<int>(lines_before)) +
              " is_same_as_original=" + std::to_string(is_same) +
              " modified=" + std::to_string(!is_same));

    if (success) {
        undo_stack_.push_back(change);
    } else {
        redo_stack_.push_back(change);
    }

    modified_ = !is_same;
    syncToBufferBackend();

    return success;
}

void Document::pushChange(const DocumentChange& change) {
    if (lazy_loaded_) {
        can_undo_ = false;
        return;
    }

    can_undo_ = true;

    if (change.content_size > MAX_CHANGE_CONTENT_SIZE) {
        DocumentChange safe_change = change;
        if (safe_change.old_content.size() > MAX_CHANGE_CONTENT_SIZE) {
            safe_change.old_content = safe_change.old_content.substr(0, MAX_CHANGE_CONTENT_SIZE);
            safe_change.old_content += "\n...[truncated]";
        }
        if (safe_change.new_content.size() > MAX_CHANGE_CONTENT_SIZE) {
            safe_change.new_content = safe_change.new_content.substr(0, MAX_CHANGE_CONTENT_SIZE);
            safe_change.new_content += "\n...[truncated]";
        }
        safe_change.content_size = safe_change.old_content.size() + safe_change.new_content.size();
        pushChangeInternal(safe_change);
        return;
    }

    pushChangeInternal(change);
}

void Document::pushChangeInternal(const DocumentChange& change) {
    constexpr auto MERGE_THRESHOLD = std::chrono::milliseconds(300);

    if (change.type == DocumentChange::Type::COMPLETION ||
        change.type == DocumentChange::Type::REPLACE ||
        change.type == DocumentChange::Type::NEWLINE) {
        undo_stack_.push_back(change);
        trimUndoStack();
        if (!undo_stack_.empty()) {
            redo_stack_.clear();
        }
        modified_ = !isContentSameAsOriginal();
        syncToBufferBackend();
        return;
    }

    if (!undo_stack_.empty()) {
        DocumentChange& last_change = undo_stack_.back();
        auto time_diff = change.timestamp - last_change.timestamp;

        if (time_diff < MERGE_THRESHOLD && change.row == last_change.row &&
            change.type == last_change.type) {
            if (change.type == DocumentChange::Type::INSERT &&
                last_change.type == DocumentChange::Type::INSERT) {
                if (last_change.old_content.empty() && last_change.col == 0) {
                    // 不合并
                } else if (change.col == last_change.col + last_change.new_content.length()) {
                    last_change.new_content += change.new_content;
                    last_change.timestamp = change.timestamp;
                    last_change.content_size =
                        last_change.old_content.size() + last_change.new_content.size();
                    syncToBufferBackend();
                    return;
                } else if (change.col == last_change.col && change.new_content.length() == 1) {
                    last_change.new_content += change.new_content;
                    last_change.timestamp = change.timestamp;
                    last_change.content_size =
                        last_change.old_content.size() + last_change.new_content.size();
                    syncToBufferBackend();
                    return;
                }
            }

            else if (change.type == DocumentChange::Type::DELETE &&
                     last_change.type == DocumentChange::Type::DELETE) {
                if (change.col == last_change.col) {
                    last_change.old_content += change.old_content;
                    last_change.timestamp = change.timestamp;
                    last_change.content_size =
                        last_change.old_content.size() + last_change.new_content.size();
                    syncToBufferBackend();
                    return;
                } else if (change.col + change.old_content.length() == last_change.col) {
                    last_change.old_content = change.old_content + last_change.old_content;
                    last_change.col = change.col;
                    last_change.timestamp = change.timestamp;
                    last_change.content_size =
                        last_change.old_content.size() + last_change.new_content.size();
                    syncToBufferBackend();
                    return;
                }
            }
        }
    }

    undo_stack_.push_back(change);
    trimUndoStack();
    if (!undo_stack_.empty()) {
        redo_stack_.clear();
    }
    modified_ = !isContentSameAsOriginal();
    syncToBufferBackend();
}

void Document::clearHistory() {
    undo_stack_.clear();
    redo_stack_.clear();
    current_undo_memory_ = 0;
}

void Document::trimUndoStack() {
    while (undo_stack_.size() > MAX_UNDO_STACK) {
        const auto& front = undo_stack_.front();
        current_undo_memory_ -= (front.old_content.size() + front.new_content.size());
        undo_stack_.pop_front();
    }

    size_t total_memory = 0;
    for (const auto& change : undo_stack_) {
        total_memory += change.old_content.size() + change.new_content.size();
    }
    current_undo_memory_ = total_memory;

    while (current_undo_memory_ > MAX_UNDO_MEMORY_BYTES && undo_stack_.size() > 10) {
        const auto& front = undo_stack_.front();
        current_undo_memory_ -= (front.old_content.size() + front.new_content.size());
        undo_stack_.pop_front();
    }
}

void Document::syncToBufferBackend() {
    if (!buffer_backend_) {
        return;
    }
    buffer_backend_->clear();
    std::string content;
    for (size_t i = 0; i < lines_.size(); ++i) {
        content += lines_[i];
        if (i < lines_.size() - 1) {
            content += "\n";
        }
    }
    buffer_backend_->insert(0, content);
}

uint64_t Document::computeFileHash(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return 0;
    }
    uint64_t hash = 0;
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        for (std::streamsize i = 0; i < file.gcount(); ++i) {
            hash = hash * 31 + static_cast<uint64_t>(static_cast<unsigned char>(buffer[i]));
        }
    }
    for (std::streamsize i = 0; i < file.gcount(); ++i) {
        hash = hash * 31 + static_cast<uint64_t>(static_cast<unsigned char>(buffer[i]));
    }
    return hash;
}

std::string Document::getSelection(size_t start_row, size_t start_col, size_t end_row,
                                   size_t end_col) const {
    const size_t n = lineCount();
    if (start_row >= n || end_row >= n) {
        return "";
    }

    if (start_row == end_row) {
        const std::string& line = getLine(start_row);
        if (start_col >= line.length()) {
            return "";
        }
        size_t len =
            (end_col <= line.length()) ? (end_col - start_col) : (line.length() - start_col);
        return line.substr(start_col, len);
    }

    std::string result;
    for (size_t row = start_row; row <= end_row; ++row) {
        const std::string& line = getLine(row);
        if (row == start_row) {
            result += line.substr(start_col);
        } else if (row == end_row) {
            result += "\n" + line.substr(0, end_col);
        } else {
            result += "\n" + line;
        }
    }

    return result;
}

void Document::detectLineEnding(const std::string& content) {
    if (content.find("\r\n") != std::string::npos) {
        line_ending_ = LineEnding::CRLF;
    } else if (content.find('\r') != std::string::npos) {
        line_ending_ = LineEnding::CR;
    } else {
        line_ending_ = LineEnding::LF;
    }
}

std::string Document::applyLineEnding(const std::string& /* line */) const {
    switch (line_ending_) {
        case LineEnding::CRLF:
            return "\r\n";
        case LineEnding::CR:
            return "\r";
        case LineEnding::LF:
        default:
            return "\n";
    }
}

void Document::saveOriginalContent() {
    original_lines_ = lines_;
}

bool Document::isContentSameAsOriginal() const {
    if (large_file_skip_original_) {
        uint64_t current_hash = computeFileHash(filepath_);
        bool same = (current_hash == original_file_hash_ && current_hash != 0);
        return same;
    }
    if (lines_.size() != original_lines_.size()) {
        return false;
    }
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (lines_[i] != original_lines_[i]) {
            return false;
        }
    }
    return true;
}

// 折叠范围管理
void Document::setFoldingRanges(const std::vector<pnana::features::FoldingRange>& ranges) {
    folding_ranges_ = ranges;
    // 注意：不要在这里清理folded_lines_，因为setFolded方法会单独调用来设置折叠状态
}

void Document::clearFoldingRanges() {
    folding_ranges_.clear();
    folded_lines_.clear();
}

void Document::setFolded(int start_line, bool folded) {
    (void)start_line;
    (void)folded;
    if (folded) {
        // 检查是否有对应的折叠范围
        for (const auto& range : folding_ranges_) {
            if (range.startLine == start_line) {
                folded_lines_.insert(start_line);
                return;
            }
        }
        // no-op for debug
    } else {
        folded_lines_.erase(start_line);
        // no-op for debug
    }
}

bool Document::isFolded(int line) const {
    return folded_lines_.count(line) > 0;
}

bool Document::isLineInFoldedRange(int line) const {
    for (const auto& range : folding_ranges_) {
        if (folded_lines_.count(range.startLine) && range.containsLine(line) &&
            line != range.startLine) {
            return true;
        }
    }
    return false;
}

void Document::toggleFold(int start_line) {
    if (isFolded(start_line)) {
        setFolded(start_line, false);
    } else {
        setFolded(start_line, true);
    }
}

void Document::unfoldAll() {
    folded_lines_.clear();
}

void Document::foldAll() {
    folded_lines_.clear();
    for (const auto& range : folding_ranges_) {
        folded_lines_.insert(range.startLine);
    }
}

std::vector<size_t> Document::getVisibleLines(size_t start_line, size_t end_line) const {
    std::vector<size_t> visible_lines;
    size_t max_line = std::min(end_line, lineCount() - 1);

    for (size_t line = start_line; line <= max_line; ++line) {
        if (!isLineInFoldedRange(static_cast<int>(line))) {
            visible_lines.push_back(line);
        }
    }

    return visible_lines;
}

size_t Document::getVisibleLineCount() const {
    return getVisibleLines(0, lineCount() - 1).size();
}

size_t Document::displayLineToActualLine(size_t display_line) const {
    auto visible_lines = getVisibleLines(0, lineCount() - 1);
    if (display_line < visible_lines.size()) {
        return visible_lines[display_line];
    }
    return lineCount() - 1; // 返回最后一行
}

size_t Document::actualLineToDisplayLine(size_t actual_line) const {
    auto visible_lines = getVisibleLines(0, actual_line);
    return visible_lines.size() - 1; // 返回可见行索引
}

} // namespace core
} // namespace pnana
