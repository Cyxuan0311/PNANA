#include "features/search.h"
#include <algorithm>
#include <cctype>

namespace pnana {
namespace features {

SearchEngine::SearchEngine() : current_match_index_(0) {}

void SearchEngine::search(const std::string& pattern, const std::vector<std::string>& lines,
                          const SearchOptions& options) {
    pattern_ = pattern;
    options_ = options;
    matches_.clear();
    current_match_index_ = 0;

    if (pattern.empty()) {
        return;
    }

    if (options.regex) {
        searchRegex(pattern, lines);
    } else if (options.whole_word) {
        searchWholeWord(pattern, lines);
    } else {
        searchLiteral(pattern, lines);
    }
}

void SearchEngine::searchLiteral(const std::string& pattern,
                                 const std::vector<std::string>& lines) {
    std::string search_pattern = pattern;

    for (size_t line_num = 0; line_num < lines.size(); ++line_num) {
        std::string line = lines[line_num];

        // 转换为小写（如果不区分大小写）
        if (!options_.case_sensitive) {
            std::transform(search_pattern.begin(), search_pattern.end(), search_pattern.begin(),
                           ::tolower);
            std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        }

        size_t pos = 0;
        while ((pos = line.find(search_pattern, pos)) != std::string::npos) {
            matches_.emplace_back(line_num, pos, pattern.length());
            pos += pattern.length();
        }
    }
}

void SearchEngine::searchRegex(const std::string& pattern, const std::vector<std::string>& lines) {
    try {
        std::regex::flag_type flags = std::regex::ECMAScript;
        if (!options_.case_sensitive) {
            flags |= std::regex::icase;
        }

        std::regex regex(pattern, flags);

        for (size_t line_num = 0; line_num < lines.size(); ++line_num) {
            const std::string& line = lines[line_num];

            auto words_begin = std::sregex_iterator(line.begin(), line.end(), regex);
            auto words_end = std::sregex_iterator();

            for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                std::smatch match = *i;
                matches_.emplace_back(line_num, match.position(), match.length());
            }
        }
    } catch (const std::regex_error&) {
        // 正则表达式错误，回退到字面搜索
        searchLiteral(pattern, lines);
    }
}

void SearchEngine::searchWholeWord(const std::string& pattern,
                                   const std::vector<std::string>& lines) {
    for (size_t line_num = 0; line_num < lines.size(); ++line_num) {
        std::string line = lines[line_num];
        std::string search_pattern = pattern;

        if (!options_.case_sensitive) {
            std::transform(search_pattern.begin(), search_pattern.end(), search_pattern.begin(),
                           ::tolower);
            std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        }

        size_t pos = 0;
        while ((pos = line.find(search_pattern, pos)) != std::string::npos) {
            // 检查是否是完整单词
            bool is_word_start = (pos == 0) || !std::isalnum(line[pos - 1]);
            bool is_word_end = (pos + search_pattern.length() >= line.length()) ||
                               !std::isalnum(line[pos + search_pattern.length()]);

            if (is_word_start && is_word_end) {
                matches_.emplace_back(line_num, pos, pattern.length());
            }

            pos += pattern.length();
        }
    }
}

bool SearchEngine::findNext() {
    if (matches_.empty()) {
        return false;
    }

    current_match_index_ = (current_match_index_ + 1) % matches_.size();
    return true;
}

bool SearchEngine::findPrevious() {
    if (matches_.empty()) {
        return false;
    }

    if (current_match_index_ == 0) {
        current_match_index_ = matches_.size() - 1;
    } else {
        current_match_index_--;
    }
    return true;
}

bool SearchEngine::jumpToMatch(size_t index) {
    if (index >= matches_.size()) {
        return false;
    }

    current_match_index_ = index;
    return true;
}

bool SearchEngine::replaceCurrentMatch(const std::string& replacement,
                                       std::vector<std::string>& lines) {
    if (matches_.empty() || current_match_index_ >= matches_.size()) {
        return false;
    }

    const SearchMatch& match = matches_[current_match_index_];

    if (match.line >= lines.size()) {
        return false;
    }

    std::string& line = lines[match.line];
    if (match.column + match.length > line.length()) {
        return false;
    }

    line.replace(match.column, match.length, replacement);

    // 移除当前匹配
    matches_.erase(matches_.begin() + current_match_index_);

    // 调整当前索引
    if (current_match_index_ >= matches_.size() && !matches_.empty()) {
        current_match_index_ = matches_.size() - 1;
    }

    return true;
}

size_t SearchEngine::replaceAll(const std::string& replacement, std::vector<std::string>& lines) {
    size_t count = 0;

    // 从后向前替换，避免位置偏移问题
    for (auto it = matches_.rbegin(); it != matches_.rend(); ++it) {
        const SearchMatch& match = *it;

        if (match.line >= lines.size()) {
            continue;
        }

        std::string& line = lines[match.line];
        if (match.column + match.length > line.length()) {
            continue;
        }

        line.replace(match.column, match.length, replacement);
        count++;
    }

    matches_.clear();
    current_match_index_ = 0;

    return count;
}

const SearchMatch* SearchEngine::getCurrentMatch() const {
    if (matches_.empty() || current_match_index_ >= matches_.size()) {
        return nullptr;
    }
    return &matches_[current_match_index_];
}

void SearchEngine::clearSearch() {
    pattern_.clear();
    matches_.clear();
    current_match_index_ = 0;
}

bool SearchEngine::isHighlightPosition(size_t line, size_t col) const {
    for (const auto& match : matches_) {
        if (match.line == line && col >= match.column && col < match.column + match.length) {
            return true;
        }
    }
    return false;
}

} // namespace features
} // namespace pnana
