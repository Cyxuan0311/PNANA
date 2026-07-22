#include "core/ui/border_manager.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace core {
namespace ui {

// 静态成员初始化
BorderConfig BorderManager::current_config_;

BorderManager::BorderManager() = default;

void BorderManager::setBorderConfig(const BorderConfig& config) {
    current_config_ = config;
}

ftxui::BorderStyle BorderManager::getCurrentStyle() {
    return stringToBorderStyle(current_config_.global_style);
}

ftxui::BorderStyle BorderManager::getStyleForContext(bool is_active) {
    if (is_active && !current_config_.active_style.empty())
        return stringToBorderStyle(current_config_.active_style);
    if (!is_active && !current_config_.inactive_style.empty())
        return stringToBorderStyle(current_config_.inactive_style);
    return stringToBorderStyle(current_config_.global_style);
}

ftxui::Color BorderManager::getActiveBorderColor(const pnana::ui::Theme& theme) const {
    return theme.getColors().keyword;
}

ftxui::Color BorderManager::getInactiveBorderColor(const pnana::ui::Theme& theme) const {
    return theme.getColors().comment;
}

ftxui::Color BorderManager::getBorderColor(EditorRegion region, bool is_active,
                                           const pnana::ui::Theme& theme) const {
    (void)region;
    return is_active ? getActiveBorderColor(theme) : getInactiveBorderColor(theme);
}

ftxui::Element BorderManager::applyBorder(ftxui::Element content, EditorRegion region,
                                          bool is_active, const pnana::ui::Theme& theme) {
    ftxui::Color border_color = getBorderColor(region, is_active, theme);
    ftxui::BorderStyle style = getStyleForContext(is_active);
    return content | ftxui::borderStyled(style) | ftxui::color(border_color);
}

ftxui::Color BorderManager::getColorFromTheme(const pnana::ui::Theme& theme,
                                              const std::string& color_name) const {
    (void)color_name;
    return theme.getColors().foreground;
}

} // namespace ui
} // namespace core
} // namespace pnana
