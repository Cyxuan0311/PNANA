// 编辑操作相关实现
#include "core/document.h"
#include "core/editor.h"
#include "features/indent/auto_indent_engine.h"
#include "utils/clipboard.h"
#include "utils/logger.h"
#include "utils/text_utils.h"
#include <iostream>
#include <sstream>

namespace pnana {
namespace core {

// 编辑操作
void Editor::insertChar(char ch) {
    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    // 记录变更（阶段2优化：增量更新）
#ifdef BUILD_LSP_SUPPORT
    if (lsp_enabled_ && document_change_tracker_) {
        std::string new_text(1, ch); // 新文本是插入的字符
        document_change_tracker_->recordInsert(static_cast<int>(cursor_row_),
                                               static_cast<int>(cursor_col_), new_text);
    }
#endif

    doc->insertChar(cursor_row_, cursor_col_, ch);
    cursor_col_++;

    // 更新markdown预览（延迟更新以提升性能）
    if (isMarkdownPreviewActive()) {
        // 使用延迟更新机制，避免每次输入都立即重新渲染
        markdown_preview_needs_update_ = true;
        last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
        last_render_source_ = "edit_insertChar";
        // 不立即设置 force_ui_update_，而是等待延迟更新
    }

    // 更新单词高亮（光标位置变化）
    updateWordHighlight();

#ifdef BUILD_LSP_SUPPORT
    // 智能LSP文档更新策略
    // 只有在以下情况下才更新LSP文档：
    // 1. 触发completion时（按需更新）
    // 2. 文件保存时
    // 3. 切换文件时
    // 不应该在每次字符输入时都更新，以避免性能问题

    // 触发代码补全（参考 VS Code：标准库/当前文档/项目/智能补全）
    if (lsp_enabled_ && lsp_manager_) {
        if (std::isalnum(ch) || ch == '_' || ch == '.' || ch == ':' || ch == '-' || ch == '>') {
            bool is_member_trigger = (ch == '.' || ch == ':');
            if (is_member_trigger) {
                last_completion_trigger_ = std::string(1, ch);
                completion_trigger_delay_ = 0;
                syncLspAfterEdit(false);
                triggerCompletion();
            } else {
                completion_trigger_delay_++;
                if (completion_trigger_delay_ >= 1) {
                    completion_trigger_delay_ = 0;
                    syncLspAfterEdit(false);
                    triggerCompletion();
                }
            }
        } else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '(' || ch == '[' || ch == '{') {
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
            last_completion_trigger_.clear();
        } else {
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
        }
    }
#endif
}

void Editor::insertText(const std::string& text) {
    if (text.empty()) {
        return;
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    // 记录变更（阶段2优化：增量更新）
#ifdef BUILD_LSP_SUPPORT
    if (lsp_enabled_ && document_change_tracker_) {
        document_change_tracker_->recordInsert(static_cast<int>(cursor_row_),
                                               static_cast<int>(cursor_col_), text);
    }
#endif

    // 使用 Document 的 insertText 方法插入文本
    doc->insertText(cursor_row_, cursor_col_, text);

    // 更新光标位置：对于UTF-8字符，需要按字符数移动光标
    // 由于 cursor_col_ 是按字节索引的，所以直接加上文本长度
    cursor_col_ += text.length();

    // 更新markdown预览（延迟更新以提升性能）
    if (isMarkdownPreviewActive()) {
        markdown_preview_needs_update_ = true;
        last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
        last_render_source_ = "edit_insertText";
    }

    // 更新单词高亮（光标位置变化）
    updateWordHighlight();

#ifdef BUILD_LSP_SUPPORT
    // 对于UTF-8字符，不触发代码补全（中文等字符通常不需要补全）
    // 但如果是ASCII字符，仍然可以触发补全
    if (lsp_enabled_ && lsp_manager_ && text.length() == 1) {
        char ch = text[0];
        if (std::isalnum(ch) || ch == '_' || ch == '.' || ch == ':' || ch == '-' || ch == '>') {
            bool is_member_trigger = (ch == '.' || ch == ':');
            if (is_member_trigger) {
                last_completion_trigger_ = std::string(1, ch);
                completion_trigger_delay_ = 0;
                syncLspAfterEdit(false);
                triggerCompletion();
            } else {
                completion_trigger_delay_++;
                if (completion_trigger_delay_ >= 1) {
                    completion_trigger_delay_ = 0;
                    syncLspAfterEdit(false);
                    triggerCompletion();
                }
            }
        } else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '(' || ch == '[' || ch == '{') {
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
            last_completion_trigger_.clear();
        } else {
            completion_popup_.hide();
            completion_trigger_delay_ = 0;
        }
    } else {
        // 多字节字符，隐藏补全弹窗
        completion_popup_.hide();
        completion_trigger_delay_ = 0;
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

    doc->getLines()[cursor_row_] = before_cursor;
    doc->getLines().insert(doc->getLines().begin() + cursor_row_ + 1, after_cursor);

    doc->pushChange(DocumentChange(DocumentChange::Type::NEWLINE, cursor_row_, cursor_col_,
                                   current_line, before_cursor, after_cursor));

    cursor_row_++;
    cursor_col_ = 0;

    // 粘贴时禁用自动缩进，避免双重缩进
    if (is_pasting_) {
        is_pasting_ = false;
    } else {
        const auto& cfg = config_manager_.getConfig();

        if (cfg.editor.auto_indent) {
#ifdef BUILD_TREE_SITTER_SUPPORT
            bool ts_enabled = auto_indent_engine_.isTreeSitterEnabled();
            if (ts_enabled) {
                std::string file_type = getFileType();
                std::string file_ext = doc->getFileExtension();
                std::string file_ext_with_dot = file_ext.empty() ? "" : "." + file_ext;
                auto it = cfg.language_indent.find(file_type);
                core::LanguageIndentConfig indent_cfg;
                if (it != cfg.language_indent.end()) {
                    indent_cfg = it->second;
                } else {
                    bool found = false;
                    for (const auto& kv : cfg.language_indent) {
                        for (const auto& ext : kv.second.file_extensions) {
                            if (ext == file_ext || ext == file_ext_with_dot ||
                                ext == "." + file_type || ext == file_type) {
                                indent_cfg = kv.second;
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                    if (!found) {
                        indent_cfg = auto_indent_engine_.getDefaultConfigForLanguage(file_type);
                    }
                }
                auto_indent_engine_.setFileType(file_type, indent_cfg);
                std::string indent = auto_indent_engine_.computeIndentAfterNewline(
                    doc->getLines(), cursor_row_, cursor_col_);
                if (!indent.empty()) {
                    doc->getLines()[cursor_row_] = indent + after_cursor;
                    cursor_col_ = indent.size();
                }
            } else {
                std::string file_type = getFileType();
                std::string file_ext = doc->getFileExtension();
                std::string file_ext_with_dot = file_ext.empty() ? "" : "." + file_ext;
                auto default_cfg = auto_indent_engine_.getDefaultConfigForLanguage(file_type);

                auto it = cfg.language_indent.find(file_type);
                if (it != cfg.language_indent.end()) {
                    default_cfg = it->second;
                } else {
                    bool found = false;
                    for (const auto& kv : cfg.language_indent) {
                        for (const auto& ext : kv.second.file_extensions) {
                            if (ext == file_ext || ext == file_ext_with_dot ||
                                ext == "." + file_type || ext == file_type) {
                                default_cfg = kv.second;
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                }

                if (default_cfg.smart_indent) {
                    auto_indent_engine_.setIndentConfig(default_cfg);
                    auto_indent_engine_.setFileType(file_type);
                    std::string indent =
                        computeAutoIndent(doc->getLines(), cursor_row_, cursor_col_, default_cfg);
                    if (!indent.empty()) {
                        doc->getLines()[cursor_row_] = indent + after_cursor;
                        cursor_col_ = indent.size();
                    }
                }
            }
#endif
        }
    }

#ifdef BUILD_LSP_SUPPORT
    syncLspAfterEdit(true);
#endif

    if (isMarkdownPreviewActive()) {
        markdown_preview_needs_update_ = true;
        last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
        last_render_source_ = "edit_newline";
    }

    adjustViewOffset();
}

std::string Editor::computeAutoIndent(const std::vector<std::string>& lines, size_t cursor_row,
                                      size_t cursor_col, const LanguageIndentConfig& cfg) const {
    (void)cursor_col;

    if (lines.empty() || cursor_row == 0 || cursor_row > lines.size()) {
        return "";
    }

    const std::string& prev_line = lines[cursor_row - 1];
    const std::string& current_line = lines[cursor_row];

    int base_indent = 0;
    for (char ch : prev_line) {
        if (ch == ' ') {
            base_indent++;
        } else if (ch == '\t') {
            base_indent += cfg.indent_size;
        } else {
            break;
        }
    }
    int base_level = base_indent / cfg.indent_size;

    std::string trimmed_prev = prev_line;
    trimmed_prev.erase(0, trimmed_prev.find_first_not_of(" \t"));

    std::string trimmed_current = current_line;
    trimmed_current.erase(0, trimmed_current.find_first_not_of(" \t"));

    auto ends_with_open = [](const std::string& s) {
        if (s.empty())
            return false;
        char last = s.back();
        return last == '{' || last == '(' || last == '[' || last == ':';
    };

    auto starts_with_close = [](const std::string& s) {
        if (s.empty())
            return false;
        return s[0] == '}' || s[0] == ')' || s[0] == ']';
    };

    bool prev_opens = ends_with_open(trimmed_prev);
    bool curr_closes = starts_with_close(trimmed_current);

    int new_level = base_level;
    if (curr_closes) {
        new_level = std::max(0, base_level - 1);
    } else if (prev_opens) {
        new_level = base_level + 1;
    }

    if (new_level <= 0) {
        return "";
    }

    std::string result;
    if (cfg.insert_spaces) {
        result = std::string(new_level * cfg.indent_size, ' ');
    } else {
        result = std::string(static_cast<size_t>(new_level), '\t');
    }
    return result;
}

void Editor::deleteChar() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    const std::string& line = doc->getLine(cursor_row_);

    // 如果光标不在行尾，删除下一个UTF-8字符
    if (cursor_col_ < line.length()) {
        // 计算需要删除的UTF-8字符的字节数
        size_t bytes_to_delete = utils::getUtf8CharBytesAfter(line, cursor_col_);

        // 确保不会越界
        if (cursor_col_ + bytes_to_delete > line.length()) {
            bytes_to_delete = line.length() - cursor_col_;
        }

        // 删除完整的UTF-8字符
        std::string& mutable_line = doc->getLines()[cursor_row_];
        std::string deleted_char = line.substr(cursor_col_, bytes_to_delete);
        mutable_line.erase(cursor_col_, bytes_to_delete);

        // 记录删除操作到撤销栈
        doc->pushChange(DocumentChange(DocumentChange::Type::DELETE, cursor_row_, cursor_col_,
                                       deleted_char, ""));
        doc->setModified(true);
    } else if (cursor_row_ < doc->lineCount() - 1) {
        // 光标在行尾，合并下一行
        std::string next_line = doc->getLine(cursor_row_ + 1);
        doc->getLines()[cursor_row_] += next_line;
        doc->deleteLine(cursor_row_ + 1);
    }

    // 更新markdown预览（延迟更新以提升性能）
    if (isMarkdownPreviewActive()) {
        markdown_preview_needs_update_ = true;
        last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
        last_render_source_ = "edit_deleteChar";
    }

#ifdef BUILD_LSP_SUPPORT
    // 普通删除走防抖同步
    syncLspAfterEdit(false);

    // 在单词内删除时，触发新的补全请求以显示匹配当前前缀的补全项
    if (lsp_enabled_ && lsp_manager_ && cursor_row_ < doc->lineCount()) {
        const std::string& line = doc->getLine(cursor_row_);
        if (cursor_col_ > 0 && static_cast<size_t>(cursor_col_) <= line.length()) {
            char prev_char = line[cursor_col_ - 1];
            if (std::isalnum(prev_char) || prev_char == '_' || prev_char == '.' ||
                prev_char == ':') {
                last_completion_trigger_.clear();
                completion_trigger_delay_ = 0;
                triggerCompletion();
            } else {
                completion_popup_.hide();
            }
        } else {
            completion_popup_.hide();
        }
    }
#endif
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
        // 选择删除通常跨范围，按结构变化处理
        syncLspAfterEdit(true);
#endif

        // 更新markdown预览（延迟更新以提升性能）
        if (isMarkdownPreviewActive()) {
            markdown_preview_needs_update_ = true;
            last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
            last_render_source_ = "edit_backspace";
        }

        return;
    }

    // 没有选中内容，执行正常的退格操作
    if (cursor_col_ > 0) {
        // 获取当前行的内容
        const std::string& line = doc->getLine(cursor_row_);

        // 计算需要删除的UTF-8字符的字节数
        size_t bytes_to_delete = utils::getUtf8CharBytesBefore(line, cursor_col_);

        // 确保不会越界
        if (bytes_to_delete > cursor_col_) {
            bytes_to_delete = cursor_col_;
        }

        if (bytes_to_delete == 0) {
            bytes_to_delete = 1;
        }

        // 删除完整的UTF-8字符
        std::string& mutable_line = doc->getLines()[cursor_row_];
        std::string deleted_char = line.substr(cursor_col_ - bytes_to_delete, bytes_to_delete);

        mutable_line.erase(cursor_col_ - bytes_to_delete, bytes_to_delete);
        cursor_col_ -= bytes_to_delete;

        // 记录删除操作到撤销栈
        doc->pushChange(DocumentChange(DocumentChange::Type::DELETE, cursor_row_, cursor_col_,
                                       deleted_char, ""));
        doc->setModified(true);
    } else if (cursor_row_ > 0) {
        size_t prev_len = doc->getLine(cursor_row_ - 1).length();
        // 合并行
        doc->getLines()[cursor_row_ - 1] += doc->getLine(cursor_row_);
        doc->deleteLine(cursor_row_);
        cursor_row_--;
        cursor_col_ = prev_len;
    }

#ifdef BUILD_LSP_SUPPORT
    // 退格合并/删除空行时按结构变化处理
    syncLspAfterEdit(true);

    // 在单词内回退时，触发新的补全请求以显示匹配当前前缀的补全项
    if (lsp_enabled_ && lsp_manager_ && cursor_row_ < doc->lineCount()) {
        const std::string& line = doc->getLine(cursor_row_);
        if (cursor_col_ > 0 && static_cast<size_t>(cursor_col_) <= line.length()) {
            char prev_char = line[cursor_col_ - 1];
            if (std::isalnum(prev_char) || prev_char == '_' || prev_char == '.' ||
                prev_char == ':') {
                last_completion_trigger_.clear();
                completion_trigger_delay_ = 0;
                triggerCompletion();
            } else {
                completion_popup_.hide();
            }
        } else {
            completion_popup_.hide();
        }
    }
#endif

    // 更新markdown预览（延迟更新以提升性能）
    if (isMarkdownPreviewActive()) {
        markdown_preview_needs_update_ = true;
        last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
        last_render_source_ = "edit_backspace";
    }
}

void Editor::deleteLine() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    doc->deleteLine(cursor_row_);
    adjustCursor();
    setStatusMessage("Line deleted");

#ifdef BUILD_LSP_SUPPORT
    // 删行属于结构变化
    syncLspAfterEdit(true);
#endif

    // 更新markdown预览（延迟更新以提升性能）
    if (isMarkdownPreviewActive()) {
        markdown_preview_needs_update_ = true;
        last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
        last_render_source_ = "edit_deleteLine";
    }
}

void Editor::deleteWord() {
    Document* doc = getCurrentDocument();
    const std::string& line = doc->getLine(cursor_row_);
    size_t start = cursor_col_;
    size_t end = start;

    while (end < line.length() && std::isalnum(line[end])) {
        end++;
    }

    if (end == start) {
        return;
    }

    std::string deleted = line.substr(start, end - start);
    std::string& mutable_line = doc->getLines()[cursor_row_];
    mutable_line.erase(start, end - start);

    doc->pushChange(DocumentChange(DocumentChange::Type::DELETE, cursor_row_, start, deleted, ""));

    doc->setModified(true);

#ifdef BUILD_LSP_SUPPORT
    syncLspAfterEdit(false);
#endif

    if (isMarkdownPreviewActive()) {
        markdown_preview_needs_update_ = true;
        last_markdown_preview_update_time_ = std::chrono::steady_clock::now();
        last_render_source_ = "edit_deleteWord";
    }
}

void Editor::duplicateLine() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    std::string line = doc->getLine(cursor_row_);

    doc->insertLine(cursor_row_ + 1);
    doc->getLines()[cursor_row_ + 1] = line;

    doc->pushChange(DocumentChange(DocumentChange::Type::INSERT, cursor_row_ + 1, 0, "", line));

    doc->setModified(true);
    cursor_row_++;
    cursor_col_ = 0;
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

        // 记录删除行的撤销操作
        Document* doc = getCurrentDocument();
        doc->pushChange(
            DocumentChange(DocumentChange::Type::DELETE, cursor_row_, 0, content + "\n", ""));

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

        // 记录删除操作到撤销栈
        doc->pushChange(
            DocumentChange(DocumentChange::Type::DELETE, start_row, start_col, content, ""));

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
    std::string content;

    if (!selection_active_) {
        // 复制当前行
        Document* doc = getCurrentDocument();
        if (!doc) {
            return;
        }
        content = doc->getLine(cursor_row_);

        if (content.empty()) {
            setStatusMessage("Line is empty");
            return;
        }
    } else {
        // 复制选中内容
        Document* doc = getCurrentDocument();
        if (!doc) {
            return;
        }
        content =
            doc->getSelection(selection_start_row_, selection_start_col_, cursor_row_, cursor_col_);

        if (content.empty()) {
            setStatusMessage("Selection is empty");
            return;
        }
    }

    // 复制到系统剪贴板
    if (utils::Clipboard::copyToSystem(content)) {
        // 同时保存到内部剪贴板（作为备份）
        getCurrentDocument()->setClipboard(content);
        setStatusMessage(selection_active_ ? "Selection copied to clipboard"
                                           : "Line copied to clipboard");
        // 显示 Toast 通知
        toast_.showSuccess("Copied " + std::to_string(content.length()) + " characters");
    } else {
        // 如果系统剪贴板不可用，使用内部剪贴板
        getCurrentDocument()->setClipboard(content);
        setStatusMessage("Copied to internal clipboard (system clipboard unavailable)");
        toast_.showWarning("Copied to internal clipboard (system clipboard unavailable)");
    }

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
    size_t paste_start_row = cursor_row_;
    size_t paste_start_col = cursor_col_;

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

    doc->pushChange(DocumentChange(DocumentChange::Type::INSERT, paste_start_row, paste_start_col,
                                   "", clipboard));

    adjustCursor();
    adjustViewOffset();
    doc->setModified(true);
    setStatusMessage("Pasted from clipboard");

#ifdef BUILD_LSP_SUPPORT
    // 粘贴是否结构变化取决于是否多行
    bool structure_changed = (clipboard.find('\n') != std::string::npos);
    syncLspAfterEdit(structure_changed);
#endif

    // 计算粘贴的行数和字符数
    size_t line_count = std::count(clipboard.begin(), clipboard.end(), '\n') + 1;
    size_t char_count = clipboard.length();

    // 显示 Toast 通知
    if (line_count == 1) {
        toast_.showSuccess("Pasted " + std::to_string(char_count) + " characters");
    } else {
        toast_.showSuccess("Pasted " + std::to_string(line_count) + " lines (" +
                           std::to_string(char_count) + " characters)");
    }
}

// 撤销/重做
void Editor::undo() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No document to undo");
        return;
    }

    size_t change_row = 0, change_col = 0;
    DocumentChange::Type change_type;

    if (doc->undo(&change_row, &change_col, &change_type)) {
        // VSCode 风格的光标定位：精确恢复到操作开始的位置
        cursor_row_ = change_row;
        cursor_col_ = change_col;

        // 确保光标位置在有效范围内
        adjustCursor();

        // VSCode 式的视图调整：保守策略，避免不必要的滚动
        // 只有当光标完全超出可见区域时才调整视图
        adjustViewOffsetForUndoConservative(cursor_row_, cursor_col_);

        // 撤销操作后清除选择状态（VSCode 行为）
        selection_active_ = false;

#ifdef BUILD_LSP_SUPPORT
        // 撤销可能引入结构变化（如恢复/删除换行），用强制同步更稳妥
        syncLspAfterEdit(true);
#endif

        // 不显示成功消息，避免UI干扰（VSCode 行为）
    } else {
        setStatusMessage("Nothing to undo");
    }
}

void Editor::redo() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No document to redo");
        return;
    }

