#include "ui/tabbar.h"

using namespace ftxui;

namespace pnana {
namespace ui {

Tabbar::Tabbar(Theme& theme)
    : theme_(theme) {
}

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
    auto new_tab_btn = hbox({
        text(" "),
        text(""), // Nerd Font: 加号图标
        text(" ")
    }) | color(colors.comment);
    
    tab_elements.push_back(new_tab_btn);
    
    return hbox(tab_elements) | bgcolor(colors.menubar_bg);
}

Element Tabbar::renderTab(const core::DocumentManager::TabInfo& tab, size_t /*index*/) {
    auto& colors = theme_.getColors();
    
    // 获取文件图标
    std::string icon = getFileIcon(tab.filename);
    
    // 文件名
    std::string display_name = tab.filename;
    if (display_name.empty()) {
        display_name = "[Untitled]";
    }
    
    // 修改标记
    std::string modified_mark = tab.is_modified ? " " : "";  // Nerd Font: 圆点表示修改
    
    // 构建标签内容
    Elements content;
    content.push_back(text(" "));
    content.push_back(text(icon));
    content.push_back(text(" "));
    content.push_back(text(display_name));
    
    if (tab.is_modified) {
        content.push_back(text(modified_mark) | color(colors.warning));
    }
    
    content.push_back(text("  "));  // Nerd Font: 关闭图标
    content.push_back(text(" "));
    
    auto tab_element = hbox(content);
    
    // 当前标签高亮
    if (tab.is_current) {
        return tab_element | bgcolor(colors.current_line) | bold | color(colors.foreground);
    } else {
        return tab_element | color(colors.comment);
    }
}

std::string Tabbar::getFileIcon(const std::string& filename) const {
    std::string ext = getFileExtension(filename);
    
    // 使用Nerd Font图标
    if (ext == "cpp" || ext == "cc" || ext == "cxx") return "";
    if (ext == "h" || ext == "hpp" || ext == "hxx") return "";
    if (ext == "c") return "";
    if (ext == "py") return "";
    if (ext == "js") return "";
    if (ext == "ts") return "";
    if (ext == "java") return "";
    if (ext == "go") return "";
    if (ext == "rs") return "";
    if (ext == "rb") return "";
    if (ext == "php") return "";
    if (ext == "html") return "";
    if (ext == "css") return "";
    if (ext == "json") return "";
    if (ext == "xml") return "";
    if (ext == "md") return "";
    if (ext == "txt") return "";
    if (ext == "pdf") return "";
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif") return "";
    if (ext == "zip" || ext == "tar" || ext == "gz") return "";
    if (ext == "sh") return "";
    if (ext == "yml" || ext == "yaml") return "";
    if (ext == "sql") return "";
    
    return "";  // 默认文件图标
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

