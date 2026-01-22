#include "ui/command_palette_ui.h"
#include "features/command_palette.h"
#include "ui/icons.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

CommandPaletteUI::CommandPaletteUI(Theme& theme)
    : theme_(theme), is_open_(false), selected_index_(0), scroll_offset_(0) {}

void CommandPaletteUI::setData(bool is_open, const std::string& input,
                               const std::vector<features::Command>& filtered_commands,
                               size_t selected_index, size_t scroll_offset) {
    is_open_ = is_open;
    input_ = input;
    filtered_commands_ = filtered_commands;
    selected_index_ = selected_index;
    scroll_offset_ = scroll_offset;
}

ftxui::Element CommandPaletteUI::render() {
    if (!is_open_) {
        return text("");
    }

    const auto& colors = theme_.getColors();
    Elements dialog_content;

    // 标题
    dialog_content.push_back(renderTitle());

    dialog_content.push_back(separator());

    // 输入框
    dialog_content.push_back(text(""));
    dialog_content.push_back(renderInputBox());

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 命令列表
    dialog_content.push_back(renderCommandList());

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 帮助栏
    dialog_content.push_back(renderHelpBar());

    int height =
        std::min(22, int(15 + static_cast<int>(std::min(filtered_commands_.size(), size_t(15)))));
    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 70) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

Element CommandPaletteUI::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(pnana::ui::icons::SEARCH) | color(colors.success),
                 text(" Command Palette "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element CommandPaletteUI::renderInputBox() const {
    const auto& colors = theme_.getColors();
    std::string input_display = input_.empty() ? "_" : input_ + "_";
    return hbox({text("  > "),
                 text(input_display) | bold | color(colors.dialog_fg) | bgcolor(colors.selection)});
}

Element CommandPaletteUI::renderCommandList() const {
    const auto& colors = theme_.getColors();
    Elements list_elements;

    // 限制显示的命令数量（最多15个）
    size_t max_display = std::min(filtered_commands_.size(), size_t(15));

    if (filtered_commands_.empty()) {
        list_elements.push_back(
            hbox({text("  "), text("No commands found") | color(colors.comment) | dim}));
    } else {
        for (size_t i = 0; i < max_display && (scroll_offset_ + i) < filtered_commands_.size();
             ++i) {
            const auto& cmd = filtered_commands_[scroll_offset_ + i];
            bool is_selected = ((scroll_offset_ + i) == selected_index_);

            Elements cmd_elements;
            cmd_elements.push_back(text("  "));

            // 选中标记
            if (is_selected) {
                cmd_elements.push_back(text("► ") | color(colors.success) | bold);
            } else {
                cmd_elements.push_back(text("  "));
            }

            // 命令名称
            cmd_elements.push_back(text(cmd.name) | (is_selected ? color(colors.dialog_fg) | bold
                                                                 : color(colors.foreground)));

            // 描述
            if (!cmd.description.empty()) {
                cmd_elements.push_back(filler());
                cmd_elements.push_back(text(cmd.description) | color(colors.comment) | dim);
            }

            Element cmd_line = hbox(cmd_elements);
            if (is_selected) {
                cmd_line = cmd_line | bgcolor(colors.selection);
            }

            list_elements.push_back(cmd_line);
        }

        // 如果还有更多命令，显示提示
        size_t displayed_count = std::min(max_display, filtered_commands_.size() - scroll_offset_);
        if (scroll_offset_ > 0 || (scroll_offset_ + displayed_count) < filtered_commands_.size()) {
            list_elements.push_back(text(""));
            std::string more_text;
            if (scroll_offset_ > 0 &&
                (scroll_offset_ + displayed_count) < filtered_commands_.size()) {
                more_text =
                    "... " + std::to_string(scroll_offset_) + " above and " +
                    std::to_string(filtered_commands_.size() - scroll_offset_ - displayed_count) +
                    " below";
            } else if (scroll_offset_ > 0) {
                more_text = "... " + std::to_string(scroll_offset_) + " more above";
            } else {
                more_text = "... " + std::to_string(filtered_commands_.size() - displayed_count) +
                            " more below";
            }
            list_elements.push_back(
                hbox({text("  "), text(more_text) | color(colors.comment) | dim}));
        }
    }

    return vbox(list_elements);
}

Element CommandPaletteUI::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
                 text("Enter") | color(colors.helpbar_key) | bold, text(": Execute  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Cancel")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

} // namespace ui
} // namespace pnana
