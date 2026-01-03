// 光标移动相关实现
#include "core/editor.h"
#include "utils/logger.h"
#include <algorithm>

namespace pnana {
namespace core {

// 光标移动
void Editor::moveCursorUp() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    if (cursor_row_ > 0) {
        cursor_row_--;
        adjustCursor();
        // 立即调整视图偏移，使光标移动更流畅
        adjustViewOffset();
    }
}

void Editor::moveCursorDown() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
        cursor_row_++;
        adjustCursor();
        // 立即调整视图偏移，使光标移动更流畅
        adjustViewOffset();
    }
}

void Editor::moveCursorLeft() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    if (cursor_col_ > 0) {
        cursor_col_--;
    } else if (cursor_row_ > 0) {
        cursor_row_--;
        cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
        adjustCursor();
        // 跨行移动时调整视图
        adjustViewOffset();
    }
}

void Editor::moveCursorRight() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    size_t line_len = getCurrentDocument()->getLine(cursor_row_).length();
    if (cursor_col_ < line_len) {
        cursor_col_++;
    } else if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
        cursor_row_++;
        cursor_col_ = 0;
        adjustCursor();
        // 跨行移动时调整视图
        adjustViewOffset();
    }
}

void Editor::moveCursorPageUp() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    // 统一计算屏幕高度：减去标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1) =
    // 6行
    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1; // 防止除零错误
    }

    size_t total_lines = doc->lineCount();
    if (total_lines == 0)
        return;

    // 计算当前光标在可见区域中的位置
    size_t cursor_visible_row =
        (cursor_row_ >= view_offset_row_) ? (cursor_row_ - view_offset_row_) : 0;

    // 向上滚动一页：视图向上移动一页
    size_t old_view_offset = view_offset_row_;
    if (view_offset_row_ >= static_cast<size_t>(screen_height)) {
        view_offset_row_ -= screen_height;
    } else {
        view_offset_row_ = 0;
    }

    // 如果视图已经到达顶部，将光标移到文件开头（保持列位置）
    if (view_offset_row_ == 0 && old_view_offset == 0) {
        cursor_row_ = 0;
        // 保持列位置，但确保不超过行长度
        size_t line_len = doc->getLine(0).length();
        if (cursor_col_ > line_len) {
            cursor_col_ = line_len;
        }
    } else {
        // 保持光标在屏幕中的相对位置，但确保在新视图范围内
        // 如果光标原本在屏幕上半部分，移到新视图顶部；否则保持相对位置
        if (cursor_visible_row < static_cast<size_t>(screen_height / 2)) {
            // 光标在屏幕上半部分，移到新视图顶部
            cursor_row_ = view_offset_row_;
        } else {
            // 光标在屏幕下半部分，保持相对位置
            cursor_row_ = view_offset_row_ + cursor_visible_row;
        }

        // 确保光标在有效范围内
        if (cursor_row_ >= total_lines) {
            cursor_row_ = total_lines - 1;
        }
    }

    adjustCursor();
    // 确保视图偏移正确（处理边界情况）
    adjustViewOffset();
}

