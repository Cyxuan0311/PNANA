#include "ui/welcome_screen.h"
#include "features/logo_manager.h"
#include "ui/icons.h"
#include <cmath>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

WelcomeScreen::WelcomeScreen(Theme& theme, core::ConfigManager& config)
    : theme_(theme), config_(config) {}

Element WelcomeScreen::render() {
    auto& colors = theme_.getColors();
    logo_animation_.setConfig(config_.getConfig().animation);

    Elements welcome_content;

    // Logo 顶部间距（仅在关闭 flex 时生效）
    if (!config_.getConfig().display.welcome_screen_flex) {
        int logo_margin = config_.getConfig().display.welcome_logo_top_margin;
        if (logo_margin < 0)
            logo_margin = 0;
        for (int i = 0; i < logo_margin; ++i) {
            welcome_content.push_back(text(""));
        }
    }

    // Logo 渐变色：根据配置决定是否使用渐变
    std::vector<Color> gradient_colors;
    if (config_.getConfig().display.logo_gradient) {
        gradient_colors = theme_.getGradientColors();
    } else {
        gradient_colors = {colors.success, colors.success, colors.success,
                           colors.success, colors.success, colors.success};
    }
    if (gradient_colors.size() < 6) {
        gradient_colors = {colors.success, colors.success, colors.success,
                           colors.success, colors.success, colors.success};
    }

    // Logo：应用色彩动画效果
    if (config_.getConfig().display.show_welcome_logo) {
        std::string logo_style = config_.getConfig().display.logo_style;
        std::vector<std::string> logo_lines = features::LogoManager::getLogoLines(logo_style);

        logo_animation_.setConfig(config_.getConfig().animation);
        const auto frame = logo_animation_.currentFrame();

        // 获取色彩动画参数
        const float glow = frame.getGlowIntensity();
        const float sharpness = frame.getSharpness();
        const float color_shift = frame.getColorShift();
        const float hue_shift = frame.getHueShift();
        const float sat_boost = frame.getSaturationBoost();
        const float val_mod = frame.getValueModulation();
        const bool is_none_mode = (config_.getConfig().animation.effect == "none");

        // 根据色相偏移计算颜色索引偏移量（0-360 度映射到颜色索引，增强变化幅度）
        const size_t hue_color_offset =
            is_none_mode
                ? 0
                : static_cast<size_t>(std::fmod(hue_shift / 30.0f, gradient_colors.size() * 2));

        for (size_t i = 0; i < logo_lines.size(); ++i) {
            size_t base_g = i % gradient_colors.size();

            // 应用色相偏移（增强 chroma_shift 和 hue_shift 的影响），none 模式不偏移
            size_t g = is_none_mode
                           ? base_g
                           : static_cast<size_t>((hue_color_offset +
                                                  static_cast<size_t>(color_shift * 4) + base_g) %
                                                 gradient_colors.size());

            // 根据饱和度和明度调整选择颜色（增强对比度）
            Element logo_line;
            if (sat_boost > 1.3f && val_mod > 1.15f) {
                // 高饱和高明度：使用更亮的颜色 + 加粗
                logo_line = text("  " + logo_lines[i]) | color(gradient_colors[g]) | bold;
            } else if (sat_boost < 0.85f || val_mod < 0.85f) {
                // 低饱和或低明度：使用较暗的颜色 + 淡化
                logo_line = text("  " + logo_lines[i]) | color(gradient_colors[g]) | dim;
            } else {
                logo_line = text("  " + logo_lines[i]) | color(gradient_colors[g]);
            }

            // 呼吸/闪烁效果：none 模式保持恒定亮度，其他模式根据脉冲波和锐度调整
            if (!is_none_mode) {
                const float adjusted_pulse = frame.pulse_wave * sharpness;
                if (adjusted_pulse > 0.0f) {
                    // 亮周期：根据发光强度决定是否加粗和闪烁
                    if (glow > 0.6f) {
                        logo_line = logo_line | bold | blink;
                    } else {
                        logo_line = logo_line | bold;
                    }
                } else {
                    // 暗周期：根据发光强度决定变暗程度
                    if (glow < 0.3f) {
                        logo_line = logo_line | dim;
                    }
                }
            } else {
                // none 模式：始终加粗，不闪烁
                logo_line = logo_line | bold;
            }

            // 抖动效果（某些动画效果会有 jitter 参数），none 模式不抖动
            if (!is_none_mode && frame.jitter > 0.3f && i % 2 == 0) {
                // 偶数行轻微偏移，模拟抖动
                logo_line = hbox({text(" "), logo_line});
            }

            welcome_content.push_back(logo_line | center);
        }
    }

    welcome_content.push_back(text(""));

    if (config_.getConfig().display.show_welcome_version) {
        welcome_content.push_back(text("Modern Terminal Text Editor") | color(colors.foreground) |
                                  bold | center);
        welcome_content.push_back(
            hbox({text("Version") | color(colors.comment) | dim, text("  "),
                  text("0.0.6") | bgcolor(colors.success) | color(colors.background) | bold}) |
            center);
    }

    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));

    // Start editing hint (highlighted)
    if (config_.getConfig().display.show_welcome_start_hint) {
        welcome_content.push_back(
            hbox({text(" "), text(icons::BULB) | color(colors.warning), text(" Press "),
                  text(" i ") | bgcolor(colors.keyword) | color(colors.background) | bold,
                  text(" to start editing a new document ")}) |
            color(colors.foreground) | center);

        welcome_content.push_back(text(""));
        welcome_content.push_back(text(""));
    }

    // Quick Start section
    if (config_.getConfig().display.show_welcome_quick_start) {
        welcome_content.push_back(hbox({text(icons::ROCKET), text(" Quick Start")}) |
                                  color(colors.keyword) | bold | center);
        welcome_content.push_back(text(""));

        welcome_content.push_back(
            hbox({text("  "), text("Ctrl+O") | color(colors.function) | bold,
                  text("  Open file    "), text("Ctrl+N") | color(colors.function) | bold,
                  text("  New file")}) |
            center);

        welcome_content.push_back(
            hbox({text("  "), text("Ctrl+S") | color(colors.function) | bold,
                  text("  Save file    "), text("Ctrl+Q") | color(colors.function) | bold,
                  text("  Quit editor")}) |
            center);

        welcome_content.push_back(text(""));
    }

    // Features section
    if (config_.getConfig().display.show_welcome_features) {
        welcome_content.push_back(hbox({text(icons::STAR), text(" Features")}) |
                                  color(colors.keyword) | bold | center);
        welcome_content.push_back(text(""));

        welcome_content.push_back(
            hbox({text("  "), text("Ctrl+F") | color(colors.function) | bold,
                  text("  Search       "), text("Ctrl+G") | color(colors.function) | bold,
                  text("  Go to line")}) |
            center);

        welcome_content.push_back(
            hbox({text("  "), text("Ctrl+T") | color(colors.function) | bold,
                  text("  Themes       "), text("Ctrl+Z") | color(colors.function) | bold,
                  text("  Undo")}) |
            center);

        welcome_content.push_back(text(""));
        welcome_content.push_back(text(""));
    }

    // 提示信息
    if (config_.getConfig().display.show_welcome_tips) {
        welcome_content.push_back(
            hbox({text(icons::BULB), text(" Tip: Just start typing to begin editing!")}) |
            color(colors.success) | bold | center);

        welcome_content.push_back(text(""));

        welcome_content.push_back(text("Press Ctrl+T to choose from multiple themes") |
                                  color(colors.comment) | dim | center);

        welcome_content.push_back(text(""));

        // 底部信息
        welcome_content.push_back(text("─────────────────────────────────────────────────") |
                                  color(colors.comment) | bold | center);
        welcome_content.push_back(text("Check the bottom bar for more shortcuts") |
                                  color(colors.comment) | dim | center);
        welcome_content.push_back(text(""));
        welcome_content.push_back(text(""));
    }

    bool use_flex = config_.getConfig().display.welcome_screen_flex;

    if (use_flex) {
        return vbox({filler(), vbox(welcome_content), filler()}) | flex;
    } else {
        return vbox(welcome_content);
    }
}

} // namespace ui
} // namespace pnana
