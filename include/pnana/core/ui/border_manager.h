#ifndef PNANA_CORE_UI_BORDER_MANAGER_H
#define PNANA_CORE_UI_BORDER_MANAGER_H

#include "core/region_manager.h"
#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace core {
namespace ui {

// 边框管理器：管理面板边框颜色变化
class BorderManager {
  public:
    BorderManager();

    // 获取激活区域的边框颜色（高亮色）
    ftxui::Color getActiveBorderColor(const pnana::ui::Theme& theme) const;

    // 获取非激活区域的边框颜色（默认色）
    ftxui::Color getInactiveBorderColor(const pnana::ui::Theme& theme) const;

    // 根据区域是否激活返回边框颜色
    ftxui::Color getBorderColor(EditorRegion region, bool is_active,
                                const pnana::ui::Theme& theme) const;

    // 应用边框到元素
    ftxui::Element applyBorder(ftxui::Element content, EditorRegion region, bool is_active,
                               const pnana::ui::Theme& theme);

  private:
    // 从主题获取颜色
    ftxui::Color getColorFromTheme(const pnana::ui::Theme& theme,
                                   const std::string& color_name) const;
};

} // namespace ui
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_UI_BORDER_MANAGER_H
