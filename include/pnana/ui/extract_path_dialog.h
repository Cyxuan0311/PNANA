#ifndef PNANA_UI_EXTRACT_PATH_DIALOG_H
#define PNANA_UI_EXTRACT_PATH_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>

namespace pnana {
namespace ui {

// 解压路径输入对话框
class ExtractPathDialog {
  public:
    explicit ExtractPathDialog(Theme& theme);

    // 显示对话框
    void show(const std::string& archive_name, const std::string& default_path,
              std::function<void(const std::string&, const std::string&)> on_confirm,
              std::function<void()> on_cancel);

    // 隐藏对话框
    void hide();

    // 检查是否可见
    bool isVisible() const {
        return visible_;
    }

    // 处理输入事件
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render();

    // 获取输入内容
    std::string getPathInput() const {
        return path_input_;
    }

    std::string getFileNameInput() const {
        return filename_input_;
    }

    // 设置输入内容
    void setPathInput(const std::string& input) {
        path_input_ = input;
        path_cursor_position_ = path_input_.length();
    }

    void setFileNameInput(const std::string& input) {
        filename_input_ = input;
        filename_cursor_position_ = filename_input_.length();
    }

  private:
    // 输入字段类型
    enum class InputField {
        FILENAME, // 文件名输入（第一个）
        PATH      // 路径输入（第二个）
    };

    Theme& theme_;
    bool visible_;
    std::string archive_name_;
    std::string default_path_;

    // 文件名输入
    std::string filename_input_;
    size_t filename_cursor_position_;

    // 路径输入
    std::string path_input_;
    size_t path_cursor_position_;

    // 当前活动的输入字段
    InputField active_field_;

    std::function<void(const std::string&, const std::string&)> on_confirm_;
    std::function<void()> on_cancel_;

    // 输入处理
    void insertChar(char ch);
    void backspace();
    void deleteChar();
    void moveCursorLeft();
    void moveCursorRight();
    void switchFieldUp();   // 向上切换字段
    void switchFieldDown(); // 向下切换字段

    // 获取当前活动的输入字段
    std::string& getCurrentInput() {
        return active_field_ == InputField::FILENAME ? filename_input_ : path_input_;
    }

    size_t& getCurrentCursorPosition() {
        return active_field_ == InputField::FILENAME ? filename_cursor_position_
                                                     : path_cursor_position_;
    }

    // 渲染输入字段（带块状光标）
    ftxui::Element renderInputField(const std::string& label, const std::string& value,
                                    size_t cursor_pos, bool is_active);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_EXTRACT_PATH_DIALOG_H
