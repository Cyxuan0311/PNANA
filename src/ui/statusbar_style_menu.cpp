#include "ui/statusbar_style_menu.h"
#include "ui/statusbar_theme.h"
#include "utils/match_highlight.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

StatusbarStyleMenu::StatusbarStyleMenu(Statusbar& statusbar, Theme& theme)
    : statusbar_(statusbar), theme_(theme), selected_index_(0), search_cursor_pos_(0) {
    available_styles_ = getAvailableStatusbarStyleNames();
    filtered_styles_ = available_styles_;
    filtered_indices_.reserve(available_styles_.size());
    for (size_t i = 0; i < available_styles_.size(); ++i) {
        filtered_indices_.push_back(i);
    }
}

void StatusbarStyleMenu::setAvailableStyles(const std::vector<std::string>& styles) {
    available_styles_ = styles;
    updateFilteredStyles();
    if (selected_index_ >= filtered_styles_.size() && !filtered_styles_.empty()) {
        selected_index_ = 0;
    }
}

void StatusbarStyleMenu::setSelectedIndex(size_t index) {
    if (index < filtered_styles_.size()) {
        selected_index_ = index;
    }
}

bool StatusbarStyleMenu::handleInput(ftxui::Event event) {
    if (event == Event::Escape) {
        if (!search_input_.empty()) {
            search_input_.clear();
            search_cursor_pos_ = 0;
            updateFilteredStyles();
            // 清空搜索后按当前选中项预览
            applySelectedStyle();
            return true;
        }
        return false;
    } else if (event == Event::Return) {
        applySelectedStyle();
        if (on_style_confirmed_)
            on_style_confirmed_(getSelectedStyle());
        return false;
    } else if (event == Event::Backspace) {
        if (search_cursor_pos_ > 0) {
            search_input_.erase(search_cursor_pos_ - 1, 1);
            search_cursor_pos_--;
            updateFilteredStyles();
            applySelectedStyle();
        }
        return true;
    } else if (event == Event::Delete) {
        if (search_cursor_pos_ < search_input_.size()) {
            search_input_.erase(search_cursor_pos_, 1);
            updateFilteredStyles();
            applySelectedStyle();
        }
        return true;
    } else if (event == Event::ArrowLeft) {
        if (search_cursor_pos_ > 0)
            search_cursor_pos_--;
        return true;
    } else if (event == Event::ArrowRight) {
        if (search_cursor_pos_ < search_input_.size())
            search_cursor_pos_++;
        return true;
    } else if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
            // 像主题菜单一样，移动选中项时实时预览
            applySelectedStyle();
        }
        return true;
    } else if (event == Event::ArrowDown) {
        if (!filtered_styles_.empty() && selected_index_ < filtered_styles_.size() - 1) {
            selected_index_++;
            applySelectedStyle();
        }
        return true;
    } else if (event == Event::PageUp) {
        if (!filtered_styles_.empty()) {
            const size_t page_size = 10;
            if (selected_index_ >= page_size) {
                selected_index_ -= page_size;
            } else {
                selected_index_ = 0;
            }
            applySelectedStyle();
        }
        return true;
    } else if (event == Event::PageDown) {
        if (!filtered_styles_.empty()) {
            const size_t page_size = 10;
            size_t next = selected_index_ + page_size;
            if (next >= filtered_styles_.size()) {
                selected_index_ = filtered_styles_.size() - 1;
            } else {
                selected_index_ = next;
            }
            applySelectedStyle();
        }
        return true;
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (!ch.empty() && ch[0] >= 32) {
            search_input_.insert(search_cursor_pos_, ch);
            search_cursor_pos_ += ch.size();
            updateFilteredStyles();
            // 输入搜索关键字时，也根据当前选中项实时预览
            applySelectedStyle();
        }
        return true;
    }

    return false;
}

