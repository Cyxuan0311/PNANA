// 编辑操作相关实现
#include "core/document.h"
#include "core/editor.h"
#include "utils/clipboard.h"
#include "utils/logger.h"
#include <iostream>
#include <sstream>

namespace pnana {
namespace core {

// 编辑操作
void Editor::insertChar(char ch) {
    LOG("[EDIT] insertChar called with char: '" + std::string(1, ch) + "' at pos (" +
        std::to_string(cursor_row_) + ", " + std::to_string(cursor_col_) + ")");

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[EDIT] No current document, returning");
        return;
    }

    // 记录变更（阶段2优化：增量更新）
#ifdef BUILD_LSP_SUPPORT
    if (lsp_enabled_ && document_change_tracker_) {
        LOG("[EDIT] Recording change to document_change_tracker_");
        std::string new_text(1, ch); // 新文本是插入的字符
        document_change_tracker_->recordInsert(static_cast<int>(cursor_row_),
                                               static_cast<int>(cursor_col_), new_text);
    }
#endif

    auto start_time = std::chrono::steady_clock::now();
    doc->insertChar(cursor_row_, cursor_col_, ch);
    cursor_col_++;
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    LOG("[EDIT] Document insertChar took " + std::to_string(duration.count()) + " us");

#ifdef BUILD_LSP_SUPPORT
    // 智能LSP文档更新策略
    // 只有在以下情况下才更新LSP文档：
    // 1. 触发completion时（按需更新）
    // 2. 文件保存时
    // 3. 切换文件时
    // 不应该在每次字符输入时都更新，以避免性能问题

    // 触发代码补全（在输入字母、数字、下划线或点号时）
    // 使用防抖机制，提升编辑流畅度
    if (lsp_enabled_ && lsp_manager_) {
        if (std::isalnum(ch) || ch == '_' || ch == '.' || ch == ':' || ch == '-' || ch == '>') {
            // 使用延迟触发，避免每次输入都立即请求（提升流畅度）
            completion_trigger_delay_++;
            LOG("[EDIT] Completion trigger delay: " + std::to_string(completion_trigger_delay_));

            // 增加延迟到3个字符后触发，平衡响应速度和性能
            if (completion_trigger_delay_ >= 3) {
                LOG("[EDIT] Triggering completion and LSP document update");
                completion_trigger_delay_ = 0;

                // 在触发completion时才更新LSP文档，确保LSP服务器有最新状态
                auto lsp_update_start = std::chrono::steady_clock::now();
                updateLspDocument();
                auto lsp_update_end = std::chrono::steady_clock::now();
                auto lsp_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    lsp_update_end - lsp_update_start);
                LOG("[EDIT] LSP document update took " + std::to_string(lsp_duration.count()) +
                    "ms");

                triggerCompletion();
            }
        } else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '(' || ch == '[' || ch == '{') {
            // 空格、制表符、换行、括号时隐藏补全弹窗
            LOG("[EDIT] Hiding completion popup (terminator char)");
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
        } else {
            // 其他字符，隐藏补全弹窗并重置延迟
            LOG("[EDIT] Hiding completion popup (other char)");
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
        }
    }
#endif
}

