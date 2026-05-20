#ifndef PNANA_FEATURES_TERMINAL_H
#define PNANA_FEATURES_TERMINAL_H

#include "features/terminal/builtin_screen.h"
#include "features/terminal/builtin_session.h"
#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifdef BUILD_LIBVTERM_SUPPORT
#include "features/terminal/terminal_session.h"
#include "features/terminal/terminal_vterm_screen.h"
#endif

namespace pnana {
namespace features {

class Terminal {
  public:
    explicit Terminal(ui::Theme& theme);
    ~Terminal();

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

    // 调整终端尺寸（cols, rows）
    void resize(int cols, int rows);

    // 清空终端
    void clear();

    // 中断 shell（发送 SIGINT）
    void interruptCommand();

    // 启动 SSH 会话（替换当前 shell）；断开后需调用 restoreLocalShell()
    void startSSHSession(const std::string& host, const std::string& user, int port,
                         const std::string& key_path, const std::string& password);
    void restoreLocalShell();

    // 获取方法（供 UI 使用）
    ui::Theme& getTheme() const {
        return theme_;
    }

    terminal::BuiltinScreenSnapshot getBuiltinScreenSnapshot() const;

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

    // 当 shell 进程退出时调用的回调（如用户输入 exit 后关闭终端面板）
    void setOnShellExit(std::function<void()> cb) {
        on_shell_exit_ = std::move(cb);
    }

    // Check if there are any active sessions (SSH or local shell)
    bool hasActiveSession() const {
#ifdef BUILD_LIBVTERM_SUPPORT
        return !sessions_.empty();
#else
        return !builtin_sessions_.empty();
#endif
    }

#ifdef BUILD_LIBVTERM_SUPPORT
    bool useLibVTermPath() const;
    terminal::ScreenSnapshot getSessionSnapshot(int view_height) const;
#endif

    int sessionCount() const;
    int activeSessionIndex() const;
    void setActiveSession(int index);
    int newLocalShellSession(const std::string& cwd = "", const std::string& shell_path = "");
    int newSSHSession(const std::string& host, const std::string& user, int port = 22,
                      const std::string& key_path = "", const std::string& password = "");
    int newContainerSession(const std::string& container_id, const std::string& shell = "/bin/sh");
    void closeSession(int index);
    std::string getSessionTitle(int index) const;

  private:
    ui::Theme& theme_;
    bool visible_;

    size_t max_output_lines_;
    mutable size_t scroll_offset_;
    mutable size_t scroll_max_ = 0;

    std::string current_directory_;

    std::function<void()> on_output_added_;
    std::function<void()> on_shell_exit_;

#ifdef BUILD_LIBVTERM_SUPPORT
    terminal::TerminalSession* getActiveSession() const;
    std::vector<std::unique_ptr<terminal::TerminalSession>> sessions_;
    int active_session_index_ = 0;
#endif

    std::vector<std::unique_ptr<terminal::BuiltinSession>> builtin_sessions_;
    int builtin_active_index_ = 0;
    terminal::BuiltinSession* getActiveBuiltinSession() const;

    void addOutputLine(const std::string& line);
    void startShellSession();
    void stopShellSession();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_H
