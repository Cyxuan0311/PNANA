#ifndef PNANA_UI_WELCOME_SCREEN_H
#define PNANA_UI_WELCOME_SCREEN_H

#include "core/config_manager.h"
#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace ui {

// 欢迎界面组件
class WelcomeScreen {
  public:
    explicit WelcomeScreen(Theme& theme, core::ConfigManager& config);

    // 渲染欢迎界面
    ftxui::Element render();

  private:
    Theme& theme_;
    core::ConfigManager& config_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_WELCOME_SCREEN_H
