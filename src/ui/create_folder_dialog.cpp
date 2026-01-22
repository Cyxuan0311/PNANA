#include "ui/create_folder_dialog.h"
#include "ui/icons.h"
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

CreateFolderDialog::CreateFolderDialog(Theme& theme) : theme_(theme) {}

void CreateFolderDialog::setCurrentDirectory(const std::string& dir) {
    current_directory_ = dir;
}

void CreateFolderDialog::setInput(const std::string& input) {
    input_ = input;
}

Element CreateFolderDialog::render() {
    auto& colors = theme_.getColors();

    Elements dialog_content;

    // 标题 - 使用更醒目的样式
    dialog_content.push_back(hbox({text(" "), text(icons::FOLDER) | color(colors.keyword) | bold,
                                   text(" Create New Folder "), text(" ")}) |
                             bold | bgcolor(colors.menubar_bg) | center);

    dialog_content.push_back(separator());

    // 当前目录 - 使用图标和更清晰的显示
    dialog_content.push_back(text(""));
    std::string display_dir =
        current_directory_.length() > 40
            ? "..." + current_directory_.substr(current_directory_.length() - 37)
            : current_directory_;
    dialog_content.push_back(
        hbox({text("  "), text(icons::LOCATION) | color(colors.comment), text(" Location: "),
              text(display_dir) | color(colors.comment)}));

    dialog_content.push_back(text(""));

    // 输入框 - 使用与file_picker一致的光标样式
    std::string folder_input_display = input_.empty() ? "_" : input_ + "_";
    dialog_content.push_back(
        hbox({text("  "), text("Folder name: ") | color(colors.foreground), text(" "),
              text(folder_input_display) | color(colors.foreground) | bgcolor(colors.selection)}));

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 提示 - 使用更清晰的格式，并添加快捷键提示
    dialog_content.push_back(
        hbox({text("  "), text("Enter") | color(colors.function) | bold, text(": Create  "),
              text("Esc") | color(colors.function) | bold, text(": Cancel")}) |
        dim);

    // 添加快捷键提示行
    dialog_content.push_back(hbox({text("  "), text("Tip: Use ") | dim,
                                   text("Alt+F") | color(colors.function) | bold | dim,
                                   text(" to create folder") | dim}) |
                             dim);

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 55) |
           size(HEIGHT, EQUAL, 13) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

} // namespace ui
} // namespace pnana
