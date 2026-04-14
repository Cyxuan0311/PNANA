#include "ui/fzf_popup.h"
#include "ui/icons.h"
#include "utils/file_info_utils.h"
#include "utils/file_type_detector.h"
#include "utils/logger.h"
#include "utils/match_highlight.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <fstream>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <tuple>

using namespace ftxui;

namespace pnana {
namespace ui {

// 自定义边框
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

// 应忽略的目录名（递归扫描时跳过）
static const std::vector<std::string> IGNORE_DIRS = {
    ".git",   "node_modules", "__pycache__", ".svn",    ".hg",  "build", "dist",
    "target", ".cache",       ".idea",       ".vscode", "venv", ".venv", "vendor"};

static bool shouldIgnoreDir(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const auto& ignore : IGNORE_DIRS) {
        if (lower == ignore)
            return true;
    }
    return false;
}

// 多线程扫描优化
static const size_t FZF_NUM_THREADS = std::thread::hardware_concurrency();

// 线程安全的文件结果收集器
class FileResultCollector {
  public:
    void addFile(const std::string& path, const std::string& display_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        results_.emplace_back(path, display_path);
    }

    std::vector<std::pair<std::string, std::string>> getResults() {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::move(results_);
    }

    void incrementTotalEntries() {
        total_entries_++;
    }
    void incrementTotalDirs() {
        total_dirs_++;
    }
    void incrementTotalFiles() {
        total_files_++;
    }
    void incrementIgnoredDirs() {
        ignored_dirs_++;
    }
    void incrementIgnoredEntries() {
        ignored_entries_++;
    }

    size_t getTotalEntries() const {
        return total_entries_.load();
    }
    size_t getTotalDirs() const {
        return total_dirs_.load();
    }
    size_t getTotalFiles() const {
        return total_files_.load();
    }
    size_t getIgnoredDirs() const {
        return ignored_dirs_.load();
    }
    size_t getIgnoredEntries() const {
        return ignored_entries_.load();
    }

  private:
    std::vector<std::pair<std::string, std::string>> results_;
    std::mutex mutex_;
    std::atomic<size_t> total_entries_{0};
    std::atomic<size_t> total_dirs_{0};
    std::atomic<size_t> total_files_{0};
    std::atomic<size_t> ignored_dirs_{0};
    std::atomic<size_t> ignored_entries_{0};
};

// 工作线程函数：处理目录队列
static void fzfWorkerThread(const std::filesystem::path& root,
                            std::queue<std::filesystem::directory_entry>& dir_queue,
                            std::mutex& queue_mutex, std::condition_variable& queue_cv,
                            std::atomic<bool>& all_roots_processed, FileResultCollector& collector,
                            int /*thread_id*/) {
    while (true) {
        std::filesystem::directory_entry entry;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            // 等待有任务或完成标志
            queue_cv.wait(lock, [&] {
                return !dir_queue.empty() || all_roots_processed.load();
            });

            if (dir_queue.empty()) {
                if (all_roots_processed.load()) {
                    return; // 所有任务完成
                }
                continue;
            }

            entry = std::move(dir_queue.front());
            dir_queue.pop();
        }

        try {
            collector.incrementTotalEntries();

            if (entry.is_directory()) {
                collector.incrementTotalDirs();
                std::string dir_name = entry.path().filename().string();

                if (shouldIgnoreDir(dir_name)) {
                    collector.incrementIgnoredDirs();
                    continue;
                }

                // 将子目录加入队列
                try {
                    for (const auto& sub_entry : std::filesystem::directory_iterator(
                             entry.path(),
                             std::filesystem::directory_options::skip_permission_denied)) {
                        {
                            std::lock_guard<std::mutex> lock(queue_mutex);
                            dir_queue.push(sub_entry);
                        }
                        queue_cv.notify_one();
                    }
                } catch (...) {
                    // 忽略无法访问的目录
                }
            } else if (entry.is_regular_file()) {
                collector.incrementTotalFiles();

                std::string path_str = entry.path().string();
                std::filesystem::path rel_path = std::filesystem::relative(entry.path(), root);

                // 检查路径组件是否有需要忽略的目录
                bool ignored = false;
                for (const auto& comp : rel_path) {
                    if (shouldIgnoreDir(comp.string())) {
                        ignored = true;
                        collector.incrementIgnoredEntries();
                        break;
                    }
                }

                if (!ignored) {
                    std::string display_path =
                        rel_path.empty() ? std::filesystem::path(path_str).filename().string()
                                         : rel_path.string();
                    collector.addFile(path_str, display_path);
                }
            } else {
                collector.incrementIgnoredEntries();
            }
        } catch (const std::exception& e) {
            // 忽略异常
        }
    }
}

