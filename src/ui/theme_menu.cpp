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

ThemeMenu::ThemeMenu(Theme& theme) : theme_(theme), selected_index_(0), show_search_(false) {
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

bool ThemeMenu::handleInput(ftxui::Event event) {
    // 搜索模式
    if (show_search_) {
        if (event == Event::Escape) {
            show_search_ = false;
            search_input_ = "";
            updateFilteredThemes();
            return true;
        } else if (event == Event::Return) {
            show_search_ = false;
            return true;
        } else if (event == Event::Backspace) {
            if (!search_input_.empty()) {
                search_input_.pop_back();
                updateFilteredThemes();
            }
            return true;
        } else if (event == Event::CtrlF) {
            // Ctrl+F 退出搜索模式
            show_search_ = false;
            search_input_ = "";
            updateFilteredThemes();
            return true;
        } else if (event.is_character()) {
            std::string ch = event.character();
            // 支持完整的UTF-8字符输入，包括中文等多字节字符
            if (!ch.empty() && ch[0] >= 32) { // 过滤控制字符
                search_input_ += ch;
                updateFilteredThemes();
            }
            return true;
        }
        return false;
    }

    // 正常模式
    if (event == Event::CtrlF) {
        // Ctrl+F 进入搜索模式
        show_search_ = true;
        search_input_ = "";
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

Element ThemeMenu::render() {
    auto& current_colors = theme_.getColors();
    Elements theme_items;

    // 标题栏
    theme_items.push_back(
        hbox({text(" "), text(icons::THEME) | color(Color::Cyan),
              text(" Select Theme ") | bold | color(current_colors.foreground), filler()}) |
        bgcolor(current_colors.menubar_bg));
    theme_items.push_back(separator());

    // 搜索输入框（如果启用）
    if (show_search_) {
        theme_items.push_back(hbox({text(" Search: ") | color(current_colors.keyword) | bold,
                                    text(search_input_ + "_") | color(current_colors.foreground) |
                                        bgcolor(current_colors.selection)}));
        theme_items.push_back(separator());
    }

    // 主题列表（使用过滤后的列表）
    for (size_t i = 0; i < filtered_themes_.size(); ++i) {
        std::string theme_name = filtered_themes_[i];

        // 获取主题颜色预览
        Theme temp_theme;
        temp_theme.setTheme(theme_name);
        auto& theme_colors = temp_theme.getColors();

        // 创建颜色预览块（显示更多颜色）
        Elements color_preview_elements = {
            // 背景色
            text("█") | color(theme_colors.background) | bgcolor(theme_colors.background),
            // 前景色
            text("█") | color(theme_colors.foreground) | bgcolor(theme_colors.foreground),
            // 关键字
            text("█") | color(theme_colors.keyword) | bgcolor(theme_colors.keyword),
            // 字符串
            text("█") | color(theme_colors.string) | bgcolor(theme_colors.string),
            // 注释
            text("█") | color(theme_colors.comment) | bgcolor(theme_colors.comment),
            // 函数
            text("█") | color(theme_colors.function) | bgcolor(theme_colors.function),
            // 类型
            text("█") | color(theme_colors.type) | bgcolor(theme_colors.type),
            // 数字
            text("█") | color(theme_colors.number) | bgcolor(theme_colors.number),
            // 运算符
            text("█") | color(theme_colors.operator_color) | bgcolor(theme_colors.operator_color),
            // 错误
            text("█") | color(theme_colors.error) | bgcolor(theme_colors.error), text(" ")};
        auto color_preview = hbox(color_preview_elements);

        // 主题名称
        std::string display_name = theme_name;
        if (theme_name == theme_.getCurrentThemeName()) {
            display_name += " " + std::string(icons::SUCCESS);
        }

        auto name_text = text(display_name);

        // 选中状态样式
        if (i == selected_index_) {
            name_text = name_text | bold | color(current_colors.function);
            color_preview = color_preview | bgcolor(current_colors.selection);
        } else {
            name_text = name_text | color(current_colors.foreground);
        }

        // 组合行
        Elements row_elements = {
            text(" "),
            (i == selected_index_ ? text("►") | color(current_colors.function) : text(" ")),
            text(" "),
            color_preview,
            text(" "),
            name_text,
            filler()};
        theme_items.push_back(hbox(row_elements) |
                              (i == selected_index_ ? bgcolor(current_colors.selection)
                                                    : bgcolor(current_colors.background)));
    }

    theme_items.push_back(separator());

    // 如果没有匹配的主题，显示提示
    if (filtered_themes_.empty() && !search_input_.empty()) {
        theme_items.push_back(
            hbox({text(" "), text("No themes found matching: \"") | color(current_colors.comment),
                  text(search_input_) | color(current_colors.foreground),
                  text("\"") | color(current_colors.comment), filler()}) |
            center);
    }

    theme_items.push_back(separator());

    // 底部提示
    Elements hints = {text(" "), text("↑↓: Navigate") | color(current_colors.comment), text("  "),
                      text("Enter: Apply") | color(current_colors.comment)};
    if (show_search_) {
        hints.push_back(text("  "));
        hints.push_back(text("Esc/Ctrl+F: Exit Search") | color(current_colors.comment));
    } else {
        hints.push_back(text("  "));
        hints.push_back(text("Ctrl+F: Search") | color(current_colors.comment));
    }
    hints.push_back(text("  "));
    hints.push_back(text("Esc: Cancel") | color(current_colors.comment));
    hints.push_back(filler());
    theme_items.push_back(hbox(hints) | bgcolor(current_colors.menubar_bg));

    return vbox(theme_items) | borderWithColor(current_colors.dialog_border) |
           bgcolor(current_colors.background) | size(WIDTH, GREATER_THAN, 50) |
           size(HEIGHT, GREATER_THAN, 16);
}

} // namespace ui
} // namespace pnana
