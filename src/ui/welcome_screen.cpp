#include "ui/welcome_screen.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

WelcomeScreen::WelcomeScreen(Theme& theme, core::ConfigManager& config)
    : theme_(theme), config_(config) {}

Element WelcomeScreen::render() {
    auto& colors = theme_.getColors();

    Elements welcome_content;

    // 空行
    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // Logo 渐变色：根据配置决定是否使用渐变
    std::vector<Color> gradient_colors;
    if (config_.getConfig().display.logo_gradient) {
        gradient_colors = theme_.getGradientColors();
    } else {
        // 不使用渐变时，使用单一颜色
        gradient_colors = {colors.success, colors.success, colors.success,
                           colors.success, colors.success, colors.success};
    }

    // Logo和标题（渐变效果）
    welcome_content.push_back(text("  ██████╗ ███╗   ██╗ █████╗ ███╗   ██╗ █████╗ ") |
                              color(gradient_colors[0]) | bold | center);
    welcome_content.push_back(text("  ██╔══██╗████╗  ██║██╔══██╗████╗  ██║██╔══██╗") |
                              color(gradient_colors[1]) | bold | center);
    welcome_content.push_back(text("  ██████╔╝██╔██╗ ██║███████║██╔██╗ ██║███████║") |
                              color(gradient_colors[2]) | bold | center);
    welcome_content.push_back(text("  ██╔═══╝ ██║╚██╗██║██╔══██║██║╚██╗██║██╔══██║") |
                              color(gradient_colors[3]) | bold | center);
    welcome_content.push_back(text("  ██║     ██║ ╚████║██║  ██║██║ ╚████║██║  ██║") |
                              color(gradient_colors[4]) | bold | center);
    welcome_content.push_back(text("  ╚═╝     ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝") |
                              color(gradient_colors[5]) | bold | center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text("Modern Terminal Text Editor") | color(colors.foreground) |
                              bold | center);
    welcome_content.push_back(text("Version 0.0.5") | color(colors.comment) | dim | center);

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
              text("Ctrl+Z") | color(colors.function) | bold, text("  Undo")}) |
        center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // 提示信息
    welcome_content.push_back(
        hbox({text(icons::BULB), text(" Tip: Just start typing to begin editing!")}) |
        color(colors.success) | center);

    welcome_content.push_back(text(""));

    welcome_content.push_back(text("Press Ctrl+T to choose from multiple themes") |
                              color(colors.comment) | dim | center);

    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // 底部信息
    welcome_content.push_back(text("─────────────────────────────────────────────────") |
                              color(colors.comment) | bold | center);
    welcome_content.push_back(text("Check the bottom bar for more shortcuts") |
                              color(colors.comment) | dim | center);

    return vbox(welcome_content) | center | flex;
}

} // namespace ui
} // namespace pnana
