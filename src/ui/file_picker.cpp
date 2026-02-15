#include "ui/file_picker.h"
#include "ui/icons.h"
#include "utils/file_type_detector.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

namespace fs = std::filesystem;

FilePicker::FilePicker(Theme& theme)
    : theme_(theme), visible_(false), picker_type_(FilePickerType::BOTH), selected_index_(0),
      show_filter_(false), focus_in_search_(false), show_path_input_(false),
      type_filter_active_(false), current_type_filter_(FilePickerType::BOTH), icon_mapper_(),
      color_mapper_(theme), cached_path_("") {}

void FilePicker::show(const std::string& start_path, FilePickerType type,
                      std::function<void(const std::string&)> on_select,
                      std::function<void()> on_cancel) {
    picker_type_ = type;
    current_path_ = start_path;
    try {
        if (fs::exists(current_path_) && fs::is_directory(current_path_)) {
            current_path_ = fs::canonical(current_path_).string();
        } else {
            current_path_ = fs::current_path().string();
        }
    } catch (...) {
        current_path_ = fs::current_path().string();
    }

    on_select_ = on_select;
    on_cancel_ = on_cancel;
    filter_input_ = "";
    path_input_ = "";
    show_filter_ = false;
    focus_in_search_ = false;
    show_path_input_ = false;
    type_filter_active_ = false;
    current_type_filter_ = type;
    selected_index_ = 0;
    visible_ = true;
    loadDirectory();
}

bool FilePicker::handleInput(ftxui::Event event) {
    if (!visible_)
        return false;

    // 路径输入模式（优先级最高）
    if (show_path_input_) {
        if (event == Event::Escape) {
            show_path_input_ = false;
            path_input_ = "";
            return true;
        } else if (event == Event::Return) {
            updatePathFromInput();
            show_path_input_ = false;
            path_input_ = "";
            loadDirectory();
            return true;
        } else if (event == Event::Tab) {
            // Tab 键自动补全路径
            completePath();
            return true;
        } else if (event == Event::Backspace) {
            if (!path_input_.empty()) {
                path_input_.pop_back();
                updatePathFromInput(); // 实时更新路径
            }
            return true;
        } else if (event.is_character()) {
            std::string ch = event.character();
            // 支持完整的UTF-8字符输入，包括中文等多字节字符
            if (!ch.empty() && ch[0] >= 32) { // 过滤控制字符
                path_input_ += ch;
                updatePathFromInput(); // 实时更新路径
            }
            return true;
        }
        return false;
    }

    // 搜索框焦点模式
    if (focus_in_search_) {
        if (event == Event::Escape) {
            // Escape 清空搜索并切换到列表
            filter_input_ = "";
            focus_in_search_ = false;
            loadDirectory();
            return true;
        } else if (event == Event::ArrowUp || event == Event::ArrowDown) {
            // 上下键切换到列表
            focus_in_search_ = false;
            // 确保选中索引有效
            if (items_.empty()) {
                selected_index_ = 0;
            } else if (selected_index_ >= items_.size()) {
                selected_index_ = items_.size() - 1;
            }
            return true;
        } else if (event == Event::Return) {
            // Enter 键切换到列表
            focus_in_search_ = false;
            // 确保选中索引有效
            if (items_.empty()) {
                selected_index_ = 0;
            } else if (selected_index_ >= items_.size()) {
                selected_index_ = 0;
            }
            return true;
        } else if (event == Event::Backspace) {
            if (!filter_input_.empty()) {
                filter_input_.pop_back();
                loadDirectory(); // 实时更新列表
            }
            return true;
        } else if (event.is_character()) {
            std::string ch = event.character();
            // 支持完整的UTF-8字符输入，包括中文等多字节字符
            if (!ch.empty() && ch[0] >= 32) { // 过滤控制字符
                filter_input_ += ch;
                loadDirectory(); // 实时更新列表
            }
            return true;
        }
        return false;
    }

    // 列表焦点模式（正常模式）
    if (event == Event::Escape) {
        cancel();
        return true;
    } else if (event == Event::Return) {
        selectItem();
        return true;
    } else if (event == Event::ArrowUp) {
        // 如果已在列表顶部，上键切换到搜索框
        if (selected_index_ == 0) {
            focus_in_search_ = true;
            return true;
        }
        navigateUp();
        return true;
    } else if (event == Event::ArrowDown) {
        navigateDown();
        return true;
    } else if (event == Event::Backspace) {
        // 返回上级目录
        try {
            if (current_path_ != "/") {
                fs::path parent = fs::path(current_path_).parent_path();
                current_path_ = parent.string();
                loadDirectory();
            }
        } catch (...) {
        }
        return true;
    } else if (event == Event::CtrlF) {
        // Ctrl+F 切换到搜索框
        focus_in_search_ = true;
        return true;
    } else if (event == Event::Character(':') || event == Event::Character('/')) {
        // : 或 / 进入路径输入模式
        show_path_input_ = true;
        path_input_ = current_path_;
        return true;
    } else if (event == Event::Tab) {
        // Tab 键切换类型筛选（文件/文件夹/全部）
        if (!type_filter_active_) {
            type_filter_active_ = true;
        }

        // 循环切换：BOTH -> FILE -> FOLDER -> BOTH
        if (current_type_filter_ == FilePickerType::BOTH) {
            current_type_filter_ = FilePickerType::FILE;
        } else if (current_type_filter_ == FilePickerType::FILE) {
            current_type_filter_ = FilePickerType::FOLDER;
        } else {
            current_type_filter_ = FilePickerType::BOTH;
        }

        picker_type_ = current_type_filter_;
        loadDirectory();
        return true;
    } else if (event.is_character()) {
        // 在列表模式下输入字符，切换到搜索框并开始输入
        std::string ch = event.character();
        if (!ch.empty() && ch[0] >= 32) { // 过滤控制字符
            focus_in_search_ = true;
            filter_input_ = ch;
            loadDirectory(); // 实时更新列表
            return true;
        }
    }

    return false;
}

