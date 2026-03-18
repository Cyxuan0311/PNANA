#ifndef PNANA_FEATURES_TERMINAL_TERMINAL_VIEW_H
#define PNANA_FEATURES_TERMINAL_TERMINAL_VIEW_H

#include <ftxui/dom/elements.hpp>

#ifdef BUILD_LIBVTERM_SUPPORT
#include "features/terminal/terminal_vterm_screen.h"
#endif

namespace pnana {
namespace features {
namespace terminal {

#ifdef BUILD_LIBVTERM_SUPPORT

// 将 ScreenSnapshot 渲染为 ftxui::Element（按 cell 上色、光标、双宽字符）
ftxui::Element renderScreenSnapshot(const ScreenSnapshot& snap, int height,
                                    const ftxui::Color& default_fg, const ftxui::Color& default_bg);

#endif

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_TERMINAL_VIEW_H
