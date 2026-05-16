#include "utils/match_highlight.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace pnana {
namespace utils {

namespace {

inline unsigned char toLowerChar(unsigned char c) {
    return static_cast<unsigned char>(std::tolower(c));
}

// KMP 前缀函数（部分匹配表）
std::vector<int> computeLPS(const std::string& pattern) {
    const int m = static_cast<int>(pattern.size());
    std::vector<int> lps(m, 0);
    int len = 0;
    int i = 1;
    while (i < m) {
        if (toLowerChar(static_cast<unsigned char>(pattern[i])) ==
            toLowerChar(static_cast<unsigned char>(pattern[len]))) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
    return lps;
}

// KMP 搜索：返回所有匹配的起始位置
std::vector<size_t> kmpSearch(const std::string& text, const std::string& pattern) {
    std::vector<size_t> positions;
    const int n = static_cast<int>(text.size());
    const int m = static_cast<int>(pattern.size());
    if (m == 0 || n < m)
        return positions;

    std::vector<int> lps = computeLPS(pattern);

    int i = 0; // text 索引
    int j = 0; // pattern 索引
    while (i < n) {
        if (toLowerChar(static_cast<unsigned char>(pattern[j])) ==
            toLowerChar(static_cast<unsigned char>(text[i]))) {
            i++;
            j++;
        }
        if (j == m) {
            positions.push_back(static_cast<size_t>(i - j));
            j = lps[j - 1];
        } else if (i < n && toLowerChar(static_cast<unsigned char>(pattern[j])) !=
                                toLowerChar(static_cast<unsigned char>(text[i]))) {
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }
    return positions;
}

// 合并重叠区间
std::vector<std::pair<size_t, size_t>> mergeRanges(std::vector<size_t>&& positions,
                                                   size_t pattern_len) {
    std::vector<std::pair<size_t, size_t>> ranges;
    if (positions.empty())
        return ranges;
    for (size_t pos : positions) {
        size_t end = pos + pattern_len;
        if (!ranges.empty() && pos <= ranges.back().second) {
            ranges.back().second = std::max(ranges.back().second, end);
        } else {
            ranges.emplace_back(pos, end);
        }
    }
    return ranges;
}

} // namespace

ftxui::Element highlightMatch(const std::string& text, const std::string& query,
                              ftxui::Color default_color, ftxui::Color highlight_color) {
    using namespace ftxui;
    if (query.empty())
        return ftxui::text(text) | color(default_color);

    auto positions = kmpSearch(text, query);
    if (positions.empty())
        return ftxui::text(text) | color(default_color);

    auto ranges = mergeRanges(std::move(positions), query.size());
    if (ranges.empty())
        return ftxui::text(text) | color(default_color);

    Elements parts;
    size_t pos = 0;
    for (const auto& [start, end] : ranges) {
        if (pos < start)
            parts.push_back(ftxui::text(text.substr(pos, start - pos)) | color(default_color));
        parts.push_back(ftxui::text(text.substr(start, end - start)) | color(highlight_color));
        pos = end;
    }
    if (pos < text.size())
        parts.push_back(ftxui::text(text.substr(pos)) | color(default_color));

    return hbox(std::move(parts));
}

} // namespace utils
} // namespace pnana
