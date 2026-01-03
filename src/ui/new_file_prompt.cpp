#include "ui/new_file_prompt.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

NewFilePrompt::NewFilePrompt(Theme& theme) : theme_(theme) {}

Element NewFilePrompt::render() {
    auto& colors = theme_.getColors();

    Elements prompt_content;

    // Empty lines (for vertical centering)
    prompt_content.push_back(text(""));
    prompt_content.push_back(text(""));
    prompt_content.push_back(text(""));
    prompt_content.push_back(text(""));

    // Main prompt message
    prompt_content.push_back(hbox({text(" "), text(icons::BULB) | color(colors.warning),
                                   text(" New file created, you can start typing!")}) |
                             color(colors.foreground) | center);

    prompt_content.push_back(text(""));
    prompt_content.push_back(text(""));

    // Input prompt
    prompt_content.push_back(
        hbox({text("  "), text("Press "),
              text(" i ") | bgcolor(colors.keyword) | color(colors.background) | bold,
              text(" to enter insert mode and start editing")}) |
        color(colors.foreground) | center);

    prompt_content.push_back(text(""));

    prompt_content.push_back(hbox({text("  "), text("Or simply start typing")}) |
                             color(colors.comment) | dim | center);

    prompt_content.push_back(text(""));
    prompt_content.push_back(text(""));

    // Common shortcuts
    prompt_content.push_back(hbox({text(icons::SETTINGS), text(" Common Shortcuts")}) |
                             color(colors.keyword) | bold | center);

    prompt_content.push_back(text(""));

    prompt_content.push_back(
        hbox({text("  "), text("Ctrl+S") | color(colors.function) | bold, text("  Save file    "),
              text("Ctrl+O") | color(colors.function) | bold, text("  Open file")}) |
        center);

    prompt_content.push_back(
        hbox({text("  "), text("Ctrl+N") | color(colors.function) | bold, text("  New file    "),
              text("Ctrl+Q") | color(colors.function) | bold, text("  Quit editor")}) |
        center);

    prompt_content.push_back(text(""));
    prompt_content.push_back(text(""));

    // Bottom tip
    prompt_content.push_back(text("─────────────────────────────────────────────────") |
                             color(colors.comment) | dim | center);

    prompt_content.push_back(text("Tip: Start typing to automatically enter edit mode") |
                             color(colors.success) | center);

    prompt_content.push_back(text(""));
    prompt_content.push_back(text(""));

    return vbox(prompt_content) | center | flex;
}

} // namespace ui
} // namespace pnana