FzfPopup::FzfPopup(Theme& theme)
    : theme_(theme), is_open_(false), is_loading_(false), input_(""), cursor_pos_(0),
      root_directory_("."), selected_index_(0), scroll_offset_(0), list_display_count_(18),
      preview_page_(0), color_mapper_(theme) {
    syntax_highlighter_ = std::make_unique<features::SyntaxHighlighter>(theme_);
}

// 静态函数：在后台线程中收集文件，返回 (文件列表，预计算显示路径，规范根路径)
// 使用多线程并行扫描加速
static std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>
collectFilesToVector(const std::string& root_directory) {
    LOG_TIMING_START("fzf_collectFiles");

    std::string canonical_root;
    FileResultCollector collector;

    try {
        std::filesystem::path root(root_directory);
        if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
            LOG_WARNING("fzf_popup - Root directory does not exist: " + root_directory);
            root = std::filesystem::current_path();
        }

        canonical_root = std::filesystem::canonical(root).string();

        LOG_DEBUG("fzf_collectFiles - Scanning directory: " + root_directory);
        LOG_DEBUG("fzf_collectFiles - Using " + std::to_string(FZF_NUM_THREADS) + " threads");

        // 目录队列和同步原语
        std::queue<std::filesystem::directory_entry> dir_queue;
        std::mutex queue_mutex;
        std::condition_variable queue_cv;
        std::atomic<bool> all_roots_processed{false};

        // 初始化：将根目录的子目录加入队列
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            for (const auto& entry : std::filesystem::directory_iterator(
                     root, std::filesystem::directory_options::skip_permission_denied)) {
                dir_queue.push(entry);
            }
        }

        // 创建工作线程
        std::vector<std::thread> threads;
        for (size_t i = 0; i < FZF_NUM_THREADS; ++i) {
            threads.emplace_back(fzfWorkerThread, std::ref(root), std::ref(dir_queue),
                                 std::ref(queue_mutex), std::ref(queue_cv),
                                 std::ref(all_roots_processed), std::ref(collector),
                                 static_cast<int>(i));
        }

        // 标记根目录处理完成
        all_roots_processed.store(true);
        queue_cv.notify_all();

        // 等待所有工作线程完成
        for (auto& thread : threads) {
            thread.join();
        }

        // 记录性能指标
        LOG_METRIC("fzf_total_entries", collector.getTotalEntries(), "root=" + root_directory);
        LOG_METRIC("fzf_total_dirs", collector.getTotalDirs(), "root=" + root_directory);
        LOG_METRIC("fzf_total_files", collector.getTotalFiles(), "root=" + root_directory);
        LOG_METRIC("fzf_ignored_dirs", collector.getIgnoredDirs(), "root=" + root_directory);
        LOG_METRIC("fzf_ignored_entries", collector.getIgnoredEntries(), "root=" + root_directory);

        auto results = collector.getResults();
        LOG_METRIC("fzf_collected_files", results.size(), "root=" + root_directory);

        // 排序阶段
        LOG_TIMING_START("fzf_sort");
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });
        LOG_TIMING_END("fzf_sort", "count=" + std::to_string(results.size()));

        std::vector<std::string> files;
        std::vector<std::string> display_paths;
        files.reserve(results.size());
        display_paths.reserve(results.size());
        for (auto& p : results) {
            files.push_back(std::move(p.first));
            display_paths.push_back(std::move(p.second));
        }

        LOG_TIMING_END("fzf_collectFiles",
                       "root=" + root_directory + ", files=" + std::to_string(files.size()));

        return {std::move(files), std::move(display_paths), std::move(canonical_root)};
    } catch (const std::exception& e) {
        LOG_ERROR("fzf_collectFiles - Exception: " + std::string(e.what()));
        LOG_TIMING_END("fzf_collectFiles", "error");
        return {{}, {}, {}};
    }
}

