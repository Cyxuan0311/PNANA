#include "core/document.h"
#include "core/buffer_factory.h"
#include "utils/logger.h"
#include <algorithm>
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
#include <unistd.h>

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
    auto load_t0 = std::chrono::high_resolution_clock::now();

    // 检查路径是否是目录
    try {
        if (std::filesystem::exists(filepath) && std::filesystem::is_directory(filepath)) {
            last_error_ = "Cannot open directory as file: " + filepath;
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

    if (pnana::utils::Logger::getInstance().isEnabled()) {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now() - load_t0)
                      .count();
        LOG("[perf] Document::load prefix_read us=" + std::to_string(us));
    }

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
    auto stream_t0 = std::chrono::high_resolution_clock::now();
    const size_t chunk_size = 512 * 1024;
    std::string buffer(chunk_size, '\0');
    line_offsets_.clear();
    line_offsets_.push_back(0);
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
    }
    if (stream_file_size > 0 && line_offsets_.back() != static_cast<uint64_t>(stream_file_size)) {
        line_offsets_.push_back(static_cast<uint64_t>(stream_file_size));
    }
    size_t num_lines = line_offsets_.empty() ? 0 : (line_offsets_.size() - 1);
    if (num_lines == 0) {
        line_offsets_.push_back(0);
    }

    if (pnana::utils::Logger::getInstance().isEnabled()) {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now() - stream_t0)
                      .count();
        LOG("[perf] Document::load line_offsets us=" + std::to_string(us) +
            " lines=" + std::to_string(num_lines));
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
        if (pnana::utils::Logger::getInstance().isEnabled()) {
            auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::high_resolution_clock::now() - load_t0)
                                .count();
            LOG("[perf] Document::load lazy_loaded total us=" + std::to_string(total_us));
        }
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
    stream_t0 = std::chrono::high_resolution_clock::now();
    while (true) {
        file.read(&buffer[0], chunk_size);
        const size_t n = static_cast<size_t>(file.gcount());
        if (n == 0)
            break;
        size_t start = 0;
        for (size_t i = 0; i < n; ++i) {
            if (buffer[i] == '\n') {
                carry.append(buffer.data() + start, i - start);
                if (!carry.empty() && carry.back() == '\r')
                    carry.pop_back();
                lines_.push_back(std::move(carry));
                carry.clear();
                carry.reserve(4096);
                start = i + 1;
            }
        }
        if (start < n)
            carry.append(buffer.data() + start, n - start);
    }
    if (!carry.empty()) {
        if (carry.back() == '\r')
            carry.pop_back();
        lines_.push_back(std::move(carry));
    }
    if (lines_.empty()) {
        lines_.push_back("");
    }
    if (pnana::utils::Logger::getInstance().isEnabled()) {
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::high_resolution_clock::now() - stream_t0)
                      .count();
        LOG("[perf] Document::load stream_lines us=" + std::to_string(us) +
            " lines=" + std::to_string(lines_.size()));
    }
    const size_t large_file_line_threshold = 500000;
    if (lines_.size() > large_file_line_threshold) {
        large_file_skip_original_ = true;
        original_lines_.clear();
        original_lines_.shrink_to_fit();
    } else {
        large_file_skip_original_ = false;
        auto save_orig_t0 = std::chrono::high_resolution_clock::now();
        saveOriginalContent();
        if (pnana::utils::Logger::getInstance().isEnabled()) {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - save_orig_t0)
                          .count();
            LOG("[perf] Document::load saveOriginalContent us=" + std::to_string(us));
        }
    }
    if (pnana::utils::Logger::getInstance().isEnabled()) {
        auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::high_resolution_clock::now() - load_t0)
                            .count();
        LOG("[perf] Document::load total us=" + std::to_string(total_us));
    }

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
            for (auto itlru = line_cache_lru_.begin(); itlru != line_cache_lru_.end(); ++itlru) {
                if (*itlru == row) {
                    line_cache_lru_.erase(itlru);
                    break;
                }
            }
            line_cache_lru_.push_front(row);
            return it->second;
        }
        std::string line = loadLineFromFile(row);
        while (line_cache_.size() >= LINE_CACHE_MAX && !line_cache_lru_.empty()) {
            size_t evict = line_cache_lru_.back();
            line_cache_lru_.pop_back();
            line_cache_.erase(evict);
        }
        line_cache_lru_.push_front(row);
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
        }
        return;
    }
    std::ifstream file(filepath_, std::ios::binary);
    if (!file.is_open()) {
        lazy_loaded_ = false;
        line_offsets_.clear();
        line_cache_.clear();
        line_cache_lru_.clear();
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
    lazy_loaded_ = false;
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

    // 插入新行需要记录到撤销栈 - 使用特殊的DELETE类型表示撤销时删除这一行
    pushChange(DocumentChange(DocumentChange::Type::DELETE, row, 0, "", ""));
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

    pushChange(DocumentChange(DocumentChange::Type::DELETE, start_row, sc, old_content, ""));
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

                // 边界检查：确保插入位置有效
                if (change.col <= line_len) {
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

            // 检查是否是多行删除（包含换行符）
            if (change.old_content.find('\n') != std::string::npos) {
                // 多行删除的撤销：需要重新构建多行内容
                std::vector<std::string> restored_lines;
                std::istringstream iss(change.old_content);
                std::string line;

                while (std::getline(iss, line)) {
                    // 处理行尾的\r字符
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    restored_lines.push_back(line);
                }

                // 如果原始内容以换行符结尾，需要添加空行
                if (!change.old_content.empty() &&
                    (change.old_content.back() == '\n' || change.old_content.back() == '\r')) {
                    restored_lines.push_back("");
                }

                if (!restored_lines.empty()) {
                    // 保存插入位置之后的内容
                    std::string remaining_part = current_line.substr(change.col);

                    // 第一行：当前行前半部分 + 恢复的第一行
                    current_line = current_line.substr(0, change.col) + restored_lines[0];

                    // 如果有多行内容需要插入
                    if (restored_lines.size() > 1) {
                        // 插入中间行
                        for (size_t i = 1; i < restored_lines.size() - 1; ++i) {
                            lines_.insert(lines_.begin() + change.row + i, restored_lines[i]);
                        }

                        // 最后一行：恢复内容 + 原始的后半部分
                        std::string last_line = restored_lines.back() + remaining_part;
                        lines_.insert(lines_.begin() + change.row + restored_lines.size() - 1,
                                      last_line);
                    } else {
                        // 只有一行，直接追加剩余内容
                        current_line += remaining_part;
                    }

                    success = true;
                }
            } else {
                // 单行删除的撤销：直接在指定位置插入内容
                current_line.insert(change.col, change.old_content);
                success = true;
            }

            // 撤销后的光标位置：回到删除开始的位置
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
            size_t target_row = change.row;

            // 查找正确的行（通过内容匹配）
            bool found_target = false;
            if (target_row < lines_.size() && lines_[target_row] == change.new_content) {
                found_target = true;
            } else {
                // 尝试查找包含新行第一部分内容的行
                for (size_t i = 0; i < lines_.size(); ++i) {
                    if (lines_[i] == change.new_content && i + 1 < lines_.size()) {
                        target_row = i;
                        found_target = true;
                        break;
                    }
                }
            }

            if (found_target && target_row + 1 < lines_.size()) {
                // 合并两行：第一行 + 换行符 + 第二行
                lines_[target_row] = change.old_content;
                lines_.erase(lines_.begin() + target_row + 1);
                success = true;
            }

            // 撤销后的光标位置：回到换行前的位置
            if (out_row)
                *out_row = target_row;
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
    }

    // 确保文档至少有一行（边界情况处理）
    if (lines_.empty()) {
        lines_.push_back("");
    }

    // 将操作移到重做栈（用于重做功能）
    redo_stack_.push_back(change);

    if (out_type) {
        *out_type = change.type;
    }

    // 改进的修改状态标记逻辑：使用 original_lines_ 进行精确比较
    modified_ = !isContentSameAsOriginal();

    return success;
}

