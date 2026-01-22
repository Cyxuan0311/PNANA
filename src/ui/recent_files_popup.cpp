#include "ui/recent_files_popup.h"
#include "ui/icons.h"
#include <algorithm>
#include <filesystem>
#include <ftxui/component/event.hpp>
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

RecentFilesPopup::RecentFilesPopup(Theme& theme)
    : theme_(theme), is_open_(false), selected_index_(0) {}

void RecentFilesPopup::setData(bool is_open, const std::vector<std::string>& recent_files,
                               size_t selected_index) {
    is_open_ = is_open;
    recent_files_ = recent_files;
    selected_index_ = selected_index;
}

ftxui::Element RecentFilesPopup::render() {
    if (!is_open_) {
        return text("");
    }

    const auto& colors = theme_.getColors();
    Elements dialog_content;

    // 标题
    dialog_content.push_back(renderTitle());

    dialog_content.push_back(separator());

    // 文件列表
    dialog_content.push_back(renderFileList());

    dialog_content.push_back(separator());

    // 帮助栏
    dialog_content.push_back(renderHelpBar());

    int height =
        std::min(22, int(10 + static_cast<int>(std::min(recent_files_.size(), size_t(8)))));

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 80) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

bool RecentFilesPopup::handleInput(ftxui::Event event) {
    if (!is_open_) {
        return false;
    }

    if (event == ftxui::Event::Escape) {
        close();
        return true;
    } else if (event == ftxui::Event::Return) {
        if (file_open_callback_ && selected_index_ < recent_files_.size()) {
            file_open_callback_(selected_index_);
        }
        close();
        return true;
    } else if (event == ftxui::Event::ArrowDown) {
        if (!recent_files_.empty()) {
            selected_index_ = (selected_index_ + 1) % recent_files_.size();
        }
        return true;
    } else if (event == ftxui::Event::ArrowUp) {
        if (!recent_files_.empty()) {
            if (selected_index_ == 0) {
                selected_index_ = recent_files_.size() - 1;
            } else {
                selected_index_--;
            }
        }
        return true;
    }

    return false;
}

void RecentFilesPopup::open() {
    is_open_ = true;
    selected_index_ = 0;
}

void RecentFilesPopup::close() {
    is_open_ = false;
    selected_index_ = 0;
}

void RecentFilesPopup::setFileOpenCallback(std::function<void(size_t)> callback) {
    file_open_callback_ = callback;
}

Element RecentFilesPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(pnana::ui::icons::GIT_HISTORY) | color(colors.success),
                 text(" Recent Files "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element RecentFilesPopup::renderFileList() const {
    const auto& colors = theme_.getColors();
    Elements list_elements;

    if (recent_files_.empty()) {
        list_elements.push_back(
            hbox({text("  "), text("No recent files") | color(colors.comment) | dim}));
    } else {
        for (size_t i = 0; i < recent_files_.size() && i < 8; ++i) {
            const auto& filepath = recent_files_[i];
            bool is_selected = (i == selected_index_);

            Elements file_elements;
            file_elements.push_back(text("  "));

            // 选中标记
            if (is_selected) {
                file_elements.push_back(text("► ") | color(colors.success) | bold);
            } else {
                file_elements.push_back(text("  "));
            }

            // 文件图标
            std::string icon = getFileIcon(filepath);
            file_elements.push_back(text(icon + " ") | color(colors.success));

            // 文件名
            std::string filename = getFileName(filepath);
            file_elements.push_back(text(filename) | (is_selected ? color(colors.dialog_fg) | bold
                                                                  : color(colors.foreground)));

            // 路径
            std::string path = getFilePath(filepath);
            if (!path.empty()) {
                file_elements.push_back(filler());
                file_elements.push_back(text(path) | color(colors.comment) | dim);
            }

            Element file_line = hbox(file_elements);
            if (is_selected) {
                file_line = file_line | bgcolor(colors.selection);
            }

            list_elements.push_back(file_line);
        }
    }

    return vbox(list_elements);
}

Element RecentFilesPopup::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
                 text("Enter") | color(colors.helpbar_key) | bold, text(": Open  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Cancel")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

std::string RecentFilesPopup::getFileName(const std::string& filepath) const {
    try {
        std::filesystem::path path(filepath);
        return path.filename().string();
    } catch (...) {
        // 如果路径解析失败，返回整个字符串
        return filepath;
    }
}

std::string RecentFilesPopup::getFilePath(const std::string& filepath) const {
    try {
        std::filesystem::path path(filepath);
        std::filesystem::path parent = path.parent_path();
        if (!parent.empty()) {
            return parent.string();
        }
    } catch (...) {
        // 如果路径解析失败，返回空字符串
    }
    return "";
}

std::string RecentFilesPopup::getFileIcon(const std::string& filepath) const {
    try {
        std::filesystem::path path(filepath);
        std::string extension = path.extension().string();
        if (!extension.empty() && extension[0] == '.') {
            extension = extension.substr(1);
        }
        return icon_mapper_.getIcon(extension);
    } catch (...) {
        return icon_mapper_.getIcon("default");
    }
}

} // namespace ui
} // namespace pnana
