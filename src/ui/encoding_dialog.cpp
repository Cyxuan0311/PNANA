#include "ui/encoding_dialog.h"
#include "ui/icons.h"
#include <algorithm>

using namespace ftxui;

namespace pnana {
namespace ui {

EncodingDialog::EncodingDialog(Theme& theme) : theme_(theme), visible_(false), selected_index_(0) {
    // 初始化支持的编码列表（按常用程度排序）
    encodings_ = {
        "UTF-8",        // 最常用
        "UTF-16",       // 常用
        "UTF-16LE",     // 常用
        "UTF-16BE",     // 常用
        "GBK",          // 中文常用
        "GB2312",       // 中文常用
        "ASCII",        // 基础编码
        "ISO-8859-1",   // Latin-1
        "Windows-1252", // Windows Latin-1扩展
    };
}

void EncodingDialog::open(const std::string& current_encoding) {
    visible_ = true;
    current_encoding_ = current_encoding;

    // 查找当前编码的索引
    selected_index_ = 0;
    std::string upper_current = current_encoding;
    std::transform(upper_current.begin(), upper_current.end(), upper_current.begin(), ::toupper);

    for (size_t i = 0; i < encodings_.size(); ++i) {
        std::string upper_enc = encodings_[i];
        std::transform(upper_enc.begin(), upper_enc.end(), upper_enc.begin(), ::toupper);
        if (upper_enc == upper_current) {
            selected_index_ = i;
            break;
        }
    }
}

void EncodingDialog::close() {
    visible_ = false;
}

bool EncodingDialog::handleInput(ftxui::Event event) {
    if (!visible_)
        return false;

    if (event == Event::Escape) {
        if (on_cancel_) {
            on_cancel_();
        }
        close();
        return true;
    } else if (event == Event::Return) {
        executeSelected();
        return true;
    } else if (event == Event::ArrowUp) {
        selectPrevious();
        return true;
    } else if (event == Event::ArrowDown) {
        selectNext();
        return true;
    }

    return false;
}

Element EncodingDialog::render() {
    if (!visible_)
        return text("");

    auto& colors = theme_.getColors();

    Elements content;

    // 标题
    content.push_back(hbox({text(" "), text(icons::INFO) | color(colors.info), text(" "),
                            text("Select Encoding") | bold | color(colors.dialog_title_fg)}) |
                      bgcolor(colors.dialog_title_bg));

    content.push_back(separator());

    // 当前编码提示
    content.push_back(hbox({text(" "), text("Current encoding: ") | color(colors.comment),
                            text(current_encoding_) | color(colors.keyword) | bold}));

    content.push_back(text(""));

    // 编码列表
    size_t max_display = std::min(encodings_.size(), size_t(12));
    size_t start_index = 0;

    // 如果选中的项不在可见范围内，调整起始索引
    if (selected_index_ >= max_display) {
        start_index = selected_index_ - max_display + 1;
    }

    // 确保起始索引不会超出范围
    if (start_index + max_display > encodings_.size()) {
        start_index = (encodings_.size() > max_display) ? encodings_.size() - max_display : 0;
    }

    for (size_t i = start_index; i < start_index + max_display && i < encodings_.size(); ++i) {
        const std::string& encoding = encodings_[i];
        bool is_selected = (i == selected_index_);

        Elements line_elements;
        line_elements.push_back(text("  "));

        // 选中标记
        if (is_selected) {
            line_elements.push_back(text("▶ ") | color(colors.info) | bold);
        } else {
            line_elements.push_back(text("  "));
        }

        // 编码名称
        Color text_color = is_selected ? colors.dialog_fg : colors.comment;
        if (is_selected) {
            line_elements.push_back(text(encoding) | color(text_color) | bold);
        } else {
            line_elements.push_back(text(encoding) | color(text_color));
        }

        // 如果是当前编码，显示标记
        std::string upper_encoding = encoding;
        std::transform(upper_encoding.begin(), upper_encoding.end(), upper_encoding.begin(),
                       ::toupper);
        std::string upper_current = current_encoding_;
        std::transform(upper_current.begin(), upper_current.end(), upper_current.begin(),
                       ::toupper);
        if (upper_encoding == upper_current) {
            line_elements.push_back(filler());
            line_elements.push_back(text("●") | color(colors.success) | bold);
            line_elements.push_back(text(" current") | color(colors.success) | dim);
        }

        Element line = hbox(line_elements);
        if (is_selected) {
            line = line | bgcolor(colors.selection) | color(colors.dialog_fg);
        }

        content.push_back(line);
    }

    // 如果还有更多编码，显示提示
    if (encodings_.size() > max_display) {
        content.push_back(text(""));
        size_t remaining = encodings_.size() - max_display;
        std::string more_text =
            (remaining == 1) ? "1 more encoding" : std::to_string(remaining) + " more encodings";
        content.push_back(
            hbox({text("  "), text("... " + more_text) | color(colors.comment) | dim}));
    }

    content.push_back(text(""));
    content.push_back(separator());

    // 提示信息（更清晰的布局）
    Elements hint_elements;
    hint_elements.push_back(text("  "));
    hint_elements.push_back(text("↑↓") | color(colors.info) | bold);
    hint_elements.push_back(text(" Navigate  "));
    hint_elements.push_back(text("Enter") | color(colors.success) | bold);
    hint_elements.push_back(text(" Select  "));
    hint_elements.push_back(text("Esc") | color(colors.error) | bold);
    hint_elements.push_back(text(" Cancel"));

    content.push_back(hbox(hint_elements) | color(colors.comment) | dim);

    // 计算对话框高度（更合理的计算）
    int base_height = 8; // 标题、分隔符、提示等固定高度
    int list_height = static_cast<int>(max_display) + 1; // 列表项高度
    int height = std::min(25, base_height + list_height);

    // 应用边框颜色装饰器
    auto applyBorderColor = [&colors](ftxui::Element child) {
        return child | borderRounded | ftxui::color(colors.dialog_border);
    };

    return applyBorderColor(window(text(""), vbox(content)) | size(WIDTH, EQUAL, 55) |
                            size(HEIGHT, EQUAL, height) | bgcolor(colors.dialog_bg) |
                            color(colors.dialog_fg) | center);
}

void EncodingDialog::selectNext() {
    if (!encodings_.empty()) {
        selected_index_ = (selected_index_ + 1) % encodings_.size();
    }
}

void EncodingDialog::selectPrevious() {
    if (!encodings_.empty()) {
        if (selected_index_ == 0) {
            selected_index_ = encodings_.size() - 1;
        } else {
            selected_index_--;
        }
    }
}

void EncodingDialog::executeSelected() {
    if (selected_index_ < encodings_.size()) {
        std::string selected = encodings_[selected_index_];
        close();
        if (on_confirm_) {
            on_confirm_(selected);
        }
    }
}

std::string EncodingDialog::getSelectedEncoding() const {
    if (selected_index_ < encodings_.size()) {
        return encodings_[selected_index_];
    }
    return "UTF-8";
}

} // namespace ui
} // namespace pnana
