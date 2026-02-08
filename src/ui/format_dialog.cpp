#include "ui/format_dialog.h"
#include "ui/icons.h"
#include "ui/theme.h"
#include "utils/file_type_color_mapper.h"
#include "utils/file_type_detector.h"
#include <algorithm>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace ui {

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

FormatDialog::FormatDialog(Theme& theme)
    : theme_(theme), color_mapper_(std::make_unique<pnana::utils::FileTypeColorMapper>(theme)),
      is_open_(false), selected_index_(0), scroll_offset_(0), max_visible_items_(10),
      search_query_(""), search_focused_(false) {}

void FormatDialog::open(const std::vector<std::string>& files, const std::string& directory_path) {
    files_ = files;
    directory_path_ = directory_path;
    selected_files_.clear();
    selected_index_ = 0;
    scroll_offset_ = 0;
    search_query_ = "";
    search_focused_ = false;
    is_open_ = true;
}

void FormatDialog::close() {
    is_open_ = false;
    files_.clear();
    selected_files_.clear();
    selected_index_ = 0;
    scroll_offset_ = 0;
}

bool FormatDialog::isOpen() const {
    return is_open_;
}

bool FormatDialog::handleInput(Event event) {
    if (!is_open_) {
        return false;
    }

    // 如果搜索框获得焦点，先处理搜索输入
    if (search_focused_) {
        if (event == Event::Escape) {
            search_focused_ = false;
            search_query_ = "";
            selected_index_ = 0;
            scroll_offset_ = 0;
            return true;
        } else if (event == Event::Return) {
            search_focused_ = false;
            return true;
        } else if (event == Event::Backspace) {
            if (!search_query_.empty()) {
                search_query_.pop_back();
            }
            selected_index_ = 0;
            scroll_offset_ = 0;
            return true;
        } else if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1 && ch[0] >= 32 && ch[0] < 127) {
                search_query_ += ch;
                selected_index_ = 0;
                scroll_offset_ = 0;
                return true;
            }
        }
        return true; // 搜索框获得焦点时，消耗所有输入
    }

    if (event == Event::Escape) {
        close();
        if (on_cancel_) {
            on_cancel_();
        }
        return true;
    } else if (event.is_character() && (event.character() == "/" || event.character() == "f")) {
        // 激活搜索框
        search_focused_ = true;
        return true;
    } else if (event == Event::Return) {
        // 执行格式化
        if (on_confirm_ && !selected_files_.empty()) {
            std::vector<std::string> selected_file_paths;
            for (size_t index : selected_files_) {
                if (index < files_.size()) {
                    selected_file_paths.push_back(files_[index]);
                }
            }
            on_confirm_(selected_file_paths);
        }
        close();
        return true;
    } else if (event == Event::ArrowUp) {
        auto display_files = search_query_.empty() ? files_ : getFilteredFiles();
        if (selected_index_ > 0) {
            selected_index_--;
            // 向上滚动
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        } else {
            selected_index_ = display_files.size() - 1;
            // 滚动到最后
            scroll_offset_ =
                std::max(static_cast<size_t>(0),
                         static_cast<size_t>(display_files.size()) - max_visible_items_);
        }
        return true;
    } else if (event == Event::ArrowDown) {
        auto display_files = search_query_.empty() ? files_ : getFilteredFiles();
        if (selected_index_ < display_files.size() - 1) {
            selected_index_++;
            // 向下滚动
            if (selected_index_ >= scroll_offset_ + max_visible_items_) {
                scroll_offset_ = selected_index_ - max_visible_items_ + 1;
            }
        } else {
            selected_index_ = 0;
            scroll_offset_ = 0;
        }
        return true;
    } else if (event.is_character() && event.character() == " ") {
        // 切换选中状态
        if (!search_query_.empty()) {
            // 在搜索模式下，需要将过滤后的索引映射到原始索引
            auto filtered_files = getFilteredFiles();
            if (selected_index_ < filtered_files.size()) {
                const auto& selected_file = filtered_files[selected_index_];
                auto it = std::find(files_.begin(), files_.end(), selected_file);
                if (it != files_.end()) {
                    size_t original_index = std::distance(files_.begin(), it);
                    toggleSelection(original_index);
                }
            }
        } else {
            toggleSelection(selected_index_);
        }
        return true;
    } else if (event == Event::Character('a') || event == Event::Character('A')) {
        // 全选/取消全选当前显示的文件
        auto display_files = search_query_.empty() ? files_ : getFilteredFiles();

        if (!search_query_.empty()) {
            // 在搜索模式下，检查是否所有过滤的文件都被选中
            bool all_filtered_selected = true;
            for (const auto& file : display_files) {
                auto it = std::find(files_.begin(), files_.end(), file);
                if (it != files_.end()) {
                    size_t original_index = std::distance(files_.begin(), it);
                    if (selected_files_.count(original_index) == 0) {
                        all_filtered_selected = false;
                        break;
                    }
                }
            }

            if (all_filtered_selected) {
                // 取消选择所有过滤的文件
                for (const auto& file : display_files) {
                    auto it = std::find(files_.begin(), files_.end(), file);
                    if (it != files_.end()) {
                        size_t original_index = std::distance(files_.begin(), it);
                        selected_files_.erase(original_index);
                    }
                }
            } else {
                // 选择所有过滤的文件
                for (const auto& file : display_files) {
                    auto it = std::find(files_.begin(), files_.end(), file);
                    if (it != files_.end()) {
                        size_t original_index = std::distance(files_.begin(), it);
                        selected_files_.insert(original_index);
                    }
                }
            }
        } else {
            // 正常模式下的全选/取消全选
            if (selected_files_.size() == files_.size()) {
                selected_files_.clear();
            } else {
                selected_files_.clear();
                for (size_t i = 0; i < files_.size(); ++i) {
                    selected_files_.insert(i);
                }
            }
        }
        return true;
    } else if (event == Event::PageUp) {
        // 向上翻页
        auto display_files = search_query_.empty() ? files_ : getFilteredFiles();
        if (scroll_offset_ >= max_visible_items_) {
            scroll_offset_ -= max_visible_items_;
            selected_index_ = scroll_offset_;
        } else {
            scroll_offset_ = 0;
            selected_index_ = 0;
        }
        return true;
    } else if (event == Event::PageDown) {
        // 向下翻页
        auto display_files = search_query_.empty() ? files_ : getFilteredFiles();
        if (scroll_offset_ + max_visible_items_ < display_files.size()) {
            scroll_offset_ += max_visible_items_;
            selected_index_ = scroll_offset_;
        }
        return true;
    }

    return false;
}

