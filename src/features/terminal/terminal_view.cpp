#ifdef BUILD_LIBVTERM_SUPPORT

#include "features/terminal/terminal_view.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace features {
namespace terminal {

namespace {

ftxui::Element cellToElement(const TerminalCell& cell, const ftxui::Color& default_fg,
                             const ftxui::Color& default_bg, bool is_cursor) {
    ftxui::Color fg = default_fg;
    ftxui::Color bg = default_bg;

    if (!cell.fg_default)
        fg = ftxui::Color::RGB(cell.fg_r, cell.fg_g, cell.fg_b);
    if (!cell.bg_default)
        bg = ftxui::Color::RGB(cell.bg_r, cell.bg_g, cell.bg_b);

    auto elem = ftxui::text(cell.text.empty() ? " " : cell.text);
    if (fg != default_fg)
        elem = elem | ftxui::color(fg);
    if (bg != default_bg)
        elem = elem | ftxui::bgcolor(bg);
    if (cell.bold)
        elem = elem | ftxui::bold;
    if (cell.underline)
        elem = elem | ftxui::underlined;
    if (cell.reverse && !is_cursor)
        elem = elem | ftxui::inverted;
    if (cell.strike)
        elem = elem | ftxui::strikethrough;

    if (is_cursor) {
        elem = elem | ftxui::inverted;
    }

    return elem;
}

} // namespace

ftxui::Element renderScreenSnapshot(const ScreenSnapshot& snap, int height,
                                    const ftxui::Color& default_fg,
                                    const ftxui::Color& default_bg) {
    using namespace ftxui;

    Elements rows;
    int row_count = 0;
    for (const auto& line : snap.visible) {
        if (height > 0 && row_count >= height)
            break;
        Elements cells;
        int col = 0;
        for (size_t c = 0; c < line.size(); c++) {
            const auto& cell = line[c];
            if (cell.width == 0) {
                continue;
            }
            bool is_cursor =
                (snap.cursor_visible && row_count == snap.cursor_row && col == snap.cursor_col);
            cells.push_back(cellToElement(cell, default_fg, default_bg, is_cursor));
            col += cell.width;
        }
        if (!cells.empty())
            rows.push_back(hbox(std::move(cells)));
        else
            rows.push_back(text(" "));
        row_count++;
    }

    while (height > 0 && static_cast<int>(rows.size()) < height) {
        rows.push_back(text(" "));
    }

    return vbox(std::move(rows)) | flex | size(HEIGHT, EQUAL, height > 0 ? height : 1) |
           bgcolor(default_bg);
}

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // BUILD_LIBVTERM_SUPPORT
