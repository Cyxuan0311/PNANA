#ifndef PNANA_UI_EXTRACT_PROGRESS_DIALOG_H
#define PNANA_UI_EXTRACT_PROGRESS_DIALOG_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace ui {

// 解压进度对话框
class ExtractProgressDialog {
  public:
    explicit ExtractProgressDialog(Theme& theme);

    // 显示对话框
    void show(const std::string& archive_name, const std::string& extract_path);

    // 隐藏对话框
    void hide();

    // 检查是否可见
    bool isVisible() const {
        return visible_;
    }

    // 设置进度（0.0 - 1.0）
    void setProgress(float progress);

    // 设置状态消息
    void setStatus(const std::string& status);

    // 渲染对话框
    ftxui::Element render();

  private:
    Theme& theme_;
    bool visible_;
    std::string archive_name_;
    std::string extract_path_;
    float progress_;
    std::string status_;

    // 渲染进度条
    ftxui::Element renderProgressBar();

    // 渲染旋转动画
    ftxui::Element renderSpinner();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_EXTRACT_PROGRESS_DIALOG_H
