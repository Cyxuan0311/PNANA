#ifndef PNANA_UI_SSH_DIALOG_H
#define PNANA_UI_SSH_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>

namespace pnana {
namespace ui {

// SSH 配置结构
struct SSHConfig {
    std::string host;
    std::string user;
    std::string password;
    std::string key_path;
    int port;
    std::string remote_path;
};

// SSH 对话框类
class SSHDialog {
  public:
    explicit SSHDialog(Theme& theme);

    // 显示 SSH 连接对话框
    void show(std::function<void(const SSHConfig&)> on_confirm = nullptr,
              std::function<void()> on_cancel = nullptr);

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

  private:
    Theme& theme_;
    bool visible_;

    // 输入字段
    std::string host_input_;
    std::string user_input_;
    std::string password_input_;
    std::string key_path_input_;
    std::string port_input_;
    std::string remote_path_input_;

    // 当前焦点字段索引（0-5）
    size_t current_field_;

    // 光标位置
    size_t cursor_position_;

    // 回调函数
    std::function<void(const SSHConfig&)> on_confirm_;
    std::function<void()> on_cancel_;

    // 辅助方法
    std::string* getCurrentField();
    void insertChar(char ch);
    void deleteChar();
    void backspace();
    void moveCursorLeft();
    void moveCursorRight();
    void moveToNextField();
    void moveToPreviousField();
    SSHConfig buildConfig() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SSH_DIALOG_H
