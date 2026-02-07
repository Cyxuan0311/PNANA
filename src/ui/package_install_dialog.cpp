#include "ui/package_install_dialog.h"
#include "ui/icons.h"
#include <algorithm>
#include <chrono>
#include <thread>

using namespace ftxui;

namespace pnana {
namespace ui {

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

PackageInstallDialog::PackageInstallDialog(Theme& theme)
    : theme_(theme), visible_(false), cursor_position_(0), operation_in_progress_(false),
      operation_success_(false) {}

void PackageInstallDialog::show(
    std::shared_ptr<features::package_manager::PackageManagerBase> manager) {
    manager_ = manager;
    visible_ = true;
    package_name_input_.clear();
    cursor_position_ = 0;
    std::lock_guard<std::mutex> lock(operation_mutex_);
    operation_status_.clear();
    operation_in_progress_ = false;
    operation_success_ = false;
}

void PackageInstallDialog::hide() {
    visible_ = false;
    package_name_input_.clear();
    cursor_position_ = 0;
}

bool PackageInstallDialog::handleInput(Event event) {
    if (!visible_) {
        return false;
    }

    // 如果操作正在进行中，只允许 Esc 退出
    {
        std::lock_guard<std::mutex> lock(operation_mutex_);
        if (operation_in_progress_) {
            if (event == Event::Escape) {
                hide();
                return true;
            }
            return true; // 忽略其他输入
        }
    }

    if (event == Event::Escape) {
        hide();
        return true;
    } else if (event == Event::Return) {
        // Enter 键：开始安装
        if (!manager_ || package_name_input_.empty()) {
            return true;
        }

        std::string package_name = package_name_input_;
        {
            std::lock_guard<std::mutex> lock(operation_mutex_);
            operation_in_progress_ = true;
            operation_status_ = "Installing package: " + package_name + "...";
        }

        std::thread([this, package_name]() {
            bool success = manager_->installPackage(package_name);

            if (!success) {
                {
                    std::lock_guard<std::mutex> lock(operation_mutex_);
                    operation_in_progress_ = false;
                    operation_success_ = false;
                    operation_status_ = "Failed to start installation: " + package_name;
                    if (manager_->hasError()) {
                        operation_status_ += " - " + manager_->getError();
                    }
                }
                return;
            }

            const int wait_seconds = 10;
            const int check_interval_ms = 500;
            int waited_milliseconds = 0;

            while (waited_milliseconds < wait_seconds * 1000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
                waited_milliseconds += check_interval_ms;

                if (manager_->hasError()) {
                    std::string error = manager_->getError();
                    if (error.find("Failed") != std::string::npos ||
                        error.find("not found") != std::string::npos) {
                        break;
                    }
                }
            }

            bool final_success = !manager_->hasError() || manager_->getError().empty();

            {
                std::lock_guard<std::mutex> lock(operation_mutex_);
                operation_in_progress_ = false;
                operation_success_ = final_success;
                if (final_success) {
                    operation_status_ = "Installation completed for: " + package_name;
                } else {
                    operation_status_ = "Failed to install: " + package_name;
                    if (manager_->hasError()) {
                        operation_status_ += " - " + manager_->getError();
                    }
                }
            }
            manager_->clearCache();
        }).detach();
        return true;
    } else if (event == Event::Backspace) {
        backspace();
        return true;
    } else if (event == Event::Delete) {
        deleteChar();
        return true;
    } else if (event == Event::ArrowLeft) {
        moveCursorLeft();
        return true;
    } else if (event == Event::ArrowRight) {
        moveCursorRight();
        return true;
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1 && ch[0] >= 32 && ch[0] < 127) {
            insertChar(ch[0]);
        }
        return true;
    }

