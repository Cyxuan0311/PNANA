#include "ui/theme_menu.h"
#include "ui/icons.h"
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

ThemeMenu::ThemeMenu(Theme& theme) : theme_(theme), selected_index_(0), search_cursor_pos_(0) {
    filtered_themes_ = available_themes_;
    filtered_indices_.reserve(available_themes_.size());
    for (size_t i = 0; i < available_themes_.size(); ++i) {
        filtered_indices_.push_back(i);
    }
}

void ThemeMenu::setAvailableThemes(const std::vector<std::string>& themes) {
    available_themes_ = themes;
    updateFilteredThemes();
    if (selected_index_ >= filtered_themes_.size() && !filtered_themes_.empty()) {
        selected_index_ = 0;
    }
}

void ThemeMenu::setSelectedIndex(size_t index) {
    if (index < filtered_themes_.size()) {
        selected_index_ = index;
    }
}

void ThemeMenu::setCursorColorGetter(std::function<ftxui::Color()> getter) {
    cursor_color_getter_ = std::move(getter);
}

bool ThemeMenu::handleInput(ftxui::Event event) {
    if (event == Event::Escape) {
        if (!search_input_.empty()) {
            search_input_.clear();
            search_cursor_pos_ = 0;
            updateFilteredThemes();
            return true; // 消费：仅清空搜索
        }
        return false; // 不消费：让父级关闭弹窗
    } else if (event == Event::Return) {
        return false; // 不消费：让父级应用主题并关闭
    } else if (event == Event::Backspace) {
        if (search_cursor_pos_ > 0) {
            search_input_.erase(search_cursor_pos_ - 1, 1);
            search_cursor_pos_--;
            updateFilteredThemes();
        }
        return true;
    } else if (event == Event::Delete) {
        if (search_cursor_pos_ < search_input_.size()) {
            search_input_.erase(search_cursor_pos_, 1);
            updateFilteredThemes();
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
        }
        return true;
    } else if (event == Event::ArrowDown) {
        if (!filtered_themes_.empty() && selected_index_ < filtered_themes_.size() - 1) {
            selected_index_++;
        }
        return true;
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (!ch.empty() && ch[0] >= 32) {
            search_input_.insert(search_cursor_pos_, ch);
            search_cursor_pos_ += ch.size();
            updateFilteredThemes();
        }
        return true;
    }

    return false;
}

void ThemeMenu::updateFilteredThemes() {
    filtered_themes_.clear();
    filtered_indices_.clear();

    if (search_input_.empty()) {
        // 没有搜索输入，显示所有主题
        filtered_themes_ = available_themes_;
        filtered_indices_.reserve(available_themes_.size());
        for (size_t i = 0; i < available_themes_.size(); ++i) {
            filtered_indices_.push_back(i);
        }
    } else {
        // 根据搜索输入过滤主题
        std::string search_lower = search_input_;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        for (size_t i = 0; i < available_themes_.size(); ++i) {
            std::string theme_lower = available_themes_[i];
            std::transform(theme_lower.begin(), theme_lower.end(), theme_lower.begin(), ::tolower);

            if (theme_lower.find(search_lower) != std::string::npos) {
                filtered_themes_.push_back(available_themes_[i]);
                filtered_indices_.push_back(i);
            }
        }
    }

    // 确保选中索引有效
    if (selected_index_ >= filtered_themes_.size() && !filtered_themes_.empty()) {
        selected_index_ = filtered_themes_.size() - 1;
    }
    if (filtered_themes_.empty()) {
        selected_index_ = 0;
    }
}

std::string ThemeMenu::getSelectedThemeName() const {
    if (selected_index_ < filtered_themes_.size()) {
        return filtered_themes_[selected_index_];
    }
    return "";
}

std::string ThemeMenu::getCurrentThemeName() const {
    return theme_.getCurrentThemeName();
}

Element ThemeMenu::renderSearchBox() const {
    auto& colors = theme_.getColors();
    std::string left = search_input_.substr(0, search_cursor_pos_);
    std::string right = search_input_.substr(search_cursor_pos_);
    // 块状光标：使用 █ 表示，颜色跟随光标配置
    ftxui::Color cursor_color = cursor_color_getter_ ? cursor_color_getter_() : colors.success;
    return hbox({text(" "), text(icons::SEARCH) | color(colors.function), text(" "),
                 text(left) | color(colors.foreground), text("█") | color(cursor_color) | bold,
                 text(right) | color(colors.foreground)}) |
           bgcolor(colors.selection);
}

