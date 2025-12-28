#include "features/command_palette.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace features {

CommandPalette::CommandPalette() 
    : is_open_(false), input_(""), selected_index_(0) {
}

void CommandPalette::registerCommand(const Command& command) {
    commands_.push_back(command);
    // 如果面板已打开，重新过滤
    if (is_open_) {
        filterCommands();
    }
}

void CommandPalette::open() {
    is_open_ = true;
    input_ = "";
    selected_index_ = 0;
    filterCommands();
}

void CommandPalette::close() {
    is_open_ = false;
    input_ = "";
    selected_index_ = 0;
    filtered_commands_.clear();
}

void CommandPalette::handleInput(const std::string& input) {
    input_ = input;
    filterCommands();
    selected_index_ = 0;  // 重置选中索引
}

bool CommandPalette::handleKeyEvent(const std::string& key) {
    if (!is_open_) {
        return false;
    }
    
    if (key == "Escape") {
        close();
        return true;
    } else if (key == "Enter" || key == "Return") {
        executeSelected();
        return true;
    } else if (key == "ArrowDown") {
        selectNext();
        return true;
    } else if (key == "ArrowUp") {
        selectPrevious();
        return true;
    }
    
    return false;
}

ftxui::Element CommandPalette::render() {
    if (!is_open_) {
        return ftxui::text("");
    }
    
    using namespace ftxui;
    Elements dialog_content;
    
    // 标题
    dialog_content.push_back(
        hbox({
            text(" "),
            text("⚡") | color(Color::Yellow),
            text(" Command Palette "),
            text(" ")
        }) | bold | bgcolor(Color::RGB(60, 60, 80)) | center
    );
    
    dialog_content.push_back(separator());
    
    // 输入框
    dialog_content.push_back(text(""));
    std::string input_display = input_.empty() ? "_" : input_ + "_";
    dialog_content.push_back(
        hbox({
            text("  > "),
            text(input_display) | bold | color(Color::White) | bgcolor(Color::RGB(40, 40, 50))
        })
    );
    
    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());
    
    // 命令列表
    if (filtered_commands_.empty()) {
        dialog_content.push_back(
            hbox({
                text("  "),
                text("No commands found") | color(Color::GrayDark) | dim
            })
        );
    } else {
        // 限制显示的命令数量（最多10个）
        size_t max_display = std::min(filtered_commands_.size(), size_t(10));
        for (size_t i = 0; i < max_display; ++i) {
            const auto& cmd = filtered_commands_[i];
            bool is_selected = (i == selected_index_);
            
            Elements cmd_elements;
            cmd_elements.push_back(text("  "));
            
            // 选中标记
            if (is_selected) {
                cmd_elements.push_back(text("► ") | color(Color::GreenLight) | bold);
            } else {
                cmd_elements.push_back(text("  "));
            }
            
            // 命令名称
            cmd_elements.push_back(text(cmd.name) | 
                (is_selected ? color(Color::White) | bold : color(Color::GrayLight)));
            
            // 描述
            if (!cmd.description.empty()) {
                cmd_elements.push_back(filler());
                cmd_elements.push_back(text(cmd.description) | 
                    color(Color::GrayDark) | dim);
            }
            
            Element cmd_line = hbox(cmd_elements);
            if (is_selected) {
                cmd_line = cmd_line | bgcolor(Color::RGB(50, 50, 70));
            }
            
            dialog_content.push_back(cmd_line);
        }
        
        // 如果还有更多命令，显示提示
        if (filtered_commands_.size() > max_display) {
            dialog_content.push_back(text(""));
            dialog_content.push_back(
                hbox({
                    text("  "),
                    text("... and " + std::to_string(filtered_commands_.size() - max_display) + 
                         " more") | color(Color::GrayDark) | dim
                })
            );
        }
    }
    
    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());
    
    // 提示信息
    dialog_content.push_back(
        hbox({
            text("  "),
            text("↑↓") | color(Color::Cyan) | bold,
            text(": Navigate  "),
            text("Enter") | color(Color::Cyan) | bold,
            text(": Execute  "),
            text("Esc") | color(Color::Cyan) | bold,
            text(": Cancel")
        }) | dim
    );
    
    int height = std::min(20, int(15 + static_cast<int>(filtered_commands_.size())));
    return window(
        text(""),
        vbox(dialog_content)
    ) | size(WIDTH, EQUAL, 70) 
      | size(HEIGHT, EQUAL, height) 
      | bgcolor(Color::RGB(30, 30, 40))
      | border;
}

void CommandPalette::executeSelected() {
    if (selected_index_ < filtered_commands_.size()) {
        const auto& cmd = filtered_commands_[selected_index_];
        close();
        if (cmd.execute) {
            cmd.execute();
        }
    }
}

void CommandPalette::filterCommands() {
    filtered_commands_.clear();
    
    if (input_.empty()) {
        // 如果没有输入，显示所有命令
        filtered_commands_ = commands_;
    } else {
        // 过滤匹配的命令
        for (const auto& cmd : commands_) {
            if (matchesCommand(cmd, input_)) {
                filtered_commands_.push_back(cmd);
            }
        }
    }
    
    // 确保选中索引有效
    if (selected_index_ >= filtered_commands_.size() && !filtered_commands_.empty()) {
        selected_index_ = filtered_commands_.size() - 1;
    }
}

bool CommandPalette::matchesCommand(const Command& cmd, const std::string& query) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    // 检查命令名称
    std::string lower_name = cmd.name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    if (lower_name.find(lower_query) != std::string::npos) {
        return true;
    }
    
    // 检查描述
    std::string lower_desc = cmd.description;
    std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);
    if (lower_desc.find(lower_query) != std::string::npos) {
        return true;
    }
    
    // 检查关键词
    for (const auto& keyword : cmd.keywords) {
        std::string lower_keyword = keyword;
        std::transform(lower_keyword.begin(), lower_keyword.end(), lower_keyword.begin(), ::tolower);
        if (lower_keyword.find(lower_query) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

void CommandPalette::selectNext() {
    if (!filtered_commands_.empty()) {
        selected_index_ = (selected_index_ + 1) % filtered_commands_.size();
    }
}

void CommandPalette::selectPrevious() {
    if (!filtered_commands_.empty()) {
        if (selected_index_ == 0) {
            selected_index_ = filtered_commands_.size() - 1;
        } else {
            selected_index_--;
        }
    }
}

} // namespace features
} // namespace pnana

