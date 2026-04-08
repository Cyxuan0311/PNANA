#include "ui/file_browser_view.h"
#include "features/file_browser.h"
#include "ui/icons.h"
#include "utils/file_info_utils.h"
#include "utils/file_type_color_mapper.h"
#include "utils/file_type_detector.h"
#include "utils/file_type_icon_mapper.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace ui {

FileBrowserView::FileBrowserView(Theme& theme)
    : theme_(theme), color_mapper_(theme), scroll_offset_(0) {}

// 滚动控制方法
void FileBrowserView::scrollTo(size_t index) {
    scroll_offset_ = index;
}

void FileBrowserView::scrollUp(size_t lines) {
    if (scroll_offset_ >= lines) {
        scroll_offset_ -= lines;
    } else {
        scroll_offset_ = 0;
    }
}

void FileBrowserView::scrollDown(size_t lines) {
    scroll_offset_ += lines;
}

void FileBrowserView::scrollToTop() {
    scroll_offset_ = 0;
}

void FileBrowserView::scrollToBottom() {
    // 这个方法需要在渲染时根据实际项目数量来设置
    // 这里先设置一个大的值，在渲染时会调整
    scroll_offset_ = SIZE_MAX / 2; // 很大的值
}

Element FileBrowserView::render(const features::FileBrowser& browser, int height) {
    auto& colors = theme_.getColors();

    Elements content;

    // 标题栏
    content.push_back(renderHeader(browser));
    content.push_back(separator());

    // 文件列表
    const auto& flat_items = browser.getFlatItems();
    size_t total_items = flat_items.size();
    size_t selected_index = browser.getSelectedIndex();

    // 计算可用高度：总高度 - 标题 (1) - 分隔符 (1) - 底部分隔符 (1) - 文件信息栏 (1) = height - 4
    size_t available_height = static_cast<size_t>(height > 4 ? height - 4 : 1);

    // 根据选中项目调整滚动位置，确保选中项目可见
    // 使用更智能的滚动策略：保持选中项目在中间偏上位置
    size_t target_scroll = scroll_offset_;

    if (selected_index < scroll_offset_) {
        // 选中项目在可见区域上方
        // 将选中项目放在可见区域的中间位置
        size_t ideal_position = available_height / 3; // 选中项目在 1/3 位置
        if (selected_index >= ideal_position) {
            target_scroll = selected_index - ideal_position;
        } else {
            target_scroll = 0;
        }
    } else if (selected_index >= scroll_offset_ + available_height) {
        // 选中项目在可见区域下方
        // 将选中项目放在可见区域的中间位置
        size_t ideal_position = available_height / 3; // 选中项目在 1/3 位置
        target_scroll = selected_index - ideal_position;
    } else {
        // 选中项目已在可见范围内，检查是否需要微调以获得更好的视觉体验
        size_t relative_pos = selected_index - scroll_offset_;
        if (relative_pos == 0 && scroll_offset_ > 0) {
            // 选中项目在顶部，稍微向上滚动以获得上下文
            target_scroll = scroll_offset_ - 1;
        } else if (relative_pos == available_height - 1 &&
                   scroll_offset_ + available_height < total_items) {
            // 选中项目在底部，稍微向下滚动以获得上下文
            target_scroll = scroll_offset_ + 1;
        }
    }

    // 确保滚动偏移量不超出有效范围
    if (target_scroll >= total_items) {
        target_scroll = (total_items > available_height) ? total_items - available_height : 0;
    }
    if (target_scroll > selected_index) {
        target_scroll = selected_index;
    }

    scroll_offset_ = target_scroll;

    // 计算实际可见范围
    size_t visible_start = scroll_offset_;
    size_t visible_end = std::min(scroll_offset_ + available_height, total_items);

    // 渲染文件列表 - 只渲染可见的项目
    Elements file_list_elements;

    for (size_t i = visible_start; i < visible_end; ++i) {
        const features::FileItem* item = flat_items[i];
        if (item) {
            file_list_elements.push_back(
                renderFileItem(item, i, selected_index, flat_items, browser));
        }
    }

    // 将文件列表添加到内容中（不再填充空行，让 vbox 自动处理布局）
    content.push_back(vbox(file_list_elements) | flex);

    // 底部文件信息栏（反白效果）- 固定在底部
    content.push_back(separator());
    content.push_back(renderFileInfoBar(browser));

    return vbox(content) | bgcolor(colors.background);
}

