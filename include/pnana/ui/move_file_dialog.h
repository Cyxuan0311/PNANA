#ifndef PNANA_UI_MOVE_FILE_DIALOG_H
#define PNANA_UI_MOVE_FILE_DIALOG_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace ui {

// 移动文件/文件夹对话框组件
class MoveFileDialog {
  public:
    explicit MoveFileDialog(Theme& theme);

    // 设置要移动的文件/文件夹路径
    void setSourcePath(const std::string& path);

    // 设置目标目录
    void setTargetDirectory(const std::string& dir);

    // 设置输入内容（目标路径）
    void setInput(const std::string& input);

    // 获取输入内容
    std::string getInput() const {
        return input_;
    }

    // 获取源路径
    std::string getSourcePath() const {
        return source_path_;
    }

    // 渲染对话框
    ftxui::Element render();

  private:
    Theme& theme_;
    std::string source_path_;
    std::string source_name_;
    std::string target_directory_;
    std::string input_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_MOVE_FILE_DIALOG_H
