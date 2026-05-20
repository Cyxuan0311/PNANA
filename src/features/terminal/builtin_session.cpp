#include "features/terminal/builtin_session.h"
#include "features/terminal.h"
#include "features/terminal/terminal_color.h"
#include "features/terminal/terminal_pty.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstring>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

namespace pnana {
namespace features {
namespace terminal {

static constexpr int REFRESH_THROTTLE_MS = 33;

BuiltinSession::BuiltinSession() {
    screen.init(80, 24);
    parser.init(&screen);
    pending_line_buffer.setPendingBackspaceCount(&pending_backspace_count);
}

BuiltinSession::~BuiltinSession() {
    stop();
}

bool BuiltinSession::start(const std::string& working_dir, const std::string& shell_path) {
    cwd = working_dir.empty() ? "." : working_dir;

    PTYResult result;
    if (shell_path.empty()) {
        result = PTYExecutor::createInteractiveShell(cwd);
    } else {
        result = PTYExecutor::createInteractiveShellWithPath(cwd, shell_path);
    }

    if (!result.success) {
        LOG("[BuiltinSession] Failed to start shell: " + result.error);
        return false;
    }

    pid = result.pid;
    pty_fd = result.master_fd;
    slave_fd = result.slave_fd;
    running.store(true);

    title = shell_path.empty() ? "Local Shell" : shell_path;

    output_thread_running.store(true);
    output_thread = std::thread([this]() {
        readOutput();
    });

    return true;
}

void BuiltinSession::stop() {
    running.store(false);
    output_thread_running.store(false);
    if (output_thread.joinable()) {
        output_thread.join();
    }
    if (pty_fd >= 0) {
        close(pty_fd);
        pty_fd = -1;
    }
    if (slave_fd >= 0) {
        close(slave_fd);
        slave_fd = -1;
    }
    if (pid > 0) {
        kill(pid, SIGTERM);
        pid = 0;
    }
}

void BuiltinSession::writeInput(const std::string& data) {
    if (pty_fd >= 0 && running.load()) {
        PTYExecutor::writeInput(pty_fd, data);
    }
}

void BuiltinSession::resize(int cols, int rows) {
    {
        std::lock_guard<std::mutex> lock(output_mutex);
        screen.resize(cols, rows);
    }
    if (pty_fd >= 0) {
        PTYExecutor::setTerminalSize(pty_fd, rows, cols);
    }
}

BuiltinScreenSnapshot BuiltinSession::getSnapshot() {
    std::lock_guard<std::mutex> lock(output_mutex);
    auto snap = screen.snapshot(1000);
    snap.scroll_offset = static_cast<int>(scroll_offset);
    return snap;
}

void BuiltinSession::readOutput() {
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];

    while (output_thread_running.load()) {
        struct pollfd pfd = {};
        pfd.fd = pty_fd;
        pfd.events = POLLIN;
        int ret = poll(&pfd, 1, 16);

        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (ret == 0)
            continue;

        if (pfd.revents & (POLLHUP | POLLERR)) {
            running.store(false);
            break;
        }

        bool had_output = false;
        while (true) {
            ssize_t n = PTYExecutor::readOutput(pty_fd, buffer, BUFFER_SIZE);
            if (n > 0) {
                had_output = true;
                std::lock_guard<std::mutex> lock(output_mutex);
                parser.feed(reinterpret_cast<const uint8_t*>(buffer), static_cast<size_t>(n));

                std::string raw = pending_raw + std::string(buffer, static_cast<size_t>(n));
                pending_raw.clear();
                size_t start = 0;
                size_t end;
                bool had_complete_line = false;
                bool is_first = true;

                while ((end = raw.find('\n', start)) != std::string::npos) {
                    std::string line_to_add;
                    if (is_first && !pending_line.empty()) {
                        line_to_add = pending_line;
                    } else {
                        std::string line_raw = raw.substr(start, end - start);
                        pending_line_buffer.reset();
                        pending_line_buffer.feed(line_raw);
                        pending_line_buffer.flushReplace();
                        line_to_add = pending_line_buffer.getLine();
                        if (line_to_add.empty())
                            line_to_add = line_raw;
                    }
                    addOutputLine(line_to_add);
                    had_complete_line = true;
                    start = end + 1;
                    is_first = false;
                    pending_line_buffer.reset();
                }

                if (start < raw.length()) {
                    std::string new_pending = raw.substr(start);
                    std::string to_feed;
                    if (had_complete_line) {
                        pending_line_buffer.reset();
                        to_feed = new_pending;
                    } else {
                        to_feed = std::string(buffer, static_cast<size_t>(n));
                    }
                    if (!to_feed.empty()) {
                        pending_line_buffer.feed(to_feed);
                        pending_line_buffer.flushReplace();
                    }
                    pending_raw = new_pending;
                    pending_line = pending_line_buffer.getLine();
                    pending_cursor_pos = pending_line_buffer.getCursorPos();
                } else {
                    pending_raw.clear();
                    pending_line.clear();
                    pending_cursor_pos = 0;
                    pending_line_buffer.reset();
                }
            } else {
                break;
            }
        }

        if (had_output && on_output_callback) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refresh_time)
                    .count();
            if (elapsed >= REFRESH_THROTTLE_MS) {
                last_refresh_time = now;
                on_output_callback();
            }
        }
    }

    running.store(false);
    if (on_exit_callback) {
        on_exit_callback();
    }
}

void BuiltinSession::addOutputLine(const std::string& line) {
    if (output_lines.size() >= max_output_lines) {
        output_lines.erase(output_lines.begin());
    }
    bool has_ansi = AnsiColorParser::hasAnsiCodes(line);
    output_lines.push_back(TerminalLine(line, has_ansi));
}

} // namespace terminal
} // namespace features
} // namespace pnana
