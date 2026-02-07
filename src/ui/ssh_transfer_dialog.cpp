#include "ui/ssh_transfer_dialog.h"
#include "ui/icons.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

SSHTransferDialog::SSHTransferDialog(Theme& theme)
    : theme_(theme), visible_(false), current_field_(0), cursor_position_(0), direction_("upload") {
}

void SSHTransferDialog::show(
    std::function<void(const std::vector<SSHTransferItem>&)> on_start_transfer,
    std::function<void()> on_cancel) {
    visible_ = true;
    current_field_ = 0;
    cursor_position_ = 0;
    on_start_transfer_ = on_start_transfer;
    on_cancel_ = on_cancel;

    // 重置输入字段
    local_path_input_.clear();
    remote_path_input_.clear();
    direction_ = "upload";
    transfer_items_.clear();
}

void SSHTransferDialog::hide() {
    visible_ = false;
}

bool SSHTransferDialog::handleInput(Event event) {
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
        if (current_field_ == 2) { // 方向选择字段
            addCurrentItem();
            return true;
        } else {
            moveToNextField();
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

    if (event == Event::F5) { // F5 开始传输
        if (!transfer_items_.empty()) {
            visible_ = false;
            if (on_start_transfer_) {
                on_start_transfer_(transfer_items_);
            }
        }
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

Element SSHTransferDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements fields;

    // 标题
    Elements title_elements;
    title_elements.push_back(text(icons::ARROW_UP) | color(Color::Cyan));
    title_elements.push_back(text(" SSH File Transfer ") | color(colors.foreground) | bold);
    fields.push_back(hbox(title_elements) | center);

    fields.push_back(separator());
    fields.push_back(text(""));

    // 输入字段
    fields.push_back(renderField("Local Path", local_path_input_, 0));
    fields.push_back(renderField("Remote Path", remote_path_input_, 1));

    // 方向选择
    Elements direction_elements;
    direction_elements.push_back(text("Direction: ") | color(colors.comment));

    bool upload_selected = (direction_ == "upload");
    bool download_selected = (direction_ == "download");

    direction_elements.push_back(text("["));
    if (current_field_ == 2 && upload_selected) {
        direction_elements.push_back(text("Upload") | bgcolor(colors.current_line) |
                                     color(colors.foreground) | bold);
    } else {
        direction_elements.push_back(text("Upload") |
                                     color(upload_selected ? colors.keyword : colors.comment));
    }
    direction_elements.push_back(text("/"));
    if (current_field_ == 2 && download_selected) {
        direction_elements.push_back(text("Download") | bgcolor(colors.current_line) |
                                     color(colors.foreground) | bold);
    } else {
        direction_elements.push_back(text("Download") |
                                     color(download_selected ? colors.keyword : colors.comment));
    }
    direction_elements.push_back(text("]"));

    fields.push_back(hbox(direction_elements));

    fields.push_back(text(""));
    fields.push_back(separator());

    // 传输队列
    if (!transfer_items_.empty()) {
        fields.push_back(text("Transfer Queue:") | color(colors.comment) | bold);
        fields.push_back(renderTransferList());
        fields.push_back(separator());
    }

    // 提示信息
    Elements hints;
    hints.push_back(text("↑↓: Navigate  "));
    hints.push_back(text("Tab: Next field  "));
    hints.push_back(text("Enter: Add to queue  "));
    hints.push_back(text("F5: Start transfer  "));
    hints.push_back(text("Esc: Cancel"));
    fields.push_back(hbox(hints) | color(colors.comment) | center);

    Element dialog_content = vbox(fields);

    return window(text("SSH File Transfer"), dialog_content) | size(WIDTH, GREATER_THAN, 80) |
           size(HEIGHT, GREATER_THAN, 25) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

void SSHTransferDialog::addTransferItem(const SSHTransferItem& item) {
    transfer_items_.push_back(item);
}

void SSHTransferDialog::updateProgress(const std::string& local_path, size_t transferred) {
    for (auto& item : transfer_items_) {
        if (item.local_path == local_path) {
            item.transferred_size = transferred;
            break;
        }
    }
}

void SSHTransferDialog::setTransferStatus(const std::string& local_path, const std::string& status,
                                          const std::string& error_message) {
    for (auto& item : transfer_items_) {
        if (item.local_path == local_path) {
            item.status = status;
            if (!error_message.empty()) {
                item.error_message = error_message;
            }
            break;
        }
    }
}

void SSHTransferDialog::clearTransfers() {
    transfer_items_.clear();
}

void SSHTransferDialog::addCurrentItem() {
    if (local_path_input_.empty() || remote_path_input_.empty()) {
        return;
    }

    SSHTransferItem item;
    item.local_path = local_path_input_;
    item.remote_path = remote_path_input_;
    item.direction = direction_;
    item.status = "pending";
    item.transferred_size = 0;
    item.file_size = 0; // 稍后计算

    transfer_items_.push_back(item);

    // 清空输入字段准备添加下一个
    local_path_input_.clear();
    remote_path_input_.clear();
    cursor_position_ = 0;
}

void SSHTransferDialog::removeItem(size_t index) {
    if (index < transfer_items_.size()) {
        transfer_items_.erase(transfer_items_.begin() + index);
    }
}

void SSHTransferDialog::moveToNextField() {
    if (current_field_ < 2) {
        current_field_++;
        std::string* field = getCurrentField();
        if (field) {
            cursor_position_ = field->length();
        }
    }
}

void SSHTransferDialog::moveToPreviousField() {
    if (current_field_ > 0) {
        current_field_--;
        std::string* field = getCurrentField();
        if (field) {
            cursor_position_ = field->length();
        }
    }
}

void SSHTransferDialog::insertChar(char ch) {
    std::string* field = getCurrentField();
    if (field && cursor_position_ <= field->length()) {
        field->insert(cursor_position_, 1, ch);
        cursor_position_++;
    } else if (current_field_ == 2) {
        // 方向字段的特殊处理
        if (ch == 'u' || ch == 'U') {
            direction_ = "upload";
        } else if (ch == 'd' || ch == 'D') {
            direction_ = "download";
        }
    }
}

void SSHTransferDialog::deleteChar() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ < field->length()) {
        field->erase(cursor_position_, 1);
    }
}

void SSHTransferDialog::backspace() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ > 0) {
        field->erase(cursor_position_ - 1, 1);
        cursor_position_--;
    }
}