void FzfPopup::open() {
    is_open_ = true;
    is_loading_ = true;
    input_.clear();
    cursor_pos_ = 0;
    selected_index_ = 0;
    scroll_offset_ = 0;
    preview_page_ = 0;
    all_files_.clear();
    all_display_paths_.clear();
    filtered_files_.clear();
    filtered_display_paths_.clear();
    root_path_.clear();
    preview_h_offset_ = 0;

    if (root_directory_.size() >= 6 && root_directory_.compare(0, 6, "ssh://") == 0 &&
        on_remote_load_callback_) {
        on_remote_load_callback_(root_directory_);
        return;
    }

    if (on_load_complete_callback_) {
        // 异步加载：立即显示弹窗，后台线程收集文件
        std::string root = root_directory_;
        auto callback = on_load_complete_callback_;
        std::thread([root, callback]() {
            auto [files, display_paths, canonical_root] = collectFilesToVector(root);
            callback(std::move(files), std::move(display_paths), std::move(canonical_root));
        }).detach();
    } else {
        // 无回调时同步加载（兼容）
        auto [files, display_paths, canonical_root] = collectFilesToVector(root_directory_);
        all_files_ = std::move(files);
        all_display_paths_ = std::move(display_paths);
        root_path_ = std::move(canonical_root);
        filterFiles();
        is_loading_ = false;
    }
}

void FzfPopup::receiveFiles(std::vector<std::string> files, std::vector<std::string> display_paths,
                            std::string root_path) {
    all_files_ = std::move(files);
    all_display_paths_ = std::move(display_paths);
    root_path_ = std::move(root_path);
    filterFiles();
    is_loading_ = false;
}

void FzfPopup::setOnLoadComplete(
    std::function<void(std::vector<std::string>, std::vector<std::string>, std::string)> callback) {
    on_load_complete_callback_ = std::move(callback);
}

void FzfPopup::setOnRemoteLoad(std::function<void(const std::string&)> callback) {
    on_remote_load_callback_ = std::move(callback);
}

void FzfPopup::close() {
    is_open_ = false;
    is_loading_ = false;
    input_.clear();
    cursor_pos_ = 0;
    selected_index_ = 0;
    preview_page_ = 0;
    all_files_.clear();
    all_display_paths_.clear();
    filtered_files_.clear();
    filtered_display_paths_.clear();
    root_path_.clear();
}

void FzfPopup::setFileOpenCallback(std::function<void(const std::string&)> callback) {
    file_open_callback_ = std::move(callback);
}

void FzfPopup::setRootDirectory(const std::string& root) {
    root_directory_ = root.empty() ? "." : root;
}

void FzfPopup::setCursorColorGetter(std::function<ftxui::Color()> getter) {
    cursor_color_getter_ = std::move(getter);
}

void FzfPopup::collectAllFiles() {
    auto [files, display_paths, canonical_root] = collectFilesToVector(root_directory_);
    all_files_ = std::move(files);
    all_display_paths_ = std::move(display_paths);
    root_path_ = std::move(canonical_root);
}

bool FzfPopup::fuzzyMatch(const std::string& path, const std::string& query) {
    if (query.empty())
        return true;
    std::string path_lower = path;
    std::string query_lower = query;
    std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(), ::tolower);
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);

    size_t pi = 0;
    for (char qc : query_lower) {
        size_t found = path_lower.find(qc, pi);
        if (found == std::string::npos)
            return false;
        pi = found + 1;
    }
    return true;
}

