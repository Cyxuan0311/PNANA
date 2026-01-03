#include "ui/split_dialog.h"
#include "ui/icons.h"

namespace pnana {
namespace ui {

SplitDialog::SplitDialog(Theme& theme)
    : theme_(theme), visible_(false), mode_(DialogMode::CREATE), selected_index_(0) {}

void SplitDialog::showCreate(std::function<void(features::SplitDirection)> on_select,
                             std::function<void()> on_cancel) {
    visible_ = true;
    mode_ = DialogMode::CREATE;
    selected_index_ = 0;
    on_create_select_ = on_select;
    on_create_cancel_ = on_cancel;
}

void SplitDialog::showClose(const std::vector<SplitInfo>& splits,
                            std::function<void(size_t)> on_close, std::function<void()> on_cancel) {
    visible_ = true;
    mode_ = DialogMode::CLOSE;
    selected_index_ = 0;
    splits_ = splits;
    on_close_ = on_close;
    on_close_cancel_ = on_cancel;
}

bool SplitDialog::handleInput(ftxui::Event event) {
    if (!visible_) {
        return false;
    }

    if (mode_ == DialogMode::CREATE) {
        // 创建分屏模式
        if (event == ftxui::Event::Escape) {
            if (on_create_cancel_) {
                on_create_cancel_();
            }
            visible_ = false;
            return true;
        } else if (event == ftxui::Event::Return) {
            if (on_create_select_) {
                features::SplitDirection direction = (selected_index_ == 0)
                                                         ? features::SplitDirection::VERTICAL
                                                         : features::SplitDirection::HORIZONTAL;
                on_create_select_(direction);
            }
            visible_ = false;
            return true;
        } else if (event == ftxui::Event::ArrowUp) {
            if (selected_index_ > 0) {
                selected_index_--;
            }
            return true;
        } else if (event == ftxui::Event::ArrowDown) {
            if (selected_index_ < 1) {
                selected_index_++;
            }
            return true;
        }
    } else {
        // 关闭分屏模式
        if (splits_.empty()) {
            return false;
        }

        if (event == ftxui::Event::Escape) {
            if (on_close_cancel_) {
                on_close_cancel_();
            }
            visible_ = false;
            return true;
        } else if (event == ftxui::Event::Delete || event == ftxui::Event::Character('d')) {
            // Delete 键或 'd' 键关闭选中的分屏
            if (selected_index_ < splits_.size()) {
                if (on_close_) {
                    on_close_(splits_[selected_index_].region_index);
                }
                visible_ = false;
            }
            return true;
        } else if (event == ftxui::Event::ArrowUp) {
            if (selected_index_ > 0) {
                selected_index_--;
            }
            return true;
        } else if (event == ftxui::Event::ArrowDown) {
            if (selected_index_ < splits_.size() - 1) {
                selected_index_++;
            }
            return true;
        }
    }

    return false;
}

ftxui::Element SplitDialog::render() {
    if (!visible_) {
        return ftxui::text("");
    }

    using namespace ftxui;
    auto& colors = theme_.getColors();

    Elements content;

    if (mode_ == DialogMode::CREATE) {
        // 创建分屏模式
        content.push_back(
            hbox({text(" "), text("⚡") | color(Color::Yellow), text(" Split View "), text(" ")}) |
            bold | bgcolor(colors.menubar_bg) | center);

        content.push_back(separator());
        content.push_back(text(""));

        // 选项
        std::vector<std::string> options = {"Vertical Split (│)", "Horizontal Split (─)"};

        std::vector<std::string> descriptions = {"Split window vertically (left/right)",
                                                 "Split window horizontally (top/bottom)"};

        for (size_t i = 0; i < options.size(); ++i) {
            Elements row;
            row.push_back(text("  "));

            // 选中标记
            if (i == selected_index_) {
                row.push_back(text("► ") | color(Color::GreenLight) | bold);
            } else {
                row.push_back(text("  "));
            }

            // 选项文本
            row.push_back(text(options[i]) | (i == selected_index_ ? color(colors.foreground) | bold
                                                                   : color(colors.comment)));

            row.push_back(filler());
            row.push_back(text(descriptions[i]) | color(colors.comment) | dim);

            Element row_elem = hbox(row);
            if (i == selected_index_) {
                row_elem = row_elem | bgcolor(colors.selection);
            }

            content.push_back(row_elem);
        }

        content.push_back(text(""));
        content.push_back(separator());

        // 提示
        content.push_back(
            hbox({text("  "), text("↑↓") | color(colors.keyword) | bold, text(": Navigate  "),
                  text("Enter") | color(colors.keyword) | bold, text(": Select  "),
                  text("Esc") | color(colors.keyword) | bold, text(": Cancel")}) |
            dim);

        return window(text(""), vbox(content)) | size(WIDTH, EQUAL, 60) | size(HEIGHT, EQUAL, 12) |
               bgcolor(colors.background) | border | center;
    } else {
        // 关闭分屏模式
        content.push_back(hbox({text(" "), text("✕") | color(Color::Red),
                                text(" Close Split View "), text(" ")}) |
                          bold | bgcolor(colors.menubar_bg) | center);

        content.push_back(separator());
        content.push_back(text(""));

        if (splits_.empty()) {
            content.push_back(
                hbox({text("  "), text("No splits to close") | color(colors.comment) | dim}));
        } else {
            // 显示所有分屏
            for (size_t i = 0; i < splits_.size(); ++i) {
                const auto& split = splits_[i];
                Elements row;
                row.push_back(text("  "));

                // 选中标记
                if (i == selected_index_) {
                    row.push_back(text("► ") | color(Color::Red) | bold);
                } else {
                    row.push_back(text("  "));
                }

                // 区域信息
                std::string region_text = "Region " + std::to_string(i + 1);
                if (split.is_active) {
                    region_text += " [Active]";
                }

                row.push_back(text(region_text) |
                              (i == selected_index_ ? color(colors.foreground) | bold
                                                    : color(colors.comment)));

                row.push_back(filler());

                // 文档名称和状态
                std::string doc_info = split.document_name;
                if (split.is_modified) {
                    doc_info += " [Modified]";
                }

                Color info_color = split.is_modified ? Color::Yellow : colors.comment;
                row.push_back(text(doc_info) | color(info_color) | dim);

                Element row_elem = hbox(row);
                if (i == selected_index_) {
                    row_elem = row_elem | bgcolor(colors.selection);
                }

                content.push_back(row_elem);
            }
        }

        content.push_back(text(""));
        content.push_back(separator());

        // 提示
        content.push_back(
            hbox({text("  "), text("↑↓") | color(colors.keyword) | bold, text(": Navigate  "),
                  text("Delete") | color(colors.keyword) | bold, text(": Close  "),
                  text("Esc") | color(colors.keyword) | bold, text(": Cancel")}) |
            dim);

        int height = std::min(20, int(10 + splits_.size()));
        return window(text(""), vbox(content)) | size(WIDTH, EQUAL, 70) |
               size(HEIGHT, EQUAL, height) | bgcolor(colors.background) | border | center;
    }
}

void SplitDialog::reset() {
    visible_ = false;
    mode_ = DialogMode::CREATE;
    selected_index_ = 0;
    on_create_select_ = nullptr;
    on_create_cancel_ = nullptr;
    on_close_ = nullptr;
    on_close_cancel_ = nullptr;
    splits_.clear();
}

} // namespace ui
} // namespace pnana
