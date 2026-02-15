#include "ui/help.h"
#include "ui/icons.h"
#include <map>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

Help::Help(Theme& theme) : theme_(theme), scroll_offset_(0), current_tab_index_(0) {}

std::vector<HelpEntry> Help::getAllHelp() {
    return {
        // 文件操作（合并了 File Browser 和 Tabs）
        {"File Operations", "Ctrl+N", "New file"},
        {"File Operations", "Ctrl+O", "Toggle file browser"},
        {"File Operations", "Ctrl+S", "Save file"},
        {"File Operations", "Alt+A", "Save as"},
        {"File Operations", "Ctrl+W", "Close tab"},
        {"File Operations", "Alt+F", "Create folder"},
        {"File Operations", "Alt+M", "Open file picker"},
        {"File Operations", "Ctrl+Q", "Quit editor"},
        {"File Operations", "Alt+Tab / Ctrl+PageDown", "Next tab"},
        {"File Operations", "Alt+Shift+Tab / Ctrl+PageUp", "Previous tab"},
        {"File Operations", "↑↓", "Navigate files (in browser)"},
        {"File Operations", "Enter", "Open file/folder"},
        {"File Operations", "Backspace", "Go to parent folder"},
        {"File Operations", "Tab", "Toggle type filter"},
        {"File Operations", ":/", "Enter path input"},

        // 编辑操作（合并了 Search 和 Selection）
        {"Editing", "Ctrl+Z", "Undo"},
        {"Editing", "Ctrl+Y / Ctrl+Shift+Z", "Redo"},
        {"Editing", "Ctrl+X", "Cut"},
        {"Editing", "Ctrl+C", "Copy"},
        {"Editing", "Ctrl+V", "Paste"},
        {"Editing", "Ctrl+A", "Select all"},
        {"Editing", "Alt+D", "Select word"},
        {"Editing", "Ctrl+D", "Duplicate line"},
        {"Editing", "Ctrl+L / Ctrl+Shift+K", "Delete line"},
        {"Editing", "Tab", "Indent line"},
        {"Editing", "Ctrl+→/←", "Free Select text"},
        {"Editing", "Shift+↑↓←→", "Select text"},
        {"Editing", "Alt+Shift+↑↓←→", "Extend selection"},
        {"Editing", "Ctrl+F", "Search text"},
        {"Editing", "Ctrl+H", "Replace text"},
        {"Editing", "Ctrl+F3", "Find next match"},
        {"Editing", "Ctrl+Shift+F3", "Find previous match"},
#ifdef BUILD_LSP_SUPPORT
        {"Editing", "Ctrl+Space", "Trigger code completion"},
#endif

        // 导航
        {"Navigation", "↑↓←→", "Move cursor"},
        {"Navigation", "Home", "Go to line start"},
        {"Navigation", "End", "Go to line end"},
        {"Navigation", "Ctrl+Home", "Go to file start"},
        {"Navigation", "Ctrl+End", "Go to file end"},
        {"Navigation", "Ctrl+G", "Go to line number"},
        {"Navigation", "Page Up/Down", "Scroll page"},

        // 视图和工具（合并了 View, Split View, Command Palette, Image Preview, Terminal, Plugin）
        {"View & Tools", "Ctrl+T", "Toggle theme menu"},
        {"View & Tools", "F1", "Show help"},
        {"View & Tools", "Ctrl+Shift+L", "Toggle line numbers"},
        {"View & Tools", "F3", "Command palette"},
        {"View & Tools", "F4", "SSH remote file editor"},
        {"View & Tools", "Ctrl+L", "Open split view dialog"},
        {"View & Tools", "Ctrl+←→↑↓", "Navigate between regions"},
        {"View & Tools", "Select image", "Auto preview in code area"},
        {"View & Tools", "F3 → terminal", "Open integrated terminal"},
        {"View & Tools", "+/-", "Adjust terminal height"},
#ifdef BUILD_LUA_SUPPORT
        {"View & Tools", "Alt+P", "Open plugin manager"},
#endif

        // 帮助
        {"Help", "F1", "Show/hide help"},
        {"Help", "Tab / ←→", "Switch tabs"},
        {"Help", "↑↓/j/k", "Scroll help"},
        {"Help", "Page Up/Down", "Page navigation"},
        {"Help", "Home/End", "Jump to top/bottom"},
        {"Help", "Esc", "Close help"},
    };
}

std::vector<std::string> Help::getCategories() {
    return {"File Operations",
            "Editing",
            "Navigation",
            "View & Tools",
            "Help"};
}

Element Help::renderTabs() {
    auto& colors = theme_.getColors();
    auto categories = getCategories();
    
    Elements tabs;
    for (size_t i = 0; i < categories.size(); ++i) {
        std::string tab_text = categories[i];
        // 缩短长标签名以适应tab栏
        if (tab_text.length() > 15) {
            tab_text = tab_text.substr(0, 13) + "..";
        }
        
        if (i == current_tab_index_) {
            // 当前选中的tab：使用更明显的背景色和加粗字体
            tabs.push_back(text(" " + tab_text + " ") | 
                          bgcolor(colors.statusbar_bg) | 
                          color(colors.statusbar_fg) | 
                          bold);
        } else {
            // 未选中的tab：使用较淡的背景色
            tabs.push_back(text(" " + tab_text + " ") | 
                          bgcolor(colors.background) | 
                          color(colors.foreground));
        }
    }
    
    return hbox(tabs) | border;
}

