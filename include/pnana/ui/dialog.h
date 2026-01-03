#ifndef PNANA_UI_DIALOG_H
#define PNANA_UI_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>

namespace pnana {
namespace ui {

// 对话框类型
enum class DialogType {
    INPUT,  // 输入对话框
    CONFIRM // 确认对话框
};

// 对话框类
class Dialog {
  public:
    explicit Dialog(Theme& theme);

    // 显示输入对话框
    void showInput(const std::string& title, const std::string& prompt,
                   const std::string& initial_value = "",
                   std::function<void(const std::string&)> on_confirm = nullptr,
                   std::function<void()> on_cancel = nullptr);

    // 显示确认对话框
    void showConfirm(const std::string& title, const std::string& message,
                     std::function<void()> on_confirm = nullptr,
                     std::function<void()> on_cancel = nullptr);

    // 处理输入
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render();

    // 是否可见
    bool isVisible() const {
        return visible_;
    }
    void setVisible(bool visible) {
        visible_ = visible;
    }

    // 获取输入值
    std::string getInputValue() const {
        return input_value_;
    }

    // 重置
    void reset();

  private:
    Theme& theme_;
    bool visible_;
    DialogType type_;
    std::string title_;
    std::string prompt_;
    std::string message_;
    std::string input_value_;
    size_t cursor_position_;

    std::function<void(const std::string&)> on_input_confirm_;
    std::function<void()> on_confirm_;
    std::function<void()> on_cancel_;

    void insertChar(char ch);
    void deleteChar();
    void backspace();
    void moveCursorLeft();
    void moveCursorRight();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_DIALOG_H
