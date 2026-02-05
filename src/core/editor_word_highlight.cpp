// 单词高亮相关实现
#include "core/editor.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>

namespace pnana {
namespace core {

// 获取光标位置的单词
std::string Editor::getWordAtCursor() const {
    const Document* doc = getCurrentDocument();
    if (!doc) {
        return "";
    }

    if (cursor_row_ >= doc->lineCount()) {
        return "";
    }

    const std::string& line = doc->getLine(cursor_row_);
    if (cursor_col_ >= line.length()) {
        return "";
    }

    // 找到单词的开始位置
    size_t start = cursor_col_;
    while (start > 0) {
        char c = line[start - 1];
        // 支持字母、数字、下划线（标识符字符）
        if (std::isalnum(c) || c == '_') {
            start--;
        } else {
            break;
        }
    }

    // 找到单词的结束位置
    size_t end = cursor_col_;
    while (end < line.length()) {
        char c = line[end];
        // 支持字母、数字、下划线（标识符字符）
        if (std::isalnum(c) || c == '_') {
            end++;
        } else {
            break;
        }
    }

    // 如果光标不在单词内（在单词边界或非单词字符上），返回空字符串
    if (start == end) {
        return "";
    }

    return line.substr(start, end - start);
}

// 更新单词高亮
void Editor::updateWordHighlight() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        clearWordHighlight();
        return;
    }

    // 如果搜索高亮激活，不显示单词高亮
    if (search_highlight_active_) {
        clearWordHighlight();
        return;
    }

    // 获取光标位置的单词
    std::string word = getWordAtCursor();

    // 如果光标不在单词上，清除高亮
    if (word.empty()) {
        clearWordHighlight();
        return;
    }

    // 如果单词没有变化，不需要重新搜索
    if (word_highlight_active_ && current_word_ == word && word_highlight_row_ == cursor_row_ &&
        word_highlight_col_ == cursor_col_) {
        return;
    }

    // 更新当前单词信息
    current_word_ = word;
    word_highlight_row_ = cursor_row_;

    // 找到单词的起始列
    const std::string& line = doc->getLine(cursor_row_);
    size_t start_col = cursor_col_;
    while (start_col > 0) {
        char c = line[start_col - 1];
        if (std::isalnum(c) || c == '_') {
            start_col--;
        } else {
            break;
        }
    }
    word_highlight_col_ = start_col;

    // 搜索文件中所有相同的单词（大小写敏感，整词匹配）
    word_matches_.clear();
    const auto& lines = doc->getLines();

    for (size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
        const std::string& current_line = lines[line_idx];
        size_t pos = 0;

        while (pos < current_line.length()) {
            // 查找单词的开始位置
            size_t word_start = current_line.find(word, pos);
            if (word_start == std::string::npos) {
                break;
            }

            // 检查是否是整词匹配（前后都不是标识符字符）
            bool is_whole_word = true;
            if (word_start > 0) {
                char before = current_line[word_start - 1];
                if (std::isalnum(before) || before == '_') {
                    is_whole_word = false;
                }
            }
            if (word_start + word.length() < current_line.length()) {
                char after = current_line[word_start + word.length()];
                if (std::isalnum(after) || after == '_') {
                    is_whole_word = false;
                }
            }

            if (is_whole_word) {
                word_matches_.emplace_back(line_idx, word_start, word.length());
            }

            pos = word_start + 1; // 继续搜索
        }
    }

    // 激活单词高亮
    word_highlight_active_ = !word_matches_.empty();
}

// 清除单词高亮
void Editor::clearWordHighlight() {
    word_highlight_active_ = false;
    current_word_.clear();
    word_matches_.clear();
    word_highlight_row_ = 0;
    word_highlight_col_ = 0;
}

} // namespace core
} // namespace pnana
