#include "ui/help.h"
#include "ui/icons.h"

using namespace ftxui;

namespace pnana {
namespace ui {

Help::Help(Theme& theme) : theme_(theme) {
}

std::vector<HelpEntry> Help::getAllHelp() {
    return {
        // 文件操作
        {"File Operations", "Ctrl+N", "New file"},
        {"File Operations", "Ctrl+O", "Open file browser"},
        {"File Operations", "Ctrl+S", "Save file"},
        {"File Operations", "Ctrl+W", "Close current tab"},
        {"File Operations", "Ctrl+Q", "Quit editor"},
        
        // 编辑操作
        {"Editing", "Ctrl+Z", "Undo"},
        {"Editing", "Ctrl+Y", "Redo"},
        {"Editing", "Ctrl+X", "Cut"},
        {"Editing", "Ctrl+C", "Copy"},
        {"Editing", "Ctrl+V", "Paste"},
        {"Editing", "Ctrl+A", "Select all"},
        {"Editing", "Ctrl+D", "Duplicate line"},
        {"Editing", "Tab", "Insert 4 spaces"},
        {"Editing", "Backspace", "Delete previous character"},
        {"Editing", "Delete", "Delete current character"},
        
        // 导航
        {"Navigation", "↑↓←→", "Move cursor"},
        {"Navigation", "Home", "Go to line start"},
        {"Navigation", "End", "Go to line end"},
        {"Navigation", "Page Up/Down", "Scroll page"},
        {"Navigation", "Ctrl+Home", "Go to file start"},
        {"Navigation", "Ctrl+End", "Go to file end"},
        {"Navigation", "Ctrl+G", "Go to line number"},
        
        // 搜索
        {"Search", "Ctrl+F", "Search text"},
        {"Search", "F3", "Find next"},
        {"Search", "Shift+F3", "Find previous"},
        {"Search", "Ctrl+H", "Replace text"},
        
        // 选择
        {"Selection", "Shift+↑↓←→", "Select text"},
        {"Selection", "Ctrl+A", "Select all"},
        
        // 标签页
        {"Tabs", "Tab", "Next tab (in search mode)"},
        {"Tabs", "Shift+Tab", "Previous tab"},
        {"Tabs", "Ctrl+W", "Close tab"},
        
        // 视图
        {"View", "Ctrl+T", "Toggle theme menu"},
        {"View", "F1", "Show this help"},
        
        // 文件浏览器
        {"File Browser", "Ctrl+O", "Toggle file browser"},
        {"File Browser", "↑↓", "Navigate (in browser)"},
        {"File Browser", "Enter", "Open file/folder"},
        {"File Browser", "Backspace", "Go to parent folder"},
        {"File Browser", "h", "Toggle hidden files"},
        {"File Browser", "r", "Refresh file list"},
        {"File Browser", "Esc", "Close browser"},
    };
}

Element Help::render(int width, int height) {
    auto& colors = theme_.getColors();
    
    Elements content;
    
    // 标题
    content.push_back(
        hbox({
            text(" "),
            text(icons::HELP),
            text(" pnana - Help "),
            filler(),
            text("Press Esc or F1 to close "),
        }) | bold | bgcolor(colors.statusbar_bg) | color(colors.statusbar_fg)
    );
    
    content.push_back(separator());
    
    // 分组帮助内容
    auto all_help = getAllHelp();
    std::map<std::string, std::vector<HelpEntry>> grouped;
    
    for (const auto& entry : all_help) {
        grouped[entry.category].push_back(entry);
    }
    
    Elements help_content;
    
    // 按分类渲染
    std::vector<std::string> categories = {
        "File Operations",
        "Editing",
        "Navigation",
        "Search",
        "Selection",
        "Tabs",
        "View",
        "File Browser"
    };
    
    for (const auto& category : categories) {
        if (grouped.find(category) != grouped.end()) {
            help_content.push_back(
                text("") | color(colors.keyword) | bold
            );
            help_content.push_back(
                text(" " + category) | color(colors.keyword) | bold
            );
            help_content.push_back(text(""));
            
            for (const auto& entry : grouped[category]) {
                help_content.push_back(
                    hbox({
                        text("  "),
                        text(entry.key) | color(colors.function) | bold | size(WIDTH, EQUAL, 18),
                        text(" "),
                        text(entry.description) | color(colors.foreground)
                    })
                );
            }
            
            help_content.push_back(text(""));
        }
    }
    
    // 底部提示
    help_content.push_back(separator());
    help_content.push_back(
        hbox({
            text(" "),
            text(icons::BULB),
            text(" Tip: Most shortcuts work in any mode!")
        }) | color(colors.success)
    );
    
    content.push_back(
        vbox(help_content) | vscroll_indicator | frame | flex
    );
    
    return vbox(content) 
        | size(WIDTH, LESS_THAN, width - 10)
        | size(HEIGHT, LESS_THAN, height - 4)
        | bgcolor(colors.background)
        | border;
}

Element Help::renderCategory(const std::string& category, 
                             const std::vector<HelpEntry>& entries) {
    auto& colors = theme_.getColors();
    
    Elements items;
    
    items.push_back(text(" " + category) | bold | color(colors.keyword));
    items.push_back(separator());
    
    for (const auto& entry : entries) {
        items.push_back(
            hbox({
                text("  "),
                text(entry.key) | color(colors.function) | bold,
                text(" - "),
                text(entry.description)
            })
        );
    }
    
    return vbox(items);
}

} // namespace ui
} // namespace pnana