void SSHTransferDialog::moveCursorLeft() {
    if (cursor_position_ > 0) {
        cursor_position_--;
    }
}

void SSHTransferDialog::moveCursorRight() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ < field->length()) {
        cursor_position_++;
    }
}

std::string* SSHTransferDialog::getCurrentField() {
    switch (current_field_) {
        case 0:
            return &local_path_input_;
        case 1:
            return &remote_path_input_;
        case 2:
            return nullptr; // 方向字段特殊处理
        default:
            return nullptr;
    }
}

Element SSHTransferDialog::renderTransferList() {
    Elements items;
    auto& colors = theme_.getColors();

    for (size_t i = 0; i < transfer_items_.size(); ++i) {
        const auto& item = transfer_items_[i];

        Elements item_elements;
        item_elements.push_back(
            text(item.direction == "upload" ? icons::ARROW_UP : icons::ARROW_DOWN) |
            color(item.direction == "upload" ? Color::Green : Color::Blue));

        // 文件路径
        std::string path_display = item.direction == "upload"
                                       ? item.local_path + " → " + item.remote_path
                                       : item.remote_path + " → " + item.local_path;
        item_elements.push_back(text(" " + path_display) | color(colors.foreground));

        // 状态
        std::string status_icon;
        Color status_color = colors.comment;
        if (item.status == "pending") {
            status_icon = "⏳";
            status_color = Color::Yellow;
        } else if (item.status == "in_progress") {
            status_icon = "⏳";
            status_color = Color::Blue;
        } else if (item.status == "completed") {
            status_icon = "✓";
            status_color = Color::Green;
        } else if (item.status == "error") {
            status_icon = "✗";
            status_color = Color::Red;
        }

        item_elements.push_back(text(" " + status_icon) | color(status_color));

        // 进度条（如果正在传输）
        if (item.status == "in_progress" && item.file_size > 0) {
            item_elements.push_back(text(" ") | flex_grow);
            item_elements.push_back(renderProgressBar(item));
        }

        items.push_back(hbox(item_elements));

        // 如果有错误信息，显示在下一行
        if (item.status == "error" && !item.error_message.empty()) {
            items.push_back(text("  Error: " + item.error_message) | color(Color::Red));
        }
    }

    return vbox(items);
}

Element SSHTransferDialog::renderProgressBar(const SSHTransferItem& item) {
    if (item.file_size == 0) {
        return text("");
    }

    auto& colors = theme_.getColors();
    float progress = static_cast<float>(item.transferred_size) / item.file_size;
    int bar_width = 20;
    int filled = static_cast<int>(progress * bar_width);

    std::string bar = "[";
    for (int i = 0; i < bar_width; ++i) {
        bar += (i < filled) ? "█" : "░";
    }
    bar += "]";

    std::ostringstream oss;
    oss << " " << static_cast<int>(progress * 100) << "%";

    return hbox({text(bar) | color(colors.success), text(oss.str()) | color(colors.comment)});
}

Element SSHTransferDialog::renderField(const std::string& label, std::string& value,
                                       size_t field_idx) {
    auto& colors = theme_.getColors();
    bool is_focused = (static_cast<size_t>(current_field_) == field_idx);

    Elements field_elements;
    field_elements.push_back(text(label + ": ") | color(colors.comment) | size(WIDTH, EQUAL, 15));

    if (is_focused) {
        size_t cursor_pos = cursor_position_;
        if (cursor_pos > value.length()) {
            cursor_pos = value.length();
        }

        std::string before = value.substr(0, cursor_pos);
        std::string cursor_char = cursor_pos < value.length() ? value.substr(cursor_pos, 1) : " ";
        std::string after = cursor_pos < value.length() ? value.substr(cursor_pos + 1) : "";

        field_elements.push_back(
            hbox({text(before) | color(colors.foreground),
                  text(cursor_char) | bgcolor(colors.foreground) | color(colors.background) | bold,
                  text(after) | color(colors.foreground)}) |
            bgcolor(colors.current_line));
    } else {
        field_elements.push_back(text(value.empty() ? "(empty)" : value) |
                                 color(value.empty() ? colors.comment : colors.foreground) | dim);
    }

    return hbox(field_elements);
}

} // namespace ui
} // namespace pnana
