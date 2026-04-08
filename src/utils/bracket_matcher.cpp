#include "utils/bracket_matcher.h"

namespace pnana {
namespace utils {
namespace {

inline bool isOpeningBracket(char c) {
    return c == '(' || c == '[' || c == '{';
}

inline bool isClosingBracket(char c) {
    return c == ')' || c == ']' || c == '}';
}

inline char matchingBracket(char c) {
    switch (c) {
        case '(':
            return ')';
        case '[':
            return ']';
        case '{':
            return '}';
        case ')':
            return '(';
        case ']':
            return '[';
        case '}':
            return '{';
        default:
            return '\0';
    }
}

} // namespace

std::optional<BracketMatchResult> findMatchingBracket(const std::vector<std::string>& lines,
                                                      size_t cursor_line, size_t cursor_col,
                                                      size_t max_scan_chars) {
    if (cursor_line >= lines.size()) {
        return std::nullopt;
    }

    const std::string& line = lines[cursor_line];
    if (cursor_col >= line.size()) {
        return std::nullopt;
    }

    const char current_char = line[cursor_col];
    if (!isOpeningBracket(current_char) && !isClosingBracket(current_char)) {
        return std::nullopt;
    }

    const char target = matchingBracket(current_char);
    if (target == '\0') {
        return std::nullopt;
    }

    BracketMatchResult result;
    result.current = {cursor_line, cursor_col};

    size_t scanned = 0;
    int depth = 0;

    if (isOpeningBracket(current_char)) {
        for (size_t l = cursor_line; l < lines.size(); ++l) {
            const std::string& cur = lines[l];
            size_t c = (l == cursor_line) ? cursor_col + 1 : 0;
            for (; c < cur.size(); ++c) {
                const char ch = cur[c];
                ++scanned;
                if (scanned > max_scan_chars) {
                    return std::nullopt;
                }
                if (ch == current_char) {
                    ++depth;
                } else if (ch == target) {
                    if (depth == 0) {
                        result.matched = {l, c};
                        return result;
                    }
                    --depth;
                }
            }
        }
    } else {
        for (size_t l = cursor_line + 1; l-- > 0;) {
            const std::string& cur = lines[l];
            size_t start = cur.size();
            if (l == cursor_line) {
                start = cursor_col;
            }
            for (size_t c = start; c-- > 0;) {
                const char ch = cur[c];
                ++scanned;
                if (scanned > max_scan_chars) {
                    return std::nullopt;
                }
                if (ch == current_char) {
                    ++depth;
                } else if (ch == target) {
                    if (depth == 0) {
                        result.matched = {l, c};
                        return result;
                    }
                    --depth;
                }
            }
            if (l == 0) {
                break;
            }
        }
    }

    return std::nullopt;
}

} // namespace utils
} // namespace pnana