Element FilePicker::render() {
    if (!visible_)
        return text("");

    auto& colors = theme_.getColors();

    Elements content;

    // 标题栏
    std::string title = "File Picker";
    std::string type_label = "[All]";
    if (picker_type_ == FilePickerType::FILE) {
        title = "Select File";
        type_label = "[Files Only]";
    } else if (picker_type_ == FilePickerType::FOLDER) {
        title = "Select Folder";
        type_label = "[Folders Only]";
    }

    content.push_back(hbox({text(" "), text(ui::icons::SEARCH) | color(Color::Cyan), text(" "),
                            text(title) | bold | color(colors.foreground), text(" "),
                            text(type_label) | color(colors.comment), filler(),
                            text(" ") | color(colors.comment)}) |
                      bgcolor(colors.menubar_bg));

    content.push_back(separator());

    // 路径输入框（如果启用）
    if (show_path_input_) {
        content.push_back(
            hbox({text(" Path: ") | color(colors.keyword) | bold,
                  text(path_input_ + "_") | color(colors.foreground) | bgcolor(colors.selection)}));
        content.push_back(separator());
    }

    // 当前路径
    content.push_back(hbox({text(" "), text(ui::icons::LOCATION) | color(colors.keyword), text(" "),
                            text(current_path_) | color(colors.comment)}));

    content.push_back(separator());

    // 搜索框（始终可见）
    Element search_box;
    if (focus_in_search_) {
        // 焦点在搜索框时高亮显示
        search_box = hbox({text(" "), text(ui::icons::SEARCH) | color(colors.keyword) | bold,
                           text(" Search: ") | color(colors.keyword) | bold,
                           text(filter_input_ + "_") | color(colors.foreground) |
                               bgcolor(colors.selection)});
    } else {
        // 焦点不在搜索框时正常显示
        std::string search_display = filter_input_.empty() ? "Type to search..." : filter_input_;
        Color search_color = filter_input_.empty() ? colors.comment : colors.foreground;
        search_box = hbox({text(" "), text(ui::icons::SEARCH) | color(colors.comment),
                           text(" Search: ") | color(colors.comment),
                           text(search_display) | color(search_color)});
    }
    content.push_back(search_box);
    content.push_back(separator());

    // 文件列表
    size_t visible_count = 15; // 显示15行
    size_t visible_start = 0;

    if (selected_index_ >= visible_start + visible_count) {
        visible_start = selected_index_ - visible_count + 1;
    }
    if (selected_index_ < visible_start) {
        visible_start = selected_index_;
    }

    for (size_t i = visible_start; i < items_.size() && i < visible_start + visible_count; ++i) {
        std::string item_path = items_[i];
        std::string item_name = getItemName(item_path);

        // 使用缓存获取元数据
        FileItemMetadata metadata = getItemMetadata(item_path, item_name);

        std::string icon = metadata.icon;
        Color item_color = metadata.color;
        bool is_dir = metadata.is_dir;

        if (is_dir) {
            item_name += "/";
        }

        // Icon and name color (use white when selected, otherwise use file type color)
        Color icon_color = (i == selected_index_) ? Color::White : item_color;
        Color name_color = (i == selected_index_) ? Color::White : item_color;

        Elements row_elements = {
            text(" "), text(icon) | color(icon_color) | ((i == selected_index_) ? bold : nothing),
            text(" "),
            text(item_name) | color(name_color) | ((i == selected_index_) ? bold : nothing),
            filler()};

        auto item_text = hbox(row_elements);

        if (i == selected_index_) {
            item_text = item_text | bgcolor(colors.selection);
        } else {
            item_text = item_text | bgcolor(colors.background);
        }

        content.push_back(item_text);
    }

    // 填充空行
    while (content.size() < 20) {
        content.push_back(text(""));
    }

    content.push_back(separator());

    // 底部提示
    Elements hints = {text(" "),  text("↑↓: Navigate/Switch") | color(colors.comment),
                      text("  "), text("Enter: Select") | color(colors.comment),
                      text("  "), text("Tab: Type Filter") | color(colors.comment),
                      text("  "), text(":/: Path Input") | color(colors.comment),
                      text("  "), text("Type: Search") | color(colors.comment),
                      text("  "), text("Esc: Cancel") | color(colors.comment),
                      filler()};
    content.push_back(hbox(hints) | bgcolor(colors.menubar_bg));

    return vbox(content) | borderWithColor(colors.dialog_border) | bgcolor(colors.background) |
           size(WIDTH, GREATER_THAN, 60) | size(HEIGHT, GREATER_THAN, 20) | center;
}

