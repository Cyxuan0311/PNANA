#include "ui/terminal_ui.h"
#include "features/terminal/builtin_renderer.h"
#include "features/terminal/terminal_color.h"
#include "utils/logger.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

Element renderTerminal(features::Terminal& terminal, int height,
                       const TerminalCursorOptions* /*cursor_options*/) {
    if (!terminal.isVisible()) {
        return text("");
    }

    int width = ftxui::Terminal::Size().dimx;
    LOG("[TerminalUI] render size w=" + std::to_string(width) + " h=" + std::to_string(height));
    const int reserved_bottom = 2;
    int terminal_height = height - reserved_bottom;
    if (terminal_height < 1)
        terminal_height = 1;

    terminal.resize(width, terminal_height);
    LOG("[TerminalUI] render size w=" + std::to_string(width) +
        " h=" + std::to_string(terminal_height));

    auto& theme = terminal.getTheme();
    auto& colors = theme.getColors();

#ifdef BUILD_LIBVTERM_SUPPORT
    if (terminal.useLibVTermPath()) {
        auto snap = terminal.getSessionSnapshot(terminal_height);
        return features::terminal::renderScreenSnapshot(snap, terminal_height, colors.foreground,
                                                        colors.background);
    }
#endif

    {
        auto snap = terminal.getBuiltinScreenSnapshot();
        return features::terminal::renderBuiltinScreen(snap, terminal_height, colors.foreground,
                                                       colors.background);
    }
}

Element renderTerminalTabs(features::Terminal& terminal) {
    if (!terminal.isVisible() || terminal.sessionCount() <= 1)
        return text("");

    auto& theme = terminal.getTheme();
    auto& colors = theme.getColors();

    Elements tabs;
    int active = terminal.activeSessionIndex();
    for (int i = 0; i < terminal.sessionCount(); i++) {
        std::string title = terminal.getSessionTitle(i);
        if (title.empty())
            title = "Terminal " + std::to_string(i + 1);
        auto tab_text = text(" " + std::to_string(i + 1) + ":" + title + " ");
        if (i == active)
            // 当前终端标签：使用 success 颜色反白高亮
            tab_text = tab_text | bgcolor(colors.success) | color(colors.background) | bold;
        else
            // 非当前终端标签：使用前景色
            tab_text = tab_text | color(colors.foreground);
        tabs.push_back(tab_text);
    }
    // 标签栏紧贴终端，使用 hbox 并设置高度为 1，去除边框和间隔
    return hbox(tabs) | size(HEIGHT, EQUAL, 1);
}

} // namespace ui
} // namespace pnana