void Editor::insertNewline() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    std::string current_line = doc->getLine(cursor_row_);
    std::string before_cursor = current_line.substr(0, cursor_col_);
    std::string after_cursor = current_line.substr(cursor_col_);

    // 先执行换行操作
    doc->getLines()[cursor_row_] = before_cursor;
    doc->getLines().insert(doc->getLines().begin() + cursor_row_ + 1, after_cursor);

    // 记录换行操作到撤销栈（作为一个完整的操作）
    doc->pushChange(DocumentChange(DocumentChange::Type::NEWLINE, cursor_row_, cursor_col_,
                                   current_line,  // old_content: 完整的原行
                                   before_cursor, // new_content: 光标前的内容
                                   after_cursor // after_cursor: 光标后的内容（会移到新行）
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
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    // 如果有选中内容，删除选中内容
    if (selection_active_) {
        // 确定选择范围的开始和结束位置
        size_t start_row = selection_start_row_;
        size_t start_col = selection_start_col_;
        size_t end_row = cursor_row_;
        size_t end_col = cursor_col_;

        // 确保开始位置在结束位置之前
        if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
            std::swap(start_row, end_row);
            std::swap(start_col, end_col);
        }

        // 获取要删除的内容（用于撤销）
        std::string deleted_content = doc->getSelection(start_row, start_col, end_row, end_col);

        // 删除选中内容
        if (start_row == end_row) {
            // 同一行内的选择
            std::string& line = doc->getLines()[start_row];
            if (start_col < line.length()) {
                size_t len = (end_col <= line.length()) ? (end_col - start_col)
                                                        : (line.length() - start_col);
                line.erase(start_col, len);
            }
        } else {
            // 跨行选择
            std::string& start_line = doc->getLines()[start_row];
            std::string& end_line = doc->getLines()[end_row];

            // 合并第一行和最后一行
            start_line = start_line.substr(0, start_col) + end_line.substr(end_col);

            // 删除中间的行和最后一行
            doc->getLines().erase(doc->getLines().begin() + start_row + 1,
                                  doc->getLines().begin() + end_row + 1);
        }

        // 记录删除操作到撤销栈
        doc->pushChange(DocumentChange(DocumentChange::Type::DELETE, start_row, start_col,
                                       deleted_content, ""));

        // 移动光标到选择开始位置
        cursor_row_ = start_row;
        cursor_col_ = start_col;

        // 清除选择状态
        endSelection();

        // 确保文档至少有一行
        if (doc->lineCount() == 0) {
            doc->getLines().push_back("");
            cursor_row_ = 0;
            cursor_col_ = 0;
        }

        doc->setModified(true);

#ifdef BUILD_LSP_SUPPORT
        // 更新 LSP 文档
        updateLspDocument();
        // 删除时隐藏补全弹窗
        completion_popup_.hide();
#endif

        return;
    }

    // 没有选中内容，执行正常的退格操作
    if (cursor_col_ > 0) {
        cursor_col_--;
        doc->deleteChar(cursor_row_, cursor_col_);
    } else if (cursor_row_ > 0) {
        size_t prev_len = doc->getLine(cursor_row_ - 1).length();
        // 合并行
        doc->getLines()[cursor_row_ - 1] += doc->getLine(cursor_row_);
        doc->deleteLine(cursor_row_);
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
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    doc->deleteLine(cursor_row_);
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
    Document* doc = getCurrentDocument();
    if (!doc || doc->lineCount() == 0) {
        setStatusMessage("No content to select");
        return;
    }

    selection_active_ = true;
    selection_start_row_ = 0;
    selection_start_col_ = 0;
    cursor_row_ = doc->lineCount() - 1;
    cursor_col_ = doc->getLine(cursor_row_).length();
    adjustViewOffset(); // 确保光标可见
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
    if (cursor_col_ >= line.length())
        return;

    // 找到单词开始（支持字母、数字、下划线）
    size_t start = cursor_col_;
    while (start > 0 && (std::isalnum(line[start - 1]) || line[start - 1] == '_')) {
        start--;
    }

    // 找到单词结束
    size_t end = cursor_col_;
    while (end < line.length() && (std::isalnum(line[end]) || line[end] == '_')) {
        end++;
    }

    selection_active_ = true;
    selection_start_row_ = cursor_row_;
    selection_start_col_ = start;
    cursor_col_ = end;
    setStatusMessage("Word selected");
}

void Editor::extendSelectionUp() {
    if (!selection_active_) {
        startSelection();
    }
    // 直接移动光标，不调用 moveCursorUp（避免取消选中）
    if (cursor_row_ > 0) {
        cursor_row_--;
        adjustCursor();
        adjustViewOffset();
    }
}

void Editor::extendSelectionDown() {
    if (!selection_active_) {
        startSelection();
    }
    // 直接移动光标，不调用 moveCursorDown（避免取消选中）
    if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
        cursor_row_++;
        adjustCursor();
        adjustViewOffset();
    }
}

void Editor::extendSelectionLeft() {
    if (!selection_active_) {
        startSelection();
    }
    // 直接移动光标，不调用 moveCursorLeft（避免取消选中）
    if (cursor_col_ > 0) {
        cursor_col_--;
    } else if (cursor_row_ > 0) {
        cursor_row_--;
        cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
        adjustCursor();
        adjustViewOffset();
    }
}

void Editor::extendSelectionRight() {
    if (!selection_active_) {
        startSelection();
    }
    // 直接移动光标，不调用 moveCursorRight（避免取消选中）
    size_t line_len = getCurrentDocument()->getLine(cursor_row_).length();
    if (cursor_col_ < line_len) {
        cursor_col_++;
    } else if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
        cursor_row_++;
        cursor_col_ = 0;
        adjustCursor();
        adjustViewOffset();
    }
}

