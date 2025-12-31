// 编辑操作相关实现
#include "core/editor.h"
#include "core/document.h"
#include "utils/logger.h"
#include <iostream>

namespace pnana {
namespace core {

// 编辑操作
void Editor::insertChar(char ch) {
    getCurrentDocument()->insertChar(cursor_row_, cursor_col_, ch);
    cursor_col_++;
    
#ifdef BUILD_LSP_SUPPORT
    // 更新 LSP 文档
    updateLspDocument();
    
    // 触发代码补全（在输入字母、数字、下划线或点号时）
    // 使用防抖机制，提升编辑流畅度
    if (lsp_enabled_ && lsp_manager_) {
        if (std::isalnum(ch) || ch == '_' || ch == '.' || ch == ':' || ch == '-' || ch == '>') {
            // 使用延迟触发，避免每次输入都立即请求（提升流畅度）
            completion_trigger_delay_++;
            // 输入3个字符后触发，进一步提升流畅度
            if (completion_trigger_delay_ >= 3) {
                completion_trigger_delay_ = 0;
                triggerCompletion();
            }
        } else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '(' || ch == '[' || ch == '{') {
            // 空格、制表符、换行、括号时隐藏补全弹窗
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
        } else {
            // 其他字符，隐藏补全弹窗并重置延迟
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
        }
    }
#endif
}

void Editor::insertNewline() {
    Document* doc = getCurrentDocument();
    if (!doc) return;
    
    std::string current_line = doc->getLine(cursor_row_);
    std::string before_cursor = current_line.substr(0, cursor_col_);
    std::string after_cursor = current_line.substr(cursor_col_);
    
    // 先执行换行操作
    doc->getLines()[cursor_row_] = before_cursor;
    doc->getLines().insert(doc->getLines().begin() + cursor_row_ + 1, after_cursor);
    
    // 记录换行操作到撤销栈（作为一个完整的操作）
    doc->pushChange(DocumentChange(
        DocumentChange::Type::NEWLINE,
        cursor_row_, cursor_col_,
        current_line,  // old_content: 完整的原行
        before_cursor, // new_content: 光标前的内容
        after_cursor   // after_cursor: 光标后的内容（会移到新行）
    ));
    
    cursor_row_++;
    cursor_col_ = 0;
    
#ifdef BUILD_LSP_SUPPORT
    // 更新 LSP 文档
    updateLspDocument();
    // 换行时隐藏补全弹窗
    completion_popup_.hide();
#endif
    
    // 调整视图偏移，确保新插入的行可见
    adjustViewOffset();
}

void Editor::deleteChar() {
    getCurrentDocument()->deleteChar(cursor_row_, cursor_col_);
}

void Editor::backspace() {
    if (cursor_col_ > 0) {
        cursor_col_--;
        getCurrentDocument()->deleteChar(cursor_row_, cursor_col_);
    } else if (cursor_row_ > 0) {
        size_t prev_len = getCurrentDocument()->getLine(cursor_row_ - 1).length();
        // 合并行
        getCurrentDocument()->getLines()[cursor_row_ - 1] += getCurrentDocument()->getLine(cursor_row_);
        getCurrentDocument()->deleteLine(cursor_row_);
        cursor_row_--;
        cursor_col_ = prev_len;
    }
    
#ifdef BUILD_LSP_SUPPORT
    // 更新 LSP 文档
    updateLspDocument();
    // 删除时隐藏补全弹窗
    completion_popup_.hide();
#endif
}

void Editor::deleteLine() {
    getCurrentDocument()->deleteLine(cursor_row_);
    adjustCursor();
    setStatusMessage("Line deleted");
}

void Editor::deleteWord() {
    const std::string& line = getCurrentDocument()->getLine(cursor_row_);
    size_t start = cursor_col_;
    
    // 找到单词结尾
    while (cursor_col_ < line.length() && std::isalnum(line[cursor_col_])) {
        cursor_col_++;
    }
    
    // 删除范围内的字符
    for (size_t i = start; i < cursor_col_; ++i) {
        getCurrentDocument()->deleteChar(cursor_row_, start);
    }
    
    cursor_col_ = start;
    getCurrentDocument()->setModified(true);
}

void Editor::duplicateLine() {
    std::string line = getCurrentDocument()->getLine(cursor_row_);
    getCurrentDocument()->insertLine(cursor_row_ + 1);
    getCurrentDocument()->getLines()[cursor_row_ + 1] = line;
    getCurrentDocument()->setModified(true);
    setStatusMessage("Line duplicated");
}

// 选择操作
void Editor::startSelection() {
    selection_active_ = true;
    selection_start_row_ = cursor_row_;
    selection_start_col_ = cursor_col_;
}

void Editor::endSelection() {
    selection_active_ = false;
}

void Editor::selectAll() {
    selection_active_ = true;
    selection_start_row_ = 0;
    selection_start_col_ = 0;
    cursor_row_ = getCurrentDocument()->lineCount() - 1;
    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
    setStatusMessage("All selected");
}

void Editor::selectLine() {
    selection_active_ = true;
    selection_start_row_ = cursor_row_;
    selection_start_col_ = 0;
    cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
    setStatusMessage("Line selected");
}

void Editor::selectWord() {
    const std::string& line = getCurrentDocument()->getLine(cursor_row_);
    if (cursor_col_ >= line.length()) return;
    
    // 找到单词开始
    size_t start = cursor_col_;
    while (start > 0 && std::isalnum(line[start - 1])) {
        start--;
    }
    
    // 找到单词结束
    size_t end = cursor_col_;
    while (end < line.length() && std::isalnum(line[end])) {
        end++;
    }
    
    selection_active_ = true;
    selection_start_row_ = cursor_row_;
    selection_start_col_ = start;
    cursor_col_ = end;
    setStatusMessage("Word selected");
}

// 剪贴板操作
void Editor::cut() {
    if (!selection_active_) {
        // 剪切当前行
        std::string line = getCurrentDocument()->getLine(cursor_row_);
        getCurrentDocument()->setClipboard(line);
        deleteLine();
        setStatusMessage("Line cut");
    } else {
        copy();
        // 删除选中内容（简化实现）
        endSelection();
        setStatusMessage("Selection cut");
    }
}

void Editor::copy() {
    if (!selection_active_) {
        // 复制当前行
        std::string line = getCurrentDocument()->getLine(cursor_row_);
        getCurrentDocument()->setClipboard(line);
        setStatusMessage("Line copied");
    } else {
        std::string content = getCurrentDocument()->getSelection(
            selection_start_row_, selection_start_col_,
            cursor_row_, cursor_col_
        );
        getCurrentDocument()->setClipboard(content);
        endSelection();
        setStatusMessage("Selection copied");
    }
}

void Editor::paste() {
    std::string clipboard = getCurrentDocument()->getClipboard();
    if (clipboard.empty()) {
        setStatusMessage("Clipboard is empty");
        return;
    }
    
    // 简化实现：插入剪贴板内容
    getCurrentDocument()->insertText(cursor_row_, cursor_col_, clipboard);
    setStatusMessage("Pasted");
}

// 撤销/重做
void Editor::undo() {
    Document* doc = getCurrentDocument();
    if (!doc) return;
    
    size_t change_row, change_col;
    if (doc->undo(&change_row, &change_col)) {
        // 恢复光标位置到修改发生的位置
        cursor_row_ = change_row;
        cursor_col_ = change_col;
        
        // 确保光标位置有效
        adjustCursor();
        
        // 只在光标超出可见范围时才调整视图偏移，减少刷新
        int screen_height = screen_.dimy() - 4;
        if (cursor_row_ >= view_offset_row_ + screen_height || cursor_row_ < view_offset_row_) {
            adjustViewOffset();
        }
        
        // 使用简洁的状态消息，减少刷新
        setStatusMessage("Undone");
    } else {
        setStatusMessage("Nothing to undo");
    }
}

void Editor::redo() {
    Document* doc = getCurrentDocument();
    if (!doc) return;
    
    size_t change_row, change_col;
    if (doc->redo(&change_row, &change_col)) {
        // 恢复光标位置到修改发生的位置
        cursor_row_ = change_row;
        cursor_col_ = change_col;
        
        // 确保光标位置有效
        adjustCursor();
        
        // 只在光标超出可见范围时才调整视图偏移，减少刷新
        int screen_height = screen_.dimy() - 4;
        if (cursor_row_ >= view_offset_row_ + screen_height || cursor_row_ < view_offset_row_) {
            adjustViewOffset();
        }
        
        // 使用简洁的状态消息，减少刷新
        setStatusMessage("Redone");
    } else {
        setStatusMessage("Nothing to redo");
    }
}

void Editor::moveLineUp() {
    if (cursor_row_ == 0) return;
    
    auto& lines = getCurrentDocument()->getLines();
    std::swap(lines[cursor_row_], lines[cursor_row_ - 1]);
    cursor_row_--;
    getCurrentDocument()->setModified(true);
    setStatusMessage("Line moved up");
}

void Editor::moveLineDown() {
    auto& lines = getCurrentDocument()->getLines();
    if (cursor_row_ >= lines.size() - 1) return;
    
    std::swap(lines[cursor_row_], lines[cursor_row_ + 1]);
    cursor_row_++;
    getCurrentDocument()->setModified(true);
    setStatusMessage("Line moved down");
}

void Editor::indentLine() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }
    
    auto& lines = doc->getLines();
    if (cursor_row_ >= lines.size()) {
        return;
    }
    
    // Tab 键行为：在光标位置插入4个空格（如果光标在行首或行首空白处，则缩进整行）
    std::string& line = lines[cursor_row_];
    
    // 检查光标是否在行首或行首空白处
    size_t first_non_space = line.find_first_not_of(" \t");
    bool at_line_start = (cursor_col_ == 0) || 
                         (first_non_space != std::string::npos && cursor_col_ <= first_non_space);
    
    if (at_line_start) {
        // 在行首插入4个空格（缩进整行）
        line = "    " + line;
        cursor_col_ += 4;
    } else {
        // 在光标位置插入4个空格
        line.insert(cursor_col_, "    ");
        cursor_col_ += 4;
    }
    
    doc->setModified(true);
    
    getCurrentDocument()->setModified(true);
}