    size_t change_row = 0, change_col = 0;

    if (doc->redo(&change_row, &change_col)) {
        // VSCode 风格的光标定位：精确恢复到操作结束的位置
        cursor_row_ = change_row;
        cursor_col_ = change_col;

        // 确保光标位置在有效范围内
        adjustCursor();

        // 使用与撤销相同的保守视图调整策略，避免不必要的视图跳跃
        adjustViewOffsetForUndoConservative(cursor_row_, cursor_col_);

        // 重做操作后清除选择状态（VSCode 行为）
        selection_active_ = false;

#ifdef BUILD_LSP_SUPPORT
        // 重做可能引入结构变化，强制同步保证折叠/诊断及时
        syncLspAfterEdit(true);
#endif

    } else {
        setStatusMessage("Nothing to redo");
    }
}

void Editor::moveLineUp() {
    if (cursor_row_ == 0)
        return;

    Document* doc = getCurrentDocument();
    auto& lines = doc->getLines();
    size_t target = cursor_row_ - 1;

    std::string moved_line = lines[cursor_row_];
    std::string swapped_line = lines[target];

    doc->pushChange(DocumentChange(DocumentChange::Type::MOVE_LINE, cursor_row_, target,
                                   moved_line + "\n" + swapped_line,
                                   swapped_line + "\n" + moved_line, DocumentChange::MOVE_TAG));

    std::swap(lines[cursor_row_], lines[target]);
    cursor_row_ = target;
    doc->setModified(true);
    setStatusMessage("Line moved up");

#ifdef BUILD_LSP_SUPPORT
    syncLspAfterEdit(true);
#endif
}

