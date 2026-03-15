#include "ui/logo_menu.h"
#include "ui/icons.h"
#include "utils/match_highlight.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

LogoMenu::LogoMenu(Theme& theme) : theme_(theme), selected_index_(0), search_cursor_pos_(0) {
    styles_ = features::LogoManager::getAvailableStyles();
    filtered_indices_.reserve(styles_.size());
    for (size_t i = 0; i < styles_.size(); ++i) {
        filtered_indices_.push_back(i);
    }
}

void LogoMenu::setCurrentStyle(const std::string& style_id) {
    current_style_id_ = style_id;
    size_t style_index = 0;
    for (size_t i = 0; i < styles_.size(); ++i) {
        if (styles_[i].id == style_id) {
            style_index = i;
            break;
        }
    }
    updateFilteredStyles();
    // 在过滤后的列表中定位当前样式，使选中项正确显示
    for (size_t j = 0; j < filtered_indices_.size(); ++j) {
        if (filtered_indices_[j] == style_index) {
            selected_index_ = j;
            return;
        }
    }
    selected_index_ = 0;
}

bool LogoMenu::handleInput(ftxui::Event event) {
    if (event == Event::Escape) {
        return false;
    }
    if (event == Event::Return) {
        return false;
    }
    if (event == Event::Backspace) {
        if (search_cursor_pos_ > 0) {
            search_input_.erase(search_cursor_pos_ - 1, 1);
            search_cursor_pos_--;
            updateFilteredStyles();
        }
        return true;
    }
    if (event == Event::Delete) {
        if (search_cursor_pos_ < search_input_.size()) {
            search_input_.erase(search_cursor_pos_, 1);
            updateFilteredStyles();
        }
        return true;
    }
    if (event == Event::ArrowLeft) {
        if (search_cursor_pos_ > 0)
            search_cursor_pos_--;
        return true;
    }
    if (event == Event::ArrowRight) {
        if (search_cursor_pos_ < search_input_.size())
            search_cursor_pos_++;
        return true;
    }
    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (!filtered_indices_.empty() && selected_index_ < filtered_indices_.size() - 1) {
            selected_index_++;
        }
        return true;
    }
    if (event == Event::PageUp) {
        const size_t page_size = 10;
        if (selected_index_ >= page_size) {
            selected_index_ -= page_size;
        } else {
            selected_index_ = 0;
        }
        return true;
    }
    if (event == Event::PageDown) {
        const size_t page_size = 10;
        size_t next = selected_index_ + page_size;
        if (next >= filtered_indices_.size()) {
            selected_index_ = filtered_indices_.empty() ? 0 : filtered_indices_.size() - 1;
        } else {
            selected_index_ = next;
        }
        return true;
    }
    if (event.is_character()) {
        std::string ch = event.character();
        if (!ch.empty() && ch[0] >= 32) {
            search_input_.insert(search_cursor_pos_, ch);
            search_cursor_pos_ += ch.size();
            updateFilteredStyles();
        }
        return true;
    }
    return false;
}

void LogoMenu::updateFilteredStyles() {
    filtered_indices_.clear();
    if (search_input_.empty()) {
        for (size_t i = 0; i < styles_.size(); ++i) {
            filtered_indices_.push_back(i);
        }
    } else {
        std::string search_lower = search_input_;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
        for (size_t i = 0; i < styles_.size(); ++i) {
            std::string id_lower = styles_[i].id;
            std::string name_lower = styles_[i].display_name;
            std::transform(id_lower.begin(), id_lower.end(), id_lower.begin(), ::tolower);
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            if (id_lower.find(search_lower) != std::string::npos ||
                name_lower.find(search_lower) != std::string::npos) {
                filtered_indices_.push_back(i);
            }
        }
    }
    if (selected_index_ >= filtered_indices_.size() && !filtered_indices_.empty()) {
        selected_index_ = filtered_indices_.size() - 1;
    }
    if (filtered_indices_.empty()) {
        selected_index_ = 0;
    }
}

std::string LogoMenu::getSelectedStyleId() const {
    if (selected_index_ < filtered_indices_.size()) {
        size_t idx = filtered_indices_[selected_index_];
        if (idx < styles_.size()) {
            return styles_[idx].id;
        }
    }
    return "block";
}

