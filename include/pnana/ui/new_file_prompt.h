#ifndef PNANA_UI_NEW_FILE_PROMPT_H
#define PNANA_UI_NEW_FILE_PROMPT_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace ui {

// 新文件输入提示界面组件
class NewFilePrompt {
  public:
    explicit NewFilePrompt(Theme& theme);

    // 渲染新文件输入提示界面
    ftxui::Element render();

  private:
    Theme& theme_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_NEW_FILE_PROMPT_H
