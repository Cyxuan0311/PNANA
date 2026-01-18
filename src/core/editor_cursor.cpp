// 光标移动相关实现

#include <cstddef>
#include <string>

// 获取UTF-8字符的辅助函数
std::string getUtf8CharAt(const std::string& str, size_t pos) {
    if (pos >= str.length()) {
        return " ";
    }

    unsigned char first_byte = static_cast<unsigned char>(str[pos]);

    // 单字节ASCII字符
    if ((first_byte & 0x80) == 0) {
        return str.substr(pos, 1);
    }

    // 多字节UTF-8字符
    int bytes_needed;
    if ((first_byte & 0xE0) == 0xC0) {
        bytes_needed = 2;
    } else if ((first_byte & 0xF0) == 0xE0) {
        bytes_needed = 3;
    } else if ((first_byte & 0xF8) == 0xF0) {
        bytes_needed = 4;
    } else {
        // 无效的UTF-8，退回到单字节
        return str.substr(pos, 1);
    }

    // 确保有足够的字节
    if (pos + bytes_needed > str.length()) {
        return str.substr(pos, 1);
    }

    return str.substr(pos, bytes_needed);
}

// 检查字符是否为中文字符（保留用于可能的未来功能）
bool isChineseChar(const std::string& ch) {
    if (ch.length() < 3) {
        return false;
    }

    // 中文UTF-8范围：E4-B8-80 到 E9-BF-BF (基本汉字)
    if (ch.length() == 3) {
        unsigned char b1 = static_cast<unsigned char>(ch[0]);
        unsigned char b2 = static_cast<unsigned char>(ch[1]);
        unsigned char b3 = static_cast<unsigned char>(ch[2]);

        // 基本检查：是否为3字节UTF-8且在中文范围内
        return (b1 >= 0xE4 && b1 <= 0xE9) && (b2 >= 0x80 && b2 <= 0xBF) &&
               (b3 >= 0x80 && b3 <= 0xBF);
    }

    return false;
}
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

    // 清除搜索高亮
    clearSearchHighlight();
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

    // 清除搜索高亮
    clearSearchHighlight();
}