Element FileBrowserView::renderHeader(const features::FileBrowser& browser) const {
    auto& colors = theme_.getColors();
    std::string current_directory = browser.getCurrentDirectory();

    std::string title;
    if (browser.isRemoteMode() && current_directory.size() > 6 &&
        current_directory.compare(0, 6, "ssh://") == 0) {
        size_t path_start = current_directory.find('/', 6);
        if (path_start != std::string::npos) {
            std::string user_host = current_directory.substr(6, path_start - 6);
            std::string path_part = current_directory.substr(path_start);
            title = "SSH: " + user_host + " " + path_part;
        } else {
            title = current_directory;
        }
    } else {
        title = current_directory;
    }

    return hbox({text(" "), text(icons::FOLDER_OPEN) | color(colors.function), text(" "),
                 text(title) | bold | color(colors.foreground), filler(),
                 text(" ") | color(colors.comment)}) |
           bgcolor(colors.menubar_bg);
}

Element FileBrowserView::renderFileList(const features::FileBrowser& browser, size_t visible_start,
                                        size_t visible_count) const {
    Elements content;
    const auto& flat_items = browser.getFlatItems();
    size_t selected_index = browser.getSelectedIndex();

    // 渲染可见的文件项
    for (size_t i = visible_start; i < flat_items.size() && i < visible_start + visible_count;
         ++i) {
        const features::FileItem* item = flat_items[i];
        if (item) {
            // 注意：这里需要 browser 参数，但 renderFileList 没有，需要修改方法签名
            // 暂时使用简化版本
            content.push_back(renderFileItem(item, i, selected_index, flat_items, browser));
        }
    }

    return vbox(content);
}

Element FileBrowserView::renderStatusBar(const features::FileBrowser& browser) const {
    auto& colors = theme_.getColors();

    std::string selected_path_display = "";
    if (browser.hasSelection()) {
        std::string full_path = browser.getSelectedPath();
        selected_path_display = truncateMiddle(full_path, 30);
    } else {
        selected_path_display = "No selection";
    }

    // 显示多选状态
    std::string selection_info = "";
    size_t selected_count = browser.getSelectedCount();
    if (selected_count > 1) {
        selection_info = " [" + std::to_string(selected_count) + " selected]";
    }

    // 显示当前选中路径以及隐藏文件显示状态提示
    std::string hidden_indicator =
        browser.getShowHidden() ? "[Hidden: ON | . to hide]" : "[Hidden: OFF | . to show]";

    return hbox({text(" "), //
                 text(icons::LOCATION) | color(colors.keyword), text(" "),
                 text(selected_path_display) | color(colors.comment),
                 text(selection_info) | color(colors.keyword) | bold, filler(),
                 text(hidden_indicator) | color(colors.comment)}) |
           bgcolor(colors.menubar_bg);
}

Element FileBrowserView::renderFileInfoBar(const features::FileBrowser& browser) const {
    auto& colors = theme_.getColors();

    if (!browser.hasSelection()) {
        return hbox({text(" No file selected") | color(colors.comment) | bold}) |
               bgcolor(colors.selection);
    }

    const auto& flat_items = browser.getFlatItems();
    size_t selected_index = browser.getSelectedIndex();

    if (selected_index >= flat_items.size()) {
        return hbox({text(" Invalid selection") | color(colors.comment) | bold}) |
               bgcolor(colors.selection);
    }

    const features::FileItem* item = flat_items[selected_index];
    if (!item) {
        return hbox({text(" Invalid selection") | color(colors.comment) | bold}) |
               bgcolor(colors.selection);
    }

    std::string size_text = "Unknown";
    std::string perm_text = "---------";

    if (browser.isRemoteMode()) {
        auto remote_info = getRemoteSizeAndPermission(browser, *item);
        size_text = remote_info.first;
        perm_text = remote_info.second;
    } else {
        auto size_info = utils::getFileSize(item->path);
        auto perm_info = utils::getFilePermission(item->path);
        size_text = size_info.formatted_size;
        perm_text = perm_info.full_string;
    }

    // 构建状态栏元素 - SSH 和本地统一展示完整权限字符串
    Elements elements = {text(" "), text(size_text) | color(colors.keyword) | bold | underlined,
                         text("  "), text(perm_text) | color(colors.foreground) | bold | underlined,
                         filler()};

    return hbox(elements) | bgcolor(colors.background);
}

