#pragma once

#include <algorithm>
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace ui {

inline int responsiveWidth(int preferred, int min_width = 30) {
    int term_w = ftxui::Terminal::Size().dimx;
    int margin = 4;
    int available = term_w - margin;
    return std::max(min_width, std::min(preferred, available));
}

inline int responsiveHeight(int preferred, int min_height = 8) {
    int term_h = ftxui::Terminal::Size().dimy;
    int margin = 4;
    int available = term_h - margin;
    return std::max(min_height, std::min(preferred, available));
}

inline int responsiveMinWidth(int min_width) {
    int term_w = ftxui::Terminal::Size().dimx;
    int margin = 4;
    int available = term_w - margin;
    return std::min(min_width, available);
}

inline int responsiveMinHeight(int min_height) {
    int term_h = ftxui::Terminal::Size().dimy;
    int margin = 4;
    int available = term_h - margin;
    return std::min(min_height, available);
}

inline int responsiveSubWidth(int preferred, int parent_width, int min_width = 10) {
    int clamped_parent = responsiveWidth(parent_width, min_width);
    if (parent_width <= 0)
        return preferred;
    int result = preferred * clamped_parent / parent_width;
    return std::max(min_width, result);
}

inline int responsiveSubWidth(int preferred, int original_parent, int actual_parent,
                              int min_width) {
    if (original_parent <= 0)
        return preferred;
    int result = preferred * actual_parent / original_parent;
    return std::max(min_width, result);
}

} // namespace ui
} // namespace pnana
