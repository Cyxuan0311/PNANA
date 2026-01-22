#include "features/command_palette.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>
#include <memory>

namespace pnana {
namespace features {

CommandPalette::CommandPalette(pnana::ui::Theme& theme)
    : is_open_(false), input_(""), selected_index_(0), scroll_offset_(0),
      ui_(std::make_unique<pnana::ui::CommandPaletteUI>(theme)) {}

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
    scroll_offset_ = 0;
    filterCommands();
}

void CommandPalette::close() {
    is_open_ = false;
    input_ = "";
    selected_index_ = 0;
    scroll_offset_ = 0;
    filtered_commands_.clear();
}

void CommandPalette::handleInput(const std::string& input) {
    input_ = input;
    filterCommands();
    selected_index_ = 0; // 重置选中索引
    scroll_offset_ = 0;  // 重置滚动偏移
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
    ui_->setData(is_open_, input_, filtered_commands_, selected_index_, scroll_offset_);
    return ui_->render();
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
        // 如果没有输入，显示所有命令（按需加载思想：只有在需要时才过滤）
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
        std::transform(lower_keyword.begin(), lower_keyword.end(), lower_keyword.begin(),
                       ::tolower);
        if (lower_keyword.find(lower_query) != std::string::npos) {
            return true;
        }
    }

    return false;
}

void CommandPalette::selectNext() {
    if (!filtered_commands_.empty()) {
        selected_index_ = (selected_index_ + 1) % filtered_commands_.size();
        updateScrollOffset();
    }
}

void CommandPalette::selectPrevious() {
    if (!filtered_commands_.empty()) {
        if (selected_index_ == 0) {
            selected_index_ = filtered_commands_.size() - 1;
        } else {
            selected_index_--;
        }
        updateScrollOffset();
    }
}

void CommandPalette::updateScrollOffset() {
    const size_t max_display = 15;
    if (selected_index_ < scroll_offset_) {
        // 选中的命令在可视区域上方，需要向上滚动
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + max_display) {
        // 选中的命令在可视区域下方，需要向下滚动
        scroll_offset_ = selected_index_ - max_display + 1;
    }
}

} // namespace features
} // namespace pnana
