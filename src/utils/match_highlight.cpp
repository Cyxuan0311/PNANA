#include "utils/match_highlight.h"
#include <algorithm>
#include <cctype>
#include <vector>

namespace pnana {
namespace utils {

namespace {

std::string toLowerAscii(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        if (static_cast<unsigned char>(c) < 128)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

// 在 text_lower 中找 query_lower 的所有起始位置，合并重叠区间
std::vector<std::pair<size_t, size_t>> findMatchRanges(const std::string& text_lower,
                                                       const std::string& query_lower) {
    std::vector<std::pair<size_t, size_t>> ranges;
    if (query_lower.empty())
        return ranges;
    size_t pos = 0;
    const size_t qlen = query_lower.size();
    while (pos + qlen <= text_lower.size()) {
        size_t found = text_lower.find(query_lower, pos);
        if (found == std::string::npos)
            break;
        size_t end = found + qlen;
        if (!ranges.empty() && found <= ranges.back().second) {
            ranges.back().second = std::max(ranges.back().second, end);
        } else {
            ranges.emplace_back(found, end);
        }
        pos = found + 1;
    }
    return ranges;
}

} // namespace

ftxui::Element highlightMatch(const std::string& text, const std::string& query,
                              ftxui::Color default_color, ftxui::Color highlight_color) {
    using namespace ftxui;
    if (query.empty())
        return ftxui::text(text) | color(default_color);

    std::string text_lower = toLowerAscii(text);
    std::string query_lower = toLowerAscii(query);
    auto ranges = findMatchRanges(text_lower, query_lower);
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
