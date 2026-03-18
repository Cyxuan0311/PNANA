#include "ui/terminal_session_dialog.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

TerminalSessionDialog::TerminalSessionDialog(Theme& theme)
    : theme_(theme), visible_(false), selected_index_(0) {
    resetChoices();
}

void TerminalSessionDialog::resetChoices() {
    choices_.clear();
    choices_.push_back({"Default (env: $SHELL)", "Use your login shell", ""});
    choices_.push_back({"/bin/bash", "GNU bash", "/bin/bash"});
    choices_.push_back({"/bin/zsh", "Z shell", "/bin/zsh"});
    choices_.push_back({"/bin/fish", "Friendly interactive shell", "/bin/fish"});
    choices_.push_back({"/bin/sh", "POSIX sh", "/bin/sh"});
}

void TerminalSessionDialog::show(std::function<void(const TerminalSessionChoice&)> on_confirm,
                                 std::function<void()> on_cancel) {
    visible_ = true;
    selected_index_ = 0;
    resetChoices();
    on_confirm_ = std::move(on_confirm);
    on_cancel_ = std::move(on_cancel);
}

bool TerminalSessionDialog::handleInput(ftxui::Event event) {
    if (!visible_) {
        return false;
    }

    if (event == Event::Escape) {
        visible_ = false;
        if (on_cancel_) {
            on_cancel_();
        }
        return true;
    }

    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
        }
        return true;
    }

    if (event == Event::ArrowDown) {
        if (selected_index_ + 1 < choices_.size()) {
            selected_index_++;
        }
        return true;
    }

    if (event == Event::Return) {
        if (selected_index_ < choices_.size() && on_confirm_) {
            on_confirm_(choices_[selected_index_]);
        }
        visible_ = false;
        return true;
    }

    return false;
}

ftxui::Element TerminalSessionDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements rows;

    rows.push_back(hbox({text(" "), text(icons::TERMINAL) | color(colors.success),
                         text(" New Terminal Session ") | bold, text(" ")}) |
                   bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center);

    rows.push_back(separator());
    rows.push_back(text(" "));

    for (size_t i = 0; i < choices_.size(); ++i) {
        const auto& choice = choices_[i];
        bool selected = (i == selected_index_);
        Elements line;
        line.push_back(text("  "));
        line.push_back(text(selected ? "► " : "  ") |
                       color(selected ? colors.success : colors.comment));
        line.push_back(text(choice.label) |
                       (selected ? color(colors.foreground) | bold : color(colors.foreground)));
        line.push_back(filler());
        line.push_back(text(choice.description) | color(colors.comment) | dim);

        Element row = hbox(line);
        if (selected) {
            row = row | bgcolor(colors.selection);
        }
        rows.push_back(row);
    }

    rows.push_back(separator());
    rows.push_back(
        hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
              text("Enter") | color(colors.helpbar_key) | bold, text(": Open  "),
              text("Esc") | color(colors.helpbar_key) | bold, text(": Cancel")}) |
        bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim);

    return window(text(""), vbox(rows)) | size(WIDTH, EQUAL, 72) | size(HEIGHT, EQUAL, 13) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border) | center;
}

} // namespace ui
} // namespace pnana
