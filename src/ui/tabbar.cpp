#include "ui/tabbar.h"
#include "ui/icons.h"
#include "utils/file_type_detector.h"

using namespace ftxui;

namespace pnana {
namespace ui {

Tabbar::Tabbar(Theme& theme, const core::ConfigManager& config_manager)
    : theme_(theme), config_manager_(config_manager) {}

Element Tabbar::render(const std::vector<core::DocumentManager::TabInfo>& tabs) {
    if (tabs.empty()) {
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements tab_elements;

    for (size_t i = 0; i < tabs.size(); ++i) {
        tab_elements.push_back(renderTab(tabs[i], i));
    }

    // 添加新建标签按钮
    auto new_tab_btn = hbox({text(" "),
                             text(""), // Nerd Font: 加号图标
                             text(" ")}) |
                       color(colors.comment);

    tab_elements.push_back(new_tab_btn);

    return hbox(tab_elements) | bgcolor(colors.menubar_bg);
}

Element Tabbar::renderTab(const core::DocumentManager::TabInfo& tab, size_t /*index*/) {
    auto& colors = theme_.getColors();

    // 获取配置
    const auto& display_config = config_manager_.getConfig().display;

    // 获取文件图标
    std::string icon = getFileIcon(tab.filename);

    // 文件名
    std::string display_name = tab.filename;
    if (display_name.empty()) {
        display_name = "[Untitled]";
    }

    // 状态标记：根据编辑状态显示不同符号
    // - 未修改（没有编辑）：显示 × 号（如果配置启用）
    // - 已修改（编辑未保存）：显示圆形符号 ●
    std::string status_symbol = "";
    Color status_color = colors.comment;

    if (tab.is_modified) {
        // 编辑未保存：使用圆形符号 ●
        status_symbol = "●";
        status_color = colors.warning; // 使用警告色（橙色/黄色）突出显示
    } else {
        // 没有编辑：根据配置决定是否显示 × 号
        if (display_config.show_tab_close_indicator) {
            status_symbol = "×";
            status_color = colors.comment; // 使用注释色（灰色）
        }
    }

    // 构建标签内容
    Elements content;
    content.push_back(text(" "));
    content.push_back(text(icon));
    content.push_back(text(" "));
    content.push_back(text(display_name));

    // 如果标签被固定，添加锁图标
    if (tab.is_pinned) {
        content.push_back(text(" "));
        content.push_back(text(pnana::ui::icons::LOCK) | color(colors.keyword));
    }

    if (!status_symbol.empty()) {
        content.push_back(text(" "));
        content.push_back(text(status_symbol) | color(status_color));
    }

    content.push_back(text(" "));

    auto tab_element = hbox(content);

    // 当前标签高亮：使用主题色反白显示
    if (tab.is_current) {
        // 使用 success 颜色 + 前景色反白，加粗显示
        return tab_element | bgcolor(colors.success) | color(colors.background) | bold;
    } else {
        // 非当前标签使用注释颜色
        return tab_element | color(colors.comment);
    }
}

std::string Tabbar::getFileIcon(const std::string& filename) const {
    std::string ext = getFileExtension(filename);

    // 使用文件类型检测器获取完整的文件类型信息
    std::string file_type = utils::FileTypeDetector::getFileTypeForIcon(filename, ext);

    // 使用图标映射函数获取对应的图标
    return ui::icons::getFileTypeIcon(file_type);
}

std::string Tabbar::getFileExtension(const std::string& filename) const {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return "";
}

} // namespace ui
} // namespace pnana