void Editor::moveCursorLeft() {
    // 如果当前有选中状态，且不是通过 Shift 键移动，取消选中
    if (selection_active_) {
        endSelection();
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    const std::string& line = doc->getLine(cursor_row_);

    if (cursor_col_ > 0) {
        // Nano风格：向左移动到前一个字符的起始位置
        // 从当前位置向前查找UTF-8字符的起始位置
        size_t new_pos = cursor_col_ - 1;

        // 如果当前位置不是UTF-8字符的起始，找到前一个字符的起始
        while (new_pos > 0) {
            unsigned char byte = static_cast<unsigned char>(line[new_pos]);
            // 检查是否为UTF-8字符的起始字节
            if ((byte & 0xC0) != 0x80) { // 不是延续字节
                break;
            }
            new_pos--;
        }

        cursor_col_ = new_pos;
    } else if (cursor_row_ > 0) {
        cursor_row_--;
        cursor_col_ = doc->getLine(cursor_row_).length();
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

    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    const std::string& line = doc->getLine(cursor_row_);
    size_t line_len = line.length();

    if (cursor_col_ < line_len) {
        // Nano风格：向右移动到下一个字符的起始位置
        // 从当前位置开始查找下一个UTF-8字符的起始位置
        size_t new_pos = cursor_col_;

        // 跳过当前字符
        std::string current_char = getUtf8CharAt(line, new_pos);
        new_pos += current_char.length();

        // 确保不超过行长度
        if (new_pos > line_len) {
            new_pos = line_len;
        }

        cursor_col_ = new_pos;
    } else if (cursor_row_ < doc->lineCount() - 1) {
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
        LOG("[DEBUG VIEW] No document, returning");
        return;
    }

    size_t total_lines = doc->lineCount();
    if (total_lines == 0) {
        LOG("[DEBUG VIEW] Empty document, setting offset to 0");
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

// Neovim风格的撤销视图调整：更保守，只在必要时微调
void Editor::adjustViewOffsetForUndo(size_t target_row, size_t /*target_col*/) {
    // 撤销操作应该尽量保持用户的视觉上下文，避免剧烈跳跃
    // 使用更小的scrolloff值，让调整更平滑

    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1;
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

    // 计算目标光标在当前视图中的位置
    int cursor_screen_pos = static_cast<int>(target_row) - static_cast<int>(view_offset_row_);

    // 撤销操作使用极度保守的策略：
    // 1. 如果光标已在可见范围内，绝对不调整视图
    // 2. 只在光标完全不可见时才调整，且调整幅度最小化

    if (cursor_screen_pos < 0) {
        // 光标在可见区域上方，向上滚动刚好使光标可见
        // 使用1行buffer，避免紧贴边缘
        view_offset_row_ = (target_row > 1) ? target_row - 1 : 0;
    } else if (cursor_screen_pos >= screen_height) {
        // 光标在可见区域下方，向下滚动刚好使光标可见
        // 使用1行buffer，避免紧贴边缘
        size_t max_offset =
            (total_lines > static_cast<size_t>(screen_height)) ? total_lines - screen_height : 0;
        view_offset_row_ = target_row + 1 - screen_height;
        if (view_offset_row_ > max_offset) {
            view_offset_row_ = max_offset;
        }
    }
    // 如果光标已在可见范围内，完全不调整视图，这是Neovim风格的核心优化
}

// Neovim/VSCode 极度保守的撤销视图调整：最大限度避免闪烁
void Editor::adjustViewOffsetForUndoConservative(size_t target_row, size_t /*target_col*/) {
    // 撤销操作的核心优化原则：
    // 1. 优先保持视觉连续性，避免任何不必要的视图跳跃
    // 2. 只有当光标完全超出可见区域时才调整，且调整幅度最小
    // 3. 完全避免使用scrolloff机制，因为撤销应该保持用户的视觉上下文

    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1;
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

    // 计算目标光标在当前视图中的位置
    int cursor_screen_pos = static_cast<int>(target_row) - static_cast<int>(view_offset_row_);

    // 超极度保守策略：只有在光标完全不可见时才调整，且调整幅度极小
    // 这种策略最大限度地避免了撤销操作时的屏幕闪烁
    if (cursor_screen_pos < 0) {
        // 光标在可见区域上方，仅当完全不可见时才调整
        // 精确调整到刚好可见，不使用任何buffer
        view_offset_row_ = target_row;
    } else if (cursor_screen_pos >= screen_height) {
        // 光标在可见区域下方，仅当完全不可见时才调整
        // 精确调整到刚好可见，不使用任何buffer
        size_t max_offset =
            (total_lines > static_cast<size_t>(screen_height)) ? total_lines - screen_height : 0;
        view_offset_row_ = target_row - (screen_height - 1);
        if (view_offset_row_ > max_offset) {
            view_offset_row_ = max_offset;
        }
    }
    // 如果光标已在可见范围内，绝对不调整视图 - 这是避免闪烁的关键
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

// 方案4：最小化状态变更优化
// 统一进行光标和视图调整，避免多次调用导致的闪烁
void Editor::adjustCursorAndViewConservative() {
    LOG("[DEBUG OPTIMIZE] Starting unified cursor and view adjustment");

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[DEBUG OPTIMIZE] No document available, skipping adjustment");
        return;
    }

    // 1. 调整光标位置（基于adjustCursor()的逻辑）
    size_t original_cursor_row = cursor_row_;
    size_t original_cursor_col = cursor_col_;

    size_t max_row = doc->lineCount() - 1;
    if (cursor_row_ > max_row) {
        cursor_row_ = max_row;
        LOG("[DEBUG OPTIMIZE] Cursor row adjusted: " + std::to_string(original_cursor_row) +
            " -> " + std::to_string(cursor_row_));
    }

    if (cursor_row_ < doc->lineCount()) {
        size_t max_col = doc->getLine(cursor_row_).length();
        if (cursor_col_ > max_col) {
            cursor_col_ = max_col;
            LOG("[DEBUG OPTIMIZE] Cursor col adjusted: " + std::to_string(original_cursor_col) +
                " -> " + std::to_string(cursor_col_));
        }
    }

    // 2. 调整视图偏移（基于adjustViewOffsetForUndoConservative()的逻辑）
    size_t original_view_offset = view_offset_row_;

    // 撤销操作的核心优化原则：
    // 1. 优先保持视觉连续性，避免任何不必要的视图跳跃
    // 2. 只有当光标完全超出可见区域时才调整，且调整幅度最小
    // 3. 完全避免使用scrolloff机制，因为撤销应该保持用户的视觉上下文

    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1;
    }

    size_t total_lines = doc->lineCount();
    if (total_lines == 0) {
        view_offset_row_ = 0;
        LOG("[DEBUG OPTIMIZE] View offset reset to 0 (empty document)");
        return;
    }

    // 计算目标光标在当前视图中的位置
    int cursor_screen_pos = static_cast<int>(cursor_row_) - static_cast<int>(view_offset_row_);

    // 超极度保守策略：只有在光标完全不可见时才调整，且调整幅度极小
    // 这种策略最大限度地避免了撤销操作时的屏幕闪烁
    if (cursor_screen_pos < 0) {
        // 光标在可见区域上方，仅当完全不可见时才调整
        // 精确调整到刚好可见，不使用任何buffer
        view_offset_row_ = cursor_row_;
        LOG("[DEBUG OPTIMIZE] View offset adjusted (cursor above): " +
            std::to_string(original_view_offset) + " -> " + std::to_string(view_offset_row_));
    } else if (cursor_screen_pos >= screen_height) {
        // 光标在可见区域下方，仅当完全不可见时才调整
        // 精确调整到刚好可见，不使用任何buffer
        size_t max_offset =
            (total_lines > static_cast<size_t>(screen_height)) ? total_lines - screen_height : 0;
        view_offset_row_ = cursor_row_ - (screen_height - 1);
        if (view_offset_row_ > max_offset) {
            view_offset_row_ = max_offset;
        }
        LOG("[DEBUG OPTIMIZE] View offset adjusted (cursor below): " +
            std::to_string(original_view_offset) + " -> " + std::to_string(view_offset_row_));
    } else {
        // 如果光标已在可见范围内，绝对不调整视图 - 这是避免闪烁的关键
        LOG("[DEBUG OPTIMIZE] View offset unchanged (cursor in view)");
    }

    LOG("[DEBUG OPTIMIZE] Unified adjustment completed");
}

// 重做操作的状态调整（与撤销不同，重做可以更激进地调整视图）
void Editor::adjustCursorAndViewForRedo() {
    LOG("[DEBUG OPTIMIZE] Starting redo cursor and view adjustment");

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[DEBUG OPTIMIZE] No document available, skipping adjustment");
        return;
    }

    // 1. 调整光标位置（与adjustCursor()相同）
    size_t original_cursor_row = cursor_row_;
    size_t original_cursor_col = cursor_col_;

    size_t max_row = doc->lineCount() - 1;
    if (cursor_row_ > max_row) {
        cursor_row_ = max_row;
        LOG("[DEBUG OPTIMIZE] Cursor row adjusted: " + std::to_string(original_cursor_row) +
            " -> " + std::to_string(cursor_row_));
    }

    if (cursor_row_ < doc->lineCount()) {
        size_t max_col = doc->getLine(cursor_row_).length();
        if (cursor_col_ > max_col) {
            cursor_col_ = max_col;
            LOG("[DEBUG OPTIMIZE] Cursor col adjusted: " + std::to_string(original_cursor_col) +
                " -> " + std::to_string(cursor_col_));
        }
    }

    // 2. 重做操作的视图调整（可以更激进，因为重做通常是用户主动操作）
    size_t original_view_offset = view_offset_row_;

    int screen_height = screen_.dimy() - 4;
    if (screen_height > 0) {
        // 计算光标在屏幕上的位置
        int cursor_screen_pos = static_cast<int>(cursor_row_) - static_cast<int>(view_offset_row_);

        // 如果光标不在可见范围内，立即调整视图
        if (cursor_screen_pos < 0 || cursor_screen_pos >= screen_height) {
            // VSCode 行为：将光标行放在屏幕中央
            view_offset_row_ = (cursor_row_ >= static_cast<size_t>(screen_height / 2))
                                   ? cursor_row_ - screen_height / 2
                                   : 0;
            LOG("[DEBUG OPTIMIZE] View offset adjusted (cursor out of view): " +
                std::to_string(original_view_offset) + " -> " + std::to_string(view_offset_row_));
        } else {
            // 光标在可见范围内，但如果太靠近边缘，微调视图
            int margin = 3; // 边缘边距
            if (cursor_screen_pos < margin) {
                view_offset_row_ =
                    (cursor_row_ >= static_cast<size_t>(margin)) ? cursor_row_ - margin : 0;
                LOG("[DEBUG OPTIMIZE] View offset adjusted (cursor near top): " +
                    std::to_string(original_view_offset) + " -> " +
                    std::to_string(view_offset_row_));
            } else if (cursor_screen_pos >= screen_height - margin) {
                size_t target_offset = cursor_row_ - (screen_height - margin - 1);
                view_offset_row_ = (target_offset > cursor_row_) ? 0 : target_offset;
                LOG("[DEBUG OPTIMIZE] View offset adjusted (cursor near bottom): " +
                    std::to_string(original_view_offset) + " -> " +
                    std::to_string(view_offset_row_));
            } else {
                LOG("[DEBUG OPTIMIZE] View offset unchanged (cursor well positioned)");
            }
        }
    }

    LOG("[DEBUG OPTIMIZE] Redo adjustment completed");
}

// 高级撤销优化：完全静态撤销
// 预先调整视图，确保撤销后光标仍在可见区域，避免任何视觉跳跃
void Editor::prepareForStaticUndo(size_t change_row, size_t change_col) {
    (void)change_col; // 暂时未使用，消除编译警告

    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1;
    }

    size_t total_lines = doc->lineCount();
    if (total_lines == 0) {
        view_offset_row_ = 0;
        return;
    }

    // 计算撤销后的光标在当前视图中的位置
    int cursor_screen_pos = static_cast<int>(change_row) - static_cast<int>(view_offset_row_);

    // 预先调整视图，确保撤销后的光标位置在可见区域内
    const int margin = 2;

    if (cursor_screen_pos < margin) {
        // 光标会太靠近顶部，预先向上滚动
        size_t new_offset = (change_row >= static_cast<size_t>(margin)) ? change_row - margin : 0;
        if (new_offset != view_offset_row_) {
            view_offset_row_ = new_offset;
        }
    } else if (cursor_screen_pos >= screen_height - margin) {
        // 光标会太靠近底部，预先向下滚动
        size_t max_offset =
            (total_lines > static_cast<size_t>(screen_height)) ? total_lines - screen_height : 0;
        size_t new_offset = change_row - (screen_height - margin - 1);
        if (new_offset > max_offset) {
            new_offset = max_offset;
        }
        if (new_offset != view_offset_row_) {
            view_offset_row_ = new_offset;
        }
    }
}

// 执行静态撤销：只调整光标，不调整视图
void Editor::performStaticUndo(size_t change_row, size_t change_col) {
    LOG("[DEBUG STATIC] Performing static undo - setting cursor to (" + std::to_string(change_row) +
        ", " + std::to_string(change_col) + ")");

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[DEBUG STATIC] No document available");
        return;
    }

    // 只调整光标位置，不调整视图
    size_t original_cursor_row = cursor_row_;
    size_t original_cursor_col = cursor_col_;

    cursor_row_ = change_row;
    cursor_col_ = change_col;

    // 确保光标位置在有效范围内（但不调整视图）
    size_t max_row = doc->lineCount() - 1;
    if (cursor_row_ > max_row) {
        cursor_row_ = max_row;
    }

    if (cursor_row_ < doc->lineCount()) {
        size_t max_col = doc->getLine(cursor_row_).length();
        if (cursor_col_ > max_col) {
            cursor_col_ = max_col;
        }
    }

    // 清除选择状态
    selection_active_ = false;

    LOG("[DEBUG STATIC] Static undo completed - cursor: (" + std::to_string(original_cursor_row) +
        "," + std::to_string(original_cursor_col) + ") -> (" + std::to_string(cursor_row_) + "," +
        std::to_string(cursor_col_) + ")");
}

