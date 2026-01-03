#include "features/terminal.h"
#include "features/terminal/terminal_builtin.h"
#include "features/terminal/terminal_completion.h"
#include "features/terminal/terminal_parser.h"
#include "features/terminal/terminal_shell.h"
#include "features/terminal/terminal_utils.h"
#include "ui/icons.h"
#include <cstdlib>
#include <ftxui/dom/elements.hpp>
#include <unistd.h>

using namespace ftxui;

namespace pnana {
namespace features {

Terminal::Terminal(ui::Theme& theme)
    : theme_(theme), visible_(false), history_index_(0), max_history_size_(100), current_input_(""),
      cursor_position_(0), max_output_lines_(1000), scroll_offset_(0), current_directory_(".") {
    // 初始化当前目录
    char* cwd = getcwd(nullptr, 0);
    if (cwd) {
        current_directory_ = cwd;
        free(cwd);
    }
}

void Terminal::setVisible(bool visible) {
    visible_ = visible;
    if (visible) {
        // 重置历史索引
        history_index_ = 0;
    }
}

void Terminal::handleInput(const std::string& input) {
    current_input_ = input;
    // 保持光标位置在有效范围内
    if (cursor_position_ > current_input_.length()) {
        cursor_position_ = current_input_.length();
    }
}

void Terminal::setCursorPosition(size_t pos) {
    cursor_position_ = std::min(pos, current_input_.length());
}

void Terminal::handleKeyEvent(const std::string& key) {
    if (key == "ArrowLeft") {
        if (cursor_position_ > 0) {
            cursor_position_--;
        }
    } else if (key == "ArrowRight") {
        if (cursor_position_ < current_input_.length()) {
            cursor_position_++;
        }
    } else if (key == "Home") {
        cursor_position_ = 0;
    } else if (key == "End") {
        cursor_position_ = current_input_.length();
    } else if (key == "ArrowUp") {
        // 浏览命令历史
        if (!command_history_.empty()) {
            if (history_index_ < command_history_.size()) {
                history_index_++;
            }
            if (history_index_ > 0 && history_index_ <= command_history_.size()) {
                current_input_ = command_history_[command_history_.size() - history_index_];
                cursor_position_ = current_input_.length();
            }
        }
    } else if (key == "ArrowDown") {
        // 浏览命令历史（向下）
        if (history_index_ > 0) {
            history_index_--;
            if (history_index_ == 0) {
                current_input_ = "";
            } else {
                current_input_ = command_history_[command_history_.size() - history_index_];
            }
            cursor_position_ = current_input_.length();
        }
    } else if (key == "Backspace") {
        if (cursor_position_ > 0) {
            current_input_.erase(cursor_position_ - 1, 1);
            cursor_position_--;
        }
    } else if (key == "Delete") {
        if (cursor_position_ < current_input_.length()) {
            current_input_.erase(cursor_position_, 1);
        }
    }
}

void Terminal::executeCommand(const std::string& command) {
    if (command.empty()) {
        // 空命令，不添加任何输出，输入行会显示提示符
        return;
    }

    using namespace terminal;

    // 检查是否是后台命令
    std::string cmd;
    bool is_background = CommandParser::isBackgroundCommand(command, cmd);

    // 添加到历史
    if (command_history_.empty() || command_history_.back() != command) {
        command_history_.push_back(command);
        if (command_history_.size() > max_history_size_) {
            command_history_.pop_front();
        }
    }
    history_index_ = 0;

    // 显示命令（带提示符）
    addOutputLine(buildPrompt() + command, true);

    // 方案：所有命令都通过系统 shell 执行，以支持所有 Linux 命令和参数
    // 这样可以：
    // 1. 支持所有系统命令（ls, grep, find, git, python, 等）
    // 2. 支持所有命令参数（-al, --help, -r, 等）
    // 3. 支持所有 shell 特性（管道、重定向、环境变量等）
    // 4. 自动处理命令别名、PATH 查找等

    // 特殊处理：cd 命令需要更新当前目录
    std::vector<std::string> args = CommandParser::parse(cmd);
    if (!args.empty() && args[0] == "cd") {
        // cd 命令需要特殊处理，因为它需要改变终端的工作目录
        std::string cd_result = BuiltinCommandExecutor::execute(
            "cd",
            args.size() > 1 ? std::vector<std::string>(args.begin() + 1, args.end())
                            : std::vector<std::string>(),
            current_directory_, output_lines_);
        // cd 命令通常没有输出，但如果有错误会返回错误信息
        if (!cd_result.empty()) {
            std::istringstream iss(cd_result);
            std::string line;
            while (std::getline(iss, line)) {
                addOutputLine(line, false);
            }
        }
        return;
    }

    // 特殊处理：clear 命令需要清空输出
    if (!args.empty() && (args[0] == "clear" || args[0] == "cls")) {
        BuiltinCommandExecutor::execute("clear", std::vector<std::string>(), current_directory_,
                                        output_lines_);
        return;
    }

    // 所有其他命令都通过 shell 执行
    // 这样可以支持所有 Linux 命令和参数，无需手动解析
    std::string result =
        ShellCommandExecutor::executeShellCommand(cmd, is_background, current_directory_);

    if (!result.empty()) {
        // 如果结果包含多行，需要分割
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            addOutputLine(line, false);
        }
    }
}

ftxui::Element Terminal::render(int /* height */) {
    // 渲染逻辑已迁移到 ui/terminal_ui.cpp
    // 这里保留是为了向后兼容，实际应该使用 ui::renderTerminal
    return text(""); // 占位符，实际不会使用
}

void Terminal::addOutputLine(const std::string& line, bool is_command) {
    output_lines_.push_back(TerminalLine(line, is_command));

    // 限制输出行数
    if (output_lines_.size() > max_output_lines_) {
        output_lines_.erase(output_lines_.begin(),
                            output_lines_.begin() + (output_lines_.size() - max_output_lines_));
    }
}

std::string Terminal::buildPrompt() const {
    using namespace terminal;

    // 构建提示符字符串（用于输出历史）
    std::ostringstream oss;

    // 第一部分：用户名@主机名
    oss << TerminalUtils::getUsername() << "@" << TerminalUtils::getHostname();

    // 分隔符
    oss << " · ";

    // 第二部分：时间戳
    oss << TerminalUtils::getCurrentTime();

    // 分隔符
    oss << " · ";

    // 第三部分：目录路径
    std::string dir = TerminalUtils::simplifyPath(current_directory_);
    dir = TerminalUtils::truncatePath(dir, 25);
    oss << dir;

    // 第四部分：Git 分支（如果有）
    std::string git_branch = TerminalUtils::getGitBranch(current_directory_);
    if (!git_branch.empty()) {
        oss << " · git:" << git_branch;
    }

    // 提示符结束符号（与输入行保持一致）
    oss << " → ";

    return oss.str();
}

std::string Terminal::getUsername() const {
    return terminal::TerminalUtils::getUsername();
}

std::string Terminal::getHostname() const {
    return terminal::TerminalUtils::getHostname();
}

std::string Terminal::getCurrentDir() const {
    return current_directory_;
}

std::string Terminal::getGitBranch() const {
    return terminal::TerminalUtils::getGitBranch(current_directory_);
}

std::string Terminal::getCurrentTime() const {
    return terminal::TerminalUtils::getCurrentTime();
}

ftxui::Color Terminal::getPromptColor() const {
    return Color::Green; // 使用绿色，更像真实终端
}

ftxui::Color Terminal::getCommandColor() const {
    return Color::Green; // 命令也使用绿色
}

ftxui::Color Terminal::getOutputColor() const {
    auto& colors = theme_.getColors();
    return colors.foreground;
}

ftxui::Color Terminal::getErrorColor() const {
    return Color::Red;
}

void Terminal::clear() {
    output_lines_.clear();
    // 清空后不显示任何消息，更像真实终端
}

// 保留旧的方法以保持兼容性（已移至各个模块）
std::vector<std::string> Terminal::parseCommand(const std::string& command) {
    return terminal::CommandParser::parse(command);
}

std::string Terminal::executeBuiltinCommand(const std::string& command,
                                            const std::vector<std::string>& args) {
    return terminal::BuiltinCommandExecutor::execute(command, args, current_directory_,
                                                     output_lines_);
}

std::string Terminal::executeSystemCommand(const std::string& command,
                                           const std::vector<std::string>& args) {
    return terminal::ShellCommandExecutor::executeSystemCommand(command, args, current_directory_);
}

std::string Terminal::executeShellCommand(const std::string& command, bool background) {
    return terminal::ShellCommandExecutor::executeShellCommand(command, background,
                                                               current_directory_);
}

bool Terminal::handleTabCompletion() {
    std::string completed;
    size_t new_pos;

    bool success = terminal::TerminalCompletion::complete(current_input_, cursor_position_,
                                                          current_directory_, completed, new_pos);

    if (success) {
        current_input_ = completed;
        cursor_position_ = new_pos;
        return true;
    }

    return false;
}

// 滚动功能实现
void Terminal::scrollUp() {
    // 向上滚动：增加偏移量，最多滚动到输出历史的开始
    if (scroll_offset_ < output_lines_.size()) {
        scroll_offset_ += 1;
    }
}

void Terminal::scrollDown() {
    // 向下滚动：减少偏移量，最少滚动到最新输出
    if (scroll_offset_ > 0) {
        scroll_offset_ -= 1;
    }
}

void Terminal::scrollToTop() {
    // 滚动到最顶部
    scroll_offset_ = output_lines_.size();
}

void Terminal::scrollToBottom() {
    // 滚动到最底部（最新输出）
    scroll_offset_ = 0;
}

} // namespace features
} // namespace pnana
