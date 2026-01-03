#ifndef PNANA_UI_SAVE_AS_DIALOG_H
#define PNANA_UI_SAVE_AS_DIALOG_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
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

  private:
    Theme& theme_;
    std::string current_filename_;
    std::string input_;

    // 判断是否为未命名文件
    bool isUntitled() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SAVE_AS_DIALOG_H
