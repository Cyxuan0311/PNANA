#include "ui/lsp_status_popup.h"
#include "ui/icons.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

static inline Decorator borderWithColor(Color c) {
    return [=](Element child) {
        return child | border | color(c);
    };
}

LspStatusPopup::LspStatusPopup(Theme& theme) : theme_(theme) {}

void LspStatusPopup::setStatusProvider(
    std::function<std::vector<features::LspStatusEntry>()> provider) {
    status_provider_ = std::move(provider);
}

void LspStatusPopup::open() {
    is_open_ = true;
    refreshEntries();
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void LspStatusPopup::close() {
    is_open_ = false;
}

void LspStatusPopup::refreshEntries() {
    if (status_provider_)
        entries_ = status_provider_();
    else
        entries_.clear();
}

bool LspStatusPopup::handleInput(Event event) {
    if (!is_open_)
        return false;
    if (event == Event::Escape) {
        close();
        return true;
    }
    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
            if (selected_index_ < scroll_offset_)
                scroll_offset_ = selected_index_;
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (!entries_.empty() && selected_index_ < entries_.size() - 1) {
            selected_index_++;
            if (selected_index_ >= scroll_offset_ + list_display_count_)
                scroll_offset_ = selected_index_ - list_display_count_ + 1;
        }
        return true;
    }
    return true;
}

Element LspStatusPopup::render() {
    if (!is_open_)
        return text("");
    refreshEntries();

    const auto& colors = theme_.getColors();
    Elements content;
    content.push_back(renderTitle());
    content.push_back(separator());
    content.push_back(hbox({
                          renderLeftList() | size(WIDTH, EQUAL, 52),
                          separator(),
                          renderRightDetail() | flex,
                      }) |
                      flex);
    content.push_back(separator());
    content.push_back(renderHelpBar());

    return window(text("LSP Connection Status"), vbox(content)) | size(WIDTH, EQUAL, 120) |
           size(HEIGHT, EQUAL, 28) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

Element LspStatusPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(icons::REFRESH) | color(colors.function),
                 text(" LSP — Servers & Connection "), text(" ")}) |
           bold | bgcolor(colors.current_line) | color(colors.foreground) | center;
}

Element LspStatusPopup::renderLeftList() const {
    const auto& colors = theme_.getColors();
    Elements lines;
    size_t start = scroll_offset_;
    size_t end = std::min(start + list_display_count_, entries_.size());

    for (size_t i = start; i < end; ++i) {
        const auto& e = entries_[i];
        bool selected = (i == selected_index_);
        std::string status = e.connected ? "●" : "○";
        std::string label = e.config.language_id + " | " + e.config.name + " " + status;
        Element row = text("  " + label);
        if (selected) {
            row = row | bgcolor(colors.function) | color(colors.background) | bold;
        } else {
            row = row | color(e.connected ? colors.success : colors.comment);
        }
        lines.push_back(row);
    }
    if (lines.empty()) {
        lines.push_back(text("  No LSP servers configured") | color(colors.comment) | dim);
    }
    return vbox(lines) | bgcolor(colors.background) | yframe;
}

Element LspStatusPopup::renderRightDetail() const {
    const auto& colors = theme_.getColors();
    Elements block;

    if (entries_.empty()) {
        block.push_back(text("  No servers to show.") | color(colors.comment) | dim);
        return vbox(block) | bgcolor(colors.background) | yframe;
    }
    if (selected_index_ >= entries_.size()) {
        block.push_back(text("  Select an entry.") | color(colors.comment) | dim);
        return vbox(block) | bgcolor(colors.background) | yframe;
    }

    const auto& e = entries_[selected_index_];
    const auto& c = e.config;

    auto line = [&](const std::string& key, const std::string& val) {
        return hbox({text("  " + key + ": ") | color(colors.comment) | size(WIDTH, EQUAL, 14),
                     text(val) | color(colors.foreground)});
    };

    block.push_back(line("Name", c.name));
    block.push_back(line("Command", c.command));
    block.push_back(line("Language ID", c.language_id));
    block.push_back(line("Status", e.connected ? "Connected" : "Not connected"));

    std::string exts;
    for (auto it = c.file_extensions.begin(); it != c.file_extensions.end(); ++it) {
        if (it != c.file_extensions.begin())
            exts += ", ";
        exts += *it;
    }
    block.push_back(line("Extensions", exts.empty() ? "(none)" : exts));

    std::string args_str;
    for (size_t i = 0; i < c.args.size(); ++i) {
        if (i)
            args_str += " ";
        args_str += c.args[i];
    }
    block.push_back(line("Args", args_str.empty() ? "(none)" : args_str));

    if (!c.env_vars.empty()) {
        block.push_back(text("  Env:") | color(colors.comment));
        for (const auto& [k, v] : c.env_vars) {
            block.push_back(hbox({text("    " + k + "=") | color(colors.keyword),
                                  text(v) | color(colors.foreground)}));
        }
    }

    return vbox(block) | bgcolor(colors.background) | yframe;
}

Element LspStatusPopup::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  ↑↓") | color(colors.keyword) | bold, text(": Select  "),
                 text("Esc") | color(colors.keyword) | bold, text(": Close")}) |
           color(colors.comment) | dim;
}

} // namespace ui
} // namespace pnana
