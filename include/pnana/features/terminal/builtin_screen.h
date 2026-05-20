#ifndef PNANA_FEATURES_TERMINAL_BUILTIN_SCREEN_H
#define PNANA_FEATURES_TERMINAL_BUILTIN_SCREEN_H

#include <cstdint>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace terminal {

constexpr uint32_t BUILTIN_COLOR_DEFAULT = 0x00000000u;
constexpr uint32_t BUILTIN_COLOR_TYPE_MASK = 0x03000000u;
constexpr uint32_t BUILTIN_COLOR_TYPE_16 = 0x01000000u;
constexpr uint32_t BUILTIN_COLOR_TYPE_256 = 0x02000000u;
constexpr uint32_t BUILTIN_COLOR_TYPE_RGB = 0x03000000u;
constexpr uint32_t BUILTIN_COLOR_VAL_MASK = 0x00FFFFFFu;

inline uint32_t builtinColorDefault() {
    return BUILTIN_COLOR_DEFAULT;
}
inline uint32_t builtinColor16(int idx) {
    return BUILTIN_COLOR_TYPE_16 | (uint32_t)(idx & 0xFF);
}
inline uint32_t builtinColor256(int idx) {
    return BUILTIN_COLOR_TYPE_256 | (uint32_t)(idx & 0xFF);
}
inline uint32_t builtinColorRGB(int r, int g, int b) {
    return BUILTIN_COLOR_TYPE_RGB | ((uint32_t)(r & 0xFF) << 16) | ((uint32_t)(g & 0xFF) << 8) |
           (uint32_t)(b & 0xFF);
}
inline bool builtinColorIsDefault(uint32_t c) {
    return c == BUILTIN_COLOR_DEFAULT;
}
inline int builtinColorType(uint32_t c) {
    return (int)((c & BUILTIN_COLOR_TYPE_MASK) >> 24);
}
inline int builtinColor16Idx(uint32_t c) {
    return (int)(c & 0xFF);
}
inline int builtinColor256Idx(uint32_t c) {
    return (int)(c & 0xFF);
}
inline void builtinColorRGBVal(uint32_t c, int& r, int& g, int& b) {
    r = (int)((c >> 16) & 0xFF);
    g = (int)((c >> 8) & 0xFF);
    b = (int)(c & 0xFF);
}

constexpr uint8_t BUILTIN_FLAG_BOLD = 0x01;
constexpr uint8_t BUILTIN_FLAG_ITALIC = 0x02;
constexpr uint8_t BUILTIN_FLAG_UNDERLINE = 0x04;
constexpr uint8_t BUILTIN_FLAG_STRIKE = 0x08;
constexpr uint8_t BUILTIN_FLAG_REVERSE = 0x10;
constexpr uint8_t BUILTIN_FLAG_BLINK = 0x20;
constexpr uint8_t BUILTIN_FLAG_DIM = 0x40;
constexpr uint8_t BUILTIN_FLAG_OVERLINE = 0x80;

struct BuiltinCell {
    uint32_t codepoint = 0;
    uint32_t fg_color = BUILTIN_COLOR_DEFAULT;
    uint32_t bg_color = BUILTIN_COLOR_DEFAULT;
    uint8_t flags = 0;
    uint8_t width = 1;

    void clear() {
        codepoint = 0;
        fg_color = BUILTIN_COLOR_DEFAULT;
        bg_color = BUILTIN_COLOR_DEFAULT;
        flags = 0;
        width = 1;
    }

    void erase(uint32_t fg, uint32_t bg, uint8_t fl) {
        codepoint = 0;
        fg_color = fg;
        bg_color = bg;
        flags = fl;
        width = 1;
    }

    bool isEmpty() const {
        return codepoint == 0;
    }
};

struct BuiltinScreenSnapshot {
    int rows = 0;
    int cols = 0;
    std::vector<std::vector<BuiltinCell>> visible;
    std::vector<std::vector<BuiltinCell>> scrollback;
    int cursor_row = 0;
    int cursor_col = 0;
    bool cursor_visible = true;
    int scroll_offset = 0;
};

class BuiltinScreen {
  public:
    friend class BuiltinVtParser;

    BuiltinScreen() = default;
    ~BuiltinScreen() = default;

    BuiltinScreen(const BuiltinScreen&) = delete;
    BuiltinScreen& operator=(const BuiltinScreen&) = delete;

    bool init(int cols, int rows);
    void resize(int new_cols, int new_rows);

    BuiltinCell* cell(int x, int y);
    const BuiltinCell* cell(int x, int y) const;

    void markDirty(int y);
    void markDirtyRange(int y1, int y2);
    void clearAllDirty();
    bool isRowDirty(int y) const;

    void scrollUp(int n);
    void scrollDown(int n);
    void lineFeed();
    void reverseIndex();
    void putChar(uint32_t cp, int width);
    void eraseCells(int x1, int y1, int x2, int y2);
    void eraseDisplay(int mode);
    void eraseLine(int mode);
    void insertLines(int n);
    void deleteLines(int n);
    void deleteChars(int n);
    void insertChars(int n);
    void eraseChars(int n);
    void setScrollingRegion(int top, int bot);
    void switchAltScreen(bool enable);

    BuiltinScreenSnapshot snapshot(int max_scrollback = 1000) const;

    int cols() const {
        return cols_;
    }
    int rows() const {
        return rows_;
    }
    size_t scrollbackSize() const {
        return scrollback_.size();
    }

    int cursor_x = 0;
    int cursor_y = 0;
    int cursor_saved_x = 0;
    int cursor_saved_y = 0;
    int cursor_visible = 1;
    int cursor_saved_visible = 1;

    uint32_t saved_fg = BUILTIN_COLOR_DEFAULT;
    uint32_t saved_bg = BUILTIN_COLOR_DEFAULT;
    uint8_t saved_flags = 0;

    int scroll_top = 0;
    int scroll_bottom = 0;

    uint32_t cur_fg = BUILTIN_COLOR_DEFAULT;
    uint32_t cur_bg = BUILTIN_COLOR_DEFAULT;
    uint8_t cur_flags = 0;

    std::vector<uint8_t> tab_stops;
    int charset_G[2] = {0, 0};
    int charset_active = 0;
    int origin_mode = 0;
    int auto_wrap = 1;
    int pending_wrap = 0;
    int use_alt_screen = 0;

  private:
    std::vector<BuiltinCell> cells_;
    std::vector<BuiltinCell> alt_cells_;
    std::vector<BuiltinCell> saved_main_cells_;
    int saved_main_cursor_x_ = 0;
    int saved_main_cursor_y_ = 0;
    int cols_ = 0;
    int rows_ = 0;

    std::vector<uint64_t> dirty_bits_;
    int dirty_words_ = 0;

    mutable std::vector<std::vector<BuiltinCell>> scrollback_;

    void appendScrollbackLine(const BuiltinCell* row, int cols);
};

int builtinCharWidth(uint32_t cp);

} // namespace terminal
} // namespace features
} // namespace pnana

#endif
