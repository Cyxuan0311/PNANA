#include "ui/clipboard_panel.h"
#include "ui/icons.h"
#include <algorithm>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace ui {

constexpr int kClipboardContentWidth = 66;

ClipboardPanel::ClipboardPanel(Theme& theme)
    : theme_(theme), visible_(false), selected_index_(0), scroll_offset_(0), panel_width_(40) {}

Element ClipboardPanel::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements content;

    Elements title_elements;
    title_elements.push_back(text(pnana::ui::icons::PASTE));
    title_elements.push_back(text(" Clipboard History ") | color(colors.foreground) | bold);
    content.push_back(hbox(title_elements) | center);

    content.push_back(separator());

    content.push_back(renderEntries() | flex);

    content.push_back(separator());
    content.push_back(renderStats());

    content.push_back(separator());
    content.push_back(renderFooter());

    Element dialog_content = vbox(content);

    return window(text("Clipboard") | color(colors.foreground), dialog_content) |
           size(WIDTH, EQUAL, panel_width_) | size(HEIGHT, GREATER_THAN, 20) |
           bgcolor(colors.background);
}

Component ClipboardPanel::getComponent() {
    return Renderer([this] {
        return render();
    });
}

void ClipboardPanel::show() {
    visible_ = true;
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void ClipboardPanel::hide() {
    visible_ = false;
}

void ClipboardPanel::addEntry(const std::string& content) {
    history_.addEntry(content);
}

Element ClipboardPanel::renderEntries() {
    auto& colors = theme_.getColors();
    Elements items;

    if (history_.empty()) {
        return vbox({
                   text("") | center,
                   text("No clipboard history yet.") | color(colors.comment) | center,
                   text("Copy some text to populate this panel.") | color(colors.comment) | dim |
                       center,
                   text(""),
               }) |
               flex;
    }

    size_t max_display = 15;

    size_t start_idx = 0;
    if (selected_index_ >= static_cast<int>(max_display)) {
        start_idx = static_cast<size_t>(selected_index_) - max_display + 1;
    }

    size_t end_idx = std::min(start_idx + max_display, history_.size());

    for (size_t i = start_idx; i < end_idx; ++i) {
        bool is_selected = (static_cast<int>(i) == selected_index_);
        items.push_back(renderEntry(history_.getEntries()[i], is_selected, i + 1, history_.size()));
    }

    return vbox(items) | vscroll_indicator | frame | size(HEIGHT, EQUAL, 20) |
           size(WIDTH, EQUAL, panel_width_ - 4);
}

Element ClipboardPanel::renderEntry(const utils::ClipboardEntry& entry, bool is_selected,
                                    size_t index, size_t /*total*/) const {
    auto& colors = theme_.getColors();

    std::string idx = std::to_string(index);
    if (idx.size() < 2)
        idx = " " + idx;

    std::string indicator = is_selected ? "  " : "  ";
    std::string select_mark = entry.selected ? "*" : " ";

    std::string preview = entry.preview;
    size_t max_len = panel_width_ - 18;
    if (preview.size() > max_len) {
        preview = preview.substr(0, max_len - 3) + "...";
    }

    std::string line = indicator + idx + " " + select_mark + " " + entry.timestamp + " " + preview;

    Element line_el = text(line);

    if (is_selected) {
        line_el = line_el | bgcolor(colors.selection) | bold;
    } else {
        line_el = line_el | color(colors.foreground);
    }

    return line_el;
}

Element ClipboardPanel::renderStats() const {
    std::string stats = std::to_string(history_.size()) + " entries";

    size_t selected_count = 0;
    for (const auto& e : history_.getEntries()) {
        if (e.selected) {
            selected_count++;
        }
    }
    if (selected_count > 0) {
        stats += " | " + std::to_string(selected_count) + " selected";
    }

    return text(stats) | dim | center;
}

Element ClipboardPanel::renderFooter() {
    auto& colors = theme_.getColors();

    return vbox({
        text("↑↓ Navigate | Enter Insert | Space Select | d Delete | D Delete All") |
            color(colors.comment) | dim,
        text("=- Resize | Esc Close") | color(colors.comment) | dim,
    });
}

bool ClipboardPanel::handleInput(Event event) {
    if (!visible_)
        return false;

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        moveUp();
        return true;
    }
    if (event == Event::ArrowDown || event == Event::Character('j')) {
        moveDown();
        return true;
    }
    if (event == Event::Return) {
        insertSelected();
        return true;
    }
    if (event.input() == " ") {
        toggleSelect();
        return true;
    }
    if (event == Event::Character('d') || event == Event::Character('D')) {
        if (event == Event::Character('D')) {
            deleteAll();
        } else {
            deleteSelected();
        }
        return true;
    }
    if (event == Event::Character('=')) {
        resizePanel(2);
        return true;
    }
    if (event == Event::Character('-')) {
        resizePanel(-2);
        return true;
    }
    if (event == Event::Escape) {
        hide();
        return true;
    }

    return false;
}

void ClipboardPanel::resizePanel(int delta) {
    panel_width_ += delta;
    panel_width_ = std::clamp(panel_width_, 30, 80);
}

void ClipboardPanel::moveUp() {
    if (selected_index_ > 0) {
        selected_index_--;
        if (selected_index_ < scroll_offset_) {
            scroll_offset_ = selected_index_;
        }
    }
}

void ClipboardPanel::moveDown() {
    if (selected_index_ < static_cast<int>(history_.size()) - 1) {
        selected_index_++;
        if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_ENTRIES) {
            scroll_offset_ = selected_index_ - MAX_VISIBLE_ENTRIES + 1;
        }
    }
}

void ClipboardPanel::insertSelected() {
    if (selected_index_ >= 0 && selected_index_ < static_cast<int>(history_.size())) {
        const std::string& content =
            history_.getEntries()[static_cast<size_t>(selected_index_)].content;
        if (on_insert_text_) {
            on_insert_text_(content);
        }
        hide();
    }
}

void ClipboardPanel::toggleSelect() {
    if (selected_index_ >= 0 && selected_index_ < static_cast<int>(history_.size())) {
        auto& entries = history_.getEntries();
        entries[static_cast<size_t>(selected_index_)].selected =
            !entries[static_cast<size_t>(selected_index_)].selected;
    }
}

void ClipboardPanel::deleteSelected() {
    bool has_selected = false;
    for (const auto& e : history_.getEntries()) {
        if (e.selected) {
            has_selected = true;
            break;
        }
    }

    if (has_selected) {
        history_.removeSelectedEntries();
    } else if (!history_.empty()) {
        history_.removeEntry(static_cast<size_t>(selected_index_));
    }

    if (selected_index_ >= static_cast<int>(history_.size())) {
        selected_index_ = std::max(0, static_cast<int>(history_.size()) - 1);
    }
}

void ClipboardPanel::deleteAll() {
    history_.clear();
    selected_index_ = 0;
    scroll_offset_ = 0;
}

} // namespace ui
} // namespace pnana
