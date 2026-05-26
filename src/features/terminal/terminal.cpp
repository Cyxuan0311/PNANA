#include "features/terminal.h"
#include "features/terminal/terminal_color.h"
#include "features/terminal/terminal_key_map.h"
#include "features/terminal/terminal_pty.h"
#include "utils/logger.h"
#include <cstdlib>
#include <ftxui/dom/elements.hpp>
#include <signal.h>
#include <thread>

using namespace ftxui;

namespace pnana {
namespace features {

namespace {

// 将按键名映射为 PTY 应接收的字节序列
// 部分 shell 需要 \r\n 才能正确执行命令（PTY 模拟真实终端行为）
std::string keyToEscape(const std::string& key) {
    if (key == "return" || key == "ctrl_m")
        return "\r";
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
    : theme_(theme), visible_(false), max_output_lines_(1000), scroll_offset_(0),
      current_directory_(".") {
    char* cwd = getcwd(nullptr, 0);
    if (cwd) {
        current_directory_ = cwd;
        free(cwd);
    }
}

Terminal::~Terminal() {
    on_shell_exit_ = nullptr;
    on_output_added_ = nullptr;
    builtin_sessions_.clear();
}

void Terminal::setVisible(bool visible) {
    if (visible_ == visible) {
        LOG_DEBUG("[Terminal] setVisible no-op target=" + std::string(visible ? "true" : "false"));
        return;
    }
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
        if (key == "ArrowLeft" || key == "ArrowRight") {
            auto* bs = getActiveBuiltinSession();
            if (bs)
                bs->pending_backspace_count.store(0);
        } else if (esc.size() == 1 && static_cast<unsigned char>(esc[0]) == 0x08) {
            auto* bs = getActiveBuiltinSession();
            if (bs)
                bs->pending_backspace_count++;
        }
        writeToShell(esc);
        return;
    }

    if (key.length() == 1) {
        if (static_cast<unsigned char>(key[0]) == 0x08) {
            auto* bs = getActiveBuiltinSession();
            if (bs)
                bs->pending_backspace_count++;
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
    auto* bs = getActiveBuiltinSession();
    if (bs && bs->isRunning()) {
        bs->writeInput(input);
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
    auto* bs = getActiveBuiltinSession();
    if (bs) {
        bs->resize(cols, rows);
    }
}

void Terminal::clear() {
    auto* bs = getActiveBuiltinSession();
    if (bs) {
        std::lock_guard<std::mutex> lock(bs->output_mutex);
        bs->output_lines.clear();
        bs->pending_raw.clear();
        bs->pending_line.clear();
        bs->pending_cursor_pos = 0;
        bs->pending_backspace_count.store(0);
        bs->pending_line_buffer.reset();
        bs->scroll_offset = 0;
        bs->screen.eraseDisplay(2);
        bs->screen.cursor_x = 0;
        bs->screen.cursor_y = 0;
        bs->screen.clearAllDirty();
    }
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
    auto* bs = getActiveBuiltinSession();
    if (bs && bs->isRunning()) {
        terminal::PTYExecutor::sendSignal(bs->pid, SIGINT);
    }
}

void Terminal::addOutputLine(const std::string& line) {
    auto* bs = getActiveBuiltinSession();
    if (bs) {
        if (bs->output_lines.size() >= max_output_lines_) {
            bs->output_lines.erase(bs->output_lines.begin());
        }
        bool has_ansi = terminal::AnsiColorParser::hasAnsiCodes(line);
        bs->output_lines.push_back(TerminalLine(line, has_ansi));
    }
}

terminal::BuiltinScreenSnapshot Terminal::getBuiltinScreenSnapshot() const {
    auto* bs = getActiveBuiltinSession();
    if (!bs) {
        return terminal::BuiltinScreenSnapshot();
    }
    auto snap = bs->getSnapshot();
    scroll_offset_ = snap.scroll_offset;
    return snap;
}

void Terminal::startShellSession() {
    LOG("[Terminal] startShellSession enter");
#ifdef BUILD_LIBVTERM_SUPPORT
    if (sessions_.empty()) {
        LOG("[Terminal] sessions empty, calling newLocalShellSession");
        int idx = newLocalShellSession(current_directory_);
        if (idx >= 0) {
            LOG("[Terminal] newLocalShellSession ok idx=" + std::to_string(idx));
            return;
        }
        LOG("[Terminal] newLocalShellSession failed");
        addOutputLine("Error: Could not start shell session");
        return;
    }
    LOG("[Terminal] startShellSession done (sessions exist)");
    return;
#endif

    if (!builtin_sessions_.empty())
        return;

    int idx = newLocalShellSession(current_directory_);
    if (idx < 0) {
        addOutputLine("Error: Could not start shell session");
    }
}

void Terminal::stopShellSession() {
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
        return;
    }
#endif
    auto* bs = getActiveBuiltinSession();
    if (bs) {
        bs->writeInput("exit\n");
        if (builtin_sessions_.size() == 1) {
            builtin_sessions_.clear();
            builtin_active_index_ = 0;
        } else {
            closeSession(builtin_active_index_);
        }
    }
}

#ifndef BUILD_LIBVTERM_SUPPORT
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
#endif

void Terminal::startSSHSession(const std::string& host, const std::string& user, int port,
                               const std::string& key_path, const std::string& password) {
#ifdef BUILD_LIBVTERM_SUPPORT
    int idx = newSSHSession(host, user, port, key_path, password);
    if (idx >= 0)
        return;
    addOutputLine("SSH failed: could not start session");
    return;
#else
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

    auto s = std::make_unique<terminal::BuiltinSession>();
    s->on_output_callback = on_output_added_;
    s->on_exit_callback = on_shell_exit_;
    terminal::PTYResult result = terminal::PTYExecutor::createPTY(cmd, ".", {});
    if (!result.success) {
        addOutputLine("SSH failed: " + result.error);
        return;
    }
    s->pid = result.pid;
    s->pty_fd = result.master_fd;
    s->slave_fd = result.slave_fd;
    s->running.store(true);
    s->title = "SSH: " + target;
    s->output_thread_running.store(true);
    s->output_thread = std::thread([&s]() {
        s->readOutput();
    });
    int idx = static_cast<int>(builtin_sessions_.size());
    builtin_sessions_.push_back(std::move(s));
    builtin_active_index_ = idx;
#endif
}

void Terminal::restoreLocalShell() {
    builtin_sessions_.clear();
    builtin_active_index_ = 0;
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

int Terminal::activeSessionIndex() const {
    return active_session_index_;
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
    return idx;
}

void Terminal::closeSession(int index) {
    if (index < 0 || index >= static_cast<int>(sessions_.size()))
        return;
    sessions_.erase(sessions_.begin() + index);
    if (active_session_index_ >= static_cast<int>(sessions_.size()))
        active_session_index_ = std::max(0, static_cast<int>(sessions_.size()) - 1);
}

std::string Terminal::getSessionTitle(int index) const {
    if (index < 0 || index >= static_cast<int>(sessions_.size()))
        return "";
    return sessions_[index]->getTitle();
}
#endif

#ifndef BUILD_LIBVTERM_SUPPORT
int Terminal::sessionCount() const {
    return static_cast<int>(builtin_sessions_.size());
}

int Terminal::activeSessionIndex() const {
    return builtin_active_index_;
}

void Terminal::setActiveSession(int index) {
    if (index >= 0 && index < static_cast<int>(builtin_sessions_.size()))
        builtin_active_index_ = index;
}

int Terminal::newLocalShellSession(const std::string& cwd, const std::string& shell_path) {
    auto s = std::make_unique<terminal::BuiltinSession>();
    s->on_output_callback = on_output_added_;
    s->on_exit_callback = on_shell_exit_;
    std::string dir = cwd.empty() ? current_directory_ : cwd;
    if (!s->start(dir, shell_path))
        return -1;
    int idx = static_cast<int>(builtin_sessions_.size());
    builtin_sessions_.push_back(std::move(s));
    builtin_active_index_ = idx;
    return idx;
}

int Terminal::newSSHSession(const std::string& /*host*/, const std::string& /*user*/, int /*port*/,
                            const std::string& /*key_path*/, const std::string& /*password*/) {
    return -1;
}

int Terminal::newContainerSession(const std::string& /*container_id*/,
                                  const std::string& /*shell*/) {
    return -1;
}

void Terminal::closeSession(int index) {
    if (index < 0 || index >= static_cast<int>(builtin_sessions_.size()))
        return;
    builtin_sessions_.erase(builtin_sessions_.begin() + index);
    if (builtin_active_index_ >= static_cast<int>(builtin_sessions_.size()))
        builtin_active_index_ = std::max(0, static_cast<int>(builtin_sessions_.size()) - 1);
}

std::string Terminal::getSessionTitle(int index) const {
    if (index < 0 || index >= static_cast<int>(builtin_sessions_.size()))
        return "";
    return builtin_sessions_[index]->title;
}
#endif

terminal::BuiltinSession* Terminal::getActiveBuiltinSession() const {
    if (builtin_sessions_.empty() || builtin_active_index_ < 0 ||
        builtin_active_index_ >= static_cast<int>(builtin_sessions_.size()))
        return nullptr;
    return builtin_sessions_[builtin_active_index_].get();
}

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
    auto* bs = getActiveBuiltinSession();
    if (bs) {
        std::lock_guard<std::mutex> lock(bs->output_mutex);
        size_t max_scroll = bs->screen.scrollbackSize();
        if (bs->scroll_offset < max_scroll)
            bs->scroll_offset += 1;
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
    auto* bs = getActiveBuiltinSession();
    if (bs && bs->scroll_offset > 0) {
        bs->scroll_offset -= 1;
    }
}

void Terminal::scrollToTop() {
    auto* bs = getActiveBuiltinSession();
    if (bs) {
        std::lock_guard<std::mutex> lock(bs->output_mutex);
        bs->scroll_offset = bs->screen.scrollbackSize();
    }
}

void Terminal::scrollToBottom() {
    auto* bs = getActiveBuiltinSession();
    if (bs) {
        bs->scroll_offset = 0;
    }
}

} // namespace features
} // namespace pnana
