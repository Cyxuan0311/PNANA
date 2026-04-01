#include "ui/statusbar_theme.h"

namespace pnana {
namespace ui {

std::vector<std::string> getAvailableStatusbarStyleNames() {
    return {
        "default",     // 使用主题内置状态栏（无额外装饰）
        "neovim",      // Neovim 风格状态栏
        "vscode",      // VSCode 风格状态栏
        "minimal",     // 极简风格
        "classic",     // 经典风格
        "highlight",   // 全部项高亮
        "rounded",     // 圆角边框效果
        "unicode",     // Unicode 框线字符修饰（╭ ╮ ║ ─）
        "gradient",    // 渐变效果 + 阴影 + 圆角
        "powerline",   // Powerline 风格（类似 tmux/powerline）
        "modern",      // 现代简约风格（圆角 + 阴影）
        "retro",       // 复古终端风格（绿底绿字）
        "solarized",   // Solarized Dark 经典配色
        "dracula",     // Dracula 主题（紫红色调）
        "monokai",     // Monokai 经典（暖黄调）
        "oceanic",     // Oceanic Next（蓝绿色调）
        "gruvbox",     // Gruvbox Dark（暖棕复古）
        "cyberpunk",   // 赛博朋克（霓虹粉紫）
        "iceberg",     // Iceberg（冷色调）
        "nord",        // Nord（北极光配色）
        "tokyo-night", // Tokyo Night（深蓝夜景）
        "everforest",  // Everforest（森林绿调）
        "kanagawa",    // Kanagawa（浮世绘风格）
        "catppuccin",  // Catppuccin（可爱粉彩）
        "rose-pine",   // Rose Pine（玫瑰松木）
        "material",    // Material Design（材料设计）
        "one-dark",    // One Dark（Atom 经典）
        "ayu-dark",    // Ayu Dark（深沉配色）
        "alba",        // Alba（极简白昼）
        "synthwave",   // Synthwave（合成器浪潮）
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
        // Unicode 框线字符修饰（使用 ╭ ╮ ╰ ╯ ─ │ 等字符）
        config.enabled = true;
        config.follow_theme = true;
        config.style_name = "unicode";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = true;
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
    if (style == "tokyo-night") {
        // Tokyo Night 风格（深蓝夜景）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {26, 27, 38};
        config.fg_color = {192, 202, 245};
        config.style_name = "tokyo-night";
        config.icon_style = "outlined";
        config.show_gradient = true;
        config.show_shadows = true;
        config.rounded_corners = true;
        return config;
    }
    if (style == "everforest") {
        // Everforest 风格（森林绿调）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {33, 41, 38};
        config.fg_color = {214, 222, 208};
        config.style_name = "everforest";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }
    if (style == "kanagawa") {
        // Kanagawa 风格（浮世绘配色）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {31, 32, 45};
        config.fg_color = {223, 216, 191};
        config.style_name = "kanagawa";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = true;
        config.rounded_corners = true;
        return config;
    }
    if (style == "catppuccin") {
        // Catppuccin 风格（可爱粉彩）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {30, 30, 46};
        config.fg_color = {205, 214, 244};
        config.style_name = "catppuccin";
        config.icon_style = "filled";
        config.show_gradient = true;
        config.show_shadows = true;
        config.rounded_corners = true;
        return config;
    }
    if (style == "rose-pine") {
        // Rose Pine 风格（玫瑰松木）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {30, 29, 47};
        config.fg_color = {224, 222, 230};
        config.style_name = "rose-pine";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }
    if (style == "material") {
        // Material Design 风格（材料设计）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {33, 33, 33};
        config.fg_color = {255, 255, 255};
        config.style_name = "material";
        config.icon_style = "filled";
        config.show_gradient = false;
        config.show_shadows = true;
        config.rounded_corners = false;
        return config;
    }
    if (style == "one-dark") {
        // One Dark 风格（Atom 经典配色）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {40, 44, 52};
        config.fg_color = {224, 226, 228};
        config.style_name = "one-dark";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }
    if (style == "ayu-dark") {
        // Ayu Dark 风格（深沉配色）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {27, 30, 36};
        config.fg_color = {199, 205, 217};
        config.style_name = "ayu-dark";
        config.icon_style = "default";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = true;
        return config;
    }
    if (style == "alba") {
        // Alba 风格（极简白昼）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {255, 255, 255};
        config.fg_color = {50, 50, 50};
        config.style_name = "alba";
        config.icon_style = "outlined";
        config.show_gradient = false;
        config.show_shadows = false;
        config.rounded_corners = false;
        return config;
    }
    if (style == "synthwave") {
        // Synthwave 风格（合成器浪潮）
        config.enabled = true;
        config.follow_theme = false;
        config.bg_color = {30, 20, 50};
        config.fg_color = {255, 100, 200};
        config.style_name = "synthwave";
        config.icon_style = "filled";
        config.show_gradient = true;
        config.show_shadows = true;
        config.rounded_corners = false;
        return config;
    }

    config.style_name = style;
    return config;
}

} // namespace ui
} // namespace pnana
