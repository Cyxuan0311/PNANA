#ifndef PNANA_UI_BINARY_FILE_VIEW_H
#define PNANA_UI_BINARY_FILE_VIEW_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace ui {

// 二进制文件视图组件 - 显示无法打开的二进制文件提示
class BinaryFileView {
  public:
    explicit BinaryFileView(Theme& theme);

    // 设置文件路径
    void setFilePath(const std::string& filepath);

    // 渲染二进制文件视图
    ftxui::Element render();

  private:
    Theme& theme_;
    std::string filepath_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_BINARY_FILE_VIEW_H
