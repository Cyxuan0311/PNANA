#include "features/terminal.h"
#include "features/terminal/terminal_color.h"
#include "features/terminal/terminal_line_buffer.h"
#include "features/terminal/terminal_pty.h"
#include <chrono>
#include <cstdlib>
#include <ftxui/dom/elements.hpp>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

using namespace ftxui;

namespace pnana {
namespace features {

namespace {

// 将按键名映射为 PTY 应接收的字节序列
// 部分 shell 需要 \r\n 才能正确执行命令（PTY 模拟真实终端行为）
std::string keyToEscape(const std::string& key) {
    if (key == "return" || key == "ctrl_m")
        return "\r\n";
    if (key == "Tab" || key == "tab")
        return "\t";
    if (key == "Backspace")
        return "\x08"; // BS，WSL/bash 下 VERASE 常为 \x08
    if (key == "ctrl_h")
        return "\x08"; // Ctrl+H 也映射为删除
    if (key.size() == 1 && static_cast<unsigned char>(key[0]) == 0x7f)
        return "\x08"; // Character(DEL) 也转为 BS
    if (key == "ArrowUp" || key == "arrow_up")
        return "\x1b[A";
    if (key == "ArrowDown" || key == "arrow_down")
        return "\x1b[B";
    if (key == "ArrowLeft" || key == "arrow_left")
        return "\x1b[D";
    if (key == "ArrowRight" || key == "arrow_right")
        return "\x1b[C";
    if (key == "Home")
        return "\x1b[H";
    if (key == "End")
        return "\x1b[F";
    if (key == "Delete")
        return "\x1b[3~";
    if (key == "ctrl_c")
        return "\x03";
    if (key == "ctrl_d")
        return "\x04";
    if (key == "ctrl_z")
        return "\x1a";
    if (key == "ctrl_l")
        return "\x0c";
    if (key == "ctrl_u")
        return "\x15";
    if (key == "ctrl_k")
        return "\x0b";
    if (key == "ctrl_a")
        return "\x01";
    if (key == "ctrl_e")
        return "\x05";
    if (key == "ctrl_w")
        return "\x17";
    return "";
}

} // namespace

Terminal::Terminal(ui::Theme& theme)
    : theme_(theme), visible_(false), pending_line_(""), max_output_lines_(1000), scroll_offset_(0),
      current_directory_("."), shell_running_(false), current_pid_(0), current_pty_fd_(-1),
      current_slave_fd_(-1), output_thread_running_(false) {
    pending_line_buffer_.setPendingBackspaceCount(&pending_backspace_count_);
    char* cwd = getcwd(nullptr, 0);
    if (cwd) {
        current_directory_ = cwd;
        free(cwd);
    }
}

void Terminal::setVisible(bool visible) {
    if (visible_ == visible)
        return;
    visible_ = visible;
    if (visible) {
        startShellSession();
    } else {
        stopShellSession();
    }
}

void Terminal::handleKeyEvent(const std::string& key) {
    // PageUp/PageDown 用于滚动，不发送到 shell
    if (key == "PageUp") {
        scrollUp();
        return;
    }
    if (key == "PageDown") {
        scrollDown();
        return;
    }

    std::string esc = keyToEscape(key);
    if (!esc.empty()) {
        // ArrowLeft/ArrowRight 会令 shell 回显 \b 做光标移动，不应消耗 pending_backspace_count_。
        // 发送方向键前清零残留的 bs，避免误将光标移动当作 Backspace 截断。
        if (key == "ArrowLeft" || key == "ArrowRight") {
            pending_backspace_count_.store(0);
        } else if (esc.size() == 1 && static_cast<unsigned char>(esc[0]) == 0x08) {
            pending_backspace_count_++;
        }
        writeToShell(esc);
        return;
    }

    // 单字符直接写入（含 Ctrl+H 等可能产生 \b 的按键）
    if (key.length() == 1) {
        if (static_cast<unsigned char>(key[0]) == 0x08) {
            pending_backspace_count_++;
        }
        writeToShell(key);
    }
}

void Terminal::writeToShell(const std::string& input) {
    if (shell_running_ && current_pty_fd_ >= 0) {
        terminal::PTYExecutor::writeInput(current_pty_fd_, input);
    }
}

void Terminal::clear() {
    std::lock_guard<std::mutex> lock(output_mutex_);
    output_lines_.clear();
    pending_raw_.clear();
    pending_line_.clear();
    pending_cursor_pos_ = 0;
    pending_backspace_count_ = 0;
    pending_line_buffer_.reset();
    scroll_offset_ = 0;
}

void Terminal::interruptCommand() {
    if (shell_running_ && current_pid_ > 0) {
        terminal::PTYExecutor::sendSignal(current_pid_, SIGINT);
    }
}

void Terminal::addOutputLine(const std::string& line) {
    if (output_lines_.size() >= max_output_lines_) {
        output_lines_.erase(output_lines_.begin());
    }
    bool has_ansi = terminal::AnsiColorParser::hasAnsiCodes(line);
    output_lines_.push_back(TerminalLine(line, has_ansi));
}

void Terminal::addOutputLines(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
        addOutputLine(line);
    }
}

std::vector<TerminalLine> Terminal::getOutputLinesSnapshot() const {
    std::lock_guard<std::mutex> lock(output_mutex_);
    return output_lines_;
}

std::string Terminal::getPendingLineSnapshot() const {
    std::lock_guard<std::mutex> lock(output_mutex_);
    return pending_line_;
}

size_t Terminal::getPendingCursorPositionSnapshot() const {
    std::lock_guard<std::mutex> lock(output_mutex_);
    return pending_cursor_pos_;
}

void Terminal::startShellSession() {
    if (shell_running_)
        return;

    terminal::PTYResult result = terminal::PTYExecutor::createInteractiveShell(current_directory_);
    if (!result.success) {
        addOutputLine("Error: " + result.error);
        return;
    }

    shell_running_ = true;
    current_pid_ = result.pid;
    current_pty_fd_ = result.master_fd;
    current_slave_fd_ = result.slave_fd;
    startOutputThread(result.master_fd);
    // termios 已在 createInteractiveShell 子进程中设置 VERASE=\x08，与 keyToEscape 的 Backspace
    // 映射一致
}

void Terminal::stopShellSession() {
    if (!shell_running_)
        return;
    writeToShell("exit\n");
    output_thread_running_ = false;
    if (output_thread_.joinable()) {
        output_thread_.join();
    }
    cleanupShell();
}

void Terminal::startOutputThread(int pty_fd) {
    stopOutputThread();
    output_thread_running_ = true;
    output_thread_ = std::thread([this, pty_fd]() {
        readPTYOutput(pty_fd);
    });
}

void Terminal::stopOutputThread() {
    output_thread_running_ = false;
    if (output_thread_.joinable()) {
        output_thread_.join();
    }
}

void Terminal::readPTYOutput(int pty_fd) {
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];

