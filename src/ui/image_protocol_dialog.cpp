#include "ui/image_protocol_dialog.h"
#include "features/image/protocol_manager.h"
#include "ui/icons.h"
#include "ui/responsive_size.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

ImageProtocolDialog::ImageProtocolDialog(Theme& theme)
    : theme_(theme), visible_(false), protocol_enabled_(true), preferred_protocol_("auto"),
      selected_option_(0) {}

void ImageProtocolDialog::open() {
    protocol_enabled_ = pnana::features::ProtocolManager::isEnabled();
    preferred_protocol_ = pnana::features::ProtocolManager::getPreferred();
    refreshProtocolStatus();
    selected_option_ = 1; // default to the enable toggle
    visible_ = true;
}

void ImageProtocolDialog::close() {
    visible_ = false;
}

void ImageProtocolDialog::refreshProtocolStatus() {
    auto info = pnana::features::ProtocolManager::getTerminalInfo();
    std::string active = pnana::features::ProtocolManager::getActiveProtocolName();

    protocols_ = {
        {"kitty", "Kitty Protocol", info.kitty, active == "Kitty"},
        {"iterm2", "iTerm2 Protocol", info.iterm2, active == "iTerm2"},
        {"sixel", "Sixel Protocol", info.sixel, active == "Sixel"},
    };
}

bool ImageProtocolDialog::handleInput(Event event) {
    if (!visible_)
        return false;

    if (event == Event::Escape) {
        close();
        return true;
    }

    if (event == Event::Return) {
        apply();
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        selectPrevious();
        return true;
    }

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        selectNext();
        return true;
    }

    if (event == Event::Character(' ') || event == Event::Tab) {
        // Space toggles enabled or preferred
        if (selected_option_ == 0) {
            // Terminal info line - skip
        } else if (selected_option_ == 1) {
            protocol_enabled_ = !protocol_enabled_;
        } else if (selected_option_ == 2) {
            // Protocol list - do nothing
        } else if (selected_option_ >= 3 && selected_option_ <= 5) {
            // Protocol items - cycle preferred
            std::vector<std::string> prefs = {"auto", "kitty", "iterm2", "sixel", "block"};
            for (size_t i = 0; i < prefs.size(); ++i) {
                if (preferred_protocol_ == prefs[i]) {
                    preferred_protocol_ = prefs[(i + 1) % prefs.size()];
                    break;
                }
            }
        } else if (selected_option_ == 6) {
            // Preferred selector
            std::vector<std::string> prefs = {"auto", "kitty", "iterm2", "sixel", "block"};
            for (size_t i = 0; i < prefs.size(); ++i) {
                if (preferred_protocol_ == prefs[i]) {
                    preferred_protocol_ = prefs[(i + 1) % prefs.size()];
                    break;
                }
            }
        }
        return true;
    }

    return true;
}

void ImageProtocolDialog::selectNext() {
    if (selected_option_ < 6)
        ++selected_option_;
    else
        selected_option_ = 0;
}

void ImageProtocolDialog::selectPrevious() {
    if (selected_option_ > 0)
        --selected_option_;
    else
        selected_option_ = 6;
}

void ImageProtocolDialog::apply() {
    pnana::features::ProtocolManager::setEnabled(protocol_enabled_);
    pnana::features::ProtocolManager::setPreferred(preferred_protocol_);
    if (on_apply_)
        on_apply_();
    close();
}

// ========== render ==========

Element ImageProtocolDialog::renderHeader() {
    auto& colors = theme_.getColors();
    return hbox({
               text(" "),
               text(icons::SETTINGS) | color(Color::Cyan),
               text("  Image Protocol Settings") | bold | color(colors.foreground),
           }) |
           bgcolor(colors.menubar_bg);
}

