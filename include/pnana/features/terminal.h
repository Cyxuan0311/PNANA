#ifndef PNANA_FEATURES_TERMINAL_H
#define PNANA_FEATURES_TERMINAL_H

#include "ui/theme.h"
#include <deque>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace features {

// 终端输出行
struct TerminalLine {
    std::string content;
    bool is_command;      // true 表示是用户输入的命令，false 表示是输出
    bool has_ansi_colors; // 是否包含ANSI颜色码

    TerminalLine(const std::string& c, bool is_cmd = false, bool ansi_colors = false)
        : content(c), is_command(is_cmd), has_ansi_colors(ansi_colors) {}
};

// 在线终端
class Terminal {
  public:
    explicit Terminal(ui::Theme& theme);

    // 显示/隐藏
    void setVisible(bool visible);
    bool isVisible() const {
        return visible_;
    }
    void toggle() {
        visible_ = !visible_;
    }

    // 命令输入
    void handleInput(const std::string& input);
    void handleKeyEvent(const std::string& key);

    // Tab 补全
    bool handleTabCompletion();

    // 执行命令
    void executeCommand(const std::string& command);

    // 渲染
    ftxui::Element render(int height);

    // 获取当前输入
    std::string getCurrentInput() const {
        return current_input_;
    }

    // 获取光标位置
    size_t getCursorPosition() const {
        return cursor_position_;
    }

    // 设置光标位置
    void setCursorPosition(size_t pos);

    // 清空终端
    void clear();

    // 中断当前运行的命令
    void interruptCommand();

    // 获取方法（供UI使用）
    ui::Theme& getTheme() const {
        return theme_;
    }
    const std::vector<TerminalLine>& getOutputLines() const {
        return output_lines_;
    }
    std::string getUsername() const;
    std::string getHostname() const;
    std::string getCurrentDir() const;
    std::string getGitBranch() const;
    std::string getCurrentTime() const;

    // 滚动功能
    void scrollUp();
    void scrollDown();
    void scrollToTop();
    void scrollToBottom();
    size_t getScrollOffset() const {
        return scroll_offset_;
    }

  private:
    ui::Theme& theme_;
    bool visible_;

    // 命令历史
    std::deque<std::string> command_history_;
    size_t history_index_; // 当前浏览的历史索引（0 = 最新）
    size_t max_history_size_;

    // 当前输入
    std::string current_input_;
    size_t cursor_position_; // 光标在输入中的位置

    // 输出行
    std::vector<TerminalLine> output_lines_;
    size_t max_output_lines_;

    // 输出滚动
    size_t scroll_offset_; // 从输出的开头开始的偏移量（用于向上滚动）

    // 当前工作目录
    std::string current_directory_;

    // 命令执行状态
    bool command_running_; // 是否有命令正在运行
    pid_t current_pid_;    // 当前运行命令的进程ID（用于中断）

    // 命令执行（保留以保持兼容性，实际已移至各个模块）
    std::string executeBuiltinCommand(const std::string& command,
                                      const std::vector<std::string>& args);
    std::string executeSystemCommand(const std::string& command,
                                     const std::vector<std::string>& args);
    std::string executeShellCommand(const std::string& command, bool background = false);
    std::vector<std::string> parseCommand(const std::string& command);

    // 辅助方法
    void addOutputLine(const std::string& line, bool is_command = false);
    void addOutputLines(const std::vector<std::string>& lines, bool is_command = false);
    std::string buildPrompt() const; // 构建提示符字符串

    // 样式
    ftxui::Color getPromptColor() const;
    ftxui::Color getCommandColor() const;
    ftxui::Color getOutputColor() const;
    ftxui::Color getErrorColor() const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_H
