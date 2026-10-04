#ifndef PNANA_CORE_UI_BORDER_MANAGER_H
#define PNANA_CORE_UI_BORDER_MANAGER_H

#include "core/config_manager.h"
#include "core/region_manager.h"
#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <utility>

namespace pnana {
namespace core {
namespace ui {

// 边框管理器：管理面板边框颜色变化
class BorderManager {
  public:
    BorderManager();

    // 设置全局边框配置（Editor 启动 / config 热重载时调用）
    static void setBorderConfig(const BorderConfig& config);

    // 获取当前边框样式（供 popup/dialog 统一使用）
    static ftxui::BorderStyle getCurrentStyle();

    // 根据上下文获取边框样式
    static ftxui::BorderStyle getStyleForContext(bool is_active);

    // 获取激活区域的边框颜色（高亮色）
    ftxui::Color getActiveBorderColor(const pnana::ui::Theme& theme) const;

    // 获取非激活区域的边框颜色（默认色）
    ftxui::Color getInactiveBorderColor(const pnana::ui::Theme& theme) const;

    // 根据区域是否激活返回边框颜色
    ftxui::Color getBorderColor(EditorRegion region, bool is_active,
                                const pnana::ui::Theme& theme) const;

    // 应用边框到元素（使用配置的样式）
    ftxui::Element applyBorder(ftxui::Element content, EditorRegion region, bool is_active,
                               const pnana::ui::Theme& theme);

  private:
    static BorderConfig current_config_;

    // 从主题获取颜色
    ftxui::Color getColorFromTheme(const pnana::ui::Theme& theme,
                                   const std::string& color_name) const;
};

// ---- 以下为 inline 工具函数 ----

// 边框样式字符串 → ftxui::BorderStyle 映射
inline ftxui::BorderStyle stringToBorderStyle(const std::string& style) {
    if (style == "rounded")
        return ftxui::ROUNDED;
    if (style == "double")
        return ftxui::DOUBLE;
    if (style == "heavy")
        return ftxui::HEAVY;
    if (style == "dashed")
        return ftxui::DASHED;
    if (style == "empty" || style == "none")
        return ftxui::EMPTY;
    return ftxui::LIGHT;
}

// 共享边框装饰器：供各 popup/dialog 统一使用
inline ftxui::Decorator makeBorderDecorator(ftxui::Color color) {
    return [color](ftxui::Element child) -> ftxui::Element {
        return std::move(child) | ftxui::borderStyled(BorderManager::getCurrentStyle()) |
               ftxui::color(color);
    };
}

} // namespace ui
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_UI_BORDER_MANAGER_H
