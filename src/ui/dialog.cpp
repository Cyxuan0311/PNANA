#include "ui/dialog.h"
#include "ui/icons.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

Dialog::Dialog(Theme& theme)
    : theme_(theme), visible_(false), type_(DialogType::INPUT), cursor_position_(0) {}

void Dialog::showInput(const std::string& title, const std::string& prompt,
                       const std::string& initial_value,
                       std::function<void(const std::string&)> on_confirm,
                       std::function<void()> on_cancel) {
    title_ = title;
    prompt_ = prompt;
    message_ = "";
    input_value_ = initial_value;
    cursor_position_ = input_value_.length();
    type_ = DialogType::INPUT;
    on_input_confirm_ = on_confirm;
    on_confirm_ = nullptr;
    on_cancel_ = on_cancel;
    visible_ = true;
}

void Dialog::showConfirm(const std::string& title, const std::string& message,
                         std::function<void()> on_confirm, std::function<void()> on_cancel) {
    title_ = title;
    prompt_ = "";
    message_ = message;
    input_value_ = "";
    cursor_position_ = 0;
    type_ = DialogType::CONFIRM;
    on_input_confirm_ = nullptr;
    on_confirm_ = on_confirm;
    on_cancel_ = on_cancel;
    visible_ = true;
}

bool Dialog::handleInput(ftxui::Event event) {
    if (!visible_)
        return false;

    if (type_ == DialogType::INPUT) {
        if (event == Event::Return) {
            // 确认
            if (on_input_confirm_) {
                on_input_confirm_(input_value_);
            }
            visible_ = false;
            return true;
        } else if (event == Event::Escape) {
            // 取消
            if (on_cancel_) {
                on_cancel_();
            }
            visible_ = false;
            return true;
        } else if (event == Event::ArrowLeft) {
            moveCursorLeft();
            return true;
        } else if (event == Event::ArrowRight) {
            moveCursorRight();
            return true;
        } else if (event == Event::Backspace) {
            backspace();
            return true;
        } else if (event == Event::Delete) {
            deleteChar();
            return true;
        } else if (event.is_character()) {
            insertChar(event.character()[0]);
            return true;
        }
    } else if (type_ == DialogType::CONFIRM) {
        if (event == Event::Return || event.character() == "y" || event.character() == "Y") {
            // 确认
            if (on_confirm_) {
                on_confirm_();
            }
            visible_ = false;
            return true;
        } else if (event == Event::Escape || event.character() == "n" || event.character() == "N") {
            // 取消
            if (on_cancel_) {
                on_cancel_();
            }
            visible_ = false;
            return true;
        }
    }

    return false;
}

Element Dialog::render() {
    if (!visible_)
        return text("");

    auto& colors = theme_.getColors();

    if (type_ == DialogType::INPUT) {
        // 输入对话框
        Elements content;

        // 标题
        content.push_back(hbox({text(" "), text(ui::icons::INFO) | color(Color::Cyan), text(" "),
                                text(title_) | bold | color(colors.foreground)}) |
                          bgcolor(colors.menubar_bg));

        content.push_back(separator());

        // 提示信息
        content.push_back(hbox({text(" "), text(prompt_) | color(colors.comment)}));

        // 输入框
        std::string display_value = input_value_;
        if (display_value.empty()) {
            display_value = " ";
        }

        // 计算光标位置显示
        std::string before_cursor = display_value.substr(0, cursor_position_);
        std::string at_cursor = cursor_position_ < display_value.length()
                                    ? std::string(1, display_value[cursor_position_])
                                    : " ";
        std::string after_cursor = cursor_position_ < display_value.length()
                                       ? display_value.substr(cursor_position_ + 1)
                                       : "";

        content.push_back(
            hbox({text(" "), text(before_cursor) | color(colors.foreground),
                  text(at_cursor) | bgcolor(colors.foreground) | color(colors.background),
                  text(after_cursor) | color(colors.foreground),
                  text(" ") | color(colors.foreground)}) |
            bgcolor(colors.background) | border);

        // 提示
        content.push_back(hbox(
            {text(" "), text("Press Enter to confirm, Esc to cancel") | color(colors.comment)}));

        return vbox(content) | bgcolor(colors.background) | border | center;

    } else if (type_ == DialogType::CONFIRM) {
        // 确认对话框
        Elements content;

        // 标题
        content.push_back(hbox({text(" "), text(ui::icons::WARNING) | color(Color::Yellow),
                                text(" "), text(title_) | bold | color(colors.foreground)}) |
                          bgcolor(colors.menubar_bg));

        content.push_back(separator());

        // 消息
        content.push_back(hbox({text(" "), text(message_) | color(colors.foreground)}));

        content.push_back(text(""));

        // 按钮提示
        content.push_back(hbox({text(" "), text("[Y] Confirm") | color(Color::Green), text("  "),
                                text("[N] Cancel") | color(Color::Red), text("  "),
                                text("(or Enter/Esc)") | color(colors.comment)}));

        return vbox(content) | bgcolor(colors.background) | border | center;
    }

    return text("");
}

void Dialog::reset() {
    visible_ = false;
    input_value_ = "";
    cursor_position_ = 0;
    on_input_confirm_ = nullptr;
    on_confirm_ = nullptr;
    on_cancel_ = nullptr;
}

void Dialog::insertChar(char ch) {
    if (cursor_position_ <= input_value_.length()) {
        input_value_.insert(cursor_position_, 1, ch);
        cursor_position_++;
    }
}

void Dialog::deleteChar() {
    if (cursor_position_ < input_value_.length()) {
        input_value_.erase(cursor_position_, 1);
    }
}

void Dialog::backspace() {
    if (cursor_position_ > 0) {
        cursor_position_--;
        input_value_.erase(cursor_position_, 1);
    }
}

void Dialog::moveCursorLeft() {
    if (cursor_position_ > 0) {
        cursor_position_--;
    }
}

void Dialog::moveCursorRight() {
    if (cursor_position_ < input_value_.length()) {
        cursor_position_++;
    }
}

} // namespace ui
} // namespace pnana
