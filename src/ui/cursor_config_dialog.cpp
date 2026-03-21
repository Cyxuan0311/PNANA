#include "ui/cursor_config_dialog.h"
#include "ui/icons.h"
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

static inline Element padded(Element e) {
    return hbox({text(" "), std::move(e), text(" ")});
}

namespace pnana {
namespace ui {

CursorConfigDialog::CursorConfigDialog(Theme& theme)
    : theme_(theme), visible_(false), cursor_style_(CursorStyle::BLOCK),
      cursor_color_("255,255,255"), blink_rate_(500), smooth_cursor_(false), blink_enabled_(false),
      selected_option_(0), color_input_index_(1), rate_input_index_(2), blink_enabled_index_(3),
      smooth_cursor_index_(4), color_input_("255,255,255"), rate_input_("500") {
    style_options_ = {"Block", "Underline", "Bar", "Hollow"};
}

void CursorConfigDialog::open() {
    visible_ = true;
    selected_option_ = 0;
    color_input_ = cursor_color_;
    rate_input_ = std::to_string(blink_rate_);
}

void CursorConfigDialog::close() {
    visible_ = false;
}

bool CursorConfigDialog::handleInput(ftxui::Event event) {
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

    if (event == Event::ArrowLeft || event == Event::Character('h')) {
        if (selected_option_ == 0 && static_cast<int>(cursor_style_) > 0) {
            cursor_style_ = static_cast<CursorStyle>(static_cast<int>(cursor_style_) - 1);
        }
        return true;
    }

    if (event == Event::ArrowRight || event == Event::Character('l')) {
        if (selected_option_ == 0 && static_cast<int>(cursor_style_) < 3) {
            cursor_style_ = static_cast<CursorStyle>(static_cast<int>(cursor_style_) + 1);
        }
        return true;
    }

    // 处理颜色输入
    if (selected_option_ == color_input_index_) {
        if (event == Event::Backspace) {
            if (!color_input_.empty()) {
                color_input_.pop_back();
            }
            return true;
        }

        if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                if (std::isdigit(c) || c == ',' || c == ' ') {
                    color_input_ += c;
                }
            }
            return true;
        }
    }

    // 处理频率输入
    if (selected_option_ == rate_input_index_) {
        if (event == Event::Backspace) {
            if (!rate_input_.empty()) {
                rate_input_.pop_back();
            }
            return true;
        }

        if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                if (std::isdigit(c)) {
                    rate_input_ += c;
                }
            }
            return true;
        }
    }

    // 处理闪烁开关选项切换
    if (selected_option_ == blink_enabled_index_) {
        if (event == Event::Return || event == Event::ArrowLeft || event == Event::ArrowRight ||
            (event.is_character() &&
             (event.character() == " " || event.character() == "t" || event.character() == "T"))) {
            blink_enabled_ = !blink_enabled_;
            return true;
        }
    }

    // 处理流动光标选项切换
    if (selected_option_ == smooth_cursor_index_) {
        if (event == Event::Return || event == Event::ArrowLeft || event == Event::ArrowRight ||
            (event.is_character() &&
             (event.character() == " " || event.character() == "t" || event.character() == "T"))) {
            smooth_cursor_ = !smooth_cursor_;
            return true;
        }
    }

    return false;
}

Element CursorConfigDialog::render() {
    if (!visible_)
        return text("");

    auto& colors = theme_.getColors();

    Element header = hbox({
                         text(" "),
                         text(icons::SETTINGS) | color(colors.keyword) | bold,
                         text(" Cursor Configuration ") | color(colors.menubar_fg) | bold,
                         filler(),
                         text("Enter") | color(colors.helpbar_key) | bold,
                         text(" Apply  ") | color(colors.menubar_fg) | dim,
                         text("Esc") | color(colors.helpbar_key) | bold,
                         text(" Cancel ") | color(colors.menubar_fg) | dim,
                     }) |
                     bgcolor(colors.menubar_bg);

    Element body = vbox({
                       padded(renderStyleSelector()),
                       separatorEmpty(),
                       padded(renderColorSelector()),
                       separatorEmpty(),
                       padded(renderRateSelector()),
                       separatorEmpty(),
                       padded(renderBlinkEnabledSelector()),
                       separatorEmpty(),
                       padded(renderSmoothCursorSelector()),
                   }) |
                   flex;

    Element footer = hbox({
                         text(" "),
                         text("↑↓") | color(colors.helpbar_key) | bold,
                         text(": Navigate  ") | color(colors.helpbar_fg) | dim,
                         text("←→") | color(colors.helpbar_key) | bold,
                         text(": Change  ") | color(colors.helpbar_fg) | dim,
                         text("Space/T") | color(colors.helpbar_key) | bold,
                         text(": Toggle") | color(colors.helpbar_fg) | dim,
                         filler(),
                     }) |
                     bgcolor(colors.helpbar_bg);

    return window(text(" Cursor ") | color(colors.keyword) | bold, vbox({
                                                                       header,
                                                                       separator(),
                                                                       body,
                                                                       separator(),
                                                                       footer,
                                                                   })) |
           size(WIDTH, GREATER_THAN, 66) | size(HEIGHT, GREATER_THAN, 16) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border) | center;
}

