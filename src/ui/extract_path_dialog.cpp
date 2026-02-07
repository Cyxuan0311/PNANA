#include "ui/extract_path_dialog.h"
#include "ui/icons.h"
#include <filesystem>
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

ExtractPathDialog::ExtractPathDialog(Theme& theme)
    : theme_(theme), visible_(false), filename_cursor_position_(0), path_cursor_position_(0),
      active_field_(InputField::FILENAME) {}

void ExtractPathDialog::show(const std::string& archive_name, const std::string& default_path,
                             std::function<void(const std::string&, const std::string&)> on_confirm,
                             std::function<void()> on_cancel) {
    visible_ = true;
    archive_name_ = archive_name;
    default_path_ = default_path;

    // 初始化路径输入
    path_input_ = default_path;
    path_cursor_position_ = path_input_.length();

    // 初始化文件名输入（从压缩文件名提取，去掉扩展名）
    std::filesystem::path archive_path(archive_name);
    filename_input_ = archive_path.stem().string();
    filename_cursor_position_ = filename_input_.length();

    active_field_ = InputField::FILENAME; // 默认从文件名字段开始
    on_confirm_ = on_confirm;
    on_cancel_ = on_cancel;
}

void ExtractPathDialog::hide() {
    visible_ = false;
}

bool ExtractPathDialog::handleInput(ftxui::Event event) {
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
        if (!path_input_.empty() && on_confirm_) {
            visible_ = false;
            on_confirm_(path_input_, filename_input_);
        }
        return true;
    }

    // 上下箭头键切换字段
    if (event == Event::ArrowUp) {
        switchFieldUp();
        return true;
    }

    if (event == Event::ArrowDown) {
        switchFieldDown();
        return true;
    }

    // 左右箭头键移动光标（仅在当前字段内）
    if (event == Event::ArrowLeft) {
        moveCursorLeft();
        return true;
    }

    if (event == Event::ArrowRight) {
        moveCursorRight();
        return true;
    }

    if (event == Event::Home) {
        getCurrentCursorPosition() = 0;
        return true;
    }

    if (event == Event::End) {
        getCurrentCursorPosition() = getCurrentInput().length();
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

    if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            if (c >= 32 && c < 127) {
                insertChar(c);
            }
        }
        return true;
    }

    return false;
}

ftxui::Element ExtractPathDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements dialog_content;

    // 标题
    dialog_content.push_back(hbox({text(" "), text(icons::LOCATION) | color(colors.keyword) | bold,
                                   text(" Extract To "), text(" ")}) |
                             bold | bgcolor(colors.menubar_bg) | center);

    dialog_content.push_back(separator());

    // 压缩文件信息
    dialog_content.push_back(text(""));
    std::string display_name = archive_name_;
    if (display_name.length() > 50) {
        display_name = "..." + display_name.substr(display_name.length() - 47);
    }
    dialog_content.push_back(
        hbox({text("  "), text(icons::FILE) | color(colors.comment), text(" Archive: "),
              text(display_name) | color(colors.foreground)}));

    dialog_content.push_back(text(""));

    // 文件名输入框（第一个）
    dialog_content.push_back(renderInputField("Extract name:", filename_input_,
                                              filename_cursor_position_,
                                              active_field_ == InputField::FILENAME));

    dialog_content.push_back(text(""));

    // 路径输入框（第二个）
    dialog_content.push_back(renderInputField("Extract path:", path_input_, path_cursor_position_,
                                              active_field_ == InputField::PATH));

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 提示
    dialog_content.push_back(
        hbox({text("  "), text("↑↓") | color(colors.function) | bold, text(": Switch field  "),
              text("Enter") | color(colors.function) | bold, text(": Extract  "),
              text("Esc") | color(colors.function) | bold, text(": Cancel")}) |
        dim);

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 75) |
           size(HEIGHT, EQUAL, 16) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

void ExtractPathDialog::insertChar(char ch) {
    std::string& input = getCurrentInput();
    size_t& cursor_pos = getCurrentCursorPosition();

    if (cursor_pos <= input.length()) {
        input.insert(cursor_pos, 1, ch);
        cursor_pos++;
    }
}

void ExtractPathDialog::backspace() {
    std::string& input = getCurrentInput();
    size_t& cursor_pos = getCurrentCursorPosition();

    if (cursor_pos > 0) {
        input.erase(cursor_pos - 1, 1);
        cursor_pos--;
    }
}

void ExtractPathDialog::deleteChar() {
    std::string& input = getCurrentInput();
    size_t& cursor_pos = getCurrentCursorPosition();

    if (cursor_pos < input.length()) {
        input.erase(cursor_pos, 1);
    }
}

void ExtractPathDialog::moveCursorLeft() {
    size_t& cursor_pos = getCurrentCursorPosition();
    if (cursor_pos > 0) {
        cursor_pos--;
    }
}

void ExtractPathDialog::moveCursorRight() {
    std::string& input = getCurrentInput();
    size_t& cursor_pos = getCurrentCursorPosition();

    if (cursor_pos < input.length()) {
        cursor_pos++;
    }
}

void ExtractPathDialog::switchFieldUp() {
    // 向上切换：从路径切换到文件名
    if (active_field_ == InputField::PATH) {
        active_field_ = InputField::FILENAME;
    }
}

void ExtractPathDialog::switchFieldDown() {
    // 向下切换：从文件名切换到路径
    if (active_field_ == InputField::FILENAME) {
        active_field_ = InputField::PATH;
    }
}

ftxui::Element ExtractPathDialog::renderInputField(const std::string& label,
                                                   const std::string& value, size_t cursor_pos,
                                                   bool is_active) {
    auto& colors = theme_.getColors();

    // 确保光标位置有效
    size_t safe_cursor_pos = std::min(cursor_pos, value.length());

    Elements field_elements;
    field_elements.push_back(text("  "));
    field_elements.push_back(text(label) | color(is_active ? colors.keyword : colors.comment) |
                             (is_active ? bold : dim));
    field_elements.push_back(text(" "));

    if (value.empty()) {
        // 空输入时显示块状光标
        if (is_active) {
            field_elements.push_back(text(" ") | bgcolor(colors.selection) |
                                     color(colors.foreground));
        } else {
            field_elements.push_back(text("_") | color(colors.comment) | dim);
        }
    } else {
        // 分割字符串：光标前、光标处字符、光标后
        std::string before = value.substr(0, safe_cursor_pos);
        std::string cursor_char =
            safe_cursor_pos < value.length() ? value.substr(safe_cursor_pos, 1) : " ";
        std::string after =
            safe_cursor_pos < value.length() ? value.substr(safe_cursor_pos + 1) : "";

        // 渲染光标前的文本
        if (!before.empty()) {
            field_elements.push_back(text(before) | color(colors.foreground));
        }

        // 渲染光标处的字符（块状光标效果）
        if (is_active) {
            // 块状光标：使用背景色高亮当前字符
            field_elements.push_back(text(cursor_char) | bgcolor(colors.selection) |
                                     color(colors.background) | bold);
        } else {
            field_elements.push_back(text(cursor_char) | color(colors.foreground));
        }

        // 渲染光标后的文本
        if (!after.empty()) {
            field_elements.push_back(text(after) | color(colors.foreground));
        }
    }

    // 如果字段不活动，整体变暗
    Element field = hbox(field_elements);
    if (!is_active) {
        field = field | dim;
    }

    return field;
}

} // namespace ui
} // namespace pnana