// 根据扩展名判断是否为图片文件
static bool isImageFile(const std::string& filepath) {
    try {
        std::string ext = std::filesystem::path(filepath).extension().string();
        if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);
        std::string ext_lower = ext;
        std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

        // 图片扩展名列表
        static const std::vector<std::string> IMAGE_EXTS = {
            "png", "jpg",  "jpeg", "gif", "bmp", "webp", "ico", "tiff", "tif",
            "svg", "svgz", "xpm",  "xbm", "pcx", "tga",  "ppm", "pgm",  "pbm"};
        for (const auto& e : IMAGE_EXTS) {
            if (ext_lower == e)
                return true;
        }
    } catch (...) {
    }
    return false;
}

// 根据扩展名判断是否为不可预览文件（图片、二进制等）
static bool isNonPreviewableFile(const std::string& filepath) {
    try {
        std::string ext = std::filesystem::path(filepath).extension().string();
        if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);
        std::string ext_lower = ext;
        std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

        // 图片 - 现在可以预览，所以从不可预览列表中移除
        // static const std::vector<std::string> IMAGE_EXTS = {...};

        // 二进制 / 可执行
        static const std::vector<std::string> BINARY_EXTS = {
            "exe", "dll", "so",   "dylib", "o",     "a",   "lib", "obj", "class",
            "pyc", "pyo", "rlib", "woff",  "woff2", "ttf", "otf", "eot", "zip",
            "tar", "gz",  "xz",   "bz2",   "7z",    "rar", "iso"};
        for (const auto& e : BINARY_EXTS) {
            if (ext_lower == e)
                return true;
        }
    } catch (...) {
    }
    return false;
}

void FzfPopup::filterFiles() {
    filtered_files_.clear();
    filtered_display_paths_.clear();
    for (size_t i = 0; i < all_files_.size(); ++i) {
        const auto& path = all_files_[i];
        std::string filename = std::filesystem::path(path).filename().string();
        if (fuzzyMatch(path, input_) || fuzzyMatch(filename, input_)) {
            filtered_files_.push_back(path);
            filtered_display_paths_.push_back(i < all_display_paths_.size() ? all_display_paths_[i]
                                                                            : filename);
        }
    }
    selected_index_ = 0;
    scroll_offset_ = 0;
    preview_page_ = 0;             // 过滤变化时重置预览页
    preview_h_offset_ = 0;         // 过滤变化时重置水平偏移
    image_preview_loaded_ = false; // 过滤变化时重置图片预览
    if (selected_index_ >= filtered_files_.size() && !filtered_files_.empty()) {
        selected_index_ = filtered_files_.size() - 1;
    }
}

std::string FzfPopup::readFilePreview(const std::string& filepath, size_t max_lines,
                                      size_t skip_lines) const {
    std::ifstream file(filepath);
    if (!file)
        return "(Unable to read file)";
    std::ostringstream oss;
    std::string line;
    size_t skipped = 0;
    size_t count = 0;
    while (std::getline(file, line)) {
        if (skipped < skip_lines) {
            skipped++;
            continue;
        }
        if (count >= max_lines)
            break;
        // 简单检测二进制：含有过多非打印字符
        size_t non_print = 0;
        for (unsigned char c : line) {
            if (c < 32 && c != '\t' && c != '\n' && c != '\r')
                non_print++;
        }
        if (non_print > line.size() / 4) {
            oss << "(Binary or cannot preview)\n";
            break;
        }
        oss << line << '\n';
        count++;
    }
    return oss.str();
}

std::string FzfPopup::getFileTypeForPath(const std::string& filepath) const {
    try {
        std::filesystem::path p(filepath);
        std::string ext = p.extension().string();
        if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);
        return utils::FileTypeDetector::detectFileType(filepath, ext);
    } catch (...) {
        return "text";
    }
}

