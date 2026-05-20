#include "features/terminal/builtin_renderer.h"
#include <cstring>
#include <unordered_map>

namespace pnana {
namespace features {
namespace terminal {

namespace {

struct ColorCacheKey {
    uint8_t r, g, b;
    bool operator==(const ColorCacheKey& o) const {
        return r == o.r && g == o.g && b == o.b;
    }
};

struct ColorCacheKeyHash {
    size_t operator()(const ColorCacheKey& k) const {
        return (k.r * 31 + k.g) * 31 + k.b;
    }
};

class ColorCache {
  public:
    static ColorCache& instance() {
        static ColorCache inst;
        return inst;
    }

    ftxui::Color getRGB(uint8_t r, uint8_t g, uint8_t b) {
        ColorCacheKey key{r, g, b};
        auto it = cache_.find(key);
        if (it != cache_.end())
            return it->second;
        auto c = ftxui::Color::RGB(r, g, b);
        cache_[key] = c;
        return c;
    }

  private:
    std::unordered_map<ColorCacheKey, ftxui::Color, ColorCacheKeyHash> cache_;
};

constexpr uint8_t COLOR_256_CUBE[6] = {0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff};

ftxui::Color color256ToFtxui(int idx) {
    if (idx < 0 || idx > 255)
        return ftxui::Color::Default;
    if (idx < 16) {
        return ColorCache::instance().getRGB(BuiltinAnsi16Palette::RGB[idx][0],
                                             BuiltinAnsi16Palette::RGB[idx][1],
                                             BuiltinAnsi16Palette::RGB[idx][2]);
    }
    if (idx < 232) {
        int i = idx - 16;
        int r = COLOR_256_CUBE[i / 36];
        int g = COLOR_256_CUBE[(i / 6) % 6];
        int b = COLOR_256_CUBE[i % 6];
        return ColorCache::instance().getRGB(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                                             static_cast<uint8_t>(b));
    }
    uint8_t gray = static_cast<uint8_t>(8 + (idx - 232) * 10);
    return ColorCache::instance().getRGB(gray, gray, gray);
}

struct CellStyle {
    uint8_t flags;
    uint32_t fg;
    uint32_t bg;