void FilePicker::reset() {
    visible_ = false;
    items_.clear();
    selected_index_ = 0;
    filter_input_ = "";
    path_input_ = "";
    show_filter_ = false;
    focus_in_search_ = false;
    show_path_input_ = false;
    type_filter_active_ = false;
    current_type_filter_ = FilePickerType::BOTH;
    clearMetadataCache();
    on_select_ = nullptr;
    on_cancel_ = nullptr;
}

void FilePicker::loadDirectory() {
    items_.clear();

    // 如果路径改变，清除缓存
    if (current_path_ != cached_path_) {
        clearMetadataCache();
        cached_path_ = current_path_;
    }

    try {
        if (!fs::exists(current_path_) || !fs::is_directory(current_path_)) {
            return;
        }

        std::vector<std::string> dirs;
        std::vector<std::string> files;

        for (const auto& entry : fs::directory_iterator(current_path_)) {
            std::string path = entry.path().string();

            // 根据类型筛选
            if (picker_type_ == FilePickerType::FILE && entry.is_directory()) {
                continue;
            }
            if (picker_type_ == FilePickerType::FOLDER && !entry.is_directory()) {
                continue;
            }

            // 应用过滤
            std::string name = entry.path().filename().string();
            if (!filter_input_.empty()) {
                std::string name_lower = name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                std::string filter_lower = filter_input_;
                std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(),
                               ::tolower);
                if (name_lower.find(filter_lower) == std::string::npos) {
                    continue;
                }
            }

            if (entry.is_directory()) {
                dirs.push_back(path);
            } else {
                files.push_back(path);
            }
        }

        // 排序
        std::sort(dirs.begin(), dirs.end());
        std::sort(files.begin(), files.end());

        // 合并：目录在前，文件在后
        items_.insert(items_.end(), dirs.begin(), dirs.end());
        items_.insert(items_.end(), files.begin(), files.end());

        // 确保选中索引有效
        if (selected_index_ >= items_.size() && !items_.empty()) {
            selected_index_ = items_.size() - 1;
        }
        if (items_.empty()) {
            selected_index_ = 0;
        }

    } catch (const std::exception&) {
        // 读取失败
    }
}

void FilePicker::navigateUp() {
    if (selected_index_ > 0) {
        selected_index_--;
    }
}

void FilePicker::navigateDown() {
    if (!items_.empty() && selected_index_ < items_.size() - 1) {
        selected_index_++;
    }
}

void FilePicker::selectItem() {
    if (items_.empty() || selected_index_ >= items_.size()) {
        return;
    }

    std::string selected = items_[selected_index_];

    // 如果是目录
    if (isDirectory(selected)) {
        // 如果类型筛选是 FOLDER，选择文件夹并返回
        if (picker_type_ == FilePickerType::FOLDER) {
            if (on_select_) {
                on_select_(selected);
            }
            visible_ = false;
            return;
        }
        // 否则进入目录
        current_path_ = selected;
        selected_index_ = 0;
        loadDirectory();
        return;
    }

    // 如果是文件，调用回调
    if (on_select_) {
        on_select_(selected);
    }
    visible_ = false;
}

