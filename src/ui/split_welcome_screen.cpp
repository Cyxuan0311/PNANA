#include "ui/split_welcome_screen.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

SplitWelcomeScreen::SplitWelcomeScreen(Theme& theme) : theme_(theme) {}

Element SplitWelcomeScreen::render() {
    auto& colors = theme_.getColors();

    Elements split_content;

    // 空行
    split_content.push_back(text(""));
    split_content.push_back(text(""));

    // Split view icon and title
    split_content.push_back(text("  " + std::string(icons::SPLIT) + "  Split View Mode") |
                            color(colors.keyword) | bold | center);
    split_content.push_back(text(""));

    // Description
    split_content.push_back(text("Split view provides complete isolation between panes") |
                            color(colors.foreground) | center);
    split_content.push_back(text("Each pane maintains its own documents, cursor, and view state") |
                            color(colors.comment) | center);

    split_content.push_back(text(""));
    split_content.push_back(text(""));

    // Instructions section
    split_content.push_back(hbox({text(icons::BULB), text(" How to use split view")}) |
                            color(colors.keyword) | bold | center);
    split_content.push_back(text(""));

    // File operations
    split_content.push_back(hbox({text("  "), text("Ctrl+O") | color(colors.function) | bold,
                                  text("  Open file in active pane")}) |
                            center);
    split_content.push_back(hbox({text("  "), text("Ctrl+N") | color(colors.function) | bold,
                                  text("  Create new document")}) |
                            center);

    split_content.push_back(text(""));

    // Navigation
    split_content.push_back(
        hbox({text(icons::ARROW_LEFT), text(icons::ARROW_UP), text(icons::ARROW_DOWN),
              text(icons::ARROW_RIGHT), text("  Switch between panes")}) |
        color(colors.function) | center);
    split_content.push_back(text("Use Alt+arrow keys to switch focus between panes") |
                            color(colors.comment) | center);

    split_content.push_back(text(""));

    // Split operations
    split_content.push_back(hbox({text(icons::SPLIT), text(" Split Operations")}) |
                            color(colors.keyword) | bold | center);
    split_content.push_back(text(""));

    split_content.push_back(
        hbox({text("  "), text("Ctrl+\\") | color(colors.function) | bold,
              text("  Vertical split     "), text("Ctrl+-") | color(colors.function) | bold,
              text("  Horizontal split")}) |
        center);

    split_content.push_back(
        hbox({text("  "), text("Alt+=") | color(colors.function) | bold,
              text("  Resize active pane  "), text("Alt+-") | color(colors.function) | bold,
              text("  Shrink active pane")}) |
        center);

    split_content.push_back(text(""));

    // Features
    split_content.push_back(hbox({text(icons::STAR), text(" Features")}) | color(colors.keyword) |
                            bold | center);
    split_content.push_back(text(""));

    split_content.push_back(text("• Complete isolation between panes") | color(colors.foreground) |
                            center);
    split_content.push_back(text("• Independent document management") | color(colors.foreground) |
                            center);
    split_content.push_back(text("• Per-pane tab bars and navigation") | color(colors.foreground) |
                            center);
    split_content.push_back(text("• Independent cursor and view states") |
                            color(colors.foreground) | center);
    split_content.push_back(text("• Resizable panes with Alt+=/-") | color(colors.foreground) |
                            center);

    split_content.push_back(text(""));
    split_content.push_back(text(""));

    // Tip
    split_content.push_back(
        hbox({text(icons::BULB),
              text(" Tip: Each pane maintains completely separate editing sessions")}) |
        color(colors.success) | center);

    split_content.push_back(text(""));
    split_content.push_back(text("Press Ctrl+O to open a file or Ctrl+N to create a new document") |
                            color(colors.comment) | dim | center);

    split_content.push_back(text(""));
    split_content.push_back(text(""));

    // Bottom separator
    split_content.push_back(text("─────────────────────────────────────────────────") |
                            color(colors.comment) | dim | center);

    return vbox(split_content) | center | flex;
}

} // namespace ui
} // namespace pnana