Element Help::render(int width, int height) {
    auto& colors = theme_.getColors();

    Elements content;

    // 标题
    content.push_back(hbox({
                          text(" "),
                          text(icons::HELP),
                          text(" pnana - Help "),
                          filler(),
                          text("Press Esc or F1 to close "),
                      }) |
                      bold | bgcolor(colors.statusbar_bg) | color(colors.statusbar_fg));

    // Tab栏
    content.push_back(renderTabs());
    content.push_back(separator());

    // 分组帮助内容
    auto all_help = getAllHelp();
    std::map<std::string, std::vector<HelpEntry>> grouped;

    for (const auto& entry : all_help) {
        grouped[entry.category].push_back(entry);
    }

    Elements help_content;

    // 只渲染当前选中的分类
    auto categories = getCategories();
    if (current_tab_index_ < categories.size()) {
        std::string current_category = categories[current_tab_index_];
        
        if (grouped.find(current_category) != grouped.end()) {
            // 显示分类标题
            help_content.push_back(text("") | color(colors.keyword) | bold);
            help_content.push_back(text(" " + current_category) | color(colors.keyword) | bold);
            help_content.push_back(text(""));

            // 显示该分类的所有条目
            for (const auto& entry : grouped[current_category]) {
                help_content.push_back(
                    hbox({text("  "),
                          text(entry.key) | color(colors.function) | bold | size(WIDTH, EQUAL, 22),
                          text(" "), text(entry.description) | color(colors.foreground)}));
            }
        } else {
            help_content.push_back(text("  No help entries found for this category.") | 
                                  color(colors.foreground));
        }
    }

    // 底部提示
    help_content.push_back(separator());

    // 应用滚动偏移
    size_t visible_height = static_cast<size_t>(height - 8); // 减去标题、分隔符、状态栏等
    size_t total_items = help_content.size();

    if (scroll_offset_ > total_items) {
        scroll_offset_ = 0;
    }

    Elements visible_content;
    size_t end_index = std::min(scroll_offset_ + visible_height, total_items);
    for (size_t i = scroll_offset_; i < end_index; ++i) {
        visible_content.push_back(help_content[i]);
    }

    content.push_back(vbox(visible_content) | frame | flex);

    // 底部提示
    content.push_back(separator());
    content.push_back(
        hbox({text(" "), text(icons::BULB), text(" Tip: "),
              text("Tab") | color(colors.function) | bold, text(": Switch tabs, "),
              text("↑↓") | color(colors.function) | bold, text(": Scroll, "),
              text("Page Up/Down") | color(colors.function) | bold, text(": Page navigation, "),
              text("Home/End") | color(colors.function) | bold, text(": Jump to top/bottom")}) |
        color(colors.success));

    return vbox(content) | size(WIDTH, LESS_THAN, width - 10) |
           size(HEIGHT, LESS_THAN, height - 4) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

Element Help::renderCategory(const std::string& category, const std::vector<HelpEntry>& entries) {
    auto& colors = theme_.getColors();

    Elements items;

    items.push_back(text(" " + category) | bold | color(colors.keyword));
    items.push_back(separator());

    for (const auto& entry : entries) {
        items.push_back(hbox({text("  "), text(entry.key) | color(colors.function) | bold,
                              text(" - "), text(entry.description)}));
    }

    return vbox(items);
}

bool Help::handleInput(ftxui::Event event) {
    auto categories = getCategories();
    
    // Tab键切换tab
    if (event == Event::Tab || event == Event::Character('\t')) {
        current_tab_index_ = (current_tab_index_ + 1) % categories.size();
        scroll_offset_ = 0; // 切换tab时重置滚动位置
        return true;
    }
    
    // Shift+Tab反向切换tab
    if (event == Event::TabReverse || 
        (event.is_character() && event.character() == "\x1b[Z")) {
        if (current_tab_index_ > 0) {
            current_tab_index_--;
        } else {
            current_tab_index_ = categories.size() - 1;
        }
        scroll_offset_ = 0; // 切换tab时重置滚动位置
        return true;
    }
    
    // 左右箭头键切换tab
    if (event == Event::ArrowLeft || event == Event::Character('h')) {
        if (current_tab_index_ > 0) {
            current_tab_index_--;
        } else {
            current_tab_index_ = categories.size() - 1;
        }
        scroll_offset_ = 0;
        return true;
    }
    
    if (event == Event::ArrowRight || event == Event::Character('l')) {
        current_tab_index_ = (current_tab_index_ + 1) % categories.size();
        scroll_offset_ = 0;
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        if (scroll_offset_ > 0) {
            scroll_offset_--;
        }
        return true;
    }

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        scroll_offset_++;
        return true;
    }

    if (event == Event::PageUp) {
        size_t page_size = 10;
        if (scroll_offset_ >= page_size) {
            scroll_offset_ -= page_size;
        } else {
            scroll_offset_ = 0;
        }
        return true;
    }

    if (event == Event::PageDown) {
        size_t page_size = 10;
        scroll_offset_ += page_size;
        return true;
    }

    if (event == Event::Home) {
        scroll_offset_ = 0;
        return true;
    }

    if (event == Event::End) {
        // 设置为最大值，会在渲染时自动调整
        scroll_offset_ = 10000;
        return true;
    }

    return false;
}

void Help::reset() {
    scroll_offset_ = 0;
    current_tab_index_ = 0;
}

} // namespace ui
} // namespace pnana