void Editor::moveCursorPageDown() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    // 统一计算屏幕高度：减去标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1) =
    // 6行
    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1; // 防止除零错误
    }

    size_t total_lines = doc->lineCount();
    if (total_lines == 0)
        return;

    // 计算当前光标在可见区域中的位置
    size_t cursor_visible_row =
        (cursor_row_ >= view_offset_row_) ? (cursor_row_ - view_offset_row_) : 0;

    // 向下滚动一页：视图向下移动一页
    size_t max_offset =
        (total_lines > static_cast<size_t>(screen_height)) ? (total_lines - screen_height) : 0;
    size_t old_view_offset = view_offset_row_;
    if (view_offset_row_ + static_cast<size_t>(screen_height) <= max_offset) {
        view_offset_row_ += screen_height;
    } else {
        view_offset_row_ = max_offset;
    }

    // 如果视图已经到达底部，将光标移到文件末尾（保持列位置，但确保不超过行长度）
    if (view_offset_row_ == max_offset && old_view_offset == max_offset && max_offset > 0) {
        cursor_row_ = total_lines - 1;
        size_t line_len = doc->getLine(cursor_row_).length();
        if (cursor_col_ > line_len) {
            cursor_col_ = line_len;
        }
    } else {
        // 保持光标在屏幕中的相对位置，但确保在新视图范围内
        // 如果光标原本在屏幕下半部分，移到新视图底部；否则保持相对位置
        if (cursor_visible_row >= static_cast<size_t>(screen_height / 2)) {
            // 光标在屏幕下半部分，移到新视图底部（保留一些边距）
            size_t target_visible_row = static_cast<size_t>(screen_height) - 1;
            // 确保不超过屏幕高度（虽然理论上不会发生，但为了安全）
            if (target_visible_row >= static_cast<size_t>(screen_height)) {
                target_visible_row = static_cast<size_t>(screen_height) - 1;
            }
            cursor_row_ = view_offset_row_ + target_visible_row;
        } else {
            // 光标在屏幕上半部分，保持相对位置
            cursor_row_ = view_offset_row_ + cursor_visible_row;
        }

        // 确保光标在有效范围内
        if (cursor_row_ >= total_lines) {
            cursor_row_ = total_lines - 1;
        }
    }

    adjustCursor();
    // 确保视图偏移正确（处理边界情况）
    adjustViewOffset();
}

void Editor::moveCursorLineStart() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    cursor_col_ = 0;
    // 行首/行尾移动时也检查视图，确保光标可见
    adjustViewOffset();
}

void Editor::moveCursorLineEnd() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
    // 行首/行尾移动时也检查视图，确保光标可见
    adjustViewOffset();
}

void Editor::moveCursorFileStart() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    cursor_row_ = 0;
    cursor_col_ = 0;
    adjustViewOffset();
}

void Editor::moveCursorFileEnd() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    cursor_row_ = getCurrentDocument()->lineCount() - 1;
    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
    adjustViewOffset();
}

void Editor::moveCursorWordForward() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    const std::string& line = getCurrentDocument()->getLine(cursor_row_);
    size_t old_row = cursor_row_;
    if (cursor_col_ >= line.length()) {
        moveCursorRight();
        return;
    }

    // 跳过当前单词
    while (cursor_col_ < line.length() && std::isalnum(line[cursor_col_])) {
        cursor_col_++;
    }
    // 跳过空白
    while (cursor_col_ < line.length() && std::isspace(line[cursor_col_])) {
        cursor_col_++;
    }

    // 如果跨行了，调整视图
    if (cursor_row_ != old_row) {
        adjustViewOffset();
    }
}

void Editor::moveCursorWordBackward() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    size_t old_row = cursor_row_;
    if (cursor_col_ == 0) {
        moveCursorLeft();
        return;
    }

    const std::string& line = getCurrentDocument()->getLine(cursor_row_);
    cursor_col_--;

    // 跳过空白
    while (cursor_col_ > 0 && std::isspace(line[cursor_col_])) {
        cursor_col_--;
    }
    // 跳到单词开头
    while (cursor_col_ > 0 && std::isalnum(line[cursor_col_ - 1])) {
        cursor_col_--;
    }

    // 如果跨行了，调整视图
    if (cursor_row_ != old_row) {
        adjustViewOffset();
    }
}

// 跳转
void Editor::gotoLine(size_t line) {
    if (line > 0 && line <= getCurrentDocument()->lineCount()) {
        cursor_row_ = line - 1;
        cursor_col_ = 0;
        adjustViewOffset();
        setStatusMessage("Jumped to line " + std::to_string(line));
    }
}

void Editor::startGotoLineMode() {
    LOG("=== startGotoLineMode() called ===");

    // 检查是否有当前文档
    if (getCurrentDocument() == nullptr) {
        setStatusMessage("No document open");
        LOG("No document open, cannot goto line");
        return;
    }

    // 获取当前文档的总行数
    size_t total_lines = getCurrentDocument()->lineCount();
    size_t current_line = cursor_row_ + 1; // 转换为1-based行号

    // 显示跳转行号对话框
    dialog_.showInput(
        "Go to Line",
        "Enter line number (1-" + std::to_string(total_lines) + "):", std::to_string(current_line),
        [this, total_lines](const std::string& line_str) {
            // 解析行号
            try {
                size_t line = std::stoull(line_str);
                if (line > 0 && line <= total_lines) {
                    gotoLine(line);
                    setStatusMessage("Jumped to line " + std::to_string(line));
                } else {
                    setStatusMessage("Line number out of range (1-" + std::to_string(total_lines) +
                                     ")");
                }
            } catch (const std::exception& e) {
                setStatusMessage("Invalid line number");
            }
        },
        [this]() {
            setStatusMessage("Goto line cancelled");
        });

    LOG("Goto line dialog shown");
    LOG("=== startGotoLineMode() completed ===");
}

