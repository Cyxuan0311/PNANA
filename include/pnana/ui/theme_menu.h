#ifndef PNANA_UI_THEME_MENU_H
#define PNANA_UI_THEME_MENU_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 主题菜单组件
class ThemeMenu {
  public:
    explicit ThemeMenu(Theme& theme);

    // 设置可用主题列表
    void setAvailableThemes(const std::vector<std::string>& themes);

    // 设置当前选中的主题索引
    void setSelectedIndex(size_t index);

    // 获取当前选中的主题索引
    size_t getSelectedIndex() const {
        return selected_index_;
    }

    // 获取可用主题列表
    const std::vector<std::string>& getAvailableThemes() const {
        return available_themes_;
    }

    // 获取当前主题名称
    std::string getCurrentThemeName() const;

    // 渲染主题菜单
    ftxui::Element render();

  private:
    Theme& theme_;
    std::vector<std::string> available_themes_;
    size_t selected_index_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_THEME_MENU_H