bool Document::redo(size_t* out_row, size_t* out_col) {
    if (redo_stack_.empty()) {
        return false;
    }

    DocumentChange change = redo_stack_.back();
    redo_stack_.pop_back();

    // 重新应用操作
    switch (change.type) {
        case DocumentChange::Type::INSERT:
            if (change.row < lines_.size()) {
                lines_[change.row].insert(change.col, change.new_content);
            }
            // 光标应该移动到插入结束的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col + change.new_content.length();
            break;

        case DocumentChange::Type::DELETE:
            if (change.row < lines_.size()) {
                lines_[change.row].erase(change.col, change.old_content.length());
            }
            // 光标应该保持在删除开始的位置（与 undo 保持一致）
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;

        case DocumentChange::Type::REPLACE:
            if (change.row < lines_.size()) {
                lines_[change.row] = change.new_content;
            }
            // 光标应该移动到替换结束的位置，但保持在合理的列位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col =
                    std::min(change.col + change.new_content.length(), change.new_content.length());
            break;

        case DocumentChange::Type::NEWLINE:
            // 重做换行：分割当前行，插入新行
            if (change.row < lines_.size()) {
                // 设置当前行为 before_cursor
                lines_[change.row] = change.new_content;
                // 插入新行并设置 after_cursor
                if (change.row + 1 <= lines_.size()) {
                    lines_.insert(lines_.begin() + change.row + 1, change.after_cursor);
                }
            }
            // 光标应该移动到新行的开始
            if (out_row)
                *out_row = change.row + 1;
            if (out_col)
                *out_col = 0;
            break;

        case DocumentChange::Type::COMPLETION:
            // 重做补全：重新应用补全文本替换
            if (change.row < lines_.size()) {
                std::string& current_line = lines_[change.row];
                size_t replace_start = change.col;

                // 优先精确匹配：如果指定位置正好是原始文本，直接替换
                if (replace_start <= current_line.length()) {
                    const std::string& expected_old = change.old_content;
                    if (replace_start + expected_old.length() <= current_line.length() &&
                        current_line.substr(replace_start, expected_old.length()) == expected_old) {
                        current_line.replace(replace_start, expected_old.length(),
                                             change.new_content);
                    } else {
                        // 查找第一次出现的位置并替换
                        size_t found = current_line.find(expected_old, replace_start);
                        if (found != std::string::npos) {
                            current_line.replace(found, expected_old.length(), change.new_content);
                        }
                        // 如果找不到，说明文档已被修改，重做失败但不报错
                    }
                }
            }
            // 光标应该移动到补全文本的末尾（VSCode 行为）
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col + change.new_content.length();
            break;
    }

    undo_stack_.push_back(change);

    // 改进的修改状态标记逻辑：使用 original_lines_ 进行精确比较
    modified_ = !isContentSameAsOriginal();

    return true;
}