// 辅助方法
void Editor::adjustCursor() {
    if (cursor_row_ >= getCurrentDocument()->lineCount()) {
        cursor_row_ = getCurrentDocument()->lineCount() - 1;
    }

    size_t line_len = getCurrentDocument()->getLine(cursor_row_).length();
    if (cursor_col_ > line_len) {
        cursor_col_ = line_len;
    }
}

void Editor::adjustViewOffset() {
    // 统一计算屏幕高度：减去标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1) =
    // 6行
    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1; // 防止除零错误
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    size_t total_lines = doc->lineCount();
    if (total_lines == 0) {
        view_offset_row_ = 0;
        return;
    }

    // 类似 neovim 的 scrolloff 功能：保持光标上下各保留一定行数可见
    // 这样可以避免光标紧贴屏幕边缘，提供更好的视觉体验
    const int scrolloff = 3; // 光标上下各保留3行可见

    // 计算可见区域
    size_t visible_start = view_offset_row_;
    size_t visible_end = view_offset_row_ + screen_height;

    // 计算光标在可见区域中的位置
    size_t cursor_visible_row = cursor_row_ - view_offset_row_;

    // 如果光标超出可见区域，立即滚动
    if (cursor_row_ >= visible_end) {
        // 光标在可见区域下方，滚动使光标可见
        view_offset_row_ = cursor_row_ - screen_height + 1;
    } else if (cursor_row_ < visible_start) {
        // 光标在可见区域上方，滚动使光标可见
        view_offset_row_ = cursor_row_;
    }
    // 如果光标在可见区域内，检查是否需要滚动以保持 scrolloff
    else {
        // 检查光标是否接近上边缘（距离上边缘小于 scrolloff）
        if (cursor_visible_row < static_cast<size_t>(scrolloff)) {
            // 向上滚动，使光标上方有 scrolloff 行可见
            if (view_offset_row_ > 0) {
                size_t target_offset =
                    cursor_row_ > static_cast<size_t>(scrolloff) ? cursor_row_ - scrolloff : 0;
                if (target_offset < view_offset_row_) {
                    view_offset_row_ = target_offset;
                }
            }
        }
        // 检查光标是否接近下边缘（距离下边缘小于 scrolloff）
        else if (cursor_visible_row >= static_cast<size_t>(screen_height - scrolloff)) {
            // 向下滚动，使光标下方有 scrolloff 行可见
            size_t max_offset =
                total_lines > static_cast<size_t>(screen_height) ? total_lines - screen_height : 0;
            size_t target_offset = cursor_row_ + scrolloff + 1;
            if (target_offset > static_cast<size_t>(screen_height) &&
                target_offset <= total_lines) {
                target_offset = target_offset - screen_height;
                if (target_offset > max_offset) {
                    target_offset = max_offset;
                }
                if (target_offset > view_offset_row_) {
                    view_offset_row_ = target_offset;
                }
            }
        }
    }

    // 确保视图偏移在有效范围内
    if (total_lines <= static_cast<size_t>(screen_height)) {
        view_offset_row_ = 0;
    } else {
        size_t max_offset = total_lines - screen_height;
        if (view_offset_row_ > max_offset) {
            view_offset_row_ = max_offset;
        }
    }

    // 确保光标列位置有效
    size_t line_len = doc->getLine(cursor_row_).length();
    if (cursor_col_ > line_len) {
        cursor_col_ = line_len;
    }
}

void Editor::pageUp() {
    if (file_browser_.isVisible()) {
        file_browser_.selectPageUp();
    }
}

void Editor::pageDown() {
    if (file_browser_.isVisible()) {
        file_browser_.selectPageDown();
    }
}

} // namespace core
} // namespace pnana