// 智能静态撤销：根据操作类型智能定位光标
void Editor::performSmartStaticUndo(size_t change_row, size_t change_col,
                                    DocumentChange::Type change_type) {
    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    // 根据操作类型智能定位光标
    switch (change_type) {
        case DocumentChange::Type::INSERT: {
            // 撤销插入操作：光标应该在插入内容的开始位置
            cursor_row_ = change_row;
            cursor_col_ = change_col;
            break;
        }

        case DocumentChange::Type::DELETE: {
            // 撤销删除操作：光标应该在删除内容的开始位置
            cursor_row_ = change_row;
            cursor_col_ = change_col;
            break;
        }

        case DocumentChange::Type::REPLACE: {
            // 撤销替换操作：光标应该在替换内容的开始位置
            cursor_row_ = change_row;
            cursor_col_ = change_col;
            break;
        }

        case DocumentChange::Type::NEWLINE: {
            // 撤销换行操作：光标应该在新行合并后的位置
            cursor_row_ = change_row;
            // 尝试找到一个合适的位置，比如单词边界或行中点
            size_t line_len = doc->getLine(cursor_row_).length();
            cursor_col_ = std::min(change_col, line_len);
            break;
        }

        default: {
            // 默认行为：使用传入的位置
            cursor_row_ = change_row;
            cursor_col_ = change_col;
            break;
        }
    }

    // 确保光标位置在有效范围内
    size_t max_row = doc->lineCount() - 1;
    if (cursor_row_ > max_row) {
        cursor_row_ = max_row;
    }

    if (cursor_row_ < doc->lineCount()) {
        size_t max_col = doc->getLine(cursor_row_).length();
        if (cursor_col_ > max_col) {
            cursor_col_ = max_col;
        }
    }

    // 清除选择状态
    selection_active_ = false;
}

