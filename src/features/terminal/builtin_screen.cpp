#include "features/terminal/builtin_screen.h"
#include <algorithm>
#include <cstring>

namespace pnana {
namespace features {
namespace terminal {

int builtinCharWidth(uint32_t cp) {
    if (cp < 0x80)
        return 1;
    if (cp < 0x300)
        return 1;
    if (cp >= 0x1100 && cp <= 0x115F)
        return 2;
    if (cp >= 0x2329 && cp <= 0x232A)
        return 2;
    if (cp >= 0x2E80 && cp <= 0x303E)
        return 2;
    if (cp >= 0x3040 && cp <= 0x33BF)
        return 2;
    if (cp >= 0x3400 && cp <= 0x4DBF)
        return 2;
    if (cp >= 0x4E00 && cp <= 0xA4CF)
        return 2;
    if (cp >= 0xA960 && cp <= 0xA97F)
        return 2;
    if (cp >= 0xAC00 && cp <= 0xD7AF)
        return 2;
    if (cp >= 0xF900 && cp <= 0xFAFF)
        return 2;
    if (cp >= 0xFE10 && cp <= 0xFE1F)
        return 2;
    if (cp >= 0xFE30 && cp <= 0xFE6F)
        return 2;
    if (cp >= 0xFF01 && cp <= 0xFF60)
        return 2;
    if (cp >= 0xFFE0 && cp <= 0xFFE6)
        return 2;
    if (cp >= 0x1F000 && cp <= 0x1F9FF)
        return 2;
    if (cp >= 0x20000 && cp <= 0x2FFFD)
        return 2;
    if (cp >= 0x30000 && cp <= 0x3FFFD)
        return 2;
    return 1;
}

bool BuiltinScreen::init(int cols, int rows) {
    cols_ = cols;
    rows_ = rows;
    cells_.resize(static_cast<size_t>(rows * cols));
    for (auto& c : cells_)
        c.clear();

    dirty_words_ = (rows + 63) / 64;
    dirty_bits_.resize(static_cast<size_t>(dirty_words_), 0);

    tab_stops.resize(static_cast<size_t>(cols), 0);
    for (int x = 0; x < cols; x++)
        tab_stops[static_cast<size_t>(x)] = (x % 8 == 0) ? 1 : 0;

    cursor_visible = 1;
    cursor_saved_visible = 1;
    scroll_bottom = rows - 1;
    cur_fg = BUILTIN_COLOR_DEFAULT;
    cur_bg = BUILTIN_COLOR_DEFAULT;
    saved_fg = BUILTIN_COLOR_DEFAULT;
    saved_bg = BUILTIN_COLOR_DEFAULT;
    auto_wrap = 1;
    return true;
}

void BuiltinScreen::resize(int new_cols, int new_rows) {
    if (new_cols <= 0 || new_rows <= 0)
        return;

    auto resizeBuffer = [](std::vector<BuiltinCell>& buf, int old_cols, int old_rows, int new_cols,
                           int new_rows) {
        std::vector<BuiltinCell> new_cells(static_cast<size_t>(new_rows * new_cols));
        for (auto& c : new_cells)
            c.clear();

        int copy_rows = std::min(old_rows, new_rows);
        int copy_cols = std::min(old_cols, new_cols);
        for (int y = 0; y < copy_rows; y++) {
            std::memcpy(&new_cells[static_cast<size_t>(y * new_cols)],
                        &buf[static_cast<size_t>(y * old_cols)],
                        static_cast<size_t>(copy_cols) * sizeof(BuiltinCell));
        }
        buf = std::move(new_cells);
    };

    int old_cols = cols_;
    int old_rows = rows_;

    if (use_alt_screen) {
        resizeBuffer(alt_cells_, old_cols, old_rows, new_cols, new_rows);
        resizeBuffer(saved_main_cells_, old_cols, old_rows, new_cols, new_rows);
    } else {
        resizeBuffer(cells_, old_cols, old_rows, new_cols, new_rows);
    }

    cols_ = new_cols;
    rows_ = new_rows;

    if (cursor_x >= new_cols)
        cursor_x = new_cols - 1;
    if (cursor_y >= new_rows)
        cursor_y = new_rows - 1;
    if (cursor_saved_x >= new_cols)
        cursor_saved_x = new_cols - 1;
    if (cursor_saved_y >= new_rows)
        cursor_saved_y = new_rows - 1;
    if (saved_main_cursor_x_ >= new_cols)
        saved_main_cursor_x_ = new_cols - 1;
    if (saved_main_cursor_y_ >= new_rows)
        saved_main_cursor_y_ = new_rows - 1;
    scroll_bottom = new_rows - 1;

    dirty_words_ = (new_rows + 63) / 64;
    dirty_bits_.assign(static_cast<size_t>(dirty_words_), 0);

    tab_stops.resize(static_cast<size_t>(new_cols), 0);
    for (int x = 0; x < new_cols; x++)
        tab_stops[static_cast<size_t>(x)] = (x % 8 == 0) ? 1 : 0;

    markDirtyRange(0, new_rows - 1);
}

BuiltinCell* BuiltinScreen::cell(int x, int y) {
    if (x < 0 || x >= cols_ || y < 0 || y >= rows_)
        return nullptr;
    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return nullptr;
    return &buf[static_cast<size_t>(y * cols_ + x)];
}

const BuiltinCell* BuiltinScreen::cell(int x, int y) const {
    if (x < 0 || x >= cols_ || y < 0 || y >= rows_)
        return nullptr;
    const auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return nullptr;
    return &buf[static_cast<size_t>(y * cols_ + x)];
}

void BuiltinScreen::markDirty(int y) {
    if (y < 0 || y >= rows_)
        return;
    dirty_bits_[static_cast<size_t>(y / 64)] |= (static_cast<uint64_t>(1) << (y % 64));
}

void BuiltinScreen::markDirtyRange(int y1, int y2) {
    if (y1 < 0)
        y1 = 0;
    if (y2 >= rows_)
        y2 = rows_ - 1;
    for (int y = y1; y <= y2; y++)
        markDirty(y);
}

void BuiltinScreen::clearAllDirty() {
    std::memset(dirty_bits_.data(), 0, static_cast<size_t>(dirty_words_) * sizeof(uint64_t));
}

bool BuiltinScreen::isRowDirty(int y) const {
    if (y < 0 || y >= rows_)
        return false;
    return (dirty_bits_[static_cast<size_t>(y / 64)] & (static_cast<uint64_t>(1) << (y % 64))) != 0;
}

void BuiltinScreen::scrollUp(int n) {
    int top = scroll_top;
    int bot = scroll_bottom;
    int region_h = bot - top + 1;
    if (n > region_h)
        n = region_h;
    if (n <= 0)
        return;

    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    for (int y = top; y <= bot - n; y++) {
        std::memmove(&buf[static_cast<size_t>(y * cols_)],
                     &buf[static_cast<size_t>((y + n) * cols_)],
                     static_cast<size_t>(cols_) * sizeof(BuiltinCell));
    }
    for (int y = bot - n + 1; y <= bot; y++) {
        for (int x = 0; x < cols_; x++) {
            buf[static_cast<size_t>(y * cols_ + x)].erase(cur_fg, cur_bg, cur_flags);
        }
    }
    markDirtyRange(top, bot);
}

void BuiltinScreen::scrollDown(int n) {
    int top = scroll_top;
    int bot = scroll_bottom;
    int region_h = bot - top + 1;
    if (n > region_h)
        n = region_h;
    if (n <= 0)
        return;

    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    for (int y = bot; y >= top + n; y--) {
        std::memcpy(&buf[static_cast<size_t>(y * cols_)],
                    &buf[static_cast<size_t>((y - n) * cols_)],
                    static_cast<size_t>(cols_) * sizeof(BuiltinCell));
    }
    for (int y = top; y < top + n; y++) {
        for (int x = 0; x < cols_; x++) {
            buf[static_cast<size_t>(y * cols_ + x)].erase(cur_fg, cur_bg, cur_flags);
        }
    }
    markDirtyRange(top, bot);
}

void BuiltinScreen::lineFeed() {
    if (cursor_y == scroll_bottom) {
        if (scroll_top == 0) {
            appendScrollbackLine(&cells_[static_cast<size_t>(cursor_y * cols_)], cols_);
        }
        scrollUp(1);
    } else {
        cursor_y++;
    }
}

void BuiltinScreen::reverseIndex() {
    pending_wrap = 0;
    if (cursor_y == scroll_top) {
        scrollDown(1);
    } else {
        cursor_y--;
    }
}

void BuiltinScreen::putChar(uint32_t cp, int width) {
    if (pending_wrap) {
        cursor_x = 0;
        lineFeed();
        pending_wrap = 0;
    }
    if (cursor_x >= cols_) {
        if (auto_wrap) {
            cursor_x = 0;
            lineFeed();
        } else {
            cursor_x = cols_ - 1;
        }
    }

    BuiltinCell* c = cell(cursor_x, cursor_y);
    if (c) {
        if (c->width == 0 && cursor_x > 0) {
            BuiltinCell* prev = cell(cursor_x - 1, cursor_y);
            if (prev && prev->width > 1) {
                prev->erase(cur_fg, cur_bg, cur_flags);
            }
        }
        if (c->width > 1 && cursor_x + 1 < cols_) {
            BuiltinCell* next = cell(cursor_x + 1, cursor_y);
            if (next && next->width == 0) {
                next->erase(cur_fg, cur_bg, cur_flags);
            }
        }
        c->codepoint = cp;
        c->fg_color = cur_fg;
        c->bg_color = cur_bg;
        c->flags = cur_flags;
        c->width = static_cast<uint8_t>(width);
    }
    if (width > 1 && cursor_x + 1 < cols_) {
        BuiltinCell* next = cell(cursor_x + 1, cursor_y);
        if (next) {
            next->codepoint = 0;
            next->fg_color = cur_fg;
            next->bg_color = cur_bg;
            next->flags = cur_flags;
            next->width = 0;
        }
    }
    markDirty(cursor_y);
    cursor_x += width;
    if (cursor_x >= cols_) {
        pending_wrap = 1;
    }
}

void BuiltinScreen::eraseCells(int x1, int y1, int x2, int y2) {
    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            if (x < 0 || x >= cols_ || y < 0 || y >= rows_)
                continue;
            BuiltinCell& c = buf[static_cast<size_t>(y * cols_ + x)];
            if (c.width > 1 && x + 1 < cols_) {
                BuiltinCell& next = buf[static_cast<size_t>(y * cols_ + x + 1)];
                if (next.width == 0)
                    next.erase(cur_fg, cur_bg, cur_flags);
            }
            if (c.width == 0 && x > 0) {
                BuiltinCell& prev = buf[static_cast<size_t>(y * cols_ + x - 1)];
                if (prev.width > 1)
                    prev.erase(cur_fg, cur_bg, cur_flags);
            }
            c.erase(cur_fg, cur_bg, cur_flags);
        }
        markDirty(y);
    }
}

void BuiltinScreen::eraseDisplay(int mode) {
    int cx = cursor_x, cy = cursor_y;
    switch (mode) {
        case 0:
            eraseCells(cx, cy, cols_ - 1, cy);
            if (cy + 1 < rows_)
                eraseCells(0, cy + 1, cols_ - 1, rows_ - 1);
            break;
        case 1:
            if (cy > 0)
                eraseCells(0, 0, cols_ - 1, cy - 1);
            eraseCells(0, cy, cx, cy);
            break;
        case 2:
        case 3:
            eraseCells(0, 0, cols_ - 1, rows_ - 1);
            break;
    }
}

void BuiltinScreen::eraseLine(int mode) {
    int cx = cursor_x, cy = cursor_y;
    switch (mode) {
        case 0:
            eraseCells(cx, cy, cols_ - 1, cy);
            break;
        case 1:
            eraseCells(0, cy, cx, cy);
            break;
        case 2:
            eraseCells(0, cy, cols_ - 1, cy);
            break;
    }
}

void BuiltinScreen::insertLines(int n) {
    int top = cursor_y;
    int bot = scroll_bottom;
    if (top > bot)
        return;
    int region_h = bot - top + 1;
    if (n > region_h)
        n = region_h;
    if (n <= 0)
        return;

    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    for (int y = bot; y >= top + n; y--) {
        std::memcpy(&buf[static_cast<size_t>(y * cols_)],
                    &buf[static_cast<size_t>((y - n) * cols_)],
                    static_cast<size_t>(cols_) * sizeof(BuiltinCell));
    }
    for (int y = top; y < top + n; y++) {
        for (int x = 0; x < cols_; x++) {
            buf[static_cast<size_t>(y * cols_ + x)].erase(cur_fg, cur_bg, cur_flags);
        }
    }
    markDirtyRange(top, bot);
}

void BuiltinScreen::deleteLines(int n) {
    int top = cursor_y;
    int bot = scroll_bottom;
    if (top > bot)
        return;
    int region_h = bot - top + 1;
    if (n > region_h)
        n = region_h;
    if (n <= 0)
        return;

    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    for (int y = top; y <= bot - n; y++) {
        std::memcpy(&buf[static_cast<size_t>(y * cols_)],
                    &buf[static_cast<size_t>((y + n) * cols_)],
                    static_cast<size_t>(cols_) * sizeof(BuiltinCell));
    }
    for (int y = bot - n + 1; y <= bot; y++) {
        for (int x = 0; x < cols_; x++) {
            buf[static_cast<size_t>(y * cols_ + x)].erase(cur_fg, cur_bg, cur_flags);
        }
    }
    markDirtyRange(top, bot);
}

void BuiltinScreen::deleteChars(int n) {
    int y = cursor_y;
    if (n > cols_ - cursor_x)
        n = cols_ - cursor_x;
    if (n <= 0)
        return;

    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    BuiltinCell* row = &buf[static_cast<size_t>(y * cols_)];
    if (cursor_x > 0 && row[cursor_x].width == 0) {
        row[cursor_x - 1].erase(cur_fg, cur_bg, cur_flags);
    }
    int move_count = cols_ - cursor_x - n;
    if (move_count > 0) {
        std::memmove(&row[cursor_x], &row[cursor_x + n],
                     static_cast<size_t>(move_count) * sizeof(BuiltinCell));
    }
    for (int x = cols_ - n; x < cols_; x++) {
        row[x].erase(cur_fg, cur_bg, cur_flags);
    }
    markDirty(y);
}

void BuiltinScreen::insertChars(int n) {
    int y = cursor_y;
    if (n > cols_ - cursor_x)
        n = cols_ - cursor_x;
    if (n <= 0)
        return;

    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    BuiltinCell* row = &buf[static_cast<size_t>(y * cols_)];
    if (cursor_x > 0 && row[cursor_x].width == 0) {
        row[cursor_x - 1].erase(cur_fg, cur_bg, cur_flags);
    }
    int move_count = cols_ - cursor_x - n;
    if (move_count > 0) {
        std::memmove(&row[cursor_x + n], &row[cursor_x],
                     static_cast<size_t>(move_count) * sizeof(BuiltinCell));
    }
    for (int x = cursor_x; x < cursor_x + n; x++) {
        row[x].erase(cur_fg, cur_bg, cur_flags);
    }
    markDirty(y);
}

void BuiltinScreen::eraseChars(int n) {
    int y = cursor_y;
    if (n > cols_ - cursor_x)
        n = cols_ - cursor_x;

    auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return;

    BuiltinCell* row = &buf[static_cast<size_t>(y * cols_)];
    if (cursor_x > 0 && row[cursor_x].width == 0) {
        row[cursor_x - 1].erase(cur_fg, cur_bg, cur_flags);
    }
    for (int x = cursor_x; x < cursor_x + n; x++) {
        if (row[x].width > 1 && x + 1 < cols_ && row[x + 1].width == 0) {
            row[x + 1].erase(cur_fg, cur_bg, cur_flags);
        }
        row[x].erase(cur_fg, cur_bg, cur_flags);
    }
    markDirty(y);
}

void BuiltinScreen::setScrollingRegion(int top, int bot) {
    if (top < 0)
        top = 0;
    if (bot >= rows_)
        bot = rows_ - 1;
    if (top > bot)
        return;
    scroll_top = top;
    scroll_bottom = bot;
    cursor_x = 0;
    cursor_y = 0;
}

void BuiltinScreen::switchAltScreen(bool enable) {
    if (enable == (use_alt_screen != 0))
        return;

    if (enable) {
        saved_main_cells_ = cells_;
        saved_main_cursor_x_ = cursor_x;
        saved_main_cursor_y_ = cursor_y;

        alt_cells_.resize(static_cast<size_t>(rows_ * cols_));
        for (auto& c : alt_cells_)
            c.clear();
        cursor_x = 0;
        cursor_y = 0;
        use_alt_screen = 1;
        markDirtyRange(0, rows_ - 1);
    } else {
        if (!saved_main_cells_.empty()) {
            cells_ = std::move(saved_main_cells_);
            saved_main_cells_.clear();
            cursor_x = saved_main_cursor_x_;
            cursor_y = saved_main_cursor_y_;
        }
        use_alt_screen = 0;
        markDirtyRange(0, rows_ - 1);
    }
}

void BuiltinScreen::appendScrollbackLine(const BuiltinCell* row, int cols) {
    std::vector<BuiltinCell> line(static_cast<size_t>(cols));
    std::memcpy(line.data(), row, static_cast<size_t>(cols) * sizeof(BuiltinCell));
    scrollback_.push_back(std::move(line));
    if (scrollback_.size() > static_cast<size_t>(1000)) {
        scrollback_.erase(scrollback_.begin());
    }
}

BuiltinScreenSnapshot BuiltinScreen::snapshot(int max_scrollback) const {
    BuiltinScreenSnapshot snap;
    snap.rows = rows_;
    snap.cols = cols_;
    snap.cursor_row = cursor_y;
    snap.cursor_col = cursor_x;
    snap.cursor_visible = cursor_visible != 0;

    const auto& buf = use_alt_screen ? alt_cells_ : cells_;
    if (buf.empty())
        return snap;

    snap.visible.resize(static_cast<size_t>(rows_));
    for (int y = 0; y < rows_; y++) {
        snap.visible[static_cast<size_t>(y)].resize(static_cast<size_t>(cols_));
        for (int x = 0; x < cols_; x++) {
            snap.visible[static_cast<size_t>(y)][static_cast<size_t>(x)] =
                buf[static_cast<size_t>(y * cols_ + x)];
        }
    }

    int sb_count = 0;
    if (!use_alt_screen) {
        sb_count = std::min(static_cast<int>(scrollback_.size()), max_scrollback);
    }
    int sb_start = static_cast<int>(scrollback_.size()) - sb_count;
    snap.scrollback.resize(static_cast<size_t>(sb_count));
    for (int i = 0; i < sb_count; i++) {
        snap.scrollback[static_cast<size_t>(i)] = scrollback_[static_cast<size_t>(sb_start + i)];
    }

    return snap;
}

} // namespace terminal
} // namespace features
} // namespace pnana