void StatusbarStyleMenu::updateFilteredStyles() {
    filtered_styles_.clear();
    filtered_indices_.clear();

    if (search_input_.empty()) {
        filtered_styles_ = available_styles_;
        filtered_indices_.reserve(available_styles_.size());
        for (size_t i = 0; i < available_styles_.size(); ++i) {
            filtered_indices_.push_back(i);
        }
    } else {
        std::string search_lower = search_input_;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        for (size_t i = 0; i < available_styles_.size(); ++i) {
            std::string style_lower = available_styles_[i];
            std::transform(style_lower.begin(), style_lower.end(), style_lower.begin(), ::tolower);

            if (style_lower.find(search_lower) != std::string::npos) {
                filtered_styles_.push_back(available_styles_[i]);
                filtered_indices_.push_back(i);
            }
        }
    }

    if (selected_index_ >= filtered_styles_.size() && !filtered_styles_.empty()) {
        selected_index_ = filtered_styles_.size() - 1;
    }
    if (filtered_styles_.empty()) {
        selected_index_ = 0;
    }
}

void StatusbarStyleMenu::applySelectedStyle() {
    std::string style = getSelectedStyle();
    if (style.empty())
        return;
    // 与当前一致时跳过，避免重复 set 触发整屏重绘导致卡顿
    if (style == statusbar_.getBeautifyConfig().style_name)
        return;
    statusbar_.setBeautifyConfig(getStatusbarConfigForStyle(style));
}

std::string StatusbarStyleMenu::getSelectedStyle() const {
    if (selected_index_ < filtered_styles_.size()) {
        return filtered_styles_[selected_index_];
    }
    return "";
}

Element StatusbarStyleMenu::renderSearchBox() const {
    auto& colors = theme_.getColors();
    std::string left = search_input_.substr(0, search_cursor_pos_);
    std::string right = search_input_.substr(search_cursor_pos_);
    return hbox({text(" "), text(icons::SEARCH) | color(colors.function), text(" "),
                 text(left) | color(colors.foreground), text("█") | color(colors.keyword) | bold,
                 text(right) | color(colors.foreground)}) |
           bgcolor(colors.selection);
}

Element StatusbarStyleMenu::renderStyleList() const {
    auto& current_colors = theme_.getColors();
    Elements list_elements;

    const size_t max_display = 10;
    size_t start = 0;
    if (filtered_styles_.size() > max_display) {
        if (selected_index_ >= filtered_styles_.size() - max_display) {
            start = filtered_styles_.size() - max_display;
        } else {
            start = selected_index_;
        }
    }

    if (filtered_styles_.empty()) {
        if (!search_input_.empty()) {
            list_elements.push_back(
                hbox({text("  No styles match: \"") | color(current_colors.comment),
                      text(search_input_) | color(current_colors.foreground),
                      text("\"") | color(current_colors.comment)}));
        } else {
            list_elements.push_back(text("  No styles available") | color(current_colors.comment) |
                                    dim);
        }
    } else {
        for (size_t i = 0; i < max_display && (start + i) < filtered_styles_.size(); ++i) {
            size_t idx = start + i;
            std::string style_name = filtered_styles_[idx];
            bool is_selected = (idx == selected_index_);

            Color name_color = is_selected ? current_colors.function : current_colors.foreground;
            Element name_el = pnana::utils::highlightMatch(style_name, search_input_, name_color,
                                                           current_colors.keyword);
            if (is_selected)
                name_el = name_el | bold;
            Elements row = {
                text("  "),
                (is_selected ? text("► ") | color(current_colors.function) | bold : text("  ")),
                std::move(name_el)};

            Element line = hbox(row);
            if (is_selected) {
                line = line | bgcolor(current_colors.selection);
            }
            list_elements.push_back(line);
        }
    }

    return vbox(list_elements) | flex;
}

