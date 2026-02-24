#ifndef PNANA_FEATURES_TERMINAL_TERMINAL_LINE_BUFFER_H
#define PNANA_FEATURES_TERMINAL_TERMINAL_LINE_BUFFER_H

#include <atomic>
#include <string>

namespace pnana {
namespace features {
namespace terminal {

// 轻量级终端行解析器：维护 (line, cursor_pos)，正确解析 \b、ESC 序列、Backspace 重绘
// readline 输出 \b 用于光标左移或 Backspace，无法从字节流区分；通过调用方传入「待消费的 Backspace
// 计数」来识别
class PendingLineBuffer {
  public:
    void feed(const std::string& chunk);

    void setPendingBackspaceCount(std::atomic<int>* ptr) {
        pending_backspace_count_ = ptr;
    }
    void flushReplace();

    std::string getLine() const {
        return line_;
    }
    size_t getCursorPos() const {
        return cursor_pos_;
    }
    void reset();

  private:
    enum class State { Normal, Escape, SkipOne, CSI, OSC, AfterBackspace };
    std::string line_;
    size_t cursor_pos_ = 0;
    std::string replace_buf_;
    State state_ = State::Normal;
    std::string csi_buf_;
    bool backspace_at_end_ = false;
    std::atomic<int>* pending_backspace_count_ = nullptr;

    void feedChar(unsigned char c);
    void feedEscape(unsigned char c);
    void feedCSI(unsigned char c);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_TERMINAL_LINE_BUFFER_H
