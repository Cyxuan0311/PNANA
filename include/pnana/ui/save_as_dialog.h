#ifndef PNANA_UI_SAVE_AS_DIALOG_H
#define PNANA_UI_SAVE_AS_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <optional>
#include <string>

namespace pnana {
namespace ui {

// 另存为对话框组件
class SaveAsDialog {
  public:
    explicit SaveAsDialog(Theme& theme);

    // 设置当前文件名（用于判断是否为未命名文件）
    void setCurrentFileName(const std::string& filename);

    // 设置输入内容
    void setInput(const std::string& input);

    // 获取输入内容
    std::string getInput() const {
        return input_;
    }

    // 渲染对话框
    ftxui::Element render();

    // 处理输入事件，返回是否被处理
    bool handleInput(ftxui::Event event);

    // 设置当前目录（用于相对路径解析与补全）
    void setCurrentDirectory(const std::string& dir) {
        current_directory_ = dir;
    }

    // 设置确认/取消回调
    void setOnConfirm(std::function<void(const std::string&)> cb) {
        on_confirm_ = std::move(cb);
    }
    void setOnCancel(std::function<void()> cb) {
        on_cancel_ = std::move(cb);
    }

  private:
    Theme& theme_;
    std::string current_filename_;
    std::string input_;
    std::string current_directory_;

    std::function<void(const std::string&)> on_confirm_;
    std::function<void()> on_cancel_;

    // 判断是否为未命名文件
    bool isUntitled() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SAVE_AS_DIALOG_H
