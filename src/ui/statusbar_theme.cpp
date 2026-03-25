#include "ui/statusbar_theme.h"

namespace pnana {
namespace ui {

std::vector<std::string> getAvailableStatusbarStyleNames() {
    return {
        "default",   // 使用主题内置状态栏（无额外装饰）
        "neovim",    // Neovim 风格状态栏
        "vscode",    // VSCode 风格状态栏
        "minimal",   // 极简风格
        "classic",   // 经典风格
        "highlight", // 全部项高亮
        "rounded",   // 圆角边框效果
        "unicode",   // Unicode 框线字符修饰（╭ ╮ ║ ─）
        "gradient",  // 渐变效果 + 阴影 + 圆角
        "powerline", // Powerline 风格（类似 tmux/powerline）
        "modern",    // 现代简约风格（圆角 + 阴影）
        "retro",     // 复古终端风格（绿底绿字）
        "solarized", // Solarized Dark 经典配色
        "dracula",   // Dracula 主题（紫红色调）
        "monokai",   // Monokai 经典（暖黄调）
        "oceanic",   // Oceanic Next（蓝绿色调）
        "gruvbox",   // Gruvbox Dark（暖棕复古）
        "cyberpunk", // 赛博朋克（霓虹粉紫）
        "iceberg",   // Iceberg（冷色调）
        "nord",      // Nord（北极光配色）
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
    if (style == "gradient") {
        // 渐变效果状态栏
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {30, 30, 40};
        config.fg_color = {255, 255, 255};
        config.style_name = "gradient";
        config.icon_style = "filled";
        config.show_gradient = true;
        config.show_shadows = true;
        config.rounded_corners = true;
        return config;
    }
    if (style == "powerline") {
        // Powerline 风格（类似 tmux/powerline 插件）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {0, 0, 0};
        config.fg_color = {255, 255, 255};
        config.style_name = "powerline";
        config.icon_style = "filled";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "modern") {
        // 现代简约风格（圆角 + 阴影 + 半透明效果）
        config.enabled = true;
        config.follow_theme = true;
        config.style_name = "modern";
        config.icon_style = "outlined";
        config.show_gradient = true;
        config.show_shadows = true;
        config.rounded_corners = true;
        return config;
    }
    if (style == "retro") {
        // 复古终端风格（单色 + 粗边框）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {0, 128, 0};
        config.fg_color = {0, 255, 0};
        config.style_name = "retro";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "solarized") {
        // Solarized Dark 风格（经典配色方案）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {0, 43, 54};
        config.fg_color = {131, 148, 150};
        config.style_name = "solarized";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "dracula") {
        // Dracula 主题风格
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {40, 42, 54};
        config.fg_color = {248, 248, 242};
        config.style_name = "dracula";
        config.icon_style = "filled";
        config.show_gradient = true;
        config.show_shadows = true;
        config.rounded_corners = true;
        return config;
    }
    if (style == "monokai") {
        // Monokai 经典风格
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {39, 40, 34};
        config.fg_color = {248, 248, 242};
        config.style_name = "monokai";
        config.icon_style = "filled";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "oceanic") {
        // Oceanic Next 风格（蓝绿色调）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {23, 33, 39};
        config.fg_color = {216, 222, 233};
        config.style_name = "oceanic";
        config.icon_style = "outlined";
        config.show_gradient = true;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }
    if (style == "gruvbox") {
        // Gruvbox Dark 风格（暖色调复古）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {40, 40, 40};
        config.fg_color = {235, 219, 178};
        config.style_name = "gruvbox";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "cyberpunk") {
        // 赛博朋克风格（霓虹配色）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {15, 15, 35};
        config.fg_color = {255, 0, 255};
        config.style_name = "cyberpunk";
        config.icon_style = "filled";
        config.show_gradient = true;
        config.show_shadows = true;
        config.rounded_corners = false;
        return config;
    }
    if (style == "iceberg") {
        // Iceberg 风格（冷色调）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {35, 37, 47};
        config.fg_color = {206, 213, 227};
        config.style_name = "iceberg";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }
    if (style == "nord") {
        // Nord 风格（北极光配色）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {47, 52, 64};
        config.fg_color = {216, 222, 233};
        config.style_name = "nord";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }

    config.style_name = style;
    return config;
}

} // namespace ui
} // namespace pnana