Element StatusbarStyleMenu::renderStylePreview() const {
    auto& current_colors = theme_.getColors();
    Elements preview_elements;

    if (filtered_styles_.empty() || selected_index_ >= filtered_styles_.size()) {
        return hbox({text("  "), text("Select a style") | color(current_colors.comment) | dim}) |
               bgcolor(current_colors.dialog_bg) | flex;
    }

    std::string selected_name = filtered_styles_[selected_index_];

    preview_elements.push_back(
        hbox({text(" "), text(selected_name) | bold | color(current_colors.foreground)}) |
        bgcolor(current_colors.dialog_title_bg));
    preview_elements.push_back(separator());

    // 样式预览信息
    std::vector<std::pair<std::string, std::string>> style_info;
    if (selected_name == "default") {
        style_info = {{"Style", "Default (Theme)"},
                      {"Description", "Use theme's built-in statusbar without extra decorations"}};
    } else if (selected_name == "neovim") {
        style_info = {{"Icon Style", "default"},
                      {"Gradient", "false"},
                      {"Shadows", "false"},
                      {"Rounded Corners", "false"},
                      {"Description", "Neovim-style statusbar"}};
    } else if (selected_name == "vscode") {
        style_info = {{"Icon Style", "filled"},
                      {"Gradient", "false"},
                      {"Shadows", "false"},
                      {"Rounded Corners", "false"},
                      {"Description", "VS Code-style: flat bar, thin top line, muted region"}};
    } else if (selected_name == "minimal") {
        style_info = {{"Icon Style", "outlined"},
                      {"Gradient", "false"},
                      {"Shadows", "false"},
                      {"Rounded Corners", "false"},
                      {"Description", "Minimal statusbar"}};
    } else if (selected_name == "classic") {
        style_info = {{"Icon Style", "default"},
                      {"Gradient", "false"},
                      {"Shadows", "false"},
                      {"Rounded Corners", "false"},
                      {"Description", "Classic statusbar"}};
    } else if (selected_name == "highlight") {
        style_info = {{"Style", "Highlight"},
                      {"Description", "All statusbar items with highlighted background"}};
    } else if (selected_name == "rounded") {
        style_info = {{"Rounded Corners", "true"},
                      {"Description", "Statusbar with rounded border"}};
    } else if (selected_name == "unicode") {
        style_info = {{"Decor", "╭─ ║ ─╮"}, {"Description", "Unicode box-drawing (╭ ╮ ║ ─)"}};
    }

    for (const auto& [key, value] : style_info) {
        preview_elements.push_back(hbox({text("  " + key + ": ") | color(current_colors.comment),
                                         text(value) | color(current_colors.foreground) | bold}));
    }

    return vbox(preview_elements) | bgcolor(current_colors.dialog_bg) | size(WIDTH, EQUAL, 30) |
           borderWithColor(current_colors.dialog_border);
}

Element StatusbarStyleMenu::render() {
    auto& current_colors = theme_.getColors();

    // 标题栏
    Element title_bar =
        hbox({text(" "), text(icons::SETTINGS) | color(Color::Cyan),
              text(" Statusbar Style ") | bold | color(current_colors.foreground), filler()}) |
        bgcolor(current_colors.menubar_bg);

    // 左侧：搜索框 + 样式列表
    Elements left_content;
    left_content.push_back(renderSearchBox());
    left_content.push_back(separator());
    left_content.push_back(renderStyleList());

    Element left_panel = vbox(left_content) | size(WIDTH, EQUAL, 30) | flex;

    // 右侧：样式预览
    Element right_panel = renderStylePreview();

    // 主内容区：左右布局
    Element main_content =
        hbox({left_panel, separator(), right_panel}) | flex | size(HEIGHT, EQUAL, 15);

    // 底部提示
    Element help_bar =
        hbox({text(" "), text("↑↓") | color(current_colors.helpbar_key) | bold, text(": Select  "),
              text("Enter") | color(current_colors.helpbar_key) | bold, text(": Apply  "),
              text("Type") | color(current_colors.helpbar_key) | bold, text(": Filter  "),
              text("Esc") | color(current_colors.helpbar_key) | bold, text(": Cancel"), filler()}) |
        bgcolor(current_colors.helpbar_bg) | color(current_colors.helpbar_fg) | dim;

    return vbox({title_bar, separator(), main_content, separator(), help_bar}) |
           borderWithColor(current_colors.dialog_border) | bgcolor(current_colors.background) |
           size(WIDTH, GREATER_THAN, 70) | size(HEIGHT, GREATER_THAN, 17);
}

} // namespace ui
} // namespace pnana