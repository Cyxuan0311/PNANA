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
    }
}

void Editor::moveCursorDown() {
    if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
        cursor_row_++;
        adjustCursor();
    }
}

void Editor::moveCursorLeft() {
    if (cursor_col_ > 0) {
        cursor_col_--;
    } else if (cursor_row_ > 0) {
        cursor_row_--;
        cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
    }
}

void Editor::moveCursorRight() {
    size_t line_len = getCurrentDocument()->getLine(cursor_row_).length();
    if (cursor_col_ < line_len) {
        cursor_col_++;
    } else if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
        cursor_row_++;
        cursor_col_ = 0;
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
}

void Editor::moveCursorPageDown() {
    int page_size = screen_.dimy() - 6;  // 统一计算屏幕高度
    cursor_row_ = std::min(cursor_row_ + page_size, getCurrentDocument()->lineCount() - 1);
    adjustCursor();
}

void Editor::moveCursorLineStart() {
    cursor_col_ = 0;
}

void Editor::moveCursorLineEnd() {
    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
}

void Editor::moveCursorFileStart() {
    cursor_row_ = 0;
    cursor_col_ = 0;
}

void Editor::moveCursorFileEnd() {
    cursor_row_ = getCurrentDocument()->lineCount() - 1;
    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
}

void Editor::moveCursorWordForward() {
    const std::string& line = getCurrentDocument()->getLine(cursor_row_);
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
}

void Editor::moveCursorWordBackward() {
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
    
    // 只在光标超出可见范围时才调整视图，减少不必要的刷新
    if (cursor_row_ >= view_offset_row_ + screen_height) {
        view_offset_row_ = cursor_row_ - screen_height + 1;
    } else if (cursor_row_ < view_offset_row_) {
        view_offset_row_ = cursor_row_;
    }
    
    // 如果文件行数少于屏幕高度，确保从0开始显示（这样最后一行也能显示）
    Document* doc = getCurrentDocument();
    if (doc) {
        size_t total_lines = doc->lineCount();
        if (total_lines > 0 && total_lines <= static_cast<size_t>(screen_height)) {
            view_offset_row_ = 0;
        }
        // 如果文件行数大于屏幕高度，不强制调整视图偏移，让用户自己滚动
    }
    // 如果光标在可见范围内，不调整视图偏移，避免刷新
}

} // namespace core
} // namespace pnana