// 剪贴板操作
void Editor::cut() {
    std::string content;

    if (!selection_active_) {
        // 剪切当前行
        content = getCurrentDocument()->getLine(cursor_row_);
        if (content.empty()) {
            setStatusMessage("Line is empty");
            return;
        }
        deleteLine();
    } else {
        // 剪切选中内容
        content = getCurrentDocument()->getSelection(selection_start_row_, selection_start_col_,
                                                     cursor_row_, cursor_col_);

        // 删除选中内容
        size_t start_row = selection_start_row_;
        size_t start_col = selection_start_col_;
        size_t end_row = cursor_row_;
        size_t end_col = cursor_col_;

        // 确保开始位置在结束位置之前
        if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
            std::swap(start_row, end_row);
            std::swap(start_col, end_col);
        }

        Document* doc = getCurrentDocument();
        if (start_row == end_row) {
            // 同一行内的选择
            std::string& line = doc->getLines()[start_row];
            if (start_col < line.length()) {
                size_t len = (end_col <= line.length()) ? (end_col - start_col)
                                                        : (line.length() - start_col);
                line.erase(start_col, len);
            }
        } else {
            // 跨行选择
            std::string& start_line = doc->getLines()[start_row];
            std::string& end_line = doc->getLines()[end_row];

            // 合并第一行和最后一行
            start_line = start_line.substr(0, start_col) + end_line.substr(end_col);

            // 删除中间的行和最后一行
            doc->getLines().erase(doc->getLines().begin() + start_row + 1,
                                  doc->getLines().begin() + end_row + 1);
        }

        // 移动光标到选择开始位置
        cursor_row_ = start_row;
        cursor_col_ = start_col;
        endSelection();
        doc->setModified(true);
    }

    // 复制到系统剪贴板
    if (utils::Clipboard::copyToSystem(content)) {
        // 同时保存到内部剪贴板（作为备份）
        getCurrentDocument()->setClipboard(content);
        setStatusMessage(selection_active_ ? "Selection cut to clipboard"
                                           : "Line cut to clipboard");
    } else {
        // 如果系统剪贴板不可用，使用内部剪贴板
        getCurrentDocument()->setClipboard(content);
        setStatusMessage("Cut to internal clipboard (system clipboard unavailable)");
    }
}

void Editor::copy() {
    LOG("[DEBUG COPY] copy() called");
    LOG("[DEBUG COPY] selection_active_: " + std::string(selection_active_ ? "true" : "false"));

    std::string content;

    if (!selection_active_) {
        LOG("[DEBUG COPY] No selection, copying current line");
        LOG("[DEBUG COPY] cursor_row_: " + std::to_string(cursor_row_));

        // 复制当前行
        Document* doc = getCurrentDocument();
        if (!doc) {
            LOG_ERROR("[DEBUG COPY] ERROR: No document available!");
            return;
        }
        content = doc->getLine(cursor_row_);
        LOG("[DEBUG COPY] Line content length: " + std::to_string(content.length()));

        if (content.empty()) {
            LOG("[DEBUG COPY] Line is empty, returning");
            setStatusMessage("Line is empty");
            return;
        }
    } else {
        LOG("[DEBUG COPY] Selection active, copying selection");
        LOG("[DEBUG COPY] Selection: row " + std::to_string(selection_start_row_) + ":" +
            std::to_string(selection_start_col_) + " to " + std::to_string(cursor_row_) + ":" +
            std::to_string(cursor_col_));

        // 复制选中内容
        Document* doc = getCurrentDocument();
        if (!doc) {
            LOG_ERROR("[DEBUG COPY] ERROR: No document available for selection!");
            return;
        }
        content =
            doc->getSelection(selection_start_row_, selection_start_col_, cursor_row_, cursor_col_);
        LOG("[DEBUG COPY] Selection content length: " + std::to_string(content.length()));

        if (content.empty()) {
            LOG("[DEBUG COPY] Selection is empty, returning");
            setStatusMessage("Selection is empty");
            return;
        }
    }

    std::string preview = content.substr(0, std::min(50UL, content.length()));
    if (content.length() > 50)
        preview += "...";
    LOG("[DEBUG COPY] Content to copy: '" + preview + "'");

    // 复制到系统剪贴板
    if (utils::Clipboard::copyToSystem(content)) {
        LOG("[DEBUG COPY] Successfully copied to system clipboard");
        // 同时保存到内部剪贴板（作为备份）
        getCurrentDocument()->setClipboard(content);
        setStatusMessage(selection_active_ ? "Selection copied to clipboard"
                                           : "Line copied to clipboard");
    } else {
        LOG("[DEBUG COPY] System clipboard failed, using internal clipboard");
        // 如果系统剪贴板不可用，使用内部剪贴板
        getCurrentDocument()->setClipboard(content);
        setStatusMessage("Copied to internal clipboard (system clipboard unavailable)");
    }

    LOG("[DEBUG COPY] Copy operation completed");

    // 复制后不取消选中（保持选中状态，方便用户继续操作）
    // endSelection(); // 注释掉，保持选中状态
}