void FormatDialog::toggleSelection(size_t index) {
    if (index >= files_.size()) {
        return;
    }

    if (selected_files_.count(index)) {
        selected_files_.erase(index);
    } else {
        selected_files_.insert(index);
    }
}

Element FormatDialog::render() {
    if (!is_open_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    using namespace ftxui;
    Elements dialog_content;

    // 标题
    dialog_content.push_back(hbox({text(" "), text(pnana::ui::icons::CODE) | color(Color::Yellow),
                                   text(" Code Formatter "), text(" ")}) |
                             bold | bgcolor(Color::RGB(60, 60, 80)) | center);

    dialog_content.push_back(separator());

    // 搜索框
    std::string search_display = search_focused_ ? search_query_ + "_" : search_query_;
    if (search_display.empty() && !search_focused_) {
        search_display = "Press / or f to search...";
    }

    dialog_content.push_back(text(""));
    dialog_content.push_back(
        hbox({text("  Search: "),
              text(search_display) |
                  (search_focused_ ? color(Color::White) | bgcolor(Color::RGB(60, 60, 80))
                                   : color(Color::GrayLight))}));
    dialog_content.push_back(text(""));

    dialog_content.push_back(separator());

    // 目录信息
    dialog_content.push_back(text(""));
    dialog_content.push_back(
        hbox({text("  Directory: "), text(directory_path_) | color(Color::Cyan) | dim}));
    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 获取过滤后的文件列表
    auto filtered_files = getFilteredFiles();

    // 文件列表标题
    std::string title = "Files to format (" + std::to_string(selected_files_.size()) + "/" +
                        std::to_string(files_.size()) + " selected";
    if (!search_query_.empty()) {
        title += ", " + std::to_string(filtered_files.size()) + " filtered";
    }
    title += ")";
    dialog_content.push_back(hbox({text("  "), text(title) | color(Color::White) | bold}));
    dialog_content.push_back(text(""));

    // 文件列表
    if (files_.empty()) {
        dialog_content.push_back(
            hbox({text("  "), text("No supported files found in this directory") |
                                  color(Color::GrayDark) | dim}));
    } else if (filtered_files.empty() && !search_query_.empty()) {
        dialog_content.push_back(
            hbox({text("  "), text("No files match search: \"" + search_query_ + "\"") |
                                  color(Color::GrayDark) | dim}));
    } else {
        auto current_display_files = search_query_.empty() ? files_ : filtered_files;

        // 按需加载显示文件（虚拟滚动）
        size_t start_index = scroll_offset_;
        size_t end_index = std::min(start_index + max_visible_items_, current_display_files.size());

        for (size_t i = start_index; i < end_index; ++i) {
            const auto& file_path = current_display_files[i];
            // 找到在原始文件列表中的索引，用于选中状态检查
            auto it = std::find(files_.begin(), files_.end(), file_path);
            size_t original_index = (it != files_.end()) ? std::distance(files_.begin(), it) : i;

            bool is_selected = selected_files_.count(original_index);
            bool is_current = (i == selected_index_);

            // 获取相对路径用于显示
            std::string display_path = file_path;
            if (display_path.find(directory_path_) == 0) {
                display_path = display_path.substr(directory_path_.length());
                if (display_path.front() == '/' || display_path.front() == '\\') {
                    display_path = display_path.substr(1);
                }
            }

            // 检测文件类型并获取图标和颜色
            std::string filename = getFileName(file_path);
            std::string ext = getFileExtension(filename);
            std::string file_type = pnana::utils::FileTypeDetector::detectFileType(filename, ext);
            std::string file_icon = icons::getFileTypeIcon(file_type);
            ftxui::Color file_type_color = color_mapper_->getFileColor(filename, false);

            Elements file_elements;
            file_elements.push_back(text("  "));

            // 选中标记
            if (is_selected) {
                file_elements.push_back(text("☑ ") | color(Color::Green));
            } else {
                file_elements.push_back(text("☐ ") | color(Color::GrayDark));
            }

            // 当前选中高亮
            if (is_current) {
                file_elements.push_back(text("► ") | color(Color::Cyan) | bold);
            } else {
                file_elements.push_back(text("  "));
            }

            // 文件图标和文件名颜色
            // 当前选中时使用白色，否则使用文件类型颜色
            Color icon_color = is_current ? Color::White : file_type_color;
            Color name_color = is_current ? Color::White : file_type_color;

            // 文件图标
            if (!file_icon.empty()) {
                file_elements.push_back(text(file_icon + " ") | color(icon_color) |
                                        (is_current ? bold : nothing));
            }

            // 文件名（使用文件类型颜色）
            file_elements.push_back(text(display_path) | color(name_color) |
                                    (is_current ? bold : nothing));

            Element file_line = hbox(file_elements);
            if (is_current) {
                file_line = file_line | bgcolor(Color::RGB(50, 50, 70));
            }

            dialog_content.push_back(file_line);
        }

        // 显示滚动指示器
        if (current_display_files.size() > max_visible_items_) {
            std::string scroll_info = std::to_string(scroll_offset_ + 1) + "-" +
                                      std::to_string(std::min(scroll_offset_ + max_visible_items_,
                                                              current_display_files.size())) +
                                      " of " + std::to_string(current_display_files.size());
            dialog_content.push_back(text(""));
            dialog_content.push_back(
                hbox({text("  "), text(scroll_info) | color(Color::GrayDark) | dim}));
        }
    }

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 帮助信息
    dialog_content.push_back(
        hbox({text("  "), text("↑↓") | color(Color::Cyan) | bold, text(": Navigate  "),
              text("Space") | color(Color::Cyan) | bold, text(": Select  "),
              text("A") | color(Color::Cyan) | bold, text(": Select All")}));
    dialog_content.push_back(
        hbox({text("  "), text("/") | color(Color::Cyan) | bold, text(": Search  "),
              text("Enter") | color(Color::Cyan) | bold, text(": Format  "),
              text("Esc") | color(Color::Cyan) | bold, text(": Cancel/Exit Search")}));
    dialog_content.push_back(
        hbox({text("  "), text("PgUp/PgDn") | color(Color::Cyan) | bold, text(": Scroll  "),
              text("Backspace") | color(Color::Cyan) | bold, text(": Clear Search")}));

    // 计算对话框高度
    int height = std::min(25, int(12 + static_cast<int>(files_.size())));
    height = std::max(height, 15); // 最小高度

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 80) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

void FormatDialog::setOnConfirm(std::function<void(const std::vector<std::string>&)> callback) {
    on_confirm_ = callback;
}

void FormatDialog::setOnCancel(std::function<void()> callback) {
    on_cancel_ = callback;
}

const std::vector<std::string>& FormatDialog::getFiles() const {
    return files_;
}

std::vector<std::string> FormatDialog::getSelectedFiles() const {
    std::vector<std::string> result;
    for (size_t index : selected_files_) {
        if (index < files_.size()) {
            result.push_back(files_[index]);
        }
    }
    return result;
}

std::vector<std::string> FormatDialog::getFilteredFiles() const {
    if (search_query_.empty()) {
        return files_;
    }

    std::vector<std::string> filtered;
    std::string lower_query = search_query_;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    for (const auto& file : files_) {
        std::string filename = getFileName(file);
        std::string lower_filename = filename;
        std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(),
                       ::tolower);

        if (lower_filename.find(lower_query) != std::string::npos) {
            filtered.push_back(file);
        }
    }

    return filtered;
}

std::string FormatDialog::getFileName(const std::string& file_path) const {
    size_t last_slash = file_path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return file_path.substr(last_slash + 1);
    }
    return file_path;
}

std::string FormatDialog::getFileExtension(const std::string& filename) const {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == 0) {
        return "";
    }
    std::string ext = filename.substr(dot_pos + 1);
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

} // namespace ui
} // namespace pnana