    return true;
}

void PackageInstallDialog::insertChar(char ch) {
    if (cursor_position_ <= package_name_input_.length()) {
        package_name_input_.insert(cursor_position_, 1, ch);
        cursor_position_++;
    }
}

void PackageInstallDialog::backspace() {
    if (cursor_position_ > 0 && cursor_position_ <= package_name_input_.length()) {
        package_name_input_.erase(cursor_position_ - 1, 1);
        cursor_position_--;
    }
}

void PackageInstallDialog::deleteChar() {
    if (cursor_position_ < package_name_input_.length()) {
        package_name_input_.erase(cursor_position_, 1);
    }
}

void PackageInstallDialog::moveCursorLeft() {
    if (cursor_position_ > 0) {
        cursor_position_--;
    }
}

void PackageInstallDialog::moveCursorRight() {
    if (cursor_position_ < package_name_input_.length()) {
        cursor_position_++;
    }
}

Element PackageInstallDialog::render() const {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements content;

    // 标题
    Elements header_elements;
    header_elements.push_back(text("  ") | color(colors.menubar_fg));
    header_elements.push_back(text(icons::PACKAGE) | color(colors.success));
    header_elements.push_back(text(" Install Package ") | color(colors.menubar_fg) | bold);
    if (manager_) {
        header_elements.push_back(text(" (" + manager_->getDisplayName() + ")") |
                                  color(colors.comment) | dim);
    }
    content.push_back(hbox(std::move(header_elements)) | bgcolor(colors.menubar_bg));
    content.push_back(separator());

    // 输入字段
    content.push_back(renderInputField());

    // 操作状态
    // 注意：renderOperationStatus() 内部会获取 mutex，所以这里先检查状态，然后释放 mutex 再调用
    bool has_status = false;
    {
        std::lock_guard<std::mutex> lock(operation_mutex_);
        has_status = !operation_status_.empty();
    }
    if (has_status) {
        content.push_back(separator());
        content.push_back(renderOperationStatus());
    }

    content.push_back(separator());

    // 提示信息
    Elements hint_elements;
    hint_elements.push_back(text("  ") | color(colors.comment));
    hint_elements.push_back(text("Enter") | color(colors.helpbar_key) | bold);
    hint_elements.push_back(text(": Install  ") | color(colors.comment) | dim);
    hint_elements.push_back(text("Esc") | color(colors.helpbar_key) | bold);
    hint_elements.push_back(text(": Cancel") | color(colors.comment) | dim);
    content.push_back(hbox(std::move(hint_elements)));

    return window(text(" Install Package ") | color(colors.success) | bold,
                  vbox(std::move(content))) |
           size(WIDTH, GREATER_THAN, 70) | size(HEIGHT, GREATER_THAN, 10) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border) | center;
}

Element PackageInstallDialog::renderInputField() const {
    auto& colors = theme_.getColors();
    Elements input_elements;

    input_elements.push_back(text("  Package Name: ") | color(colors.keyword) | bold);

    // 显示输入内容和光标
    std::string display_text = package_name_input_;
    if (cursor_position_ <= display_text.length()) {
        std::string before_cursor = display_text.substr(0, cursor_position_);
        std::string at_cursor = (cursor_position_ < display_text.length())
                                    ? std::string(1, display_text[cursor_position_])
                                    : " ";
        std::string after_cursor = (cursor_position_ < display_text.length())
                                       ? display_text.substr(cursor_position_ + 1)
                                       : "";

        input_elements.push_back(text(before_cursor) | color(colors.foreground));
        input_elements.push_back(text(at_cursor) | color(colors.foreground) |
                                 bgcolor(colors.selection) | bold);
        input_elements.push_back(text(after_cursor) | color(colors.foreground));
    } else {
        input_elements.push_back(text(display_text) | color(colors.foreground));
        input_elements.push_back(text("_") | color(colors.foreground) | bgcolor(colors.selection) |
                                 bold);
    }

    return hbox(std::move(input_elements)) | bgcolor(colors.menubar_bg);
}

Element PackageInstallDialog::renderOperationStatus() const {
    auto& colors = theme_.getColors();
    Elements status_elements;

    std::lock_guard<std::mutex> lock(operation_mutex_);

    // 根据操作状态选择颜色和图标
    Color status_color;
    std::string status_icon;

    if (operation_in_progress_) {
        // 进行中：黄色警告
        status_color = colors.warning;
        status_icon = icons::REFRESH;
    } else if (operation_success_) {
        // 成功完成：绿色成功
        status_color = colors.success;
        status_icon = icons::CHECK_CIRCLE;
    } else {
        // 失败：红色错误
        status_color = colors.error;
        status_icon = icons::ERROR;
    }

    std::string status_text = operation_status_;

    status_elements.push_back(text("  ") | color(colors.comment));
    status_elements.push_back(text(status_icon) | color(status_color));
    status_elements.push_back(text(" ") | color(colors.comment));
    status_elements.push_back(text(status_text) | color(status_color) |
                              (operation_in_progress_ ? bold : dim));

    return hbox(std::move(status_elements));
}

} // namespace ui
} // namespace pnana