    bool operator==(const CellStyle& o) const {
        return flags == o.flags && fg == o.fg && bg == o.bg;
    }
    bool operator!=(const CellStyle& o) const {
        return !(*this == o);
    }
};

ftxui::Element styledText(const char* buf, int len, const CellStyle& style,
                          const ftxui::Color& default_fg, const ftxui::Color& default_bg) {
    if (len == 0)
        return ftxui::text("");

    auto elem = ftxui::text(std::string(buf, static_cast<size_t>(len)));

    ftxui::Color fg = builtinColorToFtxui(style.fg);
    ftxui::Color bg = builtinColorToFtxui(style.bg);

    if (style.flags & BUILTIN_FLAG_BOLD)
        elem = elem | ftxui::bold;
    if (style.flags & BUILTIN_FLAG_DIM)
        elem = elem | ftxui::dim;
    if (style.flags & BUILTIN_FLAG_UNDERLINE)
        elem = elem | ftxui::underlined;
    if (style.flags & BUILTIN_FLAG_BLINK)
        elem = elem | ftxui::blink;
    if (style.flags & BUILTIN_FLAG_REVERSE)
        elem = elem | ftxui::inverted;
    if (style.flags & BUILTIN_FLAG_STRIKE)
        elem = elem | ftxui::strikethrough;

    if (!builtinColorIsDefault(style.fg) && fg != default_fg) {
        elem = elem | ftxui::color(fg);
    }
    if (!builtinColorIsDefault(style.bg) && bg != default_bg) {
        elem = elem | ftxui::bgcolor(bg);
    }

    return elem;
}

CellStyle cellStyle(const BuiltinCell& c) {
    return CellStyle{c.flags, c.fg_color, c.bg_color};
}

} // namespace

int builtinGlyphToUtf8(uint32_t cp, char* buf) {
    if (cp == 0 || cp == 0x20) {
        buf[0] = ' ';
        return 1;
    }
    if (cp < 0x20) {
        if (cp == 9 || cp == 10 || cp == 13) {
            buf[0] = ' ';
            return 1;
        }
        buf[0] = '^';
        buf[1] = static_cast<char>(cp + 0x40);
        return 2;
    }
    if (cp == 0x7F) {
        buf[0] = '^';
        buf[1] = '?';
        return 2;
    }
    if (cp < 0x80) {
        buf[0] = static_cast<char>(cp);
        return 1;
    }
    if (cp < 0x800) {
        buf[0] = static_cast<char>(0xC0 | (cp >> 6));
        buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp < 0x10000) {
        buf[0] = static_cast<char>(0xE0 | (cp >> 12));
        buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
        return 3;
    }
    buf[0] = static_cast<char>(0xF0 | (cp >> 18));
    buf[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    buf[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    buf[3] = static_cast<char>(0x80 | (cp & 0x3F));
    return 4;
}

ftxui::Color builtinColorToFtxui(uint32_t color) {
    if (builtinColorIsDefault(color))
        return ftxui::Color::Default;
    int t = builtinColorType(color);
    if (t == 1) {
        int idx = builtinColor16Idx(color);
        if (idx >= 0 && idx < 16) {
            return ColorCache::instance().getRGB(BuiltinAnsi16Palette::RGB[idx][0],
                                                 BuiltinAnsi16Palette::RGB[idx][1],
                                                 BuiltinAnsi16Palette::RGB[idx][2]);
        }
        return ftxui::Color::Default;
    }
    if (t == 2) {
        return color256ToFtxui(builtinColor256Idx(color));
    }
    if (t == 3) {
        int r, g, b;
        builtinColorRGBVal(color, r, g, b);
        return ColorCache::instance().getRGB(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                                             static_cast<uint8_t>(b));
    }
    return ftxui::Color::Default;
}

ftxui::Element renderBuiltinScreen(const BuiltinScreenSnapshot& snap, int height,
                                   const ftxui::Color& default_fg, const ftxui::Color& default_bg) {
    using namespace ftxui;

    if (height <= 0)
        height = 1;

    int scroll_off = snap.scroll_offset;
    if (scroll_off < 0)
        scroll_off = 0;
    int scrollback_count = static_cast<int>(snap.scrollback.size());
    if (scroll_off > scrollback_count)
        scroll_off = scrollback_count;

    int visible_count = static_cast<int>(snap.visible.size());
    int scrollback_show = std::min(scroll_off, height);
    int visible_skip = scroll_off - scrollback_show;
    int visible_show = std::min(visible_count - visible_skip, height - scrollback_show);
    if (visible_show < 0)
        visible_show = 0;

    int sb_start = scrollback_count - scrollback_show;

    char glyph_buf[8];
    char run_buf[4096];
    int run_len = 0;
    CellStyle run_style{0, BUILTIN_COLOR_DEFAULT, BUILTIN_COLOR_DEFAULT};

    auto flushRun = [&](Elements& cells) {
        if (run_len > 0) {
            cells.push_back(styledText(run_buf, run_len, run_style, default_fg, default_bg));
            run_len = 0;
        }
    };

    auto renderRowSimple = [&](const std::vector<BuiltinCell>& line, Elements& cells) {
        int col = 0;
        for (size_t c = 0; c < line.size(); c++) {
            const BuiltinCell& cell = line[c];
            if (cell.width == 0)
                continue;

            CellStyle style = cellStyle(cell);
            if (style != run_style) {
                flushRun(cells);
                run_style = style;
            }

            int glen = builtinGlyphToUtf8(cell.codepoint, glyph_buf);
            if (run_len + glen > 4080) {
                flushRun(cells);
            }
            std::memcpy(run_buf + run_len, glyph_buf, static_cast<size_t>(glen));
            run_len += glen;

            col += cell.width;
        }
        flushRun(cells);
    };

    auto renderRowWithCursor = [&](const std::vector<BuiltinCell>& line, int actual_row,
                                   Elements& cells) {
        int col = 0;
        run_len = 0;
        run_style = CellStyle{0, BUILTIN_COLOR_DEFAULT, BUILTIN_COLOR_DEFAULT};

        for (size_t c = 0; c < line.size(); c++) {
            const BuiltinCell& cell = line[c];
            if (cell.width == 0)
                continue;

            bool is_cursor =
                (snap.cursor_visible && actual_row == snap.cursor_row && col == snap.cursor_col);

            CellStyle style = cellStyle(cell);

            if (is_cursor) {
                flushRun(cells);

                int glen = builtinGlyphToUtf8(cell.codepoint, glyph_buf);
                auto cursor_elem = text(std::string(glyph_buf, static_cast<size_t>(glen)));
                cursor_elem = cursor_elem | inverted;

                if (!builtinColorIsDefault(style.fg)) {
                    cursor_elem = cursor_elem | color(builtinColorToFtxui(style.fg));
                }
                if (!builtinColorIsDefault(style.bg)) {
                    cursor_elem = cursor_elem | bgcolor(builtinColorToFtxui(style.bg));
                }
                if (style.flags & BUILTIN_FLAG_BOLD)
                    cursor_elem = cursor_elem | bold;

                cells.push_back(cursor_elem);
                run_style = CellStyle{0, BUILTIN_COLOR_DEFAULT, BUILTIN_COLOR_DEFAULT};
            } else {
                if (style != run_style) {
                    flushRun(cells);
                    run_style = style;
                }

                int glen = builtinGlyphToUtf8(cell.codepoint, glyph_buf);
                if (run_len + glen > 4080) {
                    flushRun(cells);
                }
                std::memcpy(run_buf + run_len, glyph_buf, static_cast<size_t>(glen));
                run_len += glen;
            }

            col += cell.width;
        }
        flushRun(cells);
    };

    Elements rows;

    for (int i = 0; i < scrollback_show; i++) {
        Elements cells;
        run_len = 0;
        run_style = CellStyle{0, BUILTIN_COLOR_DEFAULT, BUILTIN_COLOR_DEFAULT};
        renderRowSimple(snap.scrollback[static_cast<size_t>(sb_start + i)], cells);
        rows.push_back(!cells.empty() ? hbox(std::move(cells)) : text(" "));
    }

    for (int i = 0; i < visible_show; i++) {
        int vi = visible_skip + i;
        Elements cells;
        renderRowWithCursor(snap.visible[static_cast<size_t>(vi)], vi, cells);
        rows.push_back(!cells.empty() ? hbox(std::move(cells)) : text(" "));
    }

    while (static_cast<int>(rows.size()) < height) {
        rows.push_back(text(" "));
    }

    return vbox(std::move(rows)) | ftxui::flex | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, height) |
           bgcolor(default_bg);
}

} // namespace terminal
} // namespace features
} // namespace pnana
