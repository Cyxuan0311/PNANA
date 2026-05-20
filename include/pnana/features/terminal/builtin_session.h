#ifndef PNANA_FEATURES_TERMINAL_BUILTIN_SESSION_H
#define PNANA_FEATURES_TERMINAL_BUILTIN_SESSION_H

#include "features/terminal/builtin_screen.h"
#include "features/terminal/builtin_vt_parser.h"
#include "features/terminal/terminal_line_buffer.h"
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace pnana {
namespace features {

struct TerminalLine {
    std::string content;
    bool has_ansi_colors;

    TerminalLine(const std::string& c, bool ansi_colors = false)
        : content(c), has_ansi_colors(ansi_colors) {}
};

namespace terminal {

class BuiltinSession {
  public:
    BuiltinScreen screen;
    BuiltinVtParser parser;
    pid_t pid = 0;
    int pty_fd = -1;
    int slave_fd = -1;
    std::string title;
    std::string cwd;
    std::atomic<bool> running{false};
    std::thread output_thread;
    std::atomic<bool> output_thread_running{false};
    std::mutex output_mutex;
    std::atomic<int> pending_backspace_count{0};
    std::string pending_raw;
    std::string pending_line;
    size_t pending_cursor_pos = 0;
    terminal::PendingLineBuffer pending_line_buffer;
    std::vector<TerminalLine> output_lines;
    size_t max_output_lines = 1000;
    size_t scroll_offset = 0;
    std::chrono::steady_clock::time_point last_refresh_time;
    std::function<void()> on_output_callback;
    std::function<void()> on_exit_callback;

    BuiltinSession();
    ~BuiltinSession();

    bool start(const std::string& working_dir, const std::string& shell_path = "");
    void stop();
    void writeInput(const std::string& data);
    void resize(int cols, int rows);
    BuiltinScreenSnapshot getSnapshot();
    bool isRunning() const {
        return running.load();
    }

    void readOutput();
    void addOutputLine(const std::string& line);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif
