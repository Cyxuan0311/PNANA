#ifndef PNANA_FEATURES_TERMINAL_TERMINAL_SESSION_H
#define PNANA_FEATURES_TERMINAL_TERMINAL_SESSION_H

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>

#ifdef BUILD_LIBVTERM_SUPPORT
#include "features/terminal/terminal_vterm_screen.h"
#endif

namespace pnana {
namespace features {
namespace terminal {

class PTYBackend;

// 抽象按键事件（供键盘映射使用，与 ftxui 解耦）
struct KeyEvent {
    enum class Type {
        Char, // 可打印字符
        KeyUp,
        KeyDown,
        KeyLeft,
        KeyRight,
        Home,
        End,
        PageUp,
        PageDown,
        Tab,
        Enter,
        Backspace,
        Delete,
        Escape,
        CtrlC,
        CtrlD,
        CtrlZ,
        CtrlL,
        CtrlU,
        CtrlK,
        CtrlA,
        CtrlE,
        CtrlW,
        CtrlH,
        CtrlX,
    };
    Type type;
    char ch = 0; // 当 type==Char 时有效
};

// 终端会话：PTY + libvterm 屏幕模型，UI 无关
class TerminalSession {
  public:
    using OnOutputCallback = std::function<void()>;
    using OnExitCallback = std::function<void(int exit_code)>;

    friend class VTermStreamFilter;

    TerminalSession();
    ~TerminalSession();

    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;

    // 启动本地 Shell
    bool startLocalShell(const std::string& cwd);
    bool startLocalShellWithPath(const std::string& cwd, const std::string& shell_path);

    // 启动 SSH 会话
    bool startSSH(const std::string& host, const std::string& user, int port,
                  const std::string& key_path, const std::string& password);

    // 启动容器 Shell（docker/podman exec -it）
    bool startContainer(const std::string& container_id, const std::string& shell = "/bin/sh");

    // 终止会话
    void terminate();

    // 输入
    void sendBytes(std::string_view data);
    void sendKey(const KeyEvent& ev);

    // 屏幕（仅当 BUILD_LIBVTERM_SUPPORT 时有效）
#ifdef BUILD_LIBVTERM_SUPPORT
    // 在主线程中处理待喂入的数据（libvterm 非线程安全，必须在单线程访问）
    void feedPending();
    ScreenSnapshot getSnapshot(int scroll_offset, int max_scrollback, int view_height) const;
    ScreenSnapshot getFullSnapshot(int max_scrollback) const;
    void setReadCoalesceWindow(std::chrono::milliseconds window);
    bool pendingFeedOverflowed() const;
#endif
    void resize(int cols, int rows);

    // 状态
    bool isRunning() const;
#ifdef BUILD_LIBVTERM_SUPPORT
    bool isReady() const {
        return vterm_ && vterm_->isReady();
    }
    void fixVTermCallbackUser() {
        if (vterm_)
            vterm_->setScreenCallbackData(vterm_.get());
    }
#endif
    const std::string& getTitle() const {
        return title_;
    }
    void setTitle(const std::string& t) {
        title_ = t;
    }

    // 回调
    void setOnOutput(OnOutputCallback cb) {
        on_output_ = std::move(cb);
    }
    void setOnExit(OnExitCallback cb) {
        on_exit_ = std::move(cb);
    }

  private:
#ifdef BUILD_LIBVTERM_SUPPORT
    friend class VTermStreamFilter;
#endif
    void onPtyRead(const char* data, size_t len);
    void onPtyExit(int exit_code);
    std::string keyEventToBytes(const KeyEvent& ev) const;

    // 重要：backend_ 必须最后声明，以便析构时先销毁 backend_（停止读线程），
    // 再销毁 vterm_，避免读线程回调时访问已释放的 VTermScreenModel
#ifdef BUILD_LIBVTERM_SUPPORT
    std::unique_ptr<VTermScreenModel> vterm_;
    std::deque<std::string> pending_chunks_;
    mutable std::mutex pending_feed_mutex_;
    std::chrono::steady_clock::time_point last_feed_push_{};
    std::chrono::milliseconds read_coalesce_window_{std::chrono::milliseconds(4)};
    size_t pending_bytes_{0};
    bool pending_overflowed_{false};

    enum class FilterMode { None, OSC, DCS, APC, PM };
    FilterMode filter_mode_{FilterMode::None};
    bool esc_pending_{false};
    bool seq_esc_pending_{false};
    std::string utf8_pending_;
#endif
    std::string title_;
    std::unique_ptr<PTYBackend> backend_;
    OnOutputCallback on_output_;
    OnExitCallback on_exit_;
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_TERMINAL_SESSION_H