std::string FzfPopup::getFileIcon(const std::string& filepath) const {
    try {
        std::filesystem::path p(filepath);
        std::string ext = p.extension().string();
        if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);
        return icon_mapper_.getIcon(ext);
    } catch (...) {
        return icon_mapper_.getIcon("default");
    }
}

ftxui::Color FzfPopup::getFileColor(const std::string& filepath) const {
    try {
        std::string name = std::filesystem::path(filepath).filename().string();
        return color_mapper_.getFileColor(name, false);
    } catch (...) {
        return ftxui::Color::RGB(140, 135, 120);
    }
}

Element FzfPopup::render() {
    if (!is_open_) {
        return text("");
    }

    const auto& colors = theme_.getColors();
    Elements dialog_content;

    dialog_content.push_back(renderTitle());
    dialog_content.push_back(separator());
    dialog_content.push_back(renderInputBox());
    dialog_content.push_back(separator());

    // 左右布局：文件列表 | 预览
    Elements main_row;
    main_row.push_back(renderFileList() | size(WIDTH, EQUAL, 70));
    main_row.push_back(separator());
    main_row.push_back(renderPreview() | flex);

    dialog_content.push_back(hbox(main_row) | flex);
    dialog_content.push_back(separator());
    dialog_content.push_back(renderFileInfoBar());
    dialog_content.push_back(separator());
    dialog_content.push_back(renderHelpBar());

    int height = std::min(35, int(12 + static_cast<int>(list_display_count_)));

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 150) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

