#include "ui/helpbar.h"

using namespace ftxui;

namespace pnana {
namespace ui {

Helpbar::Helpbar(Theme& theme) : theme_(theme) {
}

Element Helpbar::render(const std::vector<HelpItem>& items) {
    Elements help_elements;
    
    for (const auto& item : items) {
        help_elements.push_back(renderItem(item));
        help_elements.push_back(text("  "));
    }
    
    return hbox(help_elements) 
        | bgcolor(theme_.getColors().helpbar_bg) 
        | color(theme_.getColors().helpbar_fg);
}

Element Helpbar::renderItem(const HelpItem& item) {
    return hbox({
        text(item.key) | color(theme_.getColors().helpbar_key) | bold,
        text(" "),
        text(item.description)
    });
}

std::vector<HelpItem> Helpbar::getDefaultHelp() {
    return {
        {"^S", "Save"},
        {"^O", "Files"},
        {"^W", "Close"},
        {"^F", "Find"},
        {"^T", "Themes"},
        {"Tab", "Next Tab"},
        {"^Z", "Undo"},
        {"^Q", "Quit"}
    };
}

std::vector<HelpItem> Helpbar::getEditModeHelp() {
    return {
        {"^S", "Save"},
        {"^X", "Cut"},
        {"^C", "Copy"},
        {"^V", "Paste"},
        {"^A", "Select All"},
        {"^Z", "Undo"},
        {"^Q", "Quit"}
    };
}

std::vector<HelpItem> Helpbar::getSearchModeHelp() {
    return {
        {"Enter", "Search"},
        {"Esc", "Cancel"},
        {"F3", "Next"},
        {"Shift+F3", "Previous"}
    };
}

} // namespace ui
} // namespace pnana

