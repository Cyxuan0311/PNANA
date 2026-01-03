#include "core/ui/border_manager.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace core {
namespace ui {

BorderManager::BorderManager() = default;

ftxui::Color BorderManager::getActiveBorderColor(const pnana::ui::Theme& theme) const {
    // 使用主题的 keyword 颜色作为激活边框颜色（高亮色）
    return theme.getColors().keyword;
}

ftxui::Color BorderManager::getInactiveBorderColor(const pnana::ui::Theme& theme) const {
    // 使用主题的 comment 颜色作为非激活边框颜色（默认色）
    return theme.getColors().comment;
}

ftxui::Color BorderManager::getBorderColor(EditorRegion region, bool is_active,
                                           const pnana::ui::Theme& theme) const {
    (void)region; // 暂时未使用，保留用于未来扩展
    return is_active ? getActiveBorderColor(theme) : getInactiveBorderColor(theme);
}

ftxui::Element BorderManager::applyBorder(ftxui::Element content, EditorRegion region,
                                          bool is_active, const pnana::ui::Theme& theme) {
    ftxui::Color border_color = getBorderColor(region, is_active, theme);

    // 使用 ftxui 的 border 装饰器
    return content | ftxui::border | ftxui::color(border_color);
}

ftxui::Color BorderManager::getColorFromTheme(const pnana::ui::Theme& theme,
                                              const std::string& color_name) const {
    // 这个方法可以用于从主题中获取特定名称的颜色
    // 目前直接使用 theme.getColors() 访问
    // 如果需要，可以扩展 Theme 类支持按名称获取颜色
    (void)color_name; // 暂时未使用
    return theme.getColors().foreground;
}

} // namespace ui
} // namespace core
} // namespace pnana
