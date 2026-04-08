#ifndef PNANA_UTILS_BRACKET_MATCHER_H
#define PNANA_UTILS_BRACKET_MATCHER_H

#include <optional>
#include <string>
#include <vector>

namespace pnana {
namespace utils {

struct BracketPosition {
    size_t line = 0;
    size_t column = 0;
};

struct BracketMatchResult {
    BracketPosition current;
    BracketPosition matched;
};

// 在给定光标位置查找括号匹配。
// 仅在光标所在字符是 ()[]{} 之一时返回结果；否则返回 nullopt。
// 为避免大文件卡顿，扫描字符数量有上限（max_scan_chars）。
std::optional<BracketMatchResult> findMatchingBracket(const std::vector<std::string>& lines,
                                                      size_t cursor_line, size_t cursor_col,
                                                      size_t max_scan_chars = 200000);

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_BRACKET_MATCHER_H