void Editor::paste() {
    std::string clipboard;

    // 优先从系统剪贴板读取
    if (utils::Clipboard::isAvailable()) {
        clipboard = utils::Clipboard::pasteFromSystem();
    }

    // 如果系统剪贴板为空或不可用，尝试使用内部剪贴板
    if (clipboard.empty()) {
        clipboard = getCurrentDocument()->getClipboard();
    }

    if (clipboard.empty()) {
        setStatusMessage("Clipboard is empty");
        return;
    }

    // 如果有选中内容，先删除选中内容再粘贴
    if (selection_active_) {
        // 删除选中内容
        size_t start_row = selection_start_row_;
        size_t start_col = selection_start_col_;
        size_t end_row = cursor_row_;
        size_t end_col = cursor_col_;

        // 确保开始位置在结束位置之前
        if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
            std::swap(start_row, end_row);
            std::swap(start_col, end_col);
        }

        Document* doc = getCurrentDocument();
        if (start_row == end_row) {
            // 同一行内的选择
            std::string& line = doc->getLines()[start_row];
            if (start_col < line.length()) {
                size_t len = (end_col <= line.length()) ? (end_col - start_col)
                                                        : (line.length() - start_col);
                line.erase(start_col, len);
            }
        } else {
            // 跨行选择
            std::string& start_line = doc->getLines()[start_row];
            std::string& end_line = doc->getLines()[end_row];

            // 合并第一行和最后一行
            start_line = start_line.substr(0, start_col) + end_line.substr(end_col);

            // 删除中间的行和最后一行
            doc->getLines().erase(doc->getLines().begin() + start_row + 1,
                                  doc->getLines().begin() + end_row + 1);
        }

        // 移动光标到选择开始位置
        cursor_row_ = start_row;
        cursor_col_ = start_col;
        endSelection();
    }

    // 处理多行文本粘贴
    Document* doc = getCurrentDocument();

    // 如果文本包含换行符，需要特殊处理
    if (clipboard.find('\n') != std::string::npos) {
        // 多行文本：需要分割并插入
        std::istringstream stream(clipboard);
        std::string line;
        size_t current_row = cursor_row_;
        size_t current_col = cursor_col_;
        bool first_line = true;

        while (std::getline(stream, line)) {
            if (first_line) {
                // 第一行：在当前行插入
                std::string& current_line = doc->getLines()[current_row];
                current_line.insert(current_col, line);
                current_col += line.length();
                first_line = false;
            } else {
                // 后续行：插入新行
                doc->getLines().insert(doc->getLines().begin() + current_row + 1, line);
                current_row++;
                current_col = line.length();
            }
        }

        // 如果原始文本以换行符结尾，添加一个空行
        if (!clipboard.empty() && clipboard.back() == '\n') {
            doc->getLines().insert(doc->getLines().begin() + current_row + 1, "");
            current_row++;
            current_col = 0;
        }

        // 更新光标位置
        cursor_row_ = current_row;
        cursor_col_ = current_col;
    } else {
        // 单行文本：直接插入
        doc->insertText(cursor_row_, cursor_col_, clipboard);
        cursor_col_ += clipboard.length();
    }

    adjustCursor();
    adjustViewOffset();
    doc->setModified(true);
    setStatusMessage("Pasted from clipboard");
}

// 撤销/重做
void Editor::undo() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    size_t change_row = 0, change_col = 0;
    if (doc->undo(&change_row, &change_col)) {
        // VSCode 风格：精确恢复光标位置到操作开始的位置
        cursor_row_ = change_row;
        cursor_col_ = change_col;

        // 确保光标位置有效（防止越界）
        adjustCursor();

        // Neovim/VSCode 优化：撤销操作时使用极度保守的视图调整策略
        // 1. 如果光标已在可见范围内，绝对不调整视图（避免闪烁）
        // 2. 只在光标完全不可见时才调整，且调整幅度最小化
        // 3. 不使用scrolloff机制，因为撤销操作应该保持视觉连续性
        adjustViewOffsetForUndoConservative(cursor_row_, cursor_col_);

        // 清除选择（VSCode 行为：撤销后清除选择）
        selection_active_ = false;

        // 优化：撤销成功时不显示状态消息，避免不必要的UI更新
        // 只有在撤销失败时才显示消息
    } else {
        setStatusMessage("Nothing to undo");
    }
}