Element CursorConfigDialog::renderStyleSelector() {
    auto& colors = theme_.getColors();

    std::string style_name = style_options_[static_cast<int>(cursor_style_)];
    bool is_selected = (selected_option_ == 0);

    Elements style_elements;
    for (size_t i = 0; i < style_options_.size(); ++i) {
        std::string option = style_options_[i];
        bool is_current = (static_cast<size_t>(i) == static_cast<size_t>(cursor_style_));

        if (i > 0)
            style_elements.push_back(text("  "));

        if (is_current) {
            style_elements.push_back(text("[" + option + "]") | bold | color(colors.function) |
                                     bgcolor(colors.selection));
        } else {
            style_elements.push_back(text(option) |
                                     color(is_selected ? colors.foreground : colors.comment));
        }
    }

    // 使用 hflow：选项很多时可自动换行
    return hbox({(is_selected ? text("► ") | color(colors.function) : text("  ")),
                 text("Style: ") | color(is_selected ? colors.foreground : colors.comment),
                 hflow(style_elements), filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element CursorConfigDialog::renderColorSelector() {
    auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == color_input_index_);

    // 解析颜色并显示预览
    int r = 255, g = 255, b = 255;
    parseColor(color_input_, r, g, b);
    Color preview_color = Color::RGB(r, g, b);

    return hbox({(is_selected ? text("► ") | color(colors.function) : text("  ")),
                 text("Color (R,G,B): ") | color(is_selected ? colors.foreground : colors.comment),
                 text("█") | color(preview_color) | bgcolor(preview_color), text(" "),
                 text(color_input_ + (is_selected ? "_" : "")) |
                     color(is_selected ? colors.foreground : colors.comment) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element CursorConfigDialog::renderRateSelector() {
    auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == rate_input_index_);

    std::string rate_display = rate_input_ + (is_selected ? "_" : "");
    if (rate_display == "_" || rate_display.empty()) {
        rate_display = "0";
    }

    int rate_value = 0;
    try {
        rate_value = std::stoi(rate_display);
    } catch (...) {
        rate_value = 0;
    }

    std::string rate_desc = (rate_value == 0) ? " (No blink)" : " ms";

    return hbox({(is_selected ? text("► ") | color(colors.function) : text("  ")),
                 text("Blink Rate: ") | color(is_selected ? colors.foreground : colors.comment),
                 text(rate_display) | color(is_selected ? colors.foreground : colors.comment) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 text(rate_desc) | color(colors.comment), filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element CursorConfigDialog::renderBlinkEnabledSelector() {
    auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == blink_enabled_index_);

    std::string status = blink_enabled_ ? "[ON]" : "[OFF]";
    Color status_color = blink_enabled_ ? colors.success : colors.comment;

    return hbox({(is_selected ? text("► ") | color(colors.function) : text("  ")),
                 text("Blink: ") | color(is_selected ? colors.foreground : colors.comment),
                 text(" ") | color(status_color), text(status) | bold | color(status_color),
                 text(" (Space/T: toggle)") | color(colors.comment) | dim, filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element CursorConfigDialog::renderSmoothCursorSelector() {
    auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == smooth_cursor_index_);

    std::string status = smooth_cursor_ ? "[ON]" : "[OFF]";
    Color status_color = smooth_cursor_ ? colors.success : colors.comment;

    return hbox({(is_selected ? text("► ") | color(colors.function) : text("  ")),
                 text("Smooth Cursor: ") | color(is_selected ? colors.foreground : colors.comment),
                 text(" ") | color(status_color), text(status) | bold | color(status_color),
                 text(" (Space/T: toggle)") | color(colors.comment) | dim, filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

bool CursorConfigDialog::parseColor(const std::string& color_str, int& r, int& g, int& b) {
    std::string cleaned = color_str;
    // 移除所有空格
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ' '), cleaned.end());

    std::istringstream iss(cleaned);
    std::string token;

    std::vector<int> values;
    while (std::getline(iss, token, ',')) {
        try {
            int value = std::stoi(token);
            if (value < 0)
                value = 0;
            if (value > 255)
                value = 255;
            values.push_back(value);
        } catch (...) {
            return false;
        }
    }

    if (values.size() >= 3) {
        r = values[0];
        g = values[1];
        b = values[2];
        return true;
    }

    return false;
}

std::string CursorConfigDialog::formatColor(int r, int g, int b) {
    std::ostringstream oss;
    oss << r << "," << g << "," << b;
    return oss.str();
}

void CursorConfigDialog::selectNext() {
    if (selected_option_ < smooth_cursor_index_) {
        selected_option_++;
    } else {
        selected_option_ = 0;
    }
}

void CursorConfigDialog::selectPrevious() {
    if (selected_option_ > 0) {
        selected_option_--;
    } else {
        selected_option_ = smooth_cursor_index_;
    }
}

void CursorConfigDialog::apply() {
    // 更新配置
    cursor_color_ = color_input_;

    try {
        blink_rate_ = std::stoi(rate_input_);
        if (blink_rate_ < 0)
            blink_rate_ = 0;
    } catch (...) {
        blink_rate_ = 500;
    }

    // 调用回调
    if (on_apply_) {
        on_apply_();
    }

    close();
}

void CursorConfigDialog::resetToDefaults() {
    cursor_style_ = CursorStyle::BLOCK;
    cursor_color_ = "255,255,255";
    blink_rate_ = 500;
    smooth_cursor_ = false;
    blink_enabled_ = false;
    color_input_ = cursor_color_;
    rate_input_ = std::to_string(blink_rate_);
}

} // namespace ui
} // namespace pnana