void Editor::moveLineDown() {
    Document* doc = getCurrentDocument();
    auto& lines = doc->getLines();
    if (cursor_row_ >= lines.size() - 1)
        return;

    size_t target = cursor_row_ + 1;

    std::string moved_line = lines[cursor_row_];
    std::string swapped_line = lines[target];

    doc->pushChange(DocumentChange(DocumentChange::Type::MOVE_LINE, cursor_row_, target,
                                   moved_line + "\n" + swapped_line,
                                   swapped_line + "\n" + moved_line, DocumentChange::MOVE_TAG));

    std::swap(lines[cursor_row_], lines[target]);
    cursor_row_ = target;
    doc->setModified(true);
    setStatusMessage("Line moved down");

#ifdef BUILD_LSP_SUPPORT
    syncLspAfterEdit(true);
#endif
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

    std::string file_type = getFileType();
    std::string file_ext = doc->getFileExtension();
    std::string file_ext_with_dot = file_ext.empty() ? "" : "." + file_ext;
    const auto& cfg = config_manager_.getConfig();

    int tab_size = cfg.editor.tab_size;
    bool insert_spaces = cfg.editor.insert_spaces;

    auto it = cfg.language_indent.find(file_type);
    if (it != cfg.language_indent.end()) {
        tab_size = it->second.indent_size;
        insert_spaces = it->second.insert_spaces;
    } else {
        for (const auto& kv : cfg.language_indent) {
            for (const auto& ext : kv.second.file_extensions) {
                if (ext == file_ext || ext == file_ext_with_dot || ext == "." + file_type ||
                    ext == file_type) {
                    tab_size = kv.second.indent_size;
                    insert_spaces = kv.second.insert_spaces;
                    goto indent_done;
                }
            }
        }
    }
indent_done:

    tab_size = std::max(1, std::min(8, tab_size));

    std::string& line = lines[cursor_row_];

    size_t first_non_space = line.find_first_not_of(" \t");
    bool at_line_start = (cursor_col_ == 0) ||
                         (first_non_space != std::string::npos && cursor_col_ <= first_non_space);

    std::string inserted_text =
        insert_spaces ? std::string(static_cast<size_t>(tab_size), ' ') : "\t";

    size_t insert_col = at_line_start ? 0 : cursor_col_;

    doc->insertText(cursor_row_, insert_col, inserted_text);
    cursor_col_ += static_cast<size_t>(insert_spaces ? tab_size : 1);

#ifdef BUILD_LSP_SUPPORT
    if (lsp_enabled_ && document_change_tracker_) {
        document_change_tracker_->recordInsert(
            static_cast<int>(cursor_row_), static_cast<int>(at_line_start ? 0 : cursor_col_ - 4),
            inserted_text);
    }
#endif

    doc->pushChange(
        DocumentChange(DocumentChange::Type::INSERT, cursor_row_, insert_col, "", inserted_text));

    doc->setModified(true);
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
            document_change_tracker_->recordDelete(static_cast<int>(cursor_row_), 0,
                                                   static_cast<int>(spaces_to_remove));
        }
#endif

        doc->setModified(true);
    }
}

} // namespace core
} // namespace pnana
