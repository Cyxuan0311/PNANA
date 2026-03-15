#include "ui/statusbar_theme.h"

namespace pnana {
namespace ui {

std::vector<std::string> getAvailableStatusbarStyleNames() {
    return {
        "default", // 使用主题内置状态栏（无额外装饰）
        "neovim",    "vscode", "minimal", "classic",
        "highlight", // 全部项高亮
        "rounded",   // 圆角边框效果
        "unicode",   // Unicode 框线字符修饰（╭ ╮ ║ ─）
    };
}

StatusbarBeautifyConfig getStatusbarConfigForStyle(const std::string& style_name) {
    StatusbarBeautifyConfig config;
    std::string style = style_name.empty() ? "default" : style_name;

    if (style == "default") {
        config.style_name = "default";
        return config;
    }
    if (style == "neovim") {
        config.enabled = true;
        config.follow_theme = true;
        config.style_name = "neovim";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "vscode") {
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {37, 37, 38};
        config.fg_color = {220, 220, 220};
        config.style_name = "vscode";
        config.icon_style = "filled";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "minimal") {
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {20, 20, 20};
        config.fg_color = {190, 190, 190};
        config.style_name = "minimal";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "classic") {
        config.enabled = false;
        config.follow_theme = true;
        config.style_name = "classic";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "highlight") {
        config.enabled = true;
        config.follow_theme = true;
        config.style_name = "highlight";
        config.icon_style = "filled";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "rounded") {
        config.enabled = true;
        config.follow_theme = true;
        config.style_name = "rounded";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }
    if (style == "unicode") {
        config.enabled = true;
        config.follow_theme = true;
        config.style_name = "unicode";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }

    config.style_name = style;
    return config;
}

} // namespace ui
} // namespace pnana
