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

    // Logo：逐字符 2D 平滑渐变
    if (config_.getConfig().display.show_welcome_logo) {
        std::string logo_style = config_.getConfig().display.logo_style;
        std::vector<std::string> logo_lines = features::LogoManager::getLogoLines(logo_style);

        logo_animation_.setConfig(config_.getConfig().animation);
        const auto frame = logo_animation_.currentFrame();

        const float glow = frame.getGlowIntensity();
        const float sharpness = frame.getSharpness();
        const float hue_shift = frame.getHueShift();
        const float sat_boost = frame.getSaturationBoost();
        const float val_mod = frame.getValueModulation();
        const bool is_none_mode = (config_.getConfig().animation.effect == "none");

        // 1) 对调色板应用 hue_shift
        std::vector<Color> palette = gradient_colors;
        if (!is_none_mode && hue_shift != 0.0f) {
            int offset = static_cast<int>(std::fmod(hue_shift / 30.0f, palette.size() * 2));
            offset %= static_cast<int>(palette.size());
            if (offset) {
                std::rotate(palette.begin(), palette.begin() + offset, palette.end());
            }
        }

        // 2) 将每行拆分为独立字符（UTF‑8 感知），并求出最大列数
        auto splitGlyphs = [](const std::string& s) {
            std::vector<std::string> out;
            for (size_t i = 0; i < s.size();) {
                unsigned char c = s[i];
                size_t len = 1;
                if (c >= 0xF0)
                    len = 4;
                else if (c >= 0xE0)
                    len = 3;
                else if (c >= 0xC0)
                    len = 2;
                out.push_back(s.substr(i, len));
                i += len;
            }
            return out;
        };

        std::vector<std::vector<std::string>> logo_glyphs;
        size_t max_cols = 0;
        for (const auto& line : logo_lines) {
            auto glyphs = splitGlyphs(line);
            if (glyphs.size() > max_cols)
                max_cols = glyphs.size();
            logo_glyphs.push_back(std::move(glyphs));
        }

        // 3) 生成逐字符渐变
        auto smooth_colors =
            features::LogoManager::generateSmoothGradient(palette, logo_glyphs.size(), max_cols);

        // 4) 逐行渲染
        for (size_t i = 0; i < logo_glyphs.size(); ++i) {
            const auto& glyphs = logo_glyphs[i];

            Elements char_elems;
            char_elems.push_back(text("  ")); // 前缀缩进
            for (size_t j = 0; j < glyphs.size(); ++j) {
                size_t idx = i * max_cols + j;
                Color ch_color =
                    (idx < smooth_colors.size()) ? smooth_colors[idx] : gradient_colors[0];

                char_elems.push_back(text(glyphs[j]) | color(ch_color));
            }
            Element logo_line = hbox(std::move(char_elems));

            // 应用动画装饰（逐行）
            bool has_glow = false, has_dim = false;
            if (!is_none_mode) {
                float pulse = frame.pulse_wave * sharpness;
                if (pulse > 0.0f) {
                    has_glow = true;
                    if (glow > 0.6f)
                        logo_line = logo_line | bold | blink;
                    else
                        logo_line = logo_line | bold;
                } else {
                    if (glow < 0.3f) {
                        has_dim = true;
                        logo_line = logo_line | dim;
                    }
                }
            } else {
                logo_line = logo_line | bold;
            }

            if (!is_none_mode && !has_glow && !has_dim && sat_boost > 1.3f && val_mod > 1.15f)
                logo_line = logo_line | bold;
            if (!is_none_mode && !has_glow && !has_dim && (sat_boost < 0.85f || val_mod < 0.85f))
                logo_line = logo_line | dim;

            // 抖动
            if (!is_none_mode && frame.jitter > 0.3f && i % 2 == 0)
                logo_line = hbox({text(" "), logo_line});

            welcome_content.push_back(logo_line | center);
        }
    }

    welcome_content.push_back(text(""));

    if (config_.getConfig().display.show_welcome_version) {
        welcome_content.push_back(text("Modern Terminal Text Editor") | color(colors.foreground) |
                                  bold | center);
        welcome_content.push_back(
            hbox({text("Version") | color(colors.comment) | dim, text("  "),
                  text("0.0.7") | bgcolor(colors.success) | color(colors.background) | bold}) |
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