// 预先为重做操作调整视图（重做可以更激进）
void Editor::prepareForStaticRedo(size_t change_row, size_t change_col) {
    LOG("[DEBUG STATIC] Preparing for static redo - target position: (" +
        std::to_string(change_row) + ", " + std::to_string(change_col) + ")");

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[DEBUG STATIC] No document available");
        return;
    }

    int screen_height = screen_.dimy() - 6;
    if (screen_height <= 0) {
        screen_height = 1;
    }

    size_t total_lines = doc->lineCount();
    if (total_lines == 0) {
        view_offset_row_ = 0;
        LOG("[DEBUG STATIC] Empty document, reset view offset to 0");
        return;
    }

    // 计算重做后的光标在当前视图中的位置
    int cursor_screen_pos = static_cast<int>(change_row) - static_cast<int>(view_offset_row_);

    // 重做操作可以使用更激进的视图调整策略，因为用户主动执行重做
    // 将光标放在屏幕中央，提供更好的视觉体验
    if (cursor_screen_pos < 0 || cursor_screen_pos >= screen_height) {
        // 光标不在可见区域，将其放在屏幕中央
        size_t target_center = change_row;
        size_t new_offset = (target_center >= static_cast<size_t>(screen_height / 2))
                                ? target_center - screen_height / 2
                                : 0;

        // 确保不超过最大偏移
        size_t max_offset =
            (total_lines > static_cast<size_t>(screen_height)) ? total_lines - screen_height : 0;
        if (new_offset > max_offset) {
            new_offset = max_offset;
        }

        if (new_offset != view_offset_row_) {
            LOG("[DEBUG STATIC] Pre-adjusting view offset for redo centering: " +
                std::to_string(view_offset_row_) + " -> " + std::to_string(new_offset));
            view_offset_row_ = new_offset;
        }
    } else {
        LOG("[DEBUG STATIC] No view adjustment needed for redo, cursor already visible");
    }

    LOG("[DEBUG STATIC] Static redo preparation completed");
}

