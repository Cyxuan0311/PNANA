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

void RecentFilesPopup::setData(bool is_open,
                               const std::vector<features::ProjectItem>& recent_projects,
                               size_t selected_index) {
    is_open_ = is_open;
    recent_projects_ = recent_projects;
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
        std::min(22, int(10 + static_cast<int>(std::min(recent_projects_.size(), size_t(8)))));

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
        if (file_open_callback_ && selected_index_ < recent_projects_.size()) {
            file_open_callback_(selected_index_);
        }
        close();
        return true;
    } else if (event == ftxui::Event::ArrowDown) {
        if (!recent_projects_.empty()) {
            selected_index_ = (selected_index_ + 1) % recent_projects_.size();
        }
        return true;
    } else if (event == ftxui::Event::ArrowUp) {
        if (!recent_projects_.empty()) {
            if (selected_index_ == 0) {
                selected_index_ = recent_projects_.size() - 1;
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

    if (recent_projects_.empty()) {
        list_elements.push_back(
            hbox({text("  "), text("No recent projects") | color(colors.comment) | dim}));
    } else {
        for (size_t i = 0; i < recent_projects_.size() && i < 8; ++i) {
            const auto& project = recent_projects_[i];
            bool is_selected = (i == selected_index_);

            Elements project_elements;
            project_elements.push_back(text("  "));

            // 选中标记
            if (is_selected) {
                project_elements.push_back(text("► ") | color(colors.success) | bold);
            } else {
                project_elements.push_back(text("  "));
            }

            // 项目图标
            std::string icon = getProjectIcon(project);
            project_elements.push_back(text(icon + " ") | color(colors.success));

            // 项目名
            std::string name = getProjectName(project);
            project_elements.push_back(text(name) | (is_selected ? color(colors.dialog_fg) | bold
                                                                 : color(colors.foreground)));

            // 类型标识
            std::string type_str = getProjectTypeString(project);
            project_elements.push_back(text(" ") | color(colors.comment));
            project_elements.push_back(text(type_str) | color(colors.comment) | dim);

            // 路径
            std::string path = getProjectPath(project);
            if (!path.empty()) {
                project_elements.push_back(filler());
                project_elements.push_back(text(path) | color(colors.comment) | dim);
            }

            Element project_line = hbox(project_elements);
            if (is_selected) {
                project_line = project_line | bgcolor(colors.selection);
            }

            list_elements.push_back(project_line);
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

std::string RecentFilesPopup::getProjectIcon(const features::ProjectItem& project) const {
    if (project.type == features::ProjectType::FOLDER) {
        return pnana::ui::icons::FOLDER; // 文件夹图标
    } else {
        return getFileIcon(project.path);
    }
}

std::string RecentFilesPopup::getProjectName(const features::ProjectItem& project) const {
    try {
        std::filesystem::path path(project.path);
        return path.filename().string();
    } catch (...) {
        return project.path;
    }
}

std::string RecentFilesPopup::getProjectTypeString(const features::ProjectItem& project) const {
    return project.type == features::ProjectType::FOLDER ? "[DIR]" : "[FILE]";
}

std::string RecentFilesPopup::getProjectPath(const features::ProjectItem& project) const {
    try {
        std::filesystem::path path(project.path);
        std::filesystem::path parent = path.parent_path();
        if (!parent.empty()) {
            return parent.string();
        }
    } catch (...) {
        // 如果路径解析失败，返回空字符串
    }
    return "";
}

} // namespace ui
} // namespace pnana
