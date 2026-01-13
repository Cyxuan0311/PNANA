#ifndef PNANA_UI_SPLIT_WELCOME_SCREEN_H
#define PNANA_UI_SPLIT_WELCOME_SCREEN_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace ui {

// 分屏欢迎界面组件
class SplitWelcomeScreen {
  public:
    explicit SplitWelcomeScreen(Theme& theme);

    // 渲染分屏欢迎界面
    ftxui::Element render();

  private:
    Theme& theme_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SPLIT_WELCOME_SCREEN_H
