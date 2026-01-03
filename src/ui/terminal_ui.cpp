#include "ui/terminal_ui.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>
#include <algorithm>
#include <cstring>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace ui {

Element renderTerminal(features::Terminal& terminal, int height) {
    if (!terminal.isVisible()) {
        return text("");
    }
    
    auto& theme = terminal.getTheme();
    auto& colors = theme.getColors();
    
    // 输出区域和输入行
    Elements output_lines;
    const auto& output_lines_data = terminal.getOutputLines();
    size_t scroll_offset = terminal.getScrollOffset();

    // 计算可用高度：总高度 - 1（为输入行预留）
    int available_height = height - 1;
    if (available_height < 1) {
        available_height = 1;  // 至少保留1行用于输出
    }

    // 计算要显示的历史输出行数
    size_t output_count = output_lines_data.size();
    size_t start_line = 0;

    // 根据滚动偏移量调整起始行
    // scroll_offset 表示从输出开头跳过的行数（向上滚动时增加）
    if (scroll_offset >= output_count) {
        // 如果偏移量超过总行数，显示最后 available_height 行
        if (output_count > static_cast<size_t>(available_height)) {
            start_line = output_count - available_height;
        }
    } else {
        // 计算实际的起始行：从输出末尾向前 available_height 行，但考虑滚动偏移
        size_t effective_end = output_count - scroll_offset;
        if (effective_end > static_cast<size_t>(available_height)) {
            start_line = effective_end - available_height;
        }
        // 确保 start_line 不超过输出范围
        if (start_line > output_count) {
            start_line = output_count > static_cast<size_t>(available_height) ?
                        output_count - available_height : 0;
        }
    }
    
    // 显示历史输出（确保不超过可用高度）
    for (size_t i = start_line; i < output_lines_data.size() && 
         (i - start_line) < static_cast<size_t>(available_height); ++i) {
        const auto& line = output_lines_data[i];
        Color line_color;
        if (line.is_command) {
            // 命令使用绿色（类似bash/zsh）
            line_color = Color::Green;
        } else {
            // 输出使用前景色
            line_color = colors.foreground;
        }
        output_lines.push_back(
            text(line.content) | color(line_color)
        );
    }
    
    // 输入行 - 始终固定在最后一行（确保始终可见）
    std::string current_input = terminal.getCurrentInput();
    size_t cursor_position = terminal.getCursorPosition();
    
    std::string before_cursor = current_input.substr(0, cursor_position);
    std::string cursor_char = cursor_position < current_input.length() ? 
                              current_input.substr(cursor_position, 1) : " ";
    std::string after_cursor = cursor_position < current_input.length() ? 
                               current_input.substr(cursor_position + 1) : "";
    
    Elements input_elements;

    // 优化后的提示符样式（类似 zsh/powerlevel10k，使用图标和反白效果）
    // 第一部分：用户信息（用户名@主机名，反白显示）
    std::string username = terminal.getUsername();
    std::string hostname = terminal.getHostname();

    // 用户信息块（反白效果）
    input_elements.push_back(
        text(std::string(" ") + icons::TERMINAL + " ") |
        bgcolor(Color::Green) |
        color(Color::Black) |
        bold
    );

    input_elements.push_back(
        text(" " + username + "@" + hostname + " ") |
        bgcolor(Color::Green) |
        color(Color::Black) |
        bold
    );

    // 分隔符
    input_elements.push_back(
        text(" ") |
        bgcolor(colors.background)
    );

    // 第二部分：目录信息（蓝色反白）
    std::string dir = terminal.getCurrentDir();
    const char* home = getenv("HOME");
    if (home && dir.find(home) == 0) {
        dir = "~" + dir.substr(strlen(home));
    }
    // 如果目录名太长，只显示最后一部分
    if (dir.length() > 25) {
        size_t last_slash = dir.find_last_of('/');
        if (last_slash != std::string::npos && last_slash < dir.length() - 1) {
            dir = "..." + dir.substr(last_slash);
        }
    }

    input_elements.push_back(
        text(std::string(" ") + icons::FOLDER + " ") |
        bgcolor(Color::Blue) |
        color(Color::White) |
        bold
    );

    input_elements.push_back(
        text(" " + dir + " ") |
        bgcolor(Color::Blue) |
        color(Color::White) |
        bold
    );

    // 分隔符
    input_elements.push_back(
        text(" ") |
        bgcolor(colors.background)
    );

    // 第三部分：时间戳（青色背景）
    std::string time_str = terminal.getCurrentTime();
    input_elements.push_back(
        text(std::string(" ") + icons::CLOCK + " ") |
        bgcolor(Color::Cyan) |
        color(Color::Black) |
        bold
    );

    input_elements.push_back(
        text(" " + time_str + " ") |
        bgcolor(Color::Cyan) |
        color(Color::Black)
    );

    // 第四部分：Git 分支（如果有，金色背景）
    std::string git_branch = terminal.getGitBranch();
    if (!git_branch.empty()) {
        // 分隔符
        input_elements.push_back(
            text(" ") |
            bgcolor(colors.background)
        );

        // Git 分支块
        input_elements.push_back(
            text(std::string(" ") + icons::GIT + " ") |
            bgcolor(Color::Yellow) |
            color(Color::Black) |
            bold
        );

        input_elements.push_back(
            text(" " + git_branch + " ") |
            bgcolor(Color::Yellow) |
            color(Color::Black) |
            bold
        );
    }

    // 分隔符
    input_elements.push_back(
        text(" ") |
        bgcolor(colors.background)
    );

    // 第五部分：状态指示器（根据终端状态）
    Color status_bg = Color::Green; // 默认成功状态
    std::string status_icon = icons::SUCCESS;

    // 可以根据终端的最后命令状态来设置颜色
    // 这里暂时使用绿色成功状态

    input_elements.push_back(
        text(" " + status_icon + " ") |
        bgcolor(status_bg) |
        color(Color::White) |
        bold
    );

    // 最终的提示符箭头
    input_elements.push_back(
        text(" ") |
        bgcolor(colors.background)
    );

    input_elements.push_back(
        text(std::string(icons::ARROW_RIGHT) + " ") |
        color(Color::Green) |
        bold
    );
    
    // 用户输入（白色，更清晰）
    input_elements.push_back(text(before_cursor) | color(Color::White));
    
    // 块状光标（更醒目的样式）
    if (cursor_position < current_input.length()) {
        input_elements.push_back(
            text(cursor_char) | 
            bgcolor(Color::Green) | 
            color(Color::Black) | 
            bold
        );
        input_elements.push_back(text(after_cursor) | color(Color::White));
    } else {
        // 光标在行尾（显示为绿色竖线）
        input_elements.push_back(
            text("│") | 
            color(Color::Green) | 
            bold
        );
    }
    
    // 构建最终布局：输出区域 + 输入行（固定底部）
    Element output_area = vbox(output_lines);
    Element input_line = hbox(input_elements);
    
    // 使用 flex 确保输出区域可以滚动，输入行固定在底部
    return vbox({
        output_area | flex,  // 输出区域可滚动
        input_line            // 输入行固定在底部
    }) | 
           size(HEIGHT, EQUAL, height) | 
           bgcolor(Color::RGB(20, 20, 25));  // 深色终端背景
}

} // namespace ui
} // namespace pnana