void Editor::unindentLine() {
    auto& lines = getCurrentDocument()->getLines();
    if (cursor_row_ >= lines.size()) return;
    
    std::string& line = lines[cursor_row_];
    // 移除前导空格（最多4个）
    size_t spaces_to_remove = 0;
    while (spaces_to_remove < 4 && spaces_to_remove < line.length() && line[spaces_to_remove] == ' ') {
        spaces_to_remove++;
    }
    
    if (spaces_to_remove > 0) {
        line = line.substr(spaces_to_remove);
        if (cursor_col_ >= spaces_to_remove) {
            cursor_col_ -= spaces_to_remove;
        } else {
            cursor_col_ = 0;
        }
        getCurrentDocument()->setModified(true);
    }
}

void Editor::toggleComment() {
    auto& lines = getCurrentDocument()->getLines();
    if (cursor_row_ >= lines.size()) return;
    
    std::string& line = lines[cursor_row_];
    std::string file_type = getFileType();
    
    // 根据文件类型选择注释符号
    std::string comment_prefix = "//";
    if (file_type == "python" || file_type == "shell") {
        comment_prefix = "#";
    } else if (file_type == "lua") {
        comment_prefix = "--";
    } else if (file_type == "html" || file_type == "xml") {
        comment_prefix = "<!--";
        // HTML注释需要特殊处理，这里简化处理
    }
    
    // 检查是否已注释
    size_t first_non_space = line.find_first_not_of(" \t");
    if (first_non_space != std::string::npos && 
        line.substr(first_non_space, comment_prefix.length()) == comment_prefix) {
        // 取消注释
        line.erase(first_non_space, comment_prefix.length());
        if (cursor_col_ >= first_non_space + comment_prefix.length()) {
            cursor_col_ -= comment_prefix.length();
        }
    } else {
        // 添加注释
        if (first_non_space == std::string::npos) {
            first_non_space = 0;
        }
        line.insert(first_non_space, comment_prefix + " ");
        cursor_col_ += comment_prefix.length() + 1;
    }
    
    getCurrentDocument()->setModified(true);
    setStatusMessage("Comment toggled");
}

} // namespace core
} // namespace pnana