Element LogoMenu::renderSearchBox() const {
    auto& colors = theme_.getColors();
    std::string left = search_input_.substr(0, search_cursor_pos_);
    std::string right = search_input_.substr(search_cursor_pos_);
    return hbox({text(" "), text(icons::SEARCH) | color(colors.function), text(" "),
                 text(left) | color(colors.foreground), text("█") | color(colors.success) | bold,
                 text(right) | color(colors.foreground)}) |
           bgcolor(colors.selection);
}

Element LogoMenu::renderStyleList() const {
    auto& colors = theme_.getColors();
    Elements list_elements;
    const size_t max_display = 18;
    size_t start = 0;
    if (filtered_indices_.size() > max_display) {
        if (selected_index_ >= filtered_indices_.size() - max_display) {
            start = filtered_indices_.size() - max_display;
        } else {
            start = selected_index_;
        }
    }
    if (filtered_indices_.empty()) {
        if (!search_input_.empty()) {
            list_elements.push_back(hbox({text("  No styles match: \"") | color(colors.comment),
                                          text(search_input_) | color(colors.foreground),
                                          text("\"") | color(colors.comment)}));
        } else {
            list_elements.push_back(text("  No styles available") | color(colors.comment) | dim);
        }
    } else {
        for (size_t i = 0; i < max_display && (start + i) < filtered_indices_.size(); ++i) {
            size_t idx = filtered_indices_[start + i];
            const auto& entry = styles_[idx];
            bool is_selected = (start + i == selected_index_);
            bool is_current = (entry.id == current_style_id_);
            std::string display_name = entry.display_name;
            if (is_current) {
                display_name += " ";
                display_name += icons::SUCCESS;
            }
            Color name_color = is_selected ? colors.function : colors.foreground;
            Element name_el = pnana::utils::highlightMatch(display_name, search_input_, name_color,
                                                           colors.keyword);
            if (is_selected)
                name_el = name_el | bold;
            Elements row = {text("  "),
                            (is_selected ? text("► ") | color(colors.function) | bold : text("  ")),
                            std::move(name_el)};
            Element line = hbox(row);
            if (is_selected) {
                line = line | bgcolor(colors.selection);
            }
            list_elements.push_back(line);
        }
    }
    return vbox(list_elements) | flex;
}

Element LogoMenu::renderLogoPreview() const {
    auto& colors = theme_.getColors();
    std::string style_id = getSelectedStyleId();
    std::vector<std::string> lines = features::LogoManager::getLogoLines(style_id);
    std::vector<Color> gradient_colors = theme_.getGradientColors();
    if (gradient_colors.size() < 6) {
        gradient_colors = {colors.success, colors.success, colors.success,
                           colors.success, colors.success, colors.success};
    }
    Elements preview_elements;
    preview_elements.push_back(hbox({text(" "), text(style_id) | bold | color(colors.foreground)}) |
                               bgcolor(colors.dialog_title_bg));
    preview_elements.push_back(separator());
    for (size_t i = 0; i < lines.size(); ++i) {
        size_t g = i % gradient_colors.size();
        preview_elements.push_back(text("  " + lines[i]) | color(gradient_colors[g]) | bold);
    }
    return vbox(preview_elements) | bgcolor(colors.dialog_bg) | size(WIDTH, EQUAL, 70) |
           borderWithColor(colors.dialog_border);
}

Element LogoMenu::render() {
    auto& colors = theme_.getColors();
    Element title_bar =
        hbox({text(" "), text(icons::FONT) | color(Color::Cyan),
              text(" Select Logo Style ") | bold | color(colors.foreground), filler()}) |
        bgcolor(colors.menubar_bg);

    Elements left_content;
    left_content.push_back(renderSearchBox());
    left_content.push_back(separator());
    left_content.push_back(renderStyleList());
    Element left_panel = vbox(left_content) | size(WIDTH, EQUAL, 28) | flex;
    Element right_panel = renderLogoPreview();
    Element main_content =
        hbox({left_panel, separator(), right_panel}) | flex | size(HEIGHT, EQUAL, 22);

    Element help_bar =
        hbox({text(" "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Select  "),
              text("Enter") | color(colors.helpbar_key) | bold, text(": Apply  "),
              text("Type") | color(colors.helpbar_key) | bold, text(": Filter  "),
              text("Esc") | color(colors.helpbar_key) | bold, text(": Cancel"), filler()}) |
        bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;

    return vbox({title_bar, separator(), main_content, separator(), help_bar}) |
           borderWithColor(colors.dialog_border) | bgcolor(colors.background) |
           size(WIDTH, GREATER_THAN, 75) | size(HEIGHT, GREATER_THAN, 24);
}

} // namespace ui
} // namespace pnana
