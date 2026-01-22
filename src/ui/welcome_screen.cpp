#include "ui/welcome_screen.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

WelcomeScreen::WelcomeScreen(Theme& theme) : theme_(theme) {}

Element WelcomeScreen::render() {
    auto& colors = theme_.getColors();

    Elements welcome_content;

    // 空行
    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // Logo和标题
    welcome_content.push_back(text("  ██████╗ ███╗   ██╗ █████╗ ███╗   ██╗ █████╗ ") |
                              color(colors.success) | bold | center);
    welcome_content.push_back(text("  ██╔══██╗████╗  ██║██╔══██╗████╗  ██║██╔══██╗") |
                              color(colors.success) | bold | center);
    welcome_content.push_back(text("  ██████╔╝██╔██╗ ██║███████║██╔██╗ ██║███████║") |
                              color(colors.success) | bold | center);
    welcome_content.push_back(text("  ██╔═══╝ ██║╚██╗██║██╔══██║██║╚██╗██║██╔══██║") |
                              color(colors.success) | bold | center);
    welcome_content.push_back(text("  ██║     ██║ ╚████║██║  ██║██║ ╚████║██║  ██║") |
                              color(colors.success) | bold | center);
    welcome_content.push_back(text("  ╚═╝     ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝") |
                              color(colors.success) | bold | center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text("Modern Terminal Text Editor") | color(colors.foreground) |
                              center);
    welcome_content.push_back(text("Version 0.0.4") | color(colors.comment) | dim | center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // Start editing hint (highlighted)
    welcome_content.push_back(
        hbox({text(" "), text(icons::BULB) | color(colors.warning), text(" Press "),
              text(" i ") | bgcolor(colors.keyword) | color(colors.background) | bold,
              text(" to start editing a new document ")}) |
        color(colors.foreground) | center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // Quick Start section
    welcome_content.push_back(hbox({text(icons::ROCKET), text(" Quick Start")}) |
                              color(colors.keyword) | bold | center);
    welcome_content.push_back(text(""));

    welcome_content.push_back(
        hbox({text("  "), text("Ctrl+O") | color(colors.function) | bold, text("  Open file    "),
              text("Ctrl+N") | color(colors.function) | bold, text("  New file")}) |
        center);

    welcome_content.push_back(
        hbox({text("  "), text("Ctrl+S") | color(colors.function) | bold, text("  Save file    "),
              text("Ctrl+Q") | color(colors.function) | bold, text("  Quit editor")}) |
        center);

    welcome_content.push_back(text(""));

    // Features section
    welcome_content.push_back(hbox({text(icons::STAR), text(" Features")}) | color(colors.keyword) |
                              bold | center);
    welcome_content.push_back(text(""));

    welcome_content.push_back(
        hbox({text("  "), text("Ctrl+F") | color(colors.function) | bold, text("  Search       "),
              text("Ctrl+G") | color(colors.function) | bold, text("  Go to line")}) |
        center);

    welcome_content.push_back(
        hbox({text("  "), text("Ctrl+T") | color(colors.function) | bold, text("  Themes       "),
              text("Ctrl+Z") | color(colors.function) | bold, text("  Undo/Redo")}) |
        center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // 提示信息
    welcome_content.push_back(
        hbox({text(icons::BULB), text(" Tip: Just start typing to begin editing!")}) |
        color(colors.success) | center);

    welcome_content.push_back(text(""));

    welcome_content.push_back(text("Press Ctrl+T to choose from 28 beautiful themes") |
                              color(colors.comment) | dim | center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // 底部信息
    welcome_content.push_back(text("─────────────────────────────────────────────────") |
                              color(colors.comment) | dim | center);
    welcome_content.push_back(text("Check the bottom bar for more shortcuts") |
                              color(colors.comment) | dim | center);

    return vbox(welcome_content) | center | flex;
}

} // namespace ui
} // namespace pnana