void FilePicker::cancel() {
    if (on_cancel_) {
        on_cancel_();
    }
    visible_ = false;
}

bool FilePicker::isDirectory(const std::string& path) const {
    try {
        return fs::is_directory(path);
    } catch (...) {
        return false;
    }
}

std::string FilePicker::getItemName(const std::string& path) const {
    try {
        return fs::path(path).filename().string();
    } catch (...) {
        return path;
    }
}

std::string FilePicker::getFileExtension(const std::string& filename) const {
    try {
        fs::path file_path(filename);
        std::string extension = file_path.extension().string();
        // 移除扩展名前的点
        if (!extension.empty() && extension[0] == '.') {
            return extension.substr(1);
        }
        return extension;
    } catch (...) {
        return "";
    }
}

std::vector<std::string> FilePicker::filterItems(const std::vector<std::string>& items,
                                                 const std::string& filter) const {
    if (filter.empty()) {
        return items;
    }

    std::vector<std::string> filtered;
    std::string filter_lower = filter;
    std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);

    for (const auto& item : items) {
        std::string name = getItemName(item);
        std::string name_lower = name;
        std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

        if (name_lower.find(filter_lower) != std::string::npos) {
            filtered.push_back(item);
        }
    }

    return filtered;
}

void FilePicker::updatePathFromInput() {
    if (path_input_.empty()) {
        return;
    }

    try {
        std::string resolved_path = resolvePath(path_input_);

        // 如果路径存在且是目录，更新当前路径
        if (fs::exists(resolved_path) && fs::is_directory(resolved_path)) {
            current_path_ = fs::canonical(resolved_path).string();
            selected_index_ = 0;
            loadDirectory();
        }
        // 如果路径存在且是文件，可以选择该文件
        else if (fs::exists(resolved_path) && fs::is_regular_file(resolved_path)) {
            // 进入文件所在目录
            current_path_ = fs::canonical(resolved_path).parent_path().string();
            selected_index_ = 0;
            loadDirectory();

            // 尝试选中该文件
            std::string filename = fs::path(resolved_path).filename().string();
            for (size_t i = 0; i < items_.size(); ++i) {
                if (getItemName(items_[i]) == filename) {
                    selected_index_ = i;
                    break;
                }
            }
        }
        // 如果路径不存在，但父目录存在，进入父目录
        else {
            fs::path input_path(path_input_);
            if (input_path.has_parent_path()) {
                fs::path parent = input_path.parent_path();
                if (fs::exists(parent) && fs::is_directory(parent)) {
                    current_path_ = fs::canonical(parent).string();
                    selected_index_ = 0;
                    loadDirectory();
                }
            }
        }
    } catch (...) {
        // 路径解析失败，保持当前路径不变
    }
}

std::string FilePicker::resolvePath(const std::string& input_path) const {
    try {
        fs::path path(input_path);

        // 如果是绝对路径，直接返回
        if (path.is_absolute()) {
            return path.string();
        }

        // 如果是相对路径，基于当前路径解析
        fs::path base(current_path_);
        fs::path resolved = base / path;

        // 规范化路径（处理 .. 和 .）
        return resolved.string();
    } catch (...) {
        return input_path;
    }
}

