#ifndef PNANA_UI_TERMINAL_UI_H
#define PNANA_UI_TERMINAL_UI_H

#include "features/cursor/cursor_renderer.h"
#include "features/terminal.h"
#include <ftxui/dom/elements.hpp>

#ifdef BUILD_LIBVTERM_SUPPORT
#include "features/terminal/terminal_view.h"
#endif

namespace pnana {
namespace ui {

// 终端光标配置（与编辑器光标统一，复用 CursorConfig）
struct TerminalCursorOptions {
    CursorConfig config;
    int blink_rate_ms = 530; // 闪烁间隔
};

// 渲染终端UI（复用 CursorRenderer 的配置以统一光标样式）
ftxui::Element renderTerminal(features::Terminal& terminal, int height,
                              const TerminalCursorOptions* cursor_options = nullptr);

#ifdef BUILD_LIBVTERM_SUPPORT
// 渲染终端标签栏（多会话时显示）
ftxui::Element renderTerminalTabs(features::Terminal& terminal);
#endif

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_TERMINAL_UI_H
