#include "features/terminal.h"
#include "features/terminal/terminal_color.h"
#include "features/terminal/terminal_key_map.h"
#include "features/terminal/terminal_line_buffer.h"
#include "features/terminal/terminal_pty.h"
#include "utils/logger.h"
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
    if (key == "ctrl_x")
        return "\x18";
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

Terminal::~Terminal() {
    // 析构时清空回调，避免输出线程或 Post 的 lambda 使用已析构的 Editor*
    on_shell_exit_ = nullptr;
    on_output_added_ = nullptr;
    output_thread_running_ = false;
    if (output_thread_.joinable()) {
        output_thread_.join();
    }
    cleanupShell();
}

void Terminal::setVisible(bool visible) {
    if (visible_ == visible)
        return;
    LOG("[Terminal] setVisible(" + std::string(visible ? "true" : "false") + ")");
    visible_ = visible;
    if (visible) {
        startShellSession();
    } else {
        stopShellSession();
    }
    LOG("[Terminal] setVisible done");
}

void Terminal::handleKeyEvent(const std::string& key) {
    if (key == "PageUp") {
        scrollUp();
        return;
    }
    if (key == "PageDown") {
        scrollDown();
        return;
    }

#ifdef BUILD_LIBVTERM_SUPPORT
    auto* sess = getActiveSession();
    if (sess && sess->isRunning()) {
        terminal::KeyEvent ev = terminal::ftxuiKeyToKeyEvent(key);
        if (ev.type != terminal::KeyEvent::Type::Char || ev.ch != 0) {
            sess->sendKey(ev);
            return;
        }
        if (key.length() == 1) {
            ev.type = terminal::KeyEvent::Type::Char;
            ev.ch = key[0];
            sess->sendKey(ev);
        }
        return;
    }
#endif

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
#ifdef BUILD_LIBVTERM_SUPPORT
    auto* sess = getActiveSession();
    if (sess && sess->isRunning()) {
        sess->sendBytes(input);
        return;
    }
#endif
    if (shell_running_ && current_pty_fd_ >= 0) {
        terminal::PTYExecutor::writeInput(current_pty_fd_, input);
    }
}

void Terminal::resize(int cols, int rows) {
#ifdef BUILD_LIBVTERM_SUPPORT
    if (useLibVTermPath()) {
        auto* sess = getActiveSession();
        if (sess)
            sess->resize(cols, rows);
        return;
    }
#endif
    if (current_pty_fd_ >= 0) {
        terminal::PTYExecutor::setTerminalSize(current_pty_fd_, rows, cols);
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
#ifdef BUILD_LIBVTERM_SUPPORT
    auto* sess = getActiveSession();
    if (sess && sess->isRunning()) {
        terminal::KeyEvent ev;
        ev.type = terminal::KeyEvent::Type::CtrlC;
        ev.ch = 0;
        sess->sendKey(ev);
        return;
    }
#endif
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
    LOG("[Terminal] startShellSession enter");
#ifdef BUILD_LIBVTERM_SUPPORT
    if (sessions_.empty()) {
        LOG("[Terminal] sessions empty, calling newLocalShellSession");
        int idx = newLocalShellSession(current_directory_);
        if (idx >= 0) {
            LOG("[Terminal] newLocalShellSession ok idx=" + std::to_string(idx));
            shell_running_ = true;
            return;
        }
        LOG("[Terminal] newLocalShellSession failed");
        addOutputLine("Error: Could not start shell session");
        return;
    }
    LOG("[Terminal] startShellSession done (sessions exist)");
    return;
#endif

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
}

void Terminal::stopShellSession() {
    if (!shell_running_)
        return;
#ifdef BUILD_LIBVTERM_SUPPORT
    auto* sess = getActiveSession();
    if (sess) {
        sess->sendBytes("exit\n");
        if (sessions_.size() == 1) {
            sessions_.clear();
            active_session_index_ = 0;
        } else {
            closeSession(active_session_index_);
        }
        shell_running_ = sessions_.empty() ? false : true;
        return;
    }
#endif
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
            if (on_shell_exit_) {
                on_shell_exit_();
            }
            break;
        }
    }
}

void Terminal::cleanupShell() {
#ifdef BUILD_LIBVTERM_SUPPORT
    for (auto& s : sessions_)
        if (s)
            s->terminate();
    sessions_.clear();
    active_session_index_ = 0;
#endif
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

static std::string escapeSingleQuotes(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\'')
            out += "'\\''";
        else
            out += c;
    }
    return out;
}

void Terminal::startSSHSession(const std::string& host, const std::string& user, int port,
                               const std::string& key_path, const std::string& password) {
#ifdef BUILD_LIBVTERM_SUPPORT
    int idx = newSSHSession(host, user, port, key_path, password);
    if (idx >= 0)
        return;
    addOutputLine("SSH failed: could not start session");
    return;
#endif

    cleanupShell();
    int p = (port > 0) ? port : 22;
    std::string port_opt = (p != 22) ? (" -p " + std::to_string(p)) : "";
    std::string key_opt = key_path.empty() ? "" : (" -i " + key_path);
    std::string opts =
        " -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR";
    std::string target = user + "@" + host;
    std::string cmd;
    if (!password.empty()) {
        cmd = "sshpass -p '" + escapeSingleQuotes(password) + "' ssh -t" + opts + port_opt + " " +
              target;
    } else {
        cmd = "ssh -t" + opts + port_opt + key_opt + " " + target;
    }
    terminal::PTYResult result = terminal::PTYExecutor::createPTY(cmd, ".", {});
    if (!result.success) {
        addOutputLine("SSH failed: " + result.error);
        return;
    }
    shell_running_ = true;
    current_pid_ = result.pid;
    current_pty_fd_ = result.master_fd;
    current_slave_fd_ = result.slave_fd;
    startOutputThread(result.master_fd);
}