void Document::pushChange(const DocumentChange& change) {
    // 降低合并阈值到 300ms，避免慢速打字时意外合并
    constexpr auto MERGE_THRESHOLD = std::chrono::milliseconds(300);

    if (change.type == DocumentChange::Type::COMPLETION ||
        change.type == DocumentChange::Type::REPLACE ||
        change.type == DocumentChange::Type::NEWLINE) {
        undo_stack_.push_back(change);
        if (undo_stack_.size() > MAX_UNDO_STACK) {
            undo_stack_.pop_front();
        }
        redo_stack_.clear(); // 新的修改清除重做栈
        modified_ = !isContentSameAsOriginal();
        return;
    }

    // 尝试合并连续的 INSERT 或 DELETE 操作
    if (!undo_stack_.empty()) {
        DocumentChange& last_change = undo_stack_.back();
        auto time_diff = change.timestamp - last_change.timestamp;

        // 更严格的合并条件：时间阈值内 + 同行 + 同类型 + 位置连续
        if (time_diff < MERGE_THRESHOLD && change.row == last_change.row) {
            // 合并 INSERT 操作：连续输入字符
            if (change.type == DocumentChange::Type::INSERT &&
                last_change.type == DocumentChange::Type::INSERT) {
                // 严格检查：新插入位置必须正好在上次插入结束位置
                size_t last_insert_end = last_change.col + last_change.new_content.length();
                if (change.col == last_insert_end) {
                    last_change.new_content += change.new_content;
                    last_change.timestamp = change.timestamp;
                    return; // 合并完成，不创建新撤销点
                }
                // 同一位置的连续插入（快速输入场景）
                else if (change.col == last_change.col && change.new_content.length() == 1) {
                    last_change.new_content += change.new_content;
                    last_change.timestamp = change.timestamp;
                    return;
                }
            }

            // 合并 DELETE 操作：连续删除字符
            else if (change.type == DocumentChange::Type::DELETE &&
                     last_change.type == DocumentChange::Type::DELETE) {
                // 向后删除（Delete 键）：严格检查在同一位置连续删除
                if (change.col == last_change.col) {
                    last_change.old_content += change.old_content;
                    last_change.timestamp = change.timestamp;
                    return;
                }
                // 向前删除（Backspace 键）：严格检查位置连续
                else if (change.col + change.old_content.length() == last_change.col) {
                    last_change.old_content = change.old_content + last_change.old_content;
                    last_change.col = change.col; // 更新删除起始位置
                    last_change.timestamp = change.timestamp;
                    return;
                }
            }
        }
    }

    undo_stack_.push_back(change);
    if (undo_stack_.size() > MAX_UNDO_STACK) {
        undo_stack_.pop_front();
    }
    redo_stack_.clear(); // 新的修改清除重做栈
    modified_ = !isContentSameAsOriginal();
}

void Document::clearHistory() {
    undo_stack_.clear();
    redo_stack_.clear();
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
    // 大文件未保存 original 快照时，仅用 modified_ 判断
    if (large_file_skip_original_) {
        return !modified_;
    }
    // 比较当前内容与原始内容是否完全相同
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