Element ImageProtocolDialog::renderToggle() {
    auto& colors = theme_.getColors();
    bool sel = (selected_option_ == 1);
    std::string status = protocol_enabled_ ? "[ON]" : "[OFF]";
    Color status_color = protocol_enabled_ ? colors.success : colors.comment;

    return hbox({
               text("  "),
               text(sel ? "► " : "  ") | color(sel ? colors.function : colors.background),
               text("Protocol: ") | color(sel ? colors.foreground : colors.comment),
               text(" ") | color(status_color),
               text(status) | bold | color(status_color),
               text("  (Space: toggle)") | color(colors.comment) | dim,
               filler(),
           }) |
           bgcolor(sel ? colors.selection : colors.background);
}

Element ImageProtocolDialog::renderProtocolList() {
    auto& colors = theme_.getColors();
    Elements items;

    for (size_t i = 0; i < protocols_.size(); ++i) {
        bool sel = (selected_option_ == 3 + i);
        const auto& p = protocols_[i];
        std::string indicator = p.available ? "●" : "○";
        Color indicator_color = p.available ? colors.success : colors.comment;
        Color name_color =
            sel ? colors.foreground : (p.available ? colors.foreground : colors.comment);

        items.push_back(
            hbox({
                text("    "),
                text(indicator) | color(indicator_color),
                text(" ") | color(name_color),
                text(p.display) | color(name_color) | (p.active ? bold : nothing),
                text(" ") | color(colors.comment),
                text(p.available ? "available" : "not detected") | color(colors.comment) | dim,
                filler(),
            }) |
            bgcolor(sel ? colors.selection : colors.background));
    }

    return vbox(items);
}

Element ImageProtocolDialog::renderPreferredSelector() {
    auto& colors = theme_.getColors();
    bool sel = (selected_option_ == 6);

    std::vector<std::string> prefs = {"auto", "kitty", "iterm2", "sixel", "block"};
    std::string display = preferred_protocol_;
    for (const auto& p : prefs) {
        if (p == preferred_protocol_) {
            display = p;
            break;
        }
    }

    Element pref_line =
        hbox({
            text("  "),
            text(sel ? "► " : "  ") | color(sel ? colors.function : colors.background),
            text("Preferred: ") | color(sel ? colors.foreground : colors.comment),
            text("[" + display + "  ▼]") | color(colors.function) | bold,
            text("  (Space: cycle)") | color(colors.comment) | dim,
            filler(),
        }) |
        bgcolor(sel ? colors.selection : colors.background);

    return pref_line;
}

Element ImageProtocolDialog::renderHelpBar() {
    auto& colors = theme_.getColors();

    return hbox({
               text(" "),
               text("↑↓") | color(colors.helpbar_key) | bold,
               text(": Navigate  "),
               text("Space") | color(colors.helpbar_key) | bold,
               text(": Toggle  "),
               text("Enter") | color(colors.helpbar_key) | bold,
               text(": Apply  "),
               text("Esc") | color(colors.helpbar_key) | bold,
               text(": Cancel"),
               filler(),
           }) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

Element ImageProtocolDialog::render() {
    if (!visible_)
        return {};
    auto& colors = theme_.getColors();

    auto info = pnana::features::ProtocolManager::getTerminalInfo();
    std::string detected = info.name.empty() ? "unknown" : info.name;

    Element detected_line = hbox({text("  Detected Terminal: ") | color(colors.foreground),
                                  text(detected) | color(colors.function) | bold, filler()}) |
                            bgcolor(colors.background);

    auto line_sep = separator();

    int pref_h = responsiveHeight(18, 12);
    int pref_w = responsiveWidth(50, 35);

    auto dialog_content = vbox({
        renderHeader(),
        line_sep,
        detected_line | size(HEIGHT, EQUAL, 1),
        renderToggle(),
        renderProtocolList(),
        renderPreferredSelector(),
        line_sep,
        renderHelpBar(),
    });

    return dialog_content | size(WIDTH, GREATER_THAN, pref_w) | size(HEIGHT, GREATER_THAN, pref_h) |
           bgcolor(colors.background) | border | ftxui::color(colors.dialog_border) | center;
}

} // namespace ui
} // namespace pnana
