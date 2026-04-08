#include "ui/terminal_ui.h"
#include "features/terminal/terminal_color.h"
#include "utils/logger.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

namespace {

// 使用 CursorRenderer 渲染光标，覆盖在字符上而非占用额外格位
Element renderTerminalCursor(const std::string& char_at_cursor, size_t cursor_pos,
                             size_t line_length, const ThemeColors& colors,
                             const TerminalCursorOptions* cursor_options) {
    CursorRenderer renderer;
    if (cursor_options) {
        renderer.setConfig(cursor_options->config);
        renderer.setBlinkRate(cursor_options->blink_rate_ms);
    } else {
        CursorConfig default_config;
        default_config.style = CursorStyle::BLOCK;
        default_config.color = colors.foreground;
        default_config.blink_enabled = true;
        renderer.setConfig(default_config);
        renderer.setBlinkRate(530);
    }
    renderer.updateCursorState();
    std::string ch = char_at_cursor.empty() ? " " : char_at_cursor;
    return renderer.renderCursorElement(ch, cursor_pos, line_length, colors.foreground,
                                        colors.background);
}

} // namespace

Element renderTerminal(features::Terminal& terminal, int height,
                       const TerminalCursorOptions* cursor_options) {
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

    Elements output_lines;
    const auto output_lines_data = terminal.getOutputLinesSnapshot();
    const std::string pending = terminal.getPendingLineSnapshot();
    size_t output_count = output_lines_data.size();
    size_t total_lines = output_count + (pending.empty() ? 0 : 1);
    size_t scroll_offset = terminal.getScrollOffset();

    int available_height = terminal_height;
    if (available_height < 1) {
        available_height = 1;
    }

    size_t start_line = 0;
    if (scroll_offset >= total_lines) {
        if (total_lines > static_cast<size_t>(available_height)) {
            start_line = total_lines - available_height;
        }
    } else {
        size_t effective_end = total_lines - scroll_offset;
        if (effective_end > static_cast<size_t>(available_height)) {
            start_line = effective_end - available_height;
        }
        if (start_line > total_lines) {
            start_line = total_lines > static_cast<size_t>(available_height)
                             ? total_lines - available_height
                             : 0;
        }
    }

    for (size_t i = start_line; (i - start_line) < static_cast<size_t>(available_height); ++i) {
        if (i < output_count) {
            const auto& line = output_lines_data[i];
            if (line.has_ansi_colors) {
                output_lines.push_back(
                    pnana::features::terminal::AnsiColorParser::parse(line.content));
            } else {
                output_lines.push_back(text(line.content) | color(colors.foreground));
            }
        } else if (i == output_count && !pending.empty()) {
            size_t cursor_pos = terminal.getPendingCursorPositionSnapshot();
            if (cursor_pos > pending.size()) {
                cursor_pos = pending.size();
            }
            std::string char_at_cursor =
                (cursor_pos < pending.size()) ? pending.substr(cursor_pos, 1) : " ";
            std::string before_cursor = pending.substr(0, cursor_pos);
            std::string after_cursor =
                (cursor_pos < pending.size()) ? pending.substr(cursor_pos + 1) : "";
            Element before_elem =
                pnana::features::terminal::AnsiColorParser::hasAnsiCodes(before_cursor)
                    ? pnana::features::terminal::AnsiColorParser::parse(before_cursor)
                    : static_cast<Element>(text(before_cursor) | color(colors.foreground));
            Element after_elem =
                pnana::features::terminal::AnsiColorParser::hasAnsiCodes(after_cursor)
                    ? pnana::features::terminal::AnsiColorParser::parse(after_cursor)
                    : static_cast<Element>(text(after_cursor) | color(colors.foreground));
            output_lines.push_back(
                hbox({before_elem,
                      renderTerminalCursor(char_at_cursor, cursor_pos, pending.size(), colors,
                                           cursor_options),
                      after_elem}));
        } else {
            break;
        }
    }

    auto out = vbox(output_lines) | flex | size(HEIGHT, EQUAL, terminal_height) |
               bgcolor(colors.background);
    return out;
}

#ifdef BUILD_LIBVTERM_SUPPORT
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
#endif

} // namespace ui
} // namespace pnana
