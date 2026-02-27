#ifndef PNANA_FEATURES_TERMINAL_H
#define PNANA_FEATURES_TERMINAL_H

#include "features/terminal/terminal_line_buffer.h"
#include "ui/theme.h"
#include <atomic>
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace pnana {
namespace features {

// 终端输出行（持续 shell 模式下均为 PTY 原始输出）
struct TerminalLine {
    std::string content;
    bool has_ansi_colors;

    TerminalLine(const std::string& c, bool ansi_colors = false)
        : content(c), has_ansi_colors(ansi_colors) {}
};

// 在线终端：持续运行的 shell 会话，FTXUI 仅负责边框
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

    // 按键透传到 shell
    void handleKeyEvent(const std::string& key);

    // 向 shell 写入字符串（供 handler 调用）
    void writeToShell(const std::string& input);

    // 渲染
    ftxui::Element render(int height);

    // 清空终端
    void clear();

    // 中断 shell（发送 SIGINT）
    void interruptCommand();

    // 获取方法（供 UI 使用）
    ui::Theme& getTheme() const {
        return theme_;
    }
    // 线程安全快照，供主线程渲染使用（避免与输出线程数据竞争）
    std::vector<TerminalLine> getOutputLinesSnapshot() const;
    // 尚未换行的缓冲（用户输入回显等），需与 output_lines 一并显示；线程安全
    std::string getPendingLineSnapshot() const;
    size_t getPendingCursorPositionSnapshot() const;

    // 滚动功能
    void scrollUp();
    void scrollDown();
    void scrollToTop();
    void scrollToBottom();
    size_t getScrollOffset() const {
        return scroll_offset_;
    }

    // 当有新输出时调用的回调（用于触发 UI 刷新，解决 PTY 输出不显示问题）
    void setOnOutputAdded(std::function<void()> cb) {
        on_output_added_ = std::move(cb);
    }

  private:
    ui::Theme& theme_;
    bool visible_;

    // 输出行（PTY 原始输出，含 prompt）
    std::vector<TerminalLine> output_lines_;
    std::string pending_raw_;       // 尚未换行的原始 PTY 输出（用于跨 read 拼接）
    std::string pending_line_;      // 解析后的显示内容（供 UI 使用）
    size_t pending_cursor_pos_ = 0; // 光标在 pending 行中的位置
    terminal::PendingLineBuffer pending_line_buffer_;
    size_t max_output_lines_;
    size_t scroll_offset_;

    // 当前工作目录（用于启动 shell）
    std::string current_directory_;

    // Shell 会话
    bool shell_running_;
    pid_t current_pid_;
    int current_pty_fd_;
    int current_slave_fd_;

    // 输出读取线程
    std::thread output_thread_;
    std::atomic<bool> output_thread_running_;
    mutable std::mutex output_mutex_;

    // Backspace 与左箭头都发送 \b，无法从 PTY 输出区分；记录我们发送的 Backspace
    // 次数，用于解析时识别
    std::atomic<int> pending_backspace_count_{0};

    // 新输出时触发 UI 刷新的回调
    std::function<void()> on_output_added_;

    // 刷新节流：避免每字符都触发 UI 更新
    std::chrono::steady_clock::time_point last_refresh_time_;

    static constexpr int REFRESH_THROTTLE_MS = 33; // ~30fps

    void addOutputLine(const std::string& line);
    void addOutputLines(const std::vector<std::string>& lines);
    void startShellSession();
    void stopShellSession();
    void startOutputThread(int pty_fd);
    void stopOutputThread();
    void readPTYOutput(int pty_fd);
    void cleanupShell();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_H
