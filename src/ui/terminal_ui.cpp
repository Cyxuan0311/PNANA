#include "ui/terminal_ui.h"
#include "features/terminal/terminal_color.h"
#include "ui/icons.h"
#include <algorithm>
#include <cstring>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace ui {

// 解析并渲染带样式的历史命令提示符
Element renderStyledPrompt(const std::string& command_line, features::Terminal& terminal) {
    auto& theme = terminal.getTheme();
    auto& colors = theme.getColors();

    // 定义终端UI专用的颜色映射，使用主题色
    ftxui::Color terminal_user_bg = colors.success;      // 用户信息背景
    ftxui::Color terminal_user_fg = colors.background;   // 用户信息前景
    ftxui::Color terminal_dir_bg = colors.function;      // 目录信息背景
    ftxui::Color terminal_dir_fg = colors.background;    // 目录信息前景
    ftxui::Color terminal_time_bg = colors.comment;      // 时间戳背景
    ftxui::Color terminal_time_fg = colors.background;   // 时间戳前景
    ftxui::Color terminal_git_bg = colors.keyword;       // Git分支背景
    ftxui::Color terminal_git_fg = colors.background;    // Git分支前景
    ftxui::Color terminal_status_bg = colors.success;    // 状态指示器背景
    ftxui::Color terminal_status_fg = colors.background; // 状态指示器前景
    ftxui::Color terminal_arrow = colors.success;        // 箭头颜色
    ftxui::Color terminal_command = colors.foreground;   // 命令文本颜色

    Elements elements;

    // 查找箭头 " → " 的位置，这标志着提示符的结束
    size_t arrow_pos = command_line.find(" → ");
    if (arrow_pos == std::string::npos) {
        // 如果没有找到箭头，当作普通文本处理
        return text(command_line) | color(Color::Green);
    }

    std::string prompt_part = command_line.substr(0, arrow_pos);
    std::string command_part = command_line.substr(arrow_pos + 3); // 跳过 " → "

    // 解析提示符的各个部分（用 " · " 分隔）
    std::vector<std::string> prompt_parts;
    size_t start = 0;
    size_t pos = 0;

    while ((pos = prompt_part.find(" · ", start)) != std::string::npos) {
        prompt_parts.push_back(prompt_part.substr(start, pos - start));
        start = pos + 3; // 跳过 " · "
    }
    // 添加最后一个部分
    if (start < prompt_part.length()) {
        prompt_parts.push_back(prompt_part.substr(start));
    }

    // 如果解析失败，当作普通文本处理
    if (prompt_parts.size() < 3) {
        return text(command_line) | color(Color::Green);
    }

    // 第一部分：用户信息（用户名@主机名）
    if (!prompt_parts.empty()) {
        std::string user_host = prompt_parts[0];

        // 终端图标
        elements.push_back(text(std::string(" ") + icons::TERMINAL + " ") |
                           bgcolor(terminal_user_bg) | color(terminal_user_fg) | bold);

        // 用户名@主机名
        elements.push_back(text(" " + user_host + " ") | bgcolor(terminal_user_bg) |
                           color(terminal_user_fg) | bold);

        // 分隔符
        elements.push_back(text(" ") | bgcolor(colors.background));
    }

    // 第二部分：目录信息（调整顺序，与输入提示符一致）
    if (prompt_parts.size() >= 3) {
        std::string dir = prompt_parts[2];

        elements.push_back(text(std::string(" ") + icons::FOLDER + " ") | bgcolor(terminal_dir_bg) |
                           color(terminal_dir_fg) | bold);

        elements.push_back(text(" " + dir + " ") | bgcolor(terminal_dir_bg) |
                           color(terminal_dir_fg) | bold);

        // 分隔符
        elements.push_back(text(" ") | bgcolor(colors.background));
    }

    // 第三部分：时间戳（调整到目录之后）
    if (prompt_parts.size() >= 2) {
        std::string time_str = prompt_parts[1];

        elements.push_back(text(std::string(" ") + icons::CLOCK + " ") | bgcolor(terminal_time_bg) |
                           color(terminal_time_fg) | bold);

        elements.push_back(text(" " + time_str + " ") | bgcolor(terminal_time_bg) |
                           color(terminal_time_fg));
    }

    // 第四部分：Git 分支（如果存在）
    if (prompt_parts.size() >= 4) {
        std::string git_part = prompt_parts[3];
        if (git_part.find("git:") == 0) {
            std::string git_branch = git_part.substr(4); // 移除 "git:" 前缀

            elements.push_back(text(" ") | bgcolor(colors.background));

            elements.push_back(text(std::string(" ") + icons::GIT + " ") |
                               bgcolor(terminal_git_bg) | color(terminal_git_fg) | bold);

            elements.push_back(text(" " + git_branch + " ") | bgcolor(terminal_git_bg) |
                               color(terminal_git_fg) | bold);
        }
    }

    // 分隔符
    elements.push_back(text(" ") | bgcolor(colors.background));

    // 状态指示器（历史命令默认成功状态）
    elements.push_back(text(" " + std::string(icons::SUCCESS) + " ") | bgcolor(terminal_status_bg) |
                       color(terminal_status_fg) | bold);

    // 最终的提示符箭头
    elements.push_back(text(" ") | bgcolor(colors.background));
    elements.push_back(text(std::string(icons::ARROW_RIGHT) + " ") | color(terminal_arrow) | bold);

    // 命令部分
    elements.push_back(text(command_part) | color(terminal_command));

    return hbox(elements);
}

