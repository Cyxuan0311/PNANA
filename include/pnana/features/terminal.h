#ifndef PNANA_FEATURES_TERMINAL_H
#define PNANA_FEATURES_TERMINAL_H

#include <string>
#include <vector>
#include <deque>
#include <memory>
#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace features {

// 终端输出行
struct TerminalLine {
    std::string content;
    bool is_command;  // true 表示是用户输入的命令，false 表示是输出
    
    TerminalLine(const std::string& c, bool is_cmd = false)
        : content(c), is_command(is_cmd) {}
};

// 在线终端
class Terminal {
public:
    explicit Terminal(ui::Theme& theme);
    
    // 显示/隐藏
    void setVisible(bool visible);
    bool isVisible() const { return visible_; }
    void toggle() { visible_ = !visible_; }
    
    // 命令输入
    void handleInput(const std::string& input);
    void handleKeyEvent(const std::string& key);
    
    // 执行命令
    void executeCommand(const std::string& command);
    
    // 渲染
    ftxui::Element render(int height);
    
    // 获取当前输入
    std::string getCurrentInput() const { return current_input_; }
    
    // 获取光标位置
    size_t getCursorPosition() const { return cursor_position_; }
    
    // 设置光标位置
    void setCursorPosition(size_t pos);
    
    // 清空终端
    void clear();

private:
    ui::Theme& theme_;
    bool visible_;
    
    // 命令历史
    std::deque<std::string> command_history_;
    size_t history_index_;  // 当前浏览的历史索引（0 = 最新）
    size_t max_history_size_;
    
    // 当前输入
    std::string current_input_;
    size_t cursor_position_;  // 光标在输入中的位置
    
    // 输出行
    std::vector<TerminalLine> output_lines_;
    size_t max_output_lines_;
    
    // 当前工作目录
    std::string current_directory_;
    
    // 命令执行
    std::string executeBuiltinCommand(const std::string& command, const std::vector<std::string>& args);
    std::string executeSystemCommand(const std::string& command, const std::vector<std::string>& args);
    std::vector<std::string> parseCommand(const std::string& command);
    
    // 辅助方法
    void addOutputLine(const std::string& line, bool is_command = false);
    void updatePrompt();
    std::string getPrompt() const;
    std::string getHostname() const;
    std::string getUsername() const;
    std::string getCurrentDir() const;
    std::string getGitBranch() const;  // 获取 Git 分支
    std::string getCurrentTime() const;  // 获取当前时间
    
    // 样式
    ftxui::Color getPromptColor() const;
    ftxui::Color getCommandColor() const;
    ftxui::Color getOutputColor() const;
    ftxui::Color getErrorColor() const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_H