void Terminal::restoreLocalShell() {
    cleanupShell();
    startShellSession();
}

#ifdef BUILD_LIBVTERM_SUPPORT
terminal::TerminalSession* Terminal::getActiveSession() const {
    if (sessions_.empty() || active_session_index_ < 0 ||
        active_session_index_ >= static_cast<int>(sessions_.size()))
        return nullptr;
    return sessions_[active_session_index_].get();
}

bool Terminal::useLibVTermPath() const {
    auto* sess = getActiveSession();
    return sess != nullptr && sess->isRunning();
}

terminal::ScreenSnapshot Terminal::getSessionSnapshot(int view_height) const {
    auto* sess = getActiveSession();
    if (!sess || !sess->isRunning()) {
        scroll_max_ = 0;
        return terminal::ScreenSnapshot();
    }
    if (!sess->isReady()) {
        scroll_max_ = 0;
        return terminal::ScreenSnapshot();
    }
    sess->feedPending(); // 主线程处理待喂入数据，libvterm 仅在此线程访问
    auto full = sess->getFullSnapshot(1000);
    size_t total_lines = full.scrollback.size() + full.visible.size();
    size_t view_rows = full.rows > 0 ? static_cast<size_t>(full.rows) : 0;
    scroll_max_ = (total_lines > view_rows) ? (total_lines - view_rows) : 0;
    if (scroll_offset_ > scroll_max_)
        scroll_offset_ = scroll_max_;
    return sess->getSnapshot(static_cast<int>(scroll_offset_), 1000, view_height);
}

int Terminal::sessionCount() const {
    return static_cast<int>(sessions_.size());
}

void Terminal::setActiveSession(int index) {
    if (index >= 0 && index < static_cast<int>(sessions_.size()))
        active_session_index_ = index;
}

int Terminal::newLocalShellSession(const std::string& cwd, const std::string& shell_path) {
    std::string dir = cwd.empty() ? current_directory_ : cwd;
    auto s = std::make_unique<terminal::TerminalSession>();
    s->setOnOutput([this]() {
        if (on_output_added_)
            on_output_added_();
    });
    s->setOnExit([this](int) {
        if (on_shell_exit_)
            on_shell_exit_();
    });
    if (shell_path.empty()) {
        if (!s->startLocalShell(dir))
            return -1;
    } else {
        if (!s->startLocalShellWithPath(dir, shell_path))
            return -1;
    }
    int idx = static_cast<int>(sessions_.size());
    sessions_.push_back(std::move(s));
    active_session_index_ = idx;
    shell_running_ = true;
    return idx;
}

int Terminal::newSSHSession(const std::string& host, const std::string& user, int port,
                            const std::string& key_path, const std::string& password) {
    auto s = std::make_unique<terminal::TerminalSession>();
    s->setOnOutput([this]() {
        if (on_output_added_)
            on_output_added_();
    });
    s->setOnExit([this](int) {
        if (on_shell_exit_)
            on_shell_exit_();
    });
    if (!s->startSSH(host, user, port, key_path, password))
        return -1;
    int idx = static_cast<int>(sessions_.size());
    sessions_.push_back(std::move(s));
    active_session_index_ = idx;
    shell_running_ = true;
    return idx;
}

int Terminal::newContainerSession(const std::string& container_id, const std::string& shell) {
    auto s = std::make_unique<terminal::TerminalSession>();
    s->setOnOutput([this]() {
        if (on_output_added_)
            on_output_added_();
    });
    s->setOnExit([this](int) {
        if (on_shell_exit_)
            on_shell_exit_();
    });
    if (!s->startContainer(container_id, shell))
        return -1;
    int idx = static_cast<int>(sessions_.size());
    sessions_.push_back(std::move(s));
    active_session_index_ = idx;
    shell_running_ = true;
    return idx;
}

void Terminal::closeSession(int index) {
    if (index < 0 || index >= static_cast<int>(sessions_.size()))
        return;
    sessions_.erase(sessions_.begin() + index);
    if (active_session_index_ >= static_cast<int>(sessions_.size()))
        active_session_index_ = std::max(0, static_cast<int>(sessions_.size()) - 1);
    shell_running_ = !sessions_.empty();
}

std::string Terminal::getSessionTitle(int index) const {
    if (index < 0 || index >= static_cast<int>(sessions_.size()))
        return "";
    return sessions_[index]->getTitle();
}
#endif

ftxui::Element Terminal::render(int /* height */) {
    return text("");
}

void Terminal::scrollUp() {
#ifdef BUILD_LIBVTERM_SUPPORT
    if (useLibVTermPath()) {
        if (scroll_offset_ < scroll_max_)
            scroll_offset_ += 1;
        return;
    }
#endif
    std::lock_guard<std::mutex> lock(output_mutex_);
    if (scroll_offset_ < output_lines_.size()) {
        scroll_offset_ += 1;
    }
}

void Terminal::scrollDown() {
#ifdef BUILD_LIBVTERM_SUPPORT
    if (useLibVTermPath()) {
        if (scroll_offset_ > 0)
            scroll_offset_ -= 1;
        return;
    }
#endif
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
