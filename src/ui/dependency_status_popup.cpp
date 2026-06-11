#include "ui/dependency_status_popup.h"
#include "ui/icons.h"
#include "ui/responsive_size.h"
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

DependencyStatusPopup::DependencyStatusPopup(Theme& theme) : theme_(theme) {}

void DependencyStatusPopup::open() {
    is_open_ = true;
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void DependencyStatusPopup::close() {
    is_open_ = false;
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void DependencyStatusPopup::setData(const std::vector<DependencyEntry>& entries) {
    entries_ = entries;
    if (!entries_.empty() && selected_index_ >= entries_.size())
        selected_index_ = entries_.size() - 1;
}

bool DependencyStatusPopup::handleInput(Event event) {
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
            if (selected_index_ >= scroll_offset_ + display_count_)
                scroll_offset_ = selected_index_ - display_count_ + 1;
        }
        return true;
    }
    return false;
}

Element DependencyStatusPopup::render() {
    if (!is_open_)
        return text("");

    const auto& colors = theme_.getColors();
    int dialog_w = responsiveWidth(100, 50);
    int dialog_h = responsiveHeight(30, 14);

    Elements content;
    content.push_back(renderTitle());
    content.push_back(separator());
    content.push_back(renderTable() | flex);
    content.push_back(separator());
    content.push_back(renderHelpBar());

    return window(text(""), vbox(content)) | size(WIDTH, EQUAL, dialog_w) |
           size(HEIGHT, EQUAL, dialog_h) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

Element DependencyStatusPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(icons::INFO) | color(colors.function),
                 text(" Build Dependencies & Status "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element DependencyStatusPopup::renderTable() const {
    const auto& colors = theme_.getColors();
    Elements rows;

    int col_marker = 3;
    int col_name = 24;
    int col_version = 14;
    int col_type = 10;

    auto header_cell = [&](const std::string& s, int w) {
        return text(" " + s + " ") | bold | color(colors.dialog_fg) | size(WIDTH, EQUAL, w);
    };

    Elements header_row;
    header_row.push_back(text(" ") | size(WIDTH, EQUAL, col_marker));
    header_row.push_back(header_cell("Name", col_name));
    header_row.push_back(header_cell("Version", col_version));
    header_row.push_back(header_cell("Type", col_type));
    header_row.push_back(text(" Description") | bold | color(colors.dialog_fg) | flex);
    rows.push_back(hbox(header_row) | bgcolor(colors.current_line));
    rows.push_back(separator());

    size_t start = scroll_offset_;
    size_t end = std::min(start + display_count_, entries_.size());

    for (size_t i = start; i < end; ++i) {
        const auto& entry = entries_[i];
        bool is_selected = (i == selected_index_);

        Color st_color = entry.enabled ? colors.success : colors.comment;
        std::string st_icon = entry.enabled ? "\u25C9" : "\u25CB";

        Element marker;
        if (is_selected)
            marker =
                text(" " + std::string(icons::ARROW_RIGHT) + " ") | color(colors.function) | bold;
        else
            marker = text("   ") | size(WIDTH, EQUAL, col_marker);

        Color n_color = entry.enabled ? colors.foreground : colors.comment;
        auto name_el = text(" " + entry.name + " ") | color(n_color) | size(WIDTH, EQUAL, col_name);

        auto ver_el = text(" " + entry.version + " ") | color(colors.comment) |
                      size(WIDTH, EQUAL, col_version);

        Color t_color = colors.keyword;
        if (entry.type == "Required")
            t_color = colors.function;
        else if (entry.type == "Bundled")
            t_color = colors.info;
        else if (entry.type == "Build")
            t_color = colors.comment;
        auto type_el = text(" " + entry.type + " ") | color(t_color) | size(WIDTH, EQUAL, col_type);

        auto desc_el = text(" " + entry.description) | color(colors.comment) | dim | flex;

        Elements row;
        row.push_back(marker);
        row.push_back(text(st_icon) | color(st_color) | size(WIDTH, EQUAL, 1));
        row.push_back(name_el);
        row.push_back(ver_el);
        row.push_back(type_el);
        row.push_back(desc_el);

        Element row_el = hbox(row);

        if (is_selected)
            row_el = row_el | bgcolor(colors.selection) | bold;
        else if (!entry.enabled)
            row_el = row_el | dim;
        else if (i % 2 == 1)
            row_el = row_el | bgcolor(colors.current_line);

        rows.push_back(row_el);
    }

    if (entries_.empty()) {
        rows.push_back(text("  No dependency information available") | color(colors.comment) | dim);
    }

    return vbox(rows) | bgcolor(colors.background) | yframe;
}

Element DependencyStatusPopup::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Close")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

} // namespace ui
} // namespace pnana
