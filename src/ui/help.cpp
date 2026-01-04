#include "ui/help.h"
#include "ui/icons.h"

using namespace ftxui;

namespace pnana {
namespace ui {

Help::Help(Theme& theme) : theme_(theme), scroll_offset_(0) {}

std::vector<HelpEntry> Help::getAllHelp() {
    return {
        // 文件操作
        {"File Operations", "Ctrl+N", "New file"},
        {"File Operations", "Ctrl+O", "Toggle file browser"},
        {"File Operations", "Ctrl+S", "Save file"},
        {"File Operations", "Alt+A", "Save as"},
        {"File Operations", "Ctrl+W", "Close tab"},
        {"File Operations", "Alt+F", "Create folder"},
        {"File Operations", "Alt+M", "Open file picker"},
        {"File Operations", "Ctrl+Q", "Quit editor"},

        // 编辑操作
        {"Editing", "Ctrl+Z", "Undo"},
        {"Editing", "Ctrl+Y / Ctrl+Shift+Z", "Redo"},
        {"Editing", "Ctrl+X", "Cut"},
        {"Editing", "Ctrl+P", "Copy"},
        {"Editing", "Ctrl+V", "Paste"},
        {"Editing", "Ctrl+P", "Select all"},
        {"Editing", "Alt+D", "Select word"},
        {"Editing", "Ctrl+D", "Duplicate line"},
        {"Editing", "Ctrl+L / Ctrl+Shift+K", "Delete line"},
        //{"Editing", "Ctrl+Backspace", "Delete word"},
        //{"Editing", "Alt+↑/↓", "Move line up/down"},
        {"Editing", "Tab", "Indent line"},
        //{"Editing", "Shift+Tab", "Unindent line"},
        //{"Editing", "Ctrl+/", "Toggle comment"},
        {"Editing", "Ctrl+→/←", "Free Select text"},
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

        // 搜索和替换
        {"Search", "Ctrl+F", "Search text"},
        {"Search", "Ctrl+H", "Replace text"},
        {"Search", "Ctrl+F3", "Find next match"},
        {"Search", "Ctrl+Shift+F3", "Find previous match"},
        {"Search", "Ctrl+G", "Go to line number"},

        // 选择
        {"Selection", "Shift+↑↓←→", "Select text"},
        {"Selection", "Alt+Shift+↑↓←→", "Extend selection"},

        // 标签页
        {"Tabs", "Alt+Tab / Ctrl+PageDown", "Next tab"},
        {"Tabs", "Alt+Shift+Tab / Ctrl+PageUp", "Previous tab"},
        {"Tabs", "Ctrl+W", "Close tab"},

        // 视图
        {"View", "Ctrl+T", "Toggle theme menu"},
        {"View", "F1", "Show help"},
        {"View", "Ctrl+Shift+L", "Toggle line numbers"},
        {"View", "F3", "Command palette"},
        {"View", "F4", "SSH remote file editor"},
#ifdef BUILD_LUA_SUPPORT
        {"View", "Alt+P", "Open plugin manager"},
#endif

        // 分屏
        {"Split View", "Ctrl+L", "Open split view dialog"},
        {"Split View", "Ctrl+←→↑↓", "Navigate between regions"},

        // 文件浏览器
        {"File Browser", "Ctrl+O", "Toggle file browser"},
        {"File Browser", "↑↓", "Navigate files"},
        {"File Browser", "Enter", "Open file/folder"},
        {"File Browser", "Backspace", "Go to parent folder"},
        {"File Browser", "Tab", "Toggle type filter"},
        {"File Browser", "Ctrl+F", "Toggle text filter"},
        {"File Browser", ":/", "Enter path input"},
        {"File Browser", "Esc", "Close browser"},

        // 命令面板
        {"Command Palette", "F3", "Open command palette"},
        {"Command Palette", "Type 'cursor'", "Open cursor config"},
        {"Command Palette", "↑↓", "Navigate commands"},
        {"Command Palette", "Enter", "Execute command"},
        {"Command Palette", "Esc", "Close palette"},

        // 图片预览
        {"Image Preview", "Select image", "Auto preview in code area"},
        {"Image Preview", "Supported", "JPG, PNG, GIF, BMP, WEBP"},

        // 终端
        {"Terminal", "F3 → terminal", "Open integrated terminal"},
        {"Terminal", "Esc", "Close terminal"},
        {"Terminal", "+/-", "Adjust terminal height"},
        {"Terminal", "←→", "Switch between regions"},

        //插件
        {"Plugin", "Alt+P", "Open plugin manager"},
        {"Plugin", "↑↓", "Navigate plugins"},
        {"Plugin", "Enter", "Toggle plugin"},
        {"Plugin", "Esc", "Close plugin manager"},

        // 帮助
        {"Help", "F1", "Show/hide help"},
        {"Help", "↑↓/j/k", "Scroll help"},
        {"Help", "Page Up/Down", "Page navigation"},
        {"Help", "Home/End", "Jump to top/bottom"},
        {"Help", "Esc", "Close help"},
    };
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

    content.push_back(separator());

    // 分组帮助内容
    auto all_help = getAllHelp();
    std::map<std::string, std::vector<HelpEntry>> grouped;

    for (const auto& entry : all_help) {
        grouped[entry.category].push_back(entry);
    }

    Elements help_content;

    // 按分类渲染
    std::vector<std::string> categories = {"File Operations",
                                           "Editing",
                                           "Navigation",
                                           "Search",
                                           "Selection",
                                           "Tabs",
                                           "View",
                                           "Split View",
                                           "File Browser",
                                           "Command Palette",
                                           "Image Preview",
                                           "Terminal",
                                           "Help"};

    for (const auto& category : categories) {
        if (grouped.find(category) != grouped.end()) {
            help_content.push_back(text("") | color(colors.keyword) | bold);
            help_content.push_back(text(" " + category) | color(colors.keyword) | bold);
            help_content.push_back(text(""));

            for (const auto& entry : grouped[category]) {
                help_content.push_back(
                    hbox({text("  "),
                          text(entry.key) | color(colors.function) | bold | size(WIDTH, EQUAL, 22),
                          text(" "), text(entry.description) | color(colors.foreground)}));
            }

            help_content.push_back(text(""));
        }
    }

    // 底部提示
    help_content.push_back(separator());
    help_content.push_back(
        hbox({text(" "), text(icons::BULB), text(" Tip: Most shortcuts work in any mode!")}) |
        color(colors.success));

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
        hbox({text(" "), text(icons::BULB), text(" Tip: Most shortcuts work in any mode! "),
              text("↑↓") | color(colors.function) | bold, text(": Scroll, "),
              text("Page Up/Down") | color(colors.function) | bold, text(": Page navigation, "),
              text("Home/End") | color(colors.function) | bold, text(": Jump to top/bottom")}) |
        color(colors.success));

    return vbox(content) | size(WIDTH, LESS_THAN, width - 10) |
           size(HEIGHT, LESS_THAN, height - 4) | bgcolor(colors.background) | border;
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
}

} // namespace ui
} // namespace pnana