    auto drainAndAdd = [this, pty_fd, &buffer, BUFFER_SIZE]() {
        bool had_output = false;
        while (true) {
            ssize_t n = terminal::PTYExecutor::readOutput(pty_fd, buffer, BUFFER_SIZE);
            if (n > 0) {
                had_output = true;
                bool had_complete_line = false;
                {
                    std::lock_guard<std::mutex> lock(output_mutex_);
                    std::string raw = pending_raw_ + std::string(buffer, static_cast<size_t>(n));
                    pending_raw_.clear();
                    size_t start = 0;
                    size_t end;
                    bool is_first_line_in_batch = true;
                    while ((end = raw.find('\n', start)) != std::string::npos) {
                        std::string line_to_add;
                        if (is_first_line_in_batch && !pending_line_.empty()) {
                            // 第一行即用户刚提交的输入，使用我们持续解析的
                            // pending_line_（已正确处理历史切换）
                            line_to_add = pending_line_;
                        } else {
                            std::string line_raw = raw.substr(start, end - start);
                            pending_line_buffer_.reset();
                            pending_line_buffer_.feed(line_raw);
                            pending_line_buffer_.flushReplace();
                            line_to_add = pending_line_buffer_.getLine();
                            if (line_to_add.empty())
                                line_to_add = line_raw;
                        }
                        addOutputLine(line_to_add);
                        had_complete_line = true;
                        start = end + 1;
                        is_first_line_in_batch = false;
                        pending_line_buffer_.reset();
                    }
                    if (start < raw.length()) {
                        std::string new_pending = raw.substr(start);
                        std::string to_feed;
                        if (had_complete_line) {
                            // 中间有换行：reset 后 feed 剩余部分（新行起始）
                            pending_line_buffer_.reset();
                            to_feed = new_pending;
                        } else {
                            // 无换行：仅 feed 本次 read 的新字节，避免重复处理历史 \b 耗尽 bs_count
                            to_feed = std::string(buffer, static_cast<size_t>(n));
                        }
                        if (!to_feed.empty()) {
                            pending_line_buffer_.feed(to_feed);
                            pending_line_buffer_.flushReplace();
                        }
                        pending_raw_ = new_pending;
                        pending_line_ = pending_line_buffer_.getLine();
                        pending_cursor_pos_ = pending_line_buffer_.getCursorPos();
                    } else {
                        pending_raw_.clear();
                        pending_line_.clear();
                        pending_cursor_pos_ = 0;
                        pending_line_buffer_.reset();
                    }
                }
            } else {
                break;
            }
        }
        // 每个 poll 周期最多触发一次；节流到 ~30fps 避免事件队列洪泛
        if (had_output && on_output_added_) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refresh_time_)
                    .count();
            if (elapsed >= REFRESH_THROTTLE_MS) {
                last_refresh_time_ = now;
                on_output_added_();
            }
        }
    };

    constexpr int CURSOR_BLINK_INTERVAL_MS = 500;
    auto last_cursor_tick = std::chrono::steady_clock::now();

    while (output_thread_running_) {
        struct pollfd pfd = {};
        pfd.fd = pty_fd;
        pfd.events = POLLIN;
        int ret = poll(&pfd, 1, 16); // 16ms 超时 (~60fps)，输入回显更及时

        if (ret > 0 && (pfd.revents & POLLIN)) {
            drainAndAdd();
        } else if (ret < 0) {
            break;
        }

        // 定期触发刷新以保持光标闪烁（终端空闲时）
        auto now = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cursor_tick).count();
        if (elapsed >= CURSOR_BLINK_INTERVAL_MS && on_output_added_) {
            last_cursor_tick = now;
            on_output_added_();
        }

        if (current_pid_ > 0 && !terminal::PTYExecutor::isProcessRunning(current_pid_)) {
            drainAndAdd();
            shell_running_ = false;
            break;
        }
    }
}

void Terminal::cleanupShell() {
    stopOutputThread();
    if (current_pty_fd_ >= 0) {
        terminal::PTYExecutor::closePTY(current_pty_fd_);
        current_pty_fd_ = -1;
    }
    if (current_slave_fd_ >= 0) {
        terminal::PTYExecutor::closeSlave(current_slave_fd_);
        current_slave_fd_ = -1;
    }
    shell_running_ = false;
    current_pid_ = 0;
}

ftxui::Element Terminal::render(int /* height */) {
    return text("");
}

void Terminal::scrollUp() {
    std::lock_guard<std::mutex> lock(output_mutex_);
    if (scroll_offset_ < output_lines_.size()) {
        scroll_offset_ += 1;
    }
}

void Terminal::scrollDown() {
    if (scroll_offset_ > 0) {
        scroll_offset_ -= 1;
    }
}

void Terminal::scrollToTop() {
    std::lock_guard<std::mutex> lock(output_mutex_);
    scroll_offset_ = output_lines_.size();
}

void Terminal::scrollToBottom() {
    scroll_offset_ = 0;
}

} // namespace features
} // namespace pnana
