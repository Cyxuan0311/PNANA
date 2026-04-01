#include "ui/helpbar.h"

using namespace ftxui;

namespace pnana {
namespace ui {

Helpbar::Helpbar(Theme& theme) : theme_(theme) {}

Element Helpbar::render(const std::vector<HelpItem>& items) {
    Elements help_elements;

    for (const auto& item : items) {
        help_elements.push_back(renderItem(item));
        help_elements.push_back(text("  "));
    }

    return hbox(help_elements) | bgcolor(theme_.getColors().helpbar_bg) |
           color(theme_.getColors().helpbar_fg);
}

Element Helpbar::renderItem(const HelpItem& item) {
    auto& colors = theme_.getColors();
    // 操作符使用醒目的颜色（如主题中的 function 色），描述使用默认前景色
    return hbox({text(item.key) | color(colors.function) | bold,
                 text(" ") | color(colors.helpbar_fg),
                 text(item.description) | color(colors.helpbar_fg)});
}

std::vector<HelpItem> Helpbar::getDefaultHelp() {
    return {{"^S", "Save"},   {"^O", "Files"},          {"^W", "Close"}, {"^F", "Find"},
            {"^T", "Themes"}, {"Alt + T", "Tabs"},      {"^Z", "Undo"},  {"^Q", "Quit"},
            {"F1", "Help"},   {"F3", "Command Palette"}};
}

std::vector<HelpItem> Helpbar::getEditModeHelp() {
    return {{"^S", "Save"},       {"^X", "Cut"},  {"^P", "Copy"}, {"^V", "Paste"},
            {"^A", "Select All"}, {"^Z", "Undo"}, {"^Q", "Quit"}};
}

std::vector<HelpItem> Helpbar::getSearchModeHelp() {
    return {{"Enter", "Search"}, {"Esc", "Cancel"}, {"F3", "Next"}, {"Shift+F3", "Previous"}};
}

} // namespace ui
} // namespace pnana
