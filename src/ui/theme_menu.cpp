#include "ui/theme_menu.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

ThemeMenu::ThemeMenu(Theme& theme) : theme_(theme), selected_index_(0) {}

void ThemeMenu::setAvailableThemes(const std::vector<std::string>& themes) {
    available_themes_ = themes;
    if (selected_index_ >= available_themes_.size() && !available_themes_.empty()) {
        selected_index_ = 0;
    }
}

void ThemeMenu::setSelectedIndex(size_t index) {
    if (index < available_themes_.size()) {
        selected_index_ = index;
    }
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

    // 主题列表
    for (size_t i = 0; i < available_themes_.size(); ++i) {
        std::string theme_name = available_themes_[i];

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

    // 底部提示
    theme_items.push_back(
        hbox({text(" "), text("↑↓: Navigate") | color(current_colors.comment), text("  "),
              text("Enter: Apply") | color(current_colors.comment), text("  "),
              text("Esc: Cancel") | color(current_colors.comment), filler()}) |
        bgcolor(current_colors.menubar_bg));

    return vbox(theme_items) | border | bgcolor(current_colors.background) |
           size(WIDTH, GREATER_THAN, 50) | size(HEIGHT, GREATER_THAN, 16);
}

} // namespace ui
} // namespace pnana