Element FzfPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(pnana::ui::icons::SEARCH) | color(colors.success),
                 text(" FZF - Fuzzy File Find "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element FzfPopup::renderInputBox() const {
    const auto& colors = theme_.getColors();
    // 输入框：左侧内容 + 块状光标 + 右侧内容，光标颜色跟随配置
    std::string left = input_.substr(0, cursor_pos_);
    std::string right = input_.substr(cursor_pos_);
    ftxui::Color cursor_color = cursor_color_getter_ ? cursor_color_getter_() : colors.success;

    Element input_line = hbox({
        text("  > "),
        text(left) | color(colors.dialog_fg),
        text("█") | color(cursor_color) | bold,
        text(right) | color(colors.dialog_fg),
    });
    return input_line | bgcolor(colors.selection);
}

Element FzfPopup::renderFileList() const {
    const auto& colors = theme_.getColors();
    Elements list_elements;

    if (is_loading_) {
        list_elements.push_back(
            hbox({text("  "), text(pnana::ui::icons::REFRESH) | color(colors.function),
                  text(" Loading files... ") | color(colors.comment) | dim}));
        return vbox(list_elements);
    }

    size_t max_display = std::min(filtered_files_.size(), list_display_count_);
    size_t start = scroll_offset_;
    if (filtered_files_.size() > list_display_count_ &&
        selected_index_ >= scroll_offset_ + list_display_count_) {
        start = selected_index_ - list_display_count_ + 1;
    } else if (selected_index_ < scroll_offset_) {
        start = selected_index_;
    }

    if (filtered_files_.empty()) {
        list_elements.push_back(
            hbox({text("  "), text("No files match") | color(colors.comment) | dim}));
    } else {
        for (size_t i = 0; i < max_display && (start + i) < filtered_files_.size(); ++i) {
            size_t idx = start + i;
            const auto& filepath = filtered_files_[idx];
            bool is_selected = (idx == selected_index_);

            std::string icon = getFileIcon(filepath);
            ftxui::Color file_color = getFileColor(filepath);
            std::string display_path;
            if (idx < filtered_display_paths_.size()) {
                display_path = filtered_display_paths_[idx];
            } else {
                try {
                    display_path = std::filesystem::path(filepath).filename().string();
                } catch (...) {
                    display_path = filepath;
                }
            }

            Elements row;
            row.push_back(text("  "));
            if (is_selected) {
                row.push_back(text("► ") | color(colors.success) | bold);
            } else {
                row.push_back(text("  "));
            }
            row.push_back(text(icon + " ") | color(file_color));
            ftxui::Color path_color = is_selected ? colors.dialog_fg : colors.foreground;
            row.push_back(utils::highlightMatch(display_path, input_, path_color, colors.keyword) |
                          (is_selected ? bold : ftxui::nothing));

            Element line = hbox(row);
            if (is_selected) {
                line = line | bgcolor(colors.selection);
            }
            list_elements.push_back(line);
        }
    }

    return vbox(list_elements);
}

Element FzfPopup::renderPreview() const {
    const auto& colors = theme_.getColors();

    if (is_loading_) {
        return hbox({text("  "), text("Loading...") | color(colors.comment) | dim}) |
               bgcolor(colors.background);
    }

    if (filtered_files_.empty()) {
        return hbox({text("  "), text("Type to filter files") | color(colors.comment) | dim}) |
               bgcolor(colors.background);
    }

    if (selected_index_ >= filtered_files_.size()) {
        return hbox({text("  "), text("No selection") | color(colors.comment) | dim}) |
               bgcolor(colors.background);
    }

    const std::string& filepath = filtered_files_[selected_index_];

    // 图片文件：使用 ImagePreview 渲染
    if (isImageFile(filepath)) {
        if (image_preview_loaded_) {
            return image_preview_.render() | flex;
        } else {
            // 正在加载中或加载失败
            return hbox({text("  "),
                         text("Loading image preview...") | color(colors.comment) | dim}) |
                   bgcolor(colors.background) | center;
        }
    }

    // 二进制等不可预览文件显示提示
    if (isNonPreviewableFile(filepath)) {
        return hbox({text("  "),
                     text("This file cannot be previewed") | color(colors.comment) | dim}) |
               bgcolor(colors.background) | center;
    }

    const size_t skip_lines = preview_page_ * PREVIEW_LINES_PER_PAGE;
    std::string content = readFilePreview(filepath, PREVIEW_LINES_PER_PAGE, skip_lines);

    std::vector<std::string> content_lines;
    std::istringstream iss_content(content);
    std::string ln;
    while (std::getline(iss_content, ln))
        content_lines.push_back(std::move(ln));

    // 设置语法高亮类型
    syntax_highlighter_->setFileType(getFileTypeForPath(filepath));
    syntax_highlighter_->resetMultiLineState();

    Elements lines;
    size_t line_no = skip_lines + 1; // 行号从当前页起始行开始
    const size_t LINE_NUM_WIDTH = 4; // 右对齐行号，与代码区一致
    for (const auto& line : content_lines) {
        std::string visible = line;
        if (preview_h_offset_ < visible.size()) {
            visible = visible.substr(preview_h_offset_);
        } else {
            visible.clear();
        }

        Element hl_line = syntax_highlighter_->highlightLine(visible);
        std::string line_str = std::to_string(line_no);
        while (line_str.length() < LINE_NUM_WIDTH) {
            line_str = " " + line_str;
        }
        Elements line_elements;
        line_elements.push_back(text(" " + line_str + " ") | color(colors.comment) | dim);
        line_elements.push_back(hl_line);
        lines.push_back(hbox(line_elements));
        line_no++;
    }

    return vbox(lines) | bgcolor(colors.background) | yflex;
}

Element FzfPopup::renderFileInfoBar() const {
    const auto& colors = theme_.getColors();

    if (is_loading_) {
        return hbox({text(" "), text("Loading...") | color(colors.comment) | bold}) |
               bgcolor(colors.dialog_bg);
    }

    if (filtered_files_.empty()) {
        return hbox({text(" No files match") | color(colors.comment) | bold}) |
               bgcolor(colors.dialog_bg);
    }

    if (selected_index_ >= filtered_files_.size()) {
        return hbox({text(" No selection") | color(colors.comment) | bold}) |
               bgcolor(colors.dialog_bg);
    }

    const std::string& filepath = filtered_files_[selected_index_];

    // 使用工具类获取文件大小和权限
    auto size_info = utils::getFileSize(filepath);
    auto perm_info = utils::getFilePermission(filepath);

    // 构建状态栏元素 - 使用下划线和粗体效果，不同部分使用不同颜色
    Elements elements = {text(" "),
                         text(size_info.formatted_size) | color(colors.keyword) | bold | underlined,
                         text("  "),
                         text(perm_info.type) | color(colors.foreground) | bold | underlined,
                         text(perm_info.owner) | color(colors.function) | bold | underlined,
                         text(perm_info.group) | color(colors.keyword) | bold | underlined,
                         text(perm_info.others) | color(colors.comment) | bold | underlined,
                         filler()};

    return hbox(elements) | bgcolor(colors.dialog_bg);
}

Element FzfPopup::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
                 text("PgUp/PgDn") | color(colors.helpbar_key) | bold, text(": Page scroll  "),
                 text("Enter") | color(colors.helpbar_key) | bold, text(": Open  "),
                 text("Tab") | color(colors.helpbar_key) | bold, text(": Preview horizontal  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Cancel  "),
                 text("Type") | color(colors.helpbar_key) | bold, text(": Filter")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

bool FzfPopup::handleInput(ftxui::Event event) {
    if (!is_open_)
        return false;

    if (event == ftxui::Event::Escape) {
        close();
        return true;
    }

    if (event == ftxui::Event::Return) {
        if (file_open_callback_ && selected_index_ < filtered_files_.size()) {
            file_open_callback_(filtered_files_[selected_index_]);
        }
        close();
        return true;
    }

    if (event == ftxui::Event::ArrowDown) {
        if (!filtered_files_.empty()) {
            selected_index_ = (selected_index_ + 1) % filtered_files_.size();
            if (selected_index_ >= scroll_offset_ + list_display_count_) {
                scroll_offset_ = selected_index_ - list_display_count_ + 1;
            }
            preview_page_ = 0; // 切换选中文件时重置预览页
            preview_h_offset_ = 0;

            // 如果选中图片文件，加载预览
            const std::string& filepath = filtered_files_[selected_index_];
            if (isImageFile(filepath)) {
                int preview_width = 40;
                int max_height = 20;
                image_preview_loaded_ =
                    image_preview_.loadImage(filepath, preview_width, max_height);
            } else {
                image_preview_loaded_ = false;
            }
        }
        return true;
    }

    if (event == ftxui::Event::ArrowUp) {
        if (!filtered_files_.empty()) {
            if (selected_index_ == 0) {
                selected_index_ = filtered_files_.size() - 1;
                scroll_offset_ = (selected_index_ >= list_display_count_)
                                     ? selected_index_ - list_display_count_ + 1
                                     : 0;
            } else {
                selected_index_--;
                if (selected_index_ < scroll_offset_) {
                    scroll_offset_ = selected_index_;
                }
            }
            preview_page_ = 0; // 切换选中文件时重置预览页

            // 如果选中图片文件，加载预览
            const std::string& filepath = filtered_files_[selected_index_];
            if (isImageFile(filepath)) {
                int preview_width = 40;
                int max_height = 20;
                image_preview_loaded_ =
                    image_preview_.loadImage(filepath, preview_width, max_height);
            } else {
                image_preview_loaded_ = false;
            }
        }
        return true;
    }

    // Page Down: 预览面板向下翻页
    if (event == ftxui::Event::PageDown) {
        if (!filtered_files_.empty() && selected_index_ < filtered_files_.size()) {
            const std::string& filepath = filtered_files_[selected_index_];
            if (!isNonPreviewableFile(filepath)) {
                // 检查是否还有下一页
                std::string content = readFilePreview(filepath, PREVIEW_LINES_PER_PAGE,
                                                      (preview_page_ + 1) * PREVIEW_LINES_PER_PAGE);
                if (content.empty()) {
                    // 已经到底部，回到顶部
                    preview_page_ = 0;
                } else {
                    // 还有下一页
                    preview_page_++;
                }
            } else {
                // 不可预览的文件，切换到下一个文件
                if (selected_index_ + 1 < filtered_files_.size()) {
                    selected_index_++;
                    if (selected_index_ >= scroll_offset_ + list_display_count_) {
                        scroll_offset_ = selected_index_ - list_display_count_ + 1;
                    }
                    preview_page_ = 0;
                }
            }
        }
        return true;
    }

    // Page Up: 预览面板向上翻页
    if (event == ftxui::Event::PageUp) {
        if (!filtered_files_.empty() && selected_index_ < filtered_files_.size()) {
            const std::string& filepath = filtered_files_[selected_index_];
            if (!isNonPreviewableFile(filepath)) {
                // 预览面板翻页
                if (preview_page_ == 0) {
                    // 已经在顶部，需要找到最后一页
                    size_t total_lines = 0;
                    std::ifstream file(filepath);
                    if (file) {
                        std::string line;
                        while (std::getline(file, line)) {
                            total_lines++;
                        }
                    }
                    // 计算总页数
                    if (total_lines > 0) {
                        preview_page_ = (total_lines - 1) / PREVIEW_LINES_PER_PAGE;
                    }
                } else {
                    preview_page_--;
                }
            } else {
                // 不可预览的文件，切换到上一个文件
                if (selected_index_ > 0) {
                    selected_index_--;
                    if (selected_index_ < scroll_offset_) {
                        scroll_offset_ = selected_index_;
                    }
                    preview_page_ = 0;
                }
            }
        }
        return true;
    }

    // Tab: 预览横向滚动（到达边界后回到起始位置）
    if (event == ftxui::Event::Tab) {
        if (!filtered_files_.empty() && selected_index_ < filtered_files_.size()) {
            const std::string& filepath = filtered_files_[selected_index_];
            if (!isNonPreviewableFile(filepath)) {
                std::string content = readFilePreview(filepath, PREVIEW_LINES_PER_PAGE, 0);
                size_t max_len = 0;
                std::istringstream iss(content);
                std::string line;
                while (std::getline(iss, line)) {
                    max_len = std::max(max_len, line.size());
                }
                if (max_len <= preview_h_step_) {
                    preview_h_offset_ = 0;
                } else if (preview_h_offset_ + preview_h_step_ >= max_len) {
                    preview_h_offset_ = 0; // 到边界后回到起始
                } else {
                    preview_h_offset_ += preview_h_step_;
                }
            }
        }
        return true;
    }

    // Shift+Tab: 预览上一页
    if (event == ftxui::Event::TabReverse) {
        if (preview_page_ > 0)
            preview_page_--;
        return true;
    }

    if (event == ftxui::Event::Backspace) {
        if (cursor_pos_ > 0) {
            input_.erase(cursor_pos_ - 1, 1);
            cursor_pos_--;
            filterFiles();
        }
        return true;
    }

    if (event == ftxui::Event::Delete) {
        if (cursor_pos_ < input_.size()) {
            input_.erase(cursor_pos_, 1);
            filterFiles();
        }
        return true;
    }

    if (event == ftxui::Event::ArrowLeft) {
        if (cursor_pos_ > 0)
            cursor_pos_--;
        return true;
    }

    if (event == ftxui::Event::ArrowRight) {
        if (cursor_pos_ < input_.size())
            cursor_pos_++;
        return true;
    }

    if (event == ftxui::Event::Home) {
        cursor_pos_ = 0;
        return true;
    }

    if (event == ftxui::Event::End) {
        cursor_pos_ = input_.size();
        return true;
    }

    if (event.is_character()) {
        std::string ch = event.character();
        if (!ch.empty()) {
            char c = ch[0];
            if (c >= 32 && c < 127) {
                input_.insert(cursor_pos_, ch);
                cursor_pos_ += ch.size();
                filterFiles();
            }
        }
        return true;
    }

    return false;
}

} // namespace ui
} // namespace pnana
