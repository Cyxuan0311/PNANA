// 光标移动相关实现
#include "core/editor.h"
#include <algorithm>

namespace pnana {
namespace core {

// 光标移动
void Editor::moveCursorUp() {
    if (cursor_row_ > 0) {
        cursor_row_--;
        adjustCursor();
        // 立即调整视图偏移，使光标移动更流畅
        adjustViewOffset();
    }
}

void Editor::moveCursorDown() {
    if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
        cursor_row_++;
        adjustCursor();
        // 立即调整视图偏移，使光标移动更流畅
        adjustViewOffset();
    }
}

void Editor::moveCursorLeft() {
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
    int page_size = screen_.dimy() - 6;  // 统一计算屏幕高度
    if (cursor_row_ > static_cast<size_t>(page_size)) {
        cursor_row_ -= page_size;
    } else {
        cursor_row_ = 0;
    }
    adjustCursor();
    // 翻页后立即调整视图
    adjustViewOffset();
}

void Editor::moveCursorPageDown() {
    int page_size = screen_.dimy() - 6;  // 统一计算屏幕高度
    cursor_row_ = std::min(cursor_row_ + page_size, getCurrentDocument()->lineCount() - 1);
    adjustCursor();
    // 翻页后立即调整视图
    adjustViewOffset();
}

void Editor::moveCursorLineStart() {
    cursor_col_ = 0;
    // 行首/行尾移动时也检查视图，确保光标可见
    adjustViewOffset();
}

void Editor::moveCursorLineEnd() {
    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
    // 行首/行尾移动时也检查视图，确保光标可见
    adjustViewOffset();
}

void Editor::moveCursorFileStart() {
    cursor_row_ = 0;
    cursor_col_ = 0;
    adjustViewOffset();
}

void Editor::moveCursorFileEnd() {
    cursor_row_ = getCurrentDocument()->lineCount() - 1;
    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
    adjustViewOffset();
}

void Editor::moveCursorWordForward() {
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
    mode_ = EditorMode::GOTO_LINE;
    input_buffer_ = "";
    setStatusMessage("Go to line: ");
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
    // 统一计算屏幕高度：减去标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1) = 6行
    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1;  // 防止除零错误
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
    const int scrolloff = 3;  // 光标上下各保留3行可见
    
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
                size_t target_offset = cursor_row_ > static_cast<size_t>(scrolloff) 
                    ? cursor_row_ - scrolloff 
                    : 0;
                if (target_offset < view_offset_row_) {
                    view_offset_row_ = target_offset;
                }
            }
        }
        // 检查光标是否接近下边缘（距离下边缘小于 scrolloff）
        else if (cursor_visible_row >= static_cast<size_t>(screen_height - scrolloff)) {
            // 向下滚动，使光标下方有 scrolloff 行可见
            size_t max_offset = total_lines > static_cast<size_t>(screen_height) 
                ? total_lines - screen_height 
                : 0;
            size_t target_offset = cursor_row_ + scrolloff + 1;
            if (target_offset > static_cast<size_t>(screen_height) && target_offset <= total_lines) {
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

} // namespace core
} // namespace pnana

