#ifndef PNANA_UI_CREATE_FOLDER_DIALOG_H
#define PNANA_UI_CREATE_FOLDER_DIALOG_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace ui {

// 创建文件夹对话框组件
class CreateFolderDialog {
  public:
    explicit CreateFolderDialog(Theme& theme);

    // 设置当前目录
    void setCurrentDirectory(const std::string& dir);

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
    std::string current_directory_;
    std::string input_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_CREATE_FOLDER_DIALOG_H
