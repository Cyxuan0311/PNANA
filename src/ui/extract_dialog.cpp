#include "ui/extract_dialog.h"
#include "ui/icons.h"
#include <algorithm>
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

ExtractDialog::ExtractDialog(Theme& theme)
    : theme_(theme), visible_(false), selected_index_(0), scroll_offset_(0) {}

void ExtractDialog::show(const std::string& /* current_directory */,
                         std::function<void(const features::ArchiveFile&)> on_select,
                         std::function<void()> on_cancel) {
    visible_ = true;
    selected_index_ = 0;
    scroll_offset_ = 0;
    on_select_ = on_select;
    on_cancel_ = on_cancel;
}

void ExtractDialog::hide() {
    visible_ = false;
}

void ExtractDialog::setArchiveFiles(const std::vector<features::ArchiveFile>& files) {
    archive_files_ = files;
    if (selected_index_ >= archive_files_.size() && !archive_files_.empty()) {
        selected_index_ = archive_files_.size() - 1;
    }
}

bool ExtractDialog::handleInput(ftxui::Event event) {
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
        if (selected_index_ < archive_files_.size() && on_select_) {
            visible_ = false;
            on_select_(archive_files_[selected_index_]);
        }
        return true;
    }

    if (event == Event::ArrowDown) {
        if (!archive_files_.empty()) {
            selected_index_ = (selected_index_ + 1) % archive_files_.size();
            // 更新滚动偏移
            if (selected_index_ >= scroll_offset_ + 10) {
                scroll_offset_ = selected_index_ - 9;
            }
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        }
        return true;
    }

    if (event == Event::ArrowUp) {
        if (!archive_files_.empty()) {
            selected_index_ = (selected_index_ + archive_files_.size() - 1) % archive_files_.size();
            // 更新滚动偏移
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
            if (selected_index_ >= scroll_offset_ + 10) {
                scroll_offset_ = selected_index_ - 9;
            }
        }
        return true;
    }

    return false;
}

ftxui::Element ExtractDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements dialog_content;

    // 标题
    dialog_content.push_back(hbox({text(" "), text(icons::FILE) | color(colors.keyword) | bold,
                                   text(" Extract Archive "), text(" ")}) |
                             bold | bgcolor(colors.menubar_bg) | center);

    dialog_content.push_back(separator());

    // 文件列表
    if (archive_files_.empty()) {
        dialog_content.push_back(text(""));
        dialog_content.push_back(
            hbox({text("  "), text("No archive files found in current directory") |
                                  color(colors.comment) | dim}) |
            center);
    } else {
        dialog_content.push_back(renderFileList());
    }

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 帮助栏
    dialog_content.push_back(renderHelpBar());

    int height =
        archive_files_.empty()
            ? 10
            : std::min(20, int(8 + static_cast<int>(std::min(archive_files_.size(), size_t(10)))));

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 70) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

ftxui::Element ExtractDialog::renderFileList() {
    auto& colors = theme_.getColors();
    Elements items;

    size_t start_idx = scroll_offset_;
    size_t end_idx = std::min(start_idx + 10, archive_files_.size());

    for (size_t i = start_idx; i < end_idx; ++i) {
        const auto& file = archive_files_[i];
        bool is_selected = (i == selected_index_);

        Elements item_elements;

        // 选中指示器
        if (is_selected) {
            item_elements.push_back(text("► ") | color(colors.function));
        } else {
            item_elements.push_back(text("  "));
        }

        // 文件图标和名称
        std::string icon = icons::FILE;
        Color icon_color = colors.comment;

        if (file.type == "zip") {
            icon_color = colors.keyword;
        } else if (file.type.find("tar") != std::string::npos) {
            icon_color = colors.type;
        } else if (file.type == "7z" || file.type == "rar") {
            icon_color = colors.error;
        }

        item_elements.push_back(text(icon) | color(icon_color));
        item_elements.push_back(text(" "));

        // 文件名
        std::string display_name = file.name;
        if (display_name.length() > 50) {
            display_name = display_name.substr(0, 47) + "...";
        }

        item_elements.push_back(text(display_name) |
                                color(is_selected ? colors.foreground : colors.comment));

        // 类型标签
        item_elements.push_back(text(" ") | size(WIDTH, EQUAL, 1));
        item_elements.push_back(text("[" + file.type + "]") | color(colors.comment) | dim);

        Color bg_color = is_selected ? colors.selection : colors.background;
        items.push_back(hbox(item_elements) | bgcolor(bg_color));
    }

    return vbox(items);
}

ftxui::Element ExtractDialog::renderHelpBar() {
    auto& colors = theme_.getColors();

    Elements hints;
    hints.push_back(text("↑↓: Navigate  "));
    hints.push_back(text("Enter: Select  "));
    hints.push_back(text("Esc: Cancel"));

    return hbox(hints) | color(colors.comment) | center;
}

} // namespace ui
} // namespace pnana