void FilePicker::completePath() {
    if (path_input_.empty()) {
        return;
    }

    try {
        std::string input_str = path_input_;
        bool ends_with_slash =
            (!input_str.empty() && (input_str.back() == '/' || input_str.back() == '\\'));

        // 移除末尾的路径分隔符以便解析
        if (ends_with_slash && input_str.length() > 1) {
            input_str.pop_back();
        }

        fs::path input_path(input_str);
        fs::path base_dir;
        std::string partial_name;
        bool is_absolute = input_path.is_absolute();

        // 解析路径，找到基础目录和部分名称
        if (input_path.has_parent_path() && !input_path.filename().empty()) {
            // 有父路径，提取父目录和最后一个部分
            base_dir = input_path.parent_path();
            partial_name = input_path.filename().string();

            // 如果是相对路径，需要基于当前路径解析
            if (!is_absolute) {
                fs::path current_base(current_path_);
                base_dir = current_base / base_dir;
            }
        } else if (ends_with_slash || input_path.filename().empty()) {
            // 路径以分隔符结尾，表示要补全目录中的内容
            base_dir = input_path;
            partial_name = "";

            if (!is_absolute) {
                fs::path current_base(current_path_);
                base_dir = current_base / base_dir;
            }
        } else {
            // 没有父路径，使用当前目录作为基础
            base_dir = is_absolute ? fs::path("/") : fs::path(current_path_);
            partial_name = input_path.string();
        }

        // 规范化基础目录
        if (fs::exists(base_dir) && fs::is_directory(base_dir)) {
            base_dir = fs::canonical(base_dir);
        } else {
            // 基础目录不存在，无法补全
            return;
        }

        // 在基础目录中查找匹配的文件/目录
        std::vector<std::string> matches;

        try {
            for (const auto& entry : fs::directory_iterator(base_dir)) {
                std::string name = entry.path().filename().string();

                // 如果部分名称为空，显示所有项（但通常不会发生）
                if (partial_name.empty()) {
                    matches.push_back(name);
                } else {
                    // 检查是否匹配部分名称（不区分大小写）
                    if (name.length() >= partial_name.length()) {
                        std::string name_prefix = name.substr(0, partial_name.length());
                        std::string partial_lower = partial_name;
                        std::string name_prefix_lower = name_prefix;

                        // 转换为小写进行比较（类似 shell 的行为）
                        std::transform(partial_lower.begin(), partial_lower.end(),
                                       partial_lower.begin(), ::tolower);
                        std::transform(name_prefix_lower.begin(), name_prefix_lower.end(),
                                       name_prefix_lower.begin(), ::tolower);

                        if (name_prefix_lower == partial_lower) {
                            matches.push_back(name);
                        }
                    }
                }
            }
        } catch (...) {
            // 读取目录失败
            return;
        }

        if (matches.empty()) {
            // 没有匹配项，不做任何操作
            return;
        }

        // 排序匹配项
        std::sort(matches.begin(), matches.end());

        if (matches.size() == 1) {
            // 唯一匹配，直接补全
            std::string completed_name = matches[0];
            fs::path completed_path = base_dir / completed_name;

            // 构建补全后的路径，保持原始路径的格式（绝对/相对）
            fs::path result_path;
            if (input_path.has_parent_path()) {
                result_path = input_path.parent_path() / completed_name;
            } else {
                result_path = fs::path(completed_name);
            }

            // 如果是目录，添加路径分隔符
            if (fs::is_directory(completed_path)) {
                path_input_ = result_path.string() + "/";
            } else {
                path_input_ = result_path.string();
            }
            updatePathFromInput();
        } else {
            // 多个匹配，补全共同前缀
            std::string common_prefix = matches[0];

            // 找到所有匹配项的共同前缀
            for (size_t i = 1; i < matches.size(); ++i) {
                const std::string& current = matches[i];
                size_t j = 0;
                while (j < common_prefix.length() && j < current.length() &&
                       std::tolower(common_prefix[j]) == std::tolower(current[j])) {
                    j++;
                }
                common_prefix = common_prefix.substr(0, j);

                if (common_prefix.empty()) {
                    break;
                }
            }

            // 如果共同前缀比部分名称长，补全到共同前缀
            if (common_prefix.length() > partial_name.length()) {
                fs::path result_path;
                if (input_path.has_parent_path()) {
                    result_path = input_path.parent_path() / common_prefix;
                } else {
                    result_path = fs::path(common_prefix);
                }
                path_input_ = result_path.string();
                updatePathFromInput();
            }
            // 如果共同前缀等于部分名称，不做任何操作（已经有多个匹配）
        }
    } catch (...) {
        // 补全失败，不做任何操作
    }
}

FilePicker::FileItemMetadata FilePicker::getItemMetadata(const std::string& item_path,
                                                         const std::string& item_name) {
    // 检查缓存
    auto it = item_metadata_cache_.find(item_path);
    if (it != item_metadata_cache_.end()) {
        return it->second;
    }

    // 缓存未命中，计算元数据
    FileItemMetadata metadata;
    metadata.is_dir = isDirectory(item_path);

    if (metadata.is_dir) {
        metadata.icon = ui::icons::FOLDER;
        metadata.color = theme_.getColors().function;
        metadata.file_type = "directory";
    } else {
        std::string ext = getFileExtension(item_name);
        metadata.file_type = pnana::utils::FileTypeDetector::detectFileType(item_name, ext);
        metadata.icon = ui::icons::getFileTypeIcon(metadata.file_type);
        metadata.color = color_mapper_.getFileColor(item_name, false);
    }

    // 存入缓存
    item_metadata_cache_[item_path] = metadata;
    return metadata;
}

void FilePicker::clearMetadataCache() {
    item_metadata_cache_.clear();
    cached_path_ = "";
}

} // namespace ui
} // namespace pnana