Element FileBrowserView::renderFileItem(const features::FileItem* item, size_t index,
                                        size_t selected_index,
                                        const std::vector<features::FileItem*>& flat_items,
                                        const features::FileBrowser& browser) const {
    auto& colors = theme_.getColors();

    // 根据文件夹展开状态选择图标
    std::string icon = getFileIcon(*item);
    if (item->is_directory && item->expanded && item->name != "..") {
        icon = icons::FOLDER_OPEN; // 展开的文件夹使用打开的文件夹图标
    }
    Color item_color = color_mapper_.getFileColor(item->name, item->is_directory);

    // 构建树形结构连接线
    std::string tree_prefix = buildTreePrefix(item, index, flat_items);

    // 展开/折叠图标和连接线
    std::string expand_prefix = buildExpandPrefix(item, index, flat_items);
    std::string expand_icon = "";

    if (item->is_directory) {
        expand_icon = item->expanded ? "▼" : "▶";
    } else {
        expand_icon = " ";
    }

    std::string display_name = item->name;

    // 多选标记（如果被选中但不是当前焦点）
    std::string selection_marker = "";
    bool is_multi_selected = browser.isSelected(index) && index != selected_index;

    // 构建行元素
    Elements row_elements = {text(" "),
                             text(tree_prefix) | color(colors.comment),
                             text(expand_prefix) | color(colors.comment),
                             text(expand_icon) | color(item_color),
                             text(" "),
                             text(selection_marker) | color(colors.keyword),
                             text(icon) | color(item_color),
                             text(" "),
                             text(display_name) | color(item_color)};

    auto item_text = hbox(row_elements);

    // 选中项高亮
    if (index == selected_index) {
        // 当前焦点项：使用 selection 背景色
        item_text = item_text | bgcolor(colors.selection) | bold;
    } else if (is_multi_selected) {
        // 多选中的项：使用稍微不同的背景色（使用 comment 颜色的半透明效果）
        item_text = item_text | bgcolor(colors.comment) | dim;
    } else {
        item_text = item_text | bgcolor(colors.background);
    }

    return item_text;
}

std::string FileBrowserView::buildTreePrefix(
    const features::FileItem* item, size_t index,
    const std::vector<features::FileItem*>& flat_items) const {
    std::string prefix = "";

    for (int d = 0; d < item->depth; ++d) {
        // 检查这个深度层级是否有后续兄弟节点
        bool has_sibling = false;
        for (size_t j = index + 1; j < flat_items.size(); ++j) {
            if (flat_items[j]->depth == d) {
                has_sibling = true;
                break;
            }
            if (flat_items[j]->depth < d) {
                break;
            }
        }

        if (has_sibling) {
            prefix += "│ "; // 有后续兄弟，显示竖线
        } else {
            prefix += "  "; // 没有后续兄弟，显示空格
        }
    }

    return prefix;
}

std::string FileBrowserView::buildExpandPrefix(
    const features::FileItem* item, size_t index,
    const std::vector<features::FileItem*>& flat_items) const {
    // 检查是否有后续兄弟节点（同深度）
    bool has_sibling = false;
    for (size_t j = index + 1; j < flat_items.size(); ++j) {
        if (flat_items[j]->depth == item->depth) {
            has_sibling = true;
            break;
        }
        if (flat_items[j]->depth < item->depth) {
            break;
        }
    }

    return has_sibling ? "├─" : "└─";
}

std::string FileBrowserView::getFileIcon(const features::FileItem& item) const {
    if (item.is_directory) {
        if (item.name == "..")
            return icons::FOLDER_UP;
        return icons::FOLDER;
    }
    std::string ext = getFileExtension(item.name);
    return utils::getIconForFile(item.name, ext, nullptr);
}

std::string FileBrowserView::getFileExtension(const std::string& filename) const {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return "";
}

std::string FileBrowserView::truncateMiddle(const std::string& str, size_t max_length) const {
    if (str.length() <= max_length) {
        return str;
    }

    if (max_length < 5) {
        return str.substr(0, max_length);
    }

    // 计算两端保留的长度
    size_t left_len = (max_length - 3) / 2;
    size_t right_len = max_length - 3 - left_len;

    return str.substr(0, left_len) + "..." + str.substr(str.length() - right_len);
}

std::pair<std::string, std::string> FileBrowserView::getRemoteSizeAndPermission(
    const features::FileBrowser& browser, const features::FileItem& item) const {
    auto cache_it = remote_stat_cache_.find(item.path);
    if (cache_it != remote_stat_cache_.end()) {
        return cache_it->second;
    }

    std::string size_display = item.is_directory ? "Folder" : "Unknown";
    std::string perm_display = item.is_directory ? "drwxr-xr-x" : "---------";

    if (browser.getRemoteFileStat(item.path, size_display, perm_display)) {
        remote_stat_cache_[item.path] = {size_display, perm_display};
        return {size_display, perm_display};
    }

    remote_stat_cache_[item.path] = {size_display, perm_display};
    return {size_display, perm_display};
}

} // namespace ui
} // namespace pnana