Element ThemeMenu::renderThemeList() const {
    auto& current_colors = theme_.getColors();
    Elements list_elements;

    const size_t max_display = 20;
    size_t start = 0;
    if (filtered_themes_.size() > max_display) {
        if (selected_index_ >= filtered_themes_.size() - max_display) {
            start = filtered_themes_.size() - max_display;
        } else {
            start = selected_index_;
        }
    }

    if (filtered_themes_.empty()) {
        if (!search_input_.empty()) {
            list_elements.push_back(
                hbox({text("  No themes match: \"") | color(current_colors.comment),
                      text(search_input_) | color(current_colors.foreground),
                      text("\"") | color(current_colors.comment)}));
        } else {
            list_elements.push_back(text("  No themes available") | color(current_colors.comment) |
                                    dim);
        }
    } else {
        for (size_t i = 0; i < max_display && (start + i) < filtered_themes_.size(); ++i) {
            size_t idx = start + i;
            std::string theme_name = filtered_themes_[idx];
            bool is_selected = (idx == selected_index_);

            std::string display_name = theme_name;
            if (theme_name == theme_.getCurrentThemeName()) {
                display_name += " " + std::string(icons::SUCCESS);
            }

            Elements row = {
                text("  "),
                (is_selected ? text("► ") | color(current_colors.function) | bold : text("  ")),
                text(display_name) | (is_selected ? color(current_colors.function) | bold
                                                  : color(current_colors.foreground))};

            Element line = hbox(row);
            if (is_selected) {
                line = line | bgcolor(current_colors.selection);
            }
            list_elements.push_back(line);
        }
    }

    return vbox(list_elements) | flex;
}

Element ThemeMenu::renderColorPreview() const {
    auto& current_colors = theme_.getColors();
    Elements preview_elements;

    if (filtered_themes_.empty() || selected_index_ >= filtered_themes_.size()) {
        return hbox({text("  "), text("Select a theme") | color(current_colors.comment) | dim}) |
               bgcolor(current_colors.dialog_bg) | flex;
    }

    std::string selected_name = filtered_themes_[selected_index_];
    Theme temp_theme;
    temp_theme.setTheme(selected_name);
    auto& tc = temp_theme.getColors();

    // 预览标题
    preview_elements.push_back(
        hbox({text(" "), text(selected_name) | bold | color(current_colors.foreground)}) |
        bgcolor(current_colors.dialog_title_bg));
    preview_elements.push_back(separator());

    // 颜色色块网格（2列，固定宽度对齐）
    struct ColorItem {
        const char* label;
        ftxui::Color color;
    };
    constexpr size_t LABEL_WIDTH = 14; // 标签固定宽度，保证列对齐
    std::vector<ColorItem> items = {
        {"Background", tc.background},
        {"Foreground", tc.foreground},
        {"Keyword", tc.keyword},
        {"String", tc.string},
        {"Comment", tc.comment},
        {"Function", tc.function},
        {"Type", tc.type},
        {"Number", tc.number},
        {"Selection", tc.selection},
        {"Error", tc.error},
        {"Current Line", tc.current_line},
        {"Line Number", tc.line_number},
    };

    auto makeColorCell = [&tc, LABEL_WIDTH](const ColorItem& item) -> Element {
        std::string label = item.label;
        if (label.size() < LABEL_WIDTH)
            label += std::string(LABEL_WIDTH - label.size(), ' ');
        return hbox({
            text("  ") | bgcolor(item.color),
            text(" " + label + " ") | color(tc.foreground) | bgcolor(tc.background),
        });
    };

    Elements grid_rows;
    for (size_t i = 0; i < items.size(); i += 2) {
        Elements row_cells;
        row_cells.push_back(makeColorCell(items[i]));
        if (i + 1 < items.size()) {
            row_cells.push_back(text("  "));
            row_cells.push_back(makeColorCell(items[i + 1]));
        }
        grid_rows.push_back(hbox(std::move(row_cells)));
    }
    preview_elements.push_back(vbox(grid_rows));

    return vbox(preview_elements) | bgcolor(current_colors.dialog_bg) | size(WIDTH, EQUAL, 48) |
           borderWithColor(current_colors.dialog_border);
}

Element ThemeMenu::render() {
    auto& current_colors = theme_.getColors();

    // 标题栏
    Element title_bar =
        hbox({text(" "), text(icons::THEME) | color(Color::Cyan),
              text(" Select Theme ") | bold | color(current_colors.foreground), filler()}) |
        bgcolor(current_colors.menubar_bg);

    // 左侧：搜索框 + 主题列表
    Elements left_content;
    left_content.push_back(renderSearchBox());
    left_content.push_back(separator());
    left_content.push_back(renderThemeList());

    Element left_panel = vbox(left_content) | size(WIDTH, EQUAL, 38) | flex;

    // 右侧：颜色预览
    Element right_panel = renderColorPreview();

    // 主内容区：左右布局
    Element main_content =
        hbox({left_panel, separator(), right_panel}) | flex | size(HEIGHT, EQUAL, 26);

    // 底部提示
    Element help_bar =
        hbox({text(" "), text("↑↓") | color(current_colors.helpbar_key) | bold,
              text(": Navigate  "), text("Enter") | color(current_colors.helpbar_key) | bold,
              text(": Apply  "), text("Type") | color(current_colors.helpbar_key) | bold,
              text(": Filter  "), text("Esc") | color(current_colors.helpbar_key) | bold,
              text(": Cancel"), filler()}) |
        bgcolor(current_colors.helpbar_bg) | color(current_colors.helpbar_fg) | dim;

    return vbox({title_bar, separator(), main_content, separator(), help_bar}) |
           borderWithColor(current_colors.dialog_border) | bgcolor(current_colors.background) |
           size(WIDTH, GREATER_THAN, 90) | size(HEIGHT, GREATER_THAN, 28);
}

} // namespace ui
} // namespace pnana
