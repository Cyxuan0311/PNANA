#include "ui/ssh_dialog.h"
#include "ui/icons.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace ui {

SSHDialog::SSHDialog(Theme& theme)
    : theme_(theme), visible_(false), current_field_(0), cursor_position_(0) {}

void SSHDialog::show(std::function<void(const SSHConfig&)> on_confirm,
                     std::function<void()> on_cancel) {
    visible_ = true;
    current_field_ = 0;
    cursor_position_ = 0;
    on_confirm_ = on_confirm;
    on_cancel_ = on_cancel;

    // 重置所有输入字段
    host_input_ = "";
    user_input_ = "";
    password_input_ = "";
    key_path_input_ = "";
    port_input_ = "22";
    remote_path_input_ = "";
}

bool SSHDialog::handleInput(Event event) {
    if (!visible_) {
        return false;
    }

    if (event == Event::Escape) {
        visible_ = false;
        if (on_cancel_) {
            on_cancel_();
        }
        return true;
    }

    if (event == Event::Return) {
        // 如果不在最后一个字段，移动到下一个字段
        if (current_field_ < 5) {
            moveToNextField();
            return true;
        } else {
            // 在最后一个字段，确认连接
            visible_ = false;
            if (on_confirm_) {
                on_confirm_(buildConfig());
            }
            return true;
        }
    }

    if (event == Event::Tab) {
        moveToNextField();
        return true;
    }

    if (event == Event::TabReverse) {
        moveToPreviousField();
        return true;
    }

    if (event == Event::ArrowUp) {
        moveToPreviousField();
        return true;
    }

    if (event == Event::ArrowDown) {
        moveToNextField();
        return true;
    }

    if (event == Event::ArrowLeft) {
        moveCursorLeft();
        return true;
    }

    if (event == Event::ArrowRight) {
        moveCursorRight();
        return true;
    }

    if (event == Event::Backspace) {
        backspace();
        return true;
    }

    if (event == Event::Delete) {
        deleteChar();
        return true;
    }

    if (event == Event::Home) {
        cursor_position_ = 0;
        return true;
    }

    if (event == Event::End) {
        std::string* field = getCurrentField();
        if (field) {
            cursor_position_ = field->length();
        }
        return true;
    }

    if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            // 端口字段只接受数字
            if (current_field_ == 4) {
                if (c >= '0' && c <= '9') {
                    insertChar(c);
                }
            } else {
                // 其他字段接受可打印字符
                if (c >= 32 && c < 127) {
                    insertChar(c);
                }
            }
        }
        return true;
    }

    return false;
}

Element SSHDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements fields;

    // 标题
    Elements title_elements;
    title_elements.push_back(text(icons::TERMINAL) | color(Color::Cyan));
    title_elements.push_back(text(" SSH Remote File Editor ") | color(colors.foreground) | bold);
    fields.push_back(hbox(title_elements) | center);

    fields.push_back(separator());
    fields.push_back(text(""));

    // 字段标签和输入框
    auto renderField = [&](const std::string& label, std::string& value, size_t field_idx,
                           bool is_password = false) -> Element {
        bool is_focused = (current_field_ == field_idx);
        std::string display_value = is_password ? std::string(value.length(), '*') : value;

        Elements field_elements;
        field_elements.push_back(text(label + ": ") | color(colors.comment) |
                                 size(WIDTH, EQUAL, 15));

        // 输入框内容
        size_t cursor_pos = is_focused ? cursor_position_ : display_value.length();

        if (is_focused) {
            // 确保光标位置不超出字符串长度
            if (cursor_pos > display_value.length()) {
                cursor_pos = display_value.length();
            }

            std::string before = display_value.substr(0, cursor_pos);
            std::string cursor_char =
                cursor_pos < display_value.length() ? display_value.substr(cursor_pos, 1) : " ";
            std::string after =
                cursor_pos < display_value.length() ? display_value.substr(cursor_pos + 1) : "";

            field_elements.push_back(hbox({text(before) | color(colors.foreground),
                                           text(cursor_char) | bgcolor(colors.foreground) |
                                               color(colors.background) | bold,
                                           text(after) | color(colors.foreground)}) |
                                     bgcolor(colors.current_line));
        } else {
            field_elements.push_back(
                text(display_value.empty() ? "(empty)" : display_value) |
                color(display_value.empty() ? colors.comment : colors.foreground) | dim);
        }

        return hbox(field_elements);
    };

    fields.push_back(renderField("Host", host_input_, 0));
    fields.push_back(renderField("User", user_input_, 1));
    fields.push_back(renderField("Port", port_input_, 4));
    fields.push_back(renderField("Password", password_input_, 2, true));
    fields.push_back(renderField("Key Path", key_path_input_, 3));
    fields.push_back(renderField("Remote Path", remote_path_input_, 5));

    fields.push_back(text(""));
    fields.push_back(separator());

    // 提示信息
    fields.push_back(hbox({text("↑↓: Navigate fields  "), text("Tab: Next field  "),
                           text("Enter: Connect  "), text("Esc: Cancel")}) |
                     color(colors.comment) | center);

    // 使用 window 创建对话框窗口（注意：window 的参数顺序是：标题，内容）
    Element dialog_content = vbox(fields);

    return window(text("SSH Connection"), dialog_content) | size(WIDTH, GREATER_THAN, 70) |
           size(HEIGHT, GREATER_THAN, 20) | bgcolor(colors.background) | border;
}

void SSHDialog::reset() {
    visible_ = false;
    current_field_ = 0;
    cursor_position_ = 0;
    host_input_.clear();
    user_input_.clear();
    password_input_.clear();
    key_path_input_.clear();
    port_input_ = "22";
    remote_path_input_.clear();
}

std::string* SSHDialog::getCurrentField() {
    switch (current_field_) {
        case 0:
            return &host_input_;
        case 1:
            return &user_input_;
        case 2:
            return &password_input_;
        case 3:
            return &key_path_input_;
        case 4:
            return &port_input_;
        case 5:
            return &remote_path_input_;
        default:
            return nullptr;
    }
}

void SSHDialog::insertChar(char ch) {
    std::string* field = getCurrentField();
    if (field) {
        if (cursor_position_ <= field->length()) {
            field->insert(cursor_position_, 1, ch);
            cursor_position_++;
        }
    }
}

void SSHDialog::deleteChar() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ < field->length()) {
        field->erase(cursor_position_, 1);
    }
}

void SSHDialog::backspace() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ > 0) {
        field->erase(cursor_position_ - 1, 1);
        cursor_position_--;
    }
}

void SSHDialog::moveCursorLeft() {
    if (cursor_position_ > 0) {
        cursor_position_--;
    }
}

void SSHDialog::moveCursorRight() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ < field->length()) {
        cursor_position_++;
    }
}

void SSHDialog::moveToNextField() {
    if (current_field_ < 5) {
        current_field_++;
        std::string* field = getCurrentField();
        if (field) {
            cursor_position_ = field->length();
        }
    }
}

void SSHDialog::moveToPreviousField() {
    if (current_field_ > 0) {
        current_field_--;
        std::string* field = getCurrentField();
        if (field) {
            cursor_position_ = field->length();
        }
    }
}

SSHConfig SSHDialog::buildConfig() const {
    SSHConfig config;
    config.host = host_input_;
    config.user = user_input_;
    config.password = password_input_;
    config.key_path = key_path_input_;

    // 解析端口
    try {
        config.port = std::stoi(port_input_);
    } catch (...) {
        config.port = 22;
    }

    config.remote_path = remote_path_input_;
    return config;
}

} // namespace ui
} // namespace pnana
