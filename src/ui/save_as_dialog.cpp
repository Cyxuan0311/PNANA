#include "ui/save_as_dialog.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

SaveAsDialog::SaveAsDialog(Theme& theme) : theme_(theme) {}

void SaveAsDialog::setCurrentFileName(const std::string& filename) {
    current_filename_ = filename;
}

void SaveAsDialog::setInput(const std::string& input) {
    input_ = input;
}

bool SaveAsDialog::isUntitled() const {
    return current_filename_.empty();
}

Element SaveAsDialog::render() {
    auto& colors = theme_.getColors();

    Elements dialog_content;

    // 检查是否为未命名文件
    bool is_untitled = isUntitled();

    // 标题 - 根据是否为未命名文件显示不同标题，使用更醒目的样式
    dialog_content.push_back(
        hbox({text(" "), text(icons::SAVE) | color(colors.keyword) | bold,
              text(is_untitled ? " Save New File " : " Save As "), text(" ")}) |
        bold | bgcolor(colors.menubar_bg) | center);

    dialog_content.push_back(separator());

    // 当前文件信息
    dialog_content.push_back(text(""));
    if (!current_filename_.empty()) {
        if (is_untitled) {
            // 未命名文件：显示更醒目的提示信息
            dialog_content.push_back(
                hbox({text("  "), text(icons::WARNING) | color(colors.warning), text(" Status: "),
                      text("[Untitled]") | color(colors.warning) | bold,
                      text(" - Enter file name to save")}));
        } else {
            // 已命名文件：显示当前文件名
            dialog_content.push_back(
                hbox({text("  "), text(icons::FILE) | color(colors.comment), text(" Current: "),
                      text(current_filename_) | color(colors.comment)}));
        }
    }

    dialog_content.push_back(text(""));

    // 输入框 - 使用更醒目的样式突出显示输入区域
    std::string input_display = input_.empty() ? "_" : input_ + "_";
    dialog_content.push_back(hbox(
        {text("  "), text(is_untitled ? "File name: " : "File path: ") | color(colors.foreground),
         text(" "),
         text(input_display) | bold | color(colors.background) | bgcolor(colors.selection)}));

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 提示 - 使用更清晰的格式
    dialog_content.push_back(
        hbox({text("  "), text("Enter") | color(colors.function) | bold, text(": Save  "),
              text("Esc") | color(colors.function) | bold, text(": Cancel")}) |
        dim);

    // 根据是否为未命名文件调整对话框大小
    int dialog_height = is_untitled ? 13 : 12;

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 65) |
           size(HEIGHT, EQUAL, dialog_height) | bgcolor(colors.background) | border;
}

} // namespace ui
} // namespace pnana
