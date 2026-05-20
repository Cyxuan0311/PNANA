#ifndef PNANA_FEATURES_TERMINAL_BUILTIN_RENDERER_H
#define PNANA_FEATURES_TERMINAL_BUILTIN_RENDERER_H

#include "features/terminal/builtin_screen.h"
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace features {
namespace terminal {

struct BuiltinAnsi16Palette {
    static constexpr const char* HEX[16] = {
        "#262626", "#cc5555", "#55cc55", "#cdcd55", "#5555ff", "#cc55cc", "#55cccc", "#e5e5e5",
        "#666666", "#ff5555", "#55ff55", "#ffff55", "#5555ff", "#ff55ff", "#55ffff", "#ffffff",
    };

    static constexpr uint8_t RGB[16][3] = {
        {0x26, 0x26, 0x26}, {0xcc, 0x55, 0x55}, {0x55, 0xcc, 0x55}, {0xcd, 0xcd, 0x55},
        {0x55, 0x55, 0xff}, {0xcc, 0x55, 0xcc}, {0x55, 0xcc, 0xcc}, {0xe5, 0xe5, 0xe5},
        {0x66, 0x66, 0x66}, {0xff, 0x55, 0x55}, {0x55, 0xff, 0x55}, {0xff, 0xff, 0x55},
        {0x55, 0x55, 0xff}, {0xff, 0x55, 0xff}, {0x55, 0xff, 0xff}, {0xff, 0xff, 0xff},
    };
};

int builtinGlyphToUtf8(uint32_t cp, char* buf);
ftxui::Color builtinColorToFtxui(uint32_t color);

ftxui::Element renderBuiltinScreen(const BuiltinScreenSnapshot& snap, int height,
                                   const ftxui::Color& default_fg, const ftxui::Color& default_bg);

} // namespace terminal
} // namespace features
} // namespace pnana

#endif
