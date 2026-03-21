#ifndef PNANA_UI_SSH_DIALOG_H
#define PNANA_UI_SSH_DIALOG_H

#include "features/cursor/cursor_renderer.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// SSH 配置结构
struct SSHConfig {
    std::string name;
    std::string host;
    std::string user;
    std::string password;
    std::string key_path;
    int port;
    std::string remote_path;
};

// SSH 历史记录条目（不保存密码）
struct SSHHistoryEntry {
    std::string name = "New session";
    std::string host;
    std::string user;
    int port = 22;
    std::string key_path;
    std::string remote_path;

    std::string displayLabel() const {
        if (!name.empty() && name != "New session") {
            return name + " (" + (user.empty() ? host : user + "@" + host) + ")";
        }
        std::string label = user.empty() ? host : user + "@" + host;
        if (port != 22)
            label += ":" + std::to_string(port);
        return label;
    }
};

// 对话框内部模式
enum class SSHDialogMode {
    HISTORY,   // 显示历史连接列表
    NEW_FORM,  // 新建连接表单
    PASSWORD,  // 从历史选中后输入密码
    CONNECTED, // 已连接状态
    EDIT,      // 编辑历史记录
};

// SSH 对话框类
class SSHDialog {
  public:
    explicit SSHDialog(Theme& theme);

    // 显示 SSH 连接对话框
    // current_connection: 若非空且 host 非空，则显示"当前连接状态"视图，并可用 Delete 断开
    void show(std::function<void(const SSHConfig&)> on_confirm = nullptr,
              std::function<void()> on_cancel = nullptr,
              const SSHConfig* current_connection = nullptr,
              std::function<void()> on_disconnect = nullptr);

    // 处理输入
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render();

    // 是否可见
    bool isVisible() const {
        return visible_;
    }

    // 重置
    void reset();

    // 将一条连接记录写入历史（由外部在连接成功后调用）
    void pushHistory(const SSHConfig& config);

    // 同步光标配置（由 Editor::showSSHDialog 在每次打开前注入）
    void setCursorConfig(const CursorConfig& config) {
        cursor_config_ = config;
    }

  private:
    Theme& theme_;
    bool visible_;
    SSHDialogMode mode_;

    // ── 历史列表 ──────────────────────────────────────────
    std::vector<SSHHistoryEntry> history_;
    int history_index_ = 0; // 当前选中项

    // ── 新建连接表单 ──────────────────────────────────────
    std::string name_input_;
    std::string host_input_;
    std::string user_input_;
    std::string password_input_;
    std::string key_path_input_;
    std::string port_input_;
    std::string remote_path_input_;
    size_t current_field_; // 0-6 (name, host, user, password, key_path, port, remote_path)
    size_t cursor_position_;

    // ── 密码输入（从历史选中后） ──────────────────────────
    std::string history_password_input_;
    size_t history_password_cursor_ = 0;

    // ── 编辑历史记录 ──────────────────────────────────────
    int edit_entry_index_ = -1; // 被编辑的历史记录索引

    // ── 已连接状态 ────────────────────────────────────────
    const SSHConfig* current_connection_ = nullptr;

    // ── 光标配置（由外部注入，与编辑器保持同步） ──────────
    CursorConfig cursor_config_;

    // ── 回调 ──────────────────────────────────────────────
    std::function<void(const SSHConfig&)> on_confirm_;
    std::function<void()> on_cancel_;
    std::function<void()> on_disconnect_;

    // ── 历史持久化 ────────────────────────────────────────
    static constexpr int kMaxHistory = 6;
    std::string getHistoryFilePath() const;
    void loadHistory();
    void saveHistory() const;

    // ── 表单辅助 ──────────────────────────────────────────
    std::string* getCurrentField();
    void insertChar(char ch);
    void deleteChar();
    void backspace();
    void moveCursorLeft();
    void moveCursorRight();
    void moveToNextField();
    void moveToPreviousField();
    SSHConfig buildConfig() const;

    // ── 渲染子视图 ────────────────────────────────────────
    ftxui::Element renderHistoryView();
    ftxui::Element renderNewFormView();
    ftxui::Element renderPasswordView();
    ftxui::Element renderConnectedView();
    ftxui::Element renderEditView();

    // ── 输入子处理 ────────────────────────────────────────
    bool handleHistoryInput(ftxui::Event event);
    bool handleNewFormInput(ftxui::Event event);
    bool handlePasswordInput(ftxui::Event event);
    bool handleConnectedInput(ftxui::Event event);
    bool handleEditInput(ftxui::Event event);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SSH_DIALOG_H
