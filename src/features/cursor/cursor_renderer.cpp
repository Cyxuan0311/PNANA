#include "features/cursor/cursor_renderer.h"
#include "ui/cursor_config_dialog.h"
#include "utils/text_utils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

void CursorRenderer::setCursorStyle(CursorStyle style) {
    config_.style = style;
}

void CursorRenderer::setCursorColor(ftxui::Color color) {
    config_.color = color;
}

void CursorRenderer::setSmoothCursor(bool smooth) {
    config_.smooth = smooth;
}

void CursorRenderer::setSmoothIntensity(float intensity) {
    smooth_intensity_ = std::clamp(intensity, 0.0f, 1.0f);
}

void CursorRenderer::setBlinkRate(int rate_ms) {
    // rate_ms <= 0 表示不启用闪烁
    if (rate_ms <= 0) {
        blink_rate_ms_ = 0;
    } else {
        blink_rate_ms_ = std::max(50, rate_ms); // 限制一个较小的下限，避免过快闪烁
    }
}

void CursorRenderer::setConfig(const CursorConfig& config) {
    config_ = config;
    // 重置动画状态
    last_update_time_ = std::chrono::steady_clock::now();
    animation_phase_ = 0.0f;
}

CursorStyle CursorRenderer::getCursorStyle() const {
    return config_.style;
}

ftxui::Color CursorRenderer::getCursorColor() const {
    return config_.color;
}

bool CursorRenderer::getSmoothCursor() const {
    return config_.smooth;
}

const CursorConfig& CursorRenderer::getConfig() const {
    return config_;
}

void CursorRenderer::updateCursorState() {
    if (!config_.smooth) {
        return; // 不启用流动效果时不更新状态
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_time_);

    if (elapsed.count() >= 16) { // ~60fps更新频率
        // 更新动画相位（缓慢变化，不会影响性能）
        animation_phase_ += static_cast<float>(elapsed.count()) * 0.002f; // 慢速旋转
        if (animation_phase_ > 2.0f * 3.14159f) {
            animation_phase_ -= 2.0f * 3.14159f;
        }
        last_update_time_ = now;
    }
}

ftxui::Color CursorRenderer::calculateSmoothColor() const {
    if (!config_.smooth || smooth_intensity_ <= 0.0f) {
        return config_.color;
    }

    // 基于动画相位计算颜色变化
    // 使用正弦波创建柔和的颜色渐变效果
    float intensity = (std::sin(animation_phase_) + 1.0f) * 0.5f; // 0-1范围
    intensity = intensity * smooth_intensity_;                    // 应用用户设置的强度

    // 根据原始颜色计算渐变
    if (config_.color == Color::White) {
        // 白色渐变到浅灰色
        uint8_t gray = static_cast<uint8_t>(255 - intensity * 30);
        return Color::RGB(gray, gray, gray);
    } else if (config_.color == Color::Black) {
        // 黑色渐变到深灰色
        uint8_t gray = static_cast<uint8_t>(intensity * 40);
        return Color::RGB(gray, gray, gray);
    } else {
        // 对于其他颜色，稍微调整亮度
        // 这里简化处理，实际可以根据RGB值进行更精确的亮度调整
        return config_.color;
    }
}

float CursorRenderer::getCurrentAnimationPhase() const {
    return animation_phase_;
}

// 渲染光标元素
Element CursorRenderer::renderCursorElement(const std::string& cursor_char, size_t cursor_pos,
                                            size_t line_length, ftxui::Color foreground_color,
                                            ftxui::Color background_color) const {
    // 先处理闪烁逻辑：如果开启闪烁且设置了有效的间隔，根据时间决定是否显示光标
    bool show_cursor = true;
    if (config_.blink_enabled && blink_rate_ms_ > 0) {
        auto now = std::chrono::steady_clock::now();
        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // 简单的开/关闪烁：每 blink_rate_ms_ 翻转一次
        show_cursor = ((ms / blink_rate_ms_) % 2) == 0;
    }

    if (!show_cursor) {
        // 闪烁“关”相位：渲染为普通字符/空白，不附加光标样式
        if (cursor_pos < line_length) {
            return text(cursor_char) | color(foreground_color);
        }
        return text(" ") | color(background_color);
    }

    // 获取流动颜色（如果启用流动效果）
    ftxui::Color effective_color = config_.smooth ? calculateSmoothColor() : config_.color;

    // Nano风格：所有字符使用相同的光标样式，不区分中文和其他字符
    // 根据样式渲染光标
    Element cursor_elem;

    switch (config_.style) {
        case CursorStyle::BLOCK: {
            // 块状光标：背景色填充，支持流动效果
            if (cursor_pos < line_length) {
                cursor_elem =
                    text(cursor_char) | bgcolor(effective_color) | color(background_color) | bold;
            } else {
                cursor_elem = text(" ") | bgcolor(effective_color) | color(background_color) | bold;
            }
            break;
        }
        case CursorStyle::UNDERLINE: {
            // 下划线光标：
            // - 覆盖文字时：保留字符本身，只添加下划线效果，避免看起来像块状背景
            // - 行尾：使用下划线字符占位
            if (cursor_pos < line_length) {
                cursor_elem = text(cursor_char) | color(effective_color) | underlined;
            } else {
                // 行尾：显示下划线字符，使用流动颜色
                cursor_elem = text("▁") | color(effective_color) | bold;
            }
            break;
        }
        case CursorStyle::BAR: {
            // 竖线光标：使用流动颜色，尽量减少对文字的“块状”遮挡
            if (cursor_pos < line_length) {
                // 在字符左侧画一条细竖线，字符本身保持原有前景色
                cursor_elem = hbox({text("│") | color(effective_color) | bold,
                                    text(cursor_char) | color(foreground_color)});
            } else {
                cursor_elem = text("│") | color(effective_color) | bold;
            }
            break;
        }
        case CursorStyle::HOLLOW: {
            // 空心块光标：使用边框效果包裹当前字符，避免实心块遮挡内容
            if (cursor_pos < line_length) {
                cursor_elem = text(cursor_char) | frame | color(effective_color) | bold;
            } else {
                // 行尾：显示带流动颜色的方块字符
                cursor_elem = text("▯") | color(effective_color) | bold;
            }
            break;
        }
        default: {
            // 默认块状，支持流动效果
            if (cursor_pos < line_length) {
                cursor_elem =
                    text(cursor_char) | bgcolor(effective_color) | color(background_color) | bold;
            } else {
                cursor_elem = text(" ") | bgcolor(effective_color) | color(background_color) | bold;
            }
            break;
        }
    }

    return cursor_elem;
}

} // namespace ui
} // namespace pnana