void Editor::redo() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    size_t change_row, change_col;
    if (doc->redo(&change_row, &change_col)) {
        // 恢复光标位置到修改发生的位置（VSCode 行为）
        cursor_row_ = change_row;
        cursor_col_ = change_col;

        // 确保光标位置有效
        adjustCursor();

        // VSCode 风格的视图调整：平滑滚动到光标位置
        int screen_height = screen_.dimy() - 4;
        if (screen_height > 0) {
            // 计算光标在屏幕上的位置
            int cursor_screen_pos =
                static_cast<int>(cursor_row_) - static_cast<int>(view_offset_row_);

            // 如果光标不在可见范围内，立即调整视图
            if (cursor_screen_pos < 0 || cursor_screen_pos >= screen_height) {
                // VSCode 行为：将光标行放在屏幕中央
                view_offset_row_ = (cursor_row_ >= static_cast<size_t>(screen_height / 2))
                                       ? cursor_row_ - screen_height / 2
                                       : 0;
            } else {
                // 光标在可见范围内，但如果太靠近边缘，微调视图
                int margin = 3; // 边缘边距
                if (cursor_screen_pos < margin) {
                    view_offset_row_ =
                        (cursor_row_ >= static_cast<size_t>(margin)) ? cursor_row_ - margin : 0;
                } else if (cursor_screen_pos >= screen_height - margin) {
                    size_t target_offset = cursor_row_ - (screen_height - margin - 1);
                    view_offset_row_ = (target_offset > cursor_row_) ? 0 : target_offset;
                }
            }
        }

        // 清除选择（VSCode 行为：重做后清除选择）
        selection_active_ = false;

        // 使用简洁的状态消息（VSCode 风格：不显示状态消息）
    } else {
        setStatusMessage("Nothing to redo");
    }
}

void Editor::moveLineUp() {
    if (cursor_row_ == 0)
        return;

    auto& lines = getCurrentDocument()->getLines();
    std::swap(lines[cursor_row_], lines[cursor_row_ - 1]);
    cursor_row_--;
    getCurrentDocument()->setModified(true);
    setStatusMessage("Line moved up");
}

void Editor::moveLineDown() {
    auto& lines = getCurrentDocument()->getLines();
    if (cursor_row_ >= lines.size() - 1)
        return;

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

    std::string inserted_text = "    "; // 4个空格

    if (at_line_start) {
        // 在行首插入4个空格（缩进整行）
        doc->insertText(cursor_row_, 0, inserted_text);
        cursor_col_ += 4;
    } else {
        // 在光标位置插入4个空格
        doc->insertText(cursor_row_, cursor_col_, inserted_text);
        cursor_col_ += 4;
    }

    // 记录变更到撤销系统（LSP支持）
#ifdef BUILD_LSP_SUPPORT
    if (lsp_enabled_ && document_change_tracker_) {
        LOG("[EDIT] Recording indent change to document_change_tracker_");
        document_change_tracker_->recordInsert(
            static_cast<int>(cursor_row_), static_cast<int>(at_line_start ? 0 : cursor_col_ - 4),
            inserted_text);
    }
#endif

    doc->setModified(true);

    getCurrentDocument()->setModified(true);
}

void Editor::unindentLine() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    auto& lines = doc->getLines();
    if (cursor_row_ >= lines.size())
        return;

    std::string& line = lines[cursor_row_];
    // 移除前导空格（最多4个）
    size_t spaces_to_remove = 0;
    while (spaces_to_remove < 4 && spaces_to_remove < line.length() &&
           line[spaces_to_remove] == ' ') {
        spaces_to_remove++;
    }

    if (spaces_to_remove > 0) {
        // 使用deleteChar方法逐个删除，这样会自动记录到撤销系统
        for (size_t i = 0; i < spaces_to_remove; ++i) {
            doc->deleteChar(cursor_row_, 0);
        }

        if (cursor_col_ >= spaces_to_remove) {
            cursor_col_ -= spaces_to_remove;
        } else {
            cursor_col_ = 0;
        }

        // LSP变更记录
#ifdef BUILD_LSP_SUPPORT
        if (lsp_enabled_ && document_change_tracker_) {
            LOG("[EDIT] Recording unindent change to document_change_tracker_");
            document_change_tracker_->recordDelete(static_cast<int>(cursor_row_), 0,
                                                   static_cast<int>(spaces_to_remove));
        }
#endif

        doc->setModified(true);
    }
}

void Editor::toggleComment() {
    auto& lines = getCurrentDocument()->getLines();
    if (cursor_row_ >= lines.size())
        return;

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
