#include "ui/fzf_popup.h"
#include "ui/icons.h"
#include "utils/file_type_detector.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
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

FzfPopup::FzfPopup(Theme& theme)
    : theme_(theme), is_open_(false), is_loading_(false), input_(""), cursor_pos_(0),
      root_directory_("."), selected_index_(0), scroll_offset_(0), list_display_count_(18),
      preview_page_(0), color_mapper_(theme) {
    syntax_highlighter_ = std::make_unique<features::SyntaxHighlighter>(theme_);
}

// 静态函数：在后台线程中收集文件，返回 (文件列表, 预计算显示路径, 规范根路径)
// 扫描时已计算 rel_path 做 ignore 判断，复用为 display_path，避免 filterFiles 中重复调用 relative()
static std::tuple<std::vector<std::string>, std::vector<std::string>, std::string>
collectFilesToVector(const std::string& root_directory) {
    std::vector<std::pair<std::string, std::string>> result; // (path, display_path)
    std::string canonical_root;
    try {
        std::filesystem::path root(root_directory);
        if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
            root = std::filesystem::current_path();
        }
        canonical_root = std::filesystem::canonical(root).string();

        auto iter = std::filesystem::recursive_directory_iterator(
            root, std::filesystem::directory_options::skip_permission_denied);

        for (; iter != std::filesystem::recursive_directory_iterator(); ++iter) {
            const auto& entry = *iter;
            try {
                if (entry.is_directory()) {
                    std::string dir_name = entry.path().filename().string();
                    if (shouldIgnoreDir(dir_name)) {
                        iter.disable_recursion_pending();
                        continue;
                    }
                    continue;
                }
                if (!entry.is_regular_file())
                    continue;
                std::string path_str = entry.path().string();
                std::filesystem::path rel_path = std::filesystem::relative(entry.path(), root);
                bool ignored = false;
                for (const auto& comp : rel_path) {
                    if (shouldIgnoreDir(comp.string())) {
                        ignored = true;
                        break;
                    }
                }
                if (ignored)
                    continue;
                std::string display_path = rel_path.empty()
                                               ? std::filesystem::path(path_str).filename().string()
                                               : rel_path.string();
                result.push_back({path_str, std::move(display_path)});
            } catch (...) {
                continue;
            }
        }

        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        std::vector<std::string> files;
        std::vector<std::string> display_paths;
        files.reserve(result.size());
        display_paths.reserve(result.size());
        for (auto& p : result) {
            files.push_back(std::move(p.first));
            display_paths.push_back(std::move(p.second));
        }

        return {std::move(files), std::move(display_paths), std::move(canonical_root)};
    } catch (...) {
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

// 根据扩展名判断是否为不可预览文件（图片、二进制等）
static bool isNonPreviewableFile(const std::string& filepath) {
    try {
        std::string ext = std::filesystem::path(filepath).extension().string();
        if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);
        std::string ext_lower = ext;
        std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

        // 图片
        static const std::vector<std::string> IMAGE_EXTS = {
            "png", "jpg",  "jpeg", "gif", "bmp", "webp", "ico", "tiff", "tif",
            "svg", "svgz", "xpm",  "xbm", "pcx", "tga",  "ppm", "pgm",  "pbm"};
        for (const auto& e : IMAGE_EXTS) {
            if (ext_lower == e)
                return true;
        }
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
    preview_page_ = 0; // 过滤变化时重置预览页
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
            row.push_back(text(display_path) | (is_selected ? color(colors.dialog_fg) | bold
                                                            : color(colors.foreground)));

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

    // 图片、二进制等不可预览文件显示提示
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
        Element hl_line = syntax_highlighter_->highlightLine(line);
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

Element FzfPopup::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
                 text("Enter") | color(colors.helpbar_key) | bold, text(": Open  "),
                 text("Tab") | color(colors.helpbar_key) | bold, text(": Preview next page  "),
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
        }
        return true;
    }

    // Tab: 预览下一页；若已在最后一页则循环回第一页
    if (event == ftxui::Event::Tab) {
        if (!filtered_files_.empty() && selected_index_ < filtered_files_.size()) {
            const std::string& filepath = filtered_files_[selected_index_];
            if (!isNonPreviewableFile(filepath)) {
                const size_t next_skip = (preview_page_ + 1) * PREVIEW_LINES_PER_PAGE;
                std::string next_content = readFilePreview(filepath, 1, next_skip);
                const bool has_next = !next_content.empty() &&
                                      next_content.find("(Binary") == std::string::npos &&
                                      next_content.find("(Unable") == std::string::npos;
                if (has_next)
                    preview_page_++;
                else
                    preview_page_ = 0; // 最后一页再按 Tab 回到第一页
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
