#include "ui/history_diff_popup.h"
#include "ui/icons.h"
#include <algorithm>
#include <filesystem>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace {
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}
} // namespace

namespace pnana {
namespace ui {

HistoryDiffPopup::HistoryDiffPopup(Theme& theme) : theme_(theme) {}

void HistoryDiffPopup::open(const std::string& file_path, int from_version, int to_version,
                            const std::vector<pnana::features::diff::DiffRecord>& records) {
    file_path_ = file_path;
    from_version_ = from_version;
    to_version_ = to_version;
    records_ = records;
    scroll_offset_ = 0;
    horizontal_offset_ = 0;
    is_open_ = true;
}

void HistoryDiffPopup::close() {
    is_open_ = false;
    records_.clear();
    scroll_offset_ = 0;
    horizontal_offset_ = 0;
}

Element HistoryDiffPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    std::string filename = file_path_;
    try {
        filename = std::filesystem::path(file_path_).filename().string();
    } catch (...) {
    }

    return hbox({text(" "), text(pnana::ui::icons::GIT_DIFF) | color(colors.success),
                 text(" History Diff - " + filename + "  v" + std::to_string(from_version_) +
                      " -> v" + std::to_string(to_version_) + " "),
                 text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element HistoryDiffPopup::renderList() const {
    const auto& colors = theme_.getColors();
    Elements rows;

    if (records_.empty()) {
        rows.push_back(text("  No diff records") | color(colors.comment) | dim);
        return vbox(std::move(rows));
    }

    size_t total = records_.size();
    size_t start = std::min(scroll_offset_, total);
    size_t end = std::min(total, start + page_size_);

    int max_line_num = 0;
    for (size_t i = start; i < end; ++i) {
        max_line_num = std::max(max_line_num, records_[i].line_num);
    }
    // 参考 FZF 预览区：固定宽度右对齐，视觉更稳定
    const size_t LINE_NUM_WIDTH =
        std::max<size_t>(4, std::to_string(std::max(1, max_line_num)).size());

    for (size_t i = start; i < end; ++i) {
        const auto& r = records_[i];
        std::string mark = " ";
        Decorator mark_color = color(colors.comment);
        Decorator text_color = color(colors.foreground) | bold;

        if (r.op == pnana::features::diff::DiffRecord::OpType::ADD) {
            mark = "+";
            mark_color = color(colors.success) | bold;
            text_color = color(colors.success) | bold;
        } else if (r.op == pnana::features::diff::DiffRecord::OpType::DELETE) {
            mark = "-";
            mark_color = color(colors.error) | bold;
            text_color = color(colors.error) | bold;
        } else {
            mark = " ";
            mark_color = color(colors.comment);
            text_color = color(colors.foreground);
        }

        std::string line_num = std::to_string(r.line_num);
        while (line_num.length() < LINE_NUM_WIDTH) {
            line_num = " " + line_num;
        }

        // 与 FZF 预览区一致：" 空格 + 右对齐行号 + 空格 " 的单元格式
        Element line_num_cell = text(" " + line_num + " ") | color(colors.comment) | dim;

        std::string visible_content = r.content;
        if (horizontal_offset_ < visible_content.size()) {
            visible_content = visible_content.substr(horizontal_offset_);
        } else {
            visible_content.clear();
        }

        rows.push_back(hbox({text("  "), text(mark + " ") | mark_color, line_num_cell,
                             text(visible_content) | text_color}));
    }

    return vbox(std::move(rows));
}

Element HistoryDiffPopup::renderHelp() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Scroll  "),
                 text("PgUp/PgDn") | color(colors.helpbar_key) | bold, text(": Page  "),
                 text("Tab") | color(colors.helpbar_key) | bold, text(": Horizontal  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Back to History")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

Element HistoryDiffPopup::render() const {
    if (!is_open_) {
        return text("");
    }

    const auto& colors = theme_.getColors();
    Elements content;
    content.push_back(renderTitle());
    content.push_back(separator());
    content.push_back(renderList());
    content.push_back(separator());
    content.push_back(renderHelp());

    return window(text(""), vbox(std::move(content))) | size(WIDTH, EQUAL, 110) |
           size(HEIGHT, EQUAL, 26) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

bool HistoryDiffPopup::handleInput(ftxui::Event event) {
    if (!is_open_) {
        return false;
    }

    if (event == Event::Escape) {
        close();
        return true;
    }

    if (records_.empty()) {
        return true;
    }

    if (event == Event::ArrowDown) {
        if (scroll_offset_ + 1 < records_.size()) {
            scroll_offset_++;
        }
        return true;
    }

    if (event == Event::ArrowUp) {
        if (scroll_offset_ > 0) {
            scroll_offset_--;
        }
        return true;
    }

    if (event == Event::PageDown) {
        size_t max_offset = records_.size() > page_size_ ? records_.size() - page_size_ : 0;
        scroll_offset_ = std::min(max_offset, scroll_offset_ + page_size_);
        return true;
    }

    if (event == Event::PageUp) {
        if (scroll_offset_ > page_size_) {
            scroll_offset_ -= page_size_;
        } else {
            scroll_offset_ = 0;
        }
        return true;
    }

    if (event == Event::Tab) {
        size_t max_len = 0;
        size_t total = records_.size();
        size_t start = std::min(scroll_offset_, total);
        size_t end = std::min(total, start + page_size_);
        for (size_t i = start; i < end; ++i) {
            max_len = std::max(max_len, records_[i].content.size());
        }

        if (max_len <= horizontal_step_) {
            horizontal_offset_ = 0;
            return true;
        }

        size_t max_offset = max_len - horizontal_step_;
        if (horizontal_offset_ + horizontal_step_ > max_offset) {
            horizontal_offset_ = 0; // 到边界后循环回起点
        } else {
            horizontal_offset_ += horizontal_step_;
        }
        return true;
    }

    return false;
}

} // namespace ui
} // namespace pnana
