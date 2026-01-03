#ifndef PNANA_UI_TERMINAL_UI_H
#define PNANA_UI_TERMINAL_UI_H

#include "features/terminal.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace ui {

// 渲染终端UI
ftxui::Element renderTerminal(features::Terminal& terminal, int height);

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_TERMINAL_UI_H
