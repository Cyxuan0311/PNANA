#include "ui/move_file_dialog.h"
#include "ui/icons.h"
#include <filesystem>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;
namespace fs = std::filesystem;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

static bool try_complete_path(const std::string& input, const std::string& base_dir,
                              std::string& out) {
    try {
        size_t pos = input.find_last_of("/\\");
        std::string base_dir_str = base_dir;
        std::string prefix;
        if (pos == std::string::npos) {
            prefix = input;
        } else {
            std::string dir_part = input.substr(0, pos + 1);
            prefix = input.substr(pos + 1);
            fs::path p(dir_part);
            if (p.is_absolute())
                base_dir_str = p.string();
            else
                base_dir_str = (fs::path(base_dir) / p).string();
        }

        fs::path base(base_dir_str);
        if (!fs::exists(base) || !fs::is_directory(base))
            return false;

        std::vector<std::string> matches;
        for (auto& entry : fs::directory_iterator(base)) {
            std::string name = entry.path().filename().string();
            if (name.rfind(prefix, 0) == 0) {
                if (entry.is_directory())
                    name += '/';
                matches.push_back(name);
            }
        }
        if (matches.empty())
            return false;

        std::string common = matches[0];
        for (size_t i = 1; i < matches.size(); ++i) {
            size_t j = 0;
            while (j < common.size() && j < matches[i].size() && common[j] == matches[i][j])
                ++j;
            common = common.substr(0, j);
        }

        if (pos == std::string::npos)
            out = common;
        else
            out = input.substr(0, pos + 1) + common;
        return true;
    } catch (...) {
        return false;
    }
}

namespace pnana {
namespace ui {

MoveFileDialog::MoveFileDialog(Theme& theme) : theme_(theme) {}

void MoveFileDialog::setSourcePath(const std::string& path) {
    source_path_ = path;
    try {
        fs::path p(path);
        source_name_ = p.filename().string();
    } catch (...) {
        source_name_ = path;
    }
}

void MoveFileDialog::setTargetDirectory(const std::string& dir) {
    target_directory_ = dir;
}

void MoveFileDialog::setInput(const std::string& input) {
    input_ = input;
}

Element MoveFileDialog::render() {
    auto& colors = theme_.getColors();

    Elements dialog_content;

    // 标题 - 使用更醒目的样式
    dialog_content.push_back(hbox({text(" "), text(icons::FILE_MOVE) | color(colors.keyword) | bold,
                                   text(" Move File/Folder "), text(" ")}) |
                             bold | bgcolor(colors.menubar_bg) | center);

    dialog_content.push_back(separator());

    // 源文件/文件夹信息
    dialog_content.push_back(text(""));
    std::string display_source = source_name_.length() > 45
                                     ? "..." + source_name_.substr(source_name_.length() - 42)
                                     : source_name_;
    dialog_content.push_back(
        hbox({text("  "), text(icons::FILE) | color(colors.comment), text(" Source: "),
              text(display_source) | color(colors.foreground)}));

    dialog_content.push_back(text(""));

    // 当前目录 - 使用图标和更清晰的显示
    std::string display_dir =
        target_directory_.length() > 40
            ? "..." + target_directory_.substr(target_directory_.length() - 37)
            : target_directory_;
    dialog_content.push_back(hbox({text("  "), text(icons::LOCATION) | color(colors.comment),
                                   text(" Current: "), text(display_dir) | color(colors.comment)}));

    dialog_content.push_back(text(""));

    // 目标路径输入框
    std::string target_input_display = input_.empty() ? "_" : input_ + "_";
    dialog_content.push_back(
        hbox({text("  "), text("Target path: ") | color(colors.foreground), text(" "),
              text(target_input_display) | color(colors.foreground) | bgcolor(colors.selection)}));

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 提示 - 使用更清晰的格式，并添加快捷键提示
    dialog_content.push_back(
        hbox({text("  "), text("Enter") | color(colors.function) | bold, text(": Move  "),
              text("Esc") | color(colors.function) | bold, text(": Cancel")}) |
        dim);

    // Tab 补全提示
    dialog_content.push_back(
        hbox({text("  "), text("Tab") | color(colors.function) | bold, text(": Path completion")}) |
        dim);

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 60) |
           size(HEIGHT, EQUAL, 14) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

bool MoveFileDialog::handleInput(Event event) {
    if (event == Event::Escape) {
        return true;
    }

    if (event == Event::Return) {
        return true;
    }

    if (event == Event::Tab) {
        std::string input = getInput();
        std::string out;
        if (try_complete_path(input, target_directory_, out)) {
            setInput(out);
        }
        return true;
    }

    if (event == Event::Backspace) {
        std::string input = getInput();
        if (!input.empty()) {
            input.pop_back();
            setInput(input);
        }
        return true;
    }

    if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            if (c >= 32 && c < 127) {
                std::string input = getInput();
                input += c;
                setInput(input);
            }
        }
        return true;
    }

    return false;
}

} // namespace ui
} // namespace pnana
