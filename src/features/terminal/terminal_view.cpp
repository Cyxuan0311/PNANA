#ifdef BUILD_LIBVTERM_SUPPORT

#include "features/terminal/terminal_view.h"
#include "features/terminal/terminal_color.h"
#include <ftxui/dom/elements.hpp>
#include <unordered_map>

namespace pnana {
namespace features {
namespace terminal {

namespace {

// 颜色缓存键
struct ColorKey {
    uint8_t r, g, b;
    bool is_default;

    bool operator==(const ColorKey& other) const {
        return r == other.r && g == other.g && b == other.b && is_default == other.is_default;
    }
};

// 颜色键哈希函数
struct ColorKeyHash {
    std::size_t operator()(const ColorKey& key) const {
        return ((key.r * 31 + key.g) * 31 + key.b) * 2 + (key.is_default ? 1 : 0);
    }
};

// 全局颜色缓存（避免重复创建 FTXUI Color 对象）
class ColorCache {
  public:
    static ColorCache& instance() {
        static ColorCache inst;
        return inst;
    }

    ftxui::Color getColor(uint8_t r, uint8_t g, uint8_t b) {
        ColorKey key{r, g, b, false};
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        ftxui::Color color = ftxui::Color::RGB(r, g, b);
        cache_[key] = color;
        return color;
    }

    ftxui::Color getDefaultColor() {
        return ftxui::Color::Default;
    }

  private:
    std::unordered_map<ColorKey, ftxui::Color, ColorKeyHash> cache_;
};

ftxui::Element cellToElement(const TerminalCell& cell, const ftxui::Color& default_fg,
                             const ftxui::Color& default_bg, bool is_cursor) {
    ftxui::Color fg = default_fg;
    ftxui::Color bg = default_bg;

    // 使用缓存的颜色对象
    if (!cell.fg_default) {
        fg = ColorCache::instance().getColor(cell.fg_r, cell.fg_g, cell.fg_b);
    }
    if (!cell.bg_default) {
        bg = ColorCache::instance().getColor(cell.bg_r, cell.bg_g, cell.bg_b);
    }

    auto elem = ftxui::text(cell.text.empty() ? " " : cell.text);

    // 只应用与默认值不同的颜色（减少装饰器嵌套）
    if (fg != default_fg && fg != ftxui::Color::Default)
        elem = elem | ftxui::color(fg);
    if (bg != default_bg && bg != ftxui::Color::Default)
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