Element renderTerminal(features::Terminal& terminal, int height) {
    if (!terminal.isVisible()) {
        return text("");
    }

    auto& theme = terminal.getTheme();
    auto& colors = theme.getColors();

    // 定义终端UI专用的颜色映射，使用主题色
    ftxui::Color terminal_user_bg = colors.success;      // 用户信息背景
    ftxui::Color terminal_user_fg = colors.background;   // 用户信息前景
    ftxui::Color terminal_dir_bg = colors.function;      // 目录信息背景
    ftxui::Color terminal_dir_fg = colors.background;    // 目录信息前景
    ftxui::Color terminal_time_bg = colors.comment;      // 时间戳背景
    ftxui::Color terminal_time_fg = colors.background;   // 时间戳前景
    ftxui::Color terminal_git_bg = colors.keyword;       // Git分支背景
    ftxui::Color terminal_git_fg = colors.background;    // Git分支前景
    ftxui::Color terminal_status_bg = colors.success;    // 状态指示器背景
    ftxui::Color terminal_status_fg = colors.background; // 状态指示器前景
    ftxui::Color terminal_arrow = colors.success;        // 箭头颜色
    ftxui::Color terminal_command = colors.foreground;   // 命令文本颜色
    ftxui::Color terminal_cursor_bg = colors.selection;  // 光标背景色

    // 输出区域和输入行
    Elements output_lines;
    const auto& output_lines_data = terminal.getOutputLines();
    size_t scroll_offset = terminal.getScrollOffset();

    // 计算可用高度：总高度 - 1（为输入行预留）
    int available_height = height - 1;
    if (available_height < 1) {
        available_height = 1; // 至少保留1行用于输出
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
            start_line = output_count > static_cast<size_t>(available_height)
                             ? output_count - available_height
                             : 0;
        }
    }

    // 显示历史输出（确保不超过可用高度）
    for (size_t i = start_line;
         i < output_lines_data.size() && (i - start_line) < static_cast<size_t>(available_height);
         ++i) {
        const auto& line = output_lines_data[i];
        if (line.is_command) {
            // 命令行：解析并渲染带样式的提示符
            output_lines.push_back(renderStyledPrompt(line.content, terminal));
        } else {
            // 输出行：如果包含ANSI颜色码，使用颜色解析器，否则使用普通文本
            if (line.has_ansi_colors) {
                output_lines.push_back(
                    pnana::features::terminal::AnsiColorParser::parse(line.content));
            } else {
                output_lines.push_back(text(line.content) | color(colors.foreground));
            }
        }
    }

    // 输入行 - 始终固定在最后一行（确保始终可见）
    std::string current_input = terminal.getCurrentInput();
    size_t cursor_position = terminal.getCursorPosition();

    std::string before_cursor = current_input.substr(0, cursor_position);
    std::string cursor_char =
        cursor_position < current_input.length() ? current_input.substr(cursor_position, 1) : " ";
    std::string after_cursor =
        cursor_position < current_input.length() ? current_input.substr(cursor_position + 1) : "";

    Elements input_elements;

    // 优化后的提示符样式（类似 zsh/powerlevel10k，使用图标和反白效果）
    // 第一部分：用户信息（用户名@主机名，反白显示）
    std::string username = terminal.getUsername();
    std::string hostname = terminal.getHostname();

    // 用户信息块（反白效果）
    input_elements.push_back(text(std::string(" ") + icons::TERMINAL + " ") |
                             bgcolor(terminal_user_bg) | color(terminal_user_fg) | bold);

    input_elements.push_back(text(" " + username + "@" + hostname + " ") |
                             bgcolor(terminal_user_bg) | color(terminal_user_fg) | bold);

    // 分隔符
    input_elements.push_back(text(" ") | bgcolor(colors.background));

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

    input_elements.push_back(text(std::string(" ") + icons::FOLDER + " ") |
                             bgcolor(terminal_dir_bg) | color(terminal_dir_fg) | bold);

    input_elements.push_back(text(" " + dir + " ") | bgcolor(terminal_dir_bg) |
                             color(terminal_dir_fg) | bold);

    // 分隔符
    input_elements.push_back(text(" ") | bgcolor(colors.background));

    // 第三部分：时间戳（青色背景）
    std::string time_str = terminal.getCurrentTime();
    input_elements.push_back(text(std::string(" ") + icons::CLOCK + " ") |
                             bgcolor(terminal_time_bg) | color(terminal_time_fg) | bold);

    input_elements.push_back(text(" " + time_str + " ") | bgcolor(terminal_time_bg) |
                             color(terminal_time_fg));

    // 第四部分：Git 分支（如果有，金色背景）
    std::string git_branch = terminal.getGitBranch();
    if (!git_branch.empty()) {
        // 分隔符
        input_elements.push_back(text(" ") | bgcolor(colors.background));

        // Git 分支块
        input_elements.push_back(text(std::string(" ") + icons::GIT + " ") |
                                 bgcolor(terminal_git_bg) | color(terminal_git_fg) | bold);

        input_elements.push_back(text(" " + git_branch + " ") | bgcolor(terminal_git_bg) |
                                 color(terminal_git_fg) | bold);
    }

    // 分隔符
    input_elements.push_back(text(" ") | bgcolor(colors.background));

    // 第五部分：状态指示器（根据终端状态）
    ftxui::Color status_bg = terminal_status_bg; // 使用主题色
    std::string status_icon = icons::SUCCESS;

    // 可以根据终端的最后命令状态来设置颜色
    // 这里暂时使用主题的成功状态

    input_elements.push_back(text(" " + status_icon + " ") | bgcolor(status_bg) |
                             color(terminal_status_fg) | bold);

    // 最终的提示符箭头
    input_elements.push_back(text(" ") | bgcolor(colors.background));

    input_elements.push_back(text(std::string(icons::ARROW_RIGHT) + " ") | color(terminal_arrow) |
                             bold);

    // 用户输入（使用主题前景色）
    input_elements.push_back(text(before_cursor) | color(terminal_command));

    // 块状光标（更醒目的样式）
    if (cursor_position < current_input.length()) {
        input_elements.push_back(text(cursor_char) | bgcolor(terminal_cursor_bg) |
                                 color(colors.background) | bold);
        input_elements.push_back(text(after_cursor) | color(terminal_command));
    } else {
        // 光标在行尾（显示为主题色竖线）
        input_elements.push_back(text("│") | color(terminal_cursor_bg) | bold);
    }

    // 构建最终布局：输出区域 + 输入行（固定底部）
    Element output_area = vbox(output_lines);
    Element input_line = hbox(input_elements);

    // 使用 flex 确保输出区域可以滚动，输入行固定在底部
    return vbox({
               output_area | flex, // 输出区域可滚动
               input_line          // 输入行固定在底部
           }) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.background); // 使用主题背景色
}

} // namespace ui
} // namespace pnana