// 执行静态重做：只调整光标，不调整视图
void Editor::performStaticRedo(size_t change_row, size_t change_col) {
    LOG("[DEBUG STATIC] Performing static redo - setting cursor to (" + std::to_string(change_row) +
        ", " + std::to_string(change_col) + ")");

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[DEBUG STATIC] No document available");
        return;
    }

    // 只调整光标位置，不调整视图
    size_t original_cursor_row = cursor_row_;
    size_t original_cursor_col = cursor_col_;

    cursor_row_ = change_row;
    cursor_col_ = change_col;

    // 确保光标位置在有效范围内（但不调整视图）
    size_t max_row = doc->lineCount() - 1;
    if (cursor_row_ > max_row) {
        cursor_row_ = max_row;
    }

    if (cursor_row_ < doc->lineCount()) {
        size_t max_col = doc->getLine(cursor_row_).length();
        if (cursor_col_ > max_col) {
            cursor_col_ = max_col;
        }
    }

    // 清除选择状态
    selection_active_ = false;

    LOG("[DEBUG STATIC] Static redo completed - cursor: (" + std::to_string(original_cursor_row) +
        "," + std::to_string(original_cursor_col) + ") -> (" + std::to_string(cursor_row_) + "," +
        std::to_string(cursor_col_) + ")");
}

} // namespace core
} // namespace pnana
