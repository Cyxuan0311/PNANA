#include "ui/extract_progress_dialog.h"
#include "ui/icons.h"
#include <chrono>
#include <cmath>
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

ExtractProgressDialog::ExtractProgressDialog(Theme& theme)
    : theme_(theme), visible_(false), progress_(0.0f) {}

void ExtractProgressDialog::show(const std::string& archive_name, const std::string& extract_path) {
    visible_ = true;
    archive_name_ = archive_name;
    extract_path_ = extract_path;
    progress_ = 0.0f;
    status_ = "Extracting...";
}

void ExtractProgressDialog::hide() {
    visible_ = false;
}

void ExtractProgressDialog::setProgress(float progress) {
    progress_ = std::max(0.0f, std::min(1.0f, progress));
}

void ExtractProgressDialog::setStatus(const std::string& status) {
    status_ = status;
}

ftxui::Element ExtractProgressDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements dialog_content;

    // 标题
    dialog_content.push_back(hbox({text(" "), text(icons::FILE) | color(colors.keyword) | bold,
                                   text(" Extracting Archive "), text(" ")}) |
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

    // 目标路径
    std::string display_path = extract_path_;
    if (display_path.length() > 50) {
        display_path = "..." + display_path.substr(display_path.length() - 47);
    }
    dialog_content.push_back(hbox({text("  "), text(icons::LOCATION) | color(colors.comment),
                                   text(" To: "), text(display_path) | color(colors.foreground)}));

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 进度条和状态
    dialog_content.push_back(text(""));
    dialog_content.push_back(renderProgressBar());
    dialog_content.push_back(text(""));

    // 状态消息
    dialog_content.push_back(
        hbox({text("  "), renderSpinner(), text(" "), text(status_) | color(colors.info)}) |
        center);

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 提示
    dialog_content.push_back(
        hbox({text("  "), text("Please wait while extracting...") | color(colors.comment) | dim}) |
        center);

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 70) |
           size(HEIGHT, EQUAL, 15) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

ftxui::Element ExtractProgressDialog::renderProgressBar() {
    auto& colors = theme_.getColors();

    int bar_width = 50;
    int filled = static_cast<int>(progress_ * bar_width);

    std::string bar = "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < filled) {
            bar += "█";
        } else if (i == filled && progress_ < 1.0f) {
            bar += "░";
        } else {
            bar += "░";
        }
    }
    bar += "]";

    std::ostringstream oss;
    oss << " " << static_cast<int>(progress_ * 100) << "%";

    Color bar_color = colors.success;
    if (progress_ < 0.3f) {
        bar_color = colors.warning;
    } else if (progress_ < 0.7f) {
        bar_color = colors.info;
    }

    return hbox({text("  "), text(bar) | color(bar_color),
                 text(oss.str()) | color(colors.comment)}) |
           center;
}

ftxui::Element ExtractProgressDialog::renderSpinner() {
    auto& colors = theme_.getColors();

    // 使用时间戳创建旋转动画
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    int frame = (ms / 200) % 4; // 每200ms切换一帧

    std::string spinner;
    switch (frame) {
        case 0:
            spinner = "⠋";
            break;
        case 1:
            spinner = "⠙";
            break;
        case 2:
            spinner = "⠹";
            break;
        case 3:
            spinner = "⠸";
            break;
        default:
            spinner = "⠋";
    }

    return text(spinner) | color(colors.function) | bold;
}

} // namespace ui
} // namespace pnana
