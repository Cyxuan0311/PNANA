#include "ui/toast.h"
#include <cmath>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

// 静态成员初始化（默认禁用 Toast）
bool Toast::s_enabled_ = false;

Toast::Toast() = default;
Toast::~Toast() = default;

void Toast::show(const ToastConfig& config) {
    // 如果全局禁用，直接返回
    if (!s_enabled_) {
        return;
    }

    current_config_ = config;
    visible_ = true;
    fading_out_ = false;
    show_time_ = std::chrono::steady_clock::now();
}

void Toast::showSuccess(const std::string& message, int duration_ms) {
    show(ToastConfig::success(message, duration_ms));
}

void Toast::showInfo(const std::string& message, int duration_ms) {
    show(ToastConfig::info(message, duration_ms));
}

void Toast::showWarning(const std::string& message, int duration_ms) {
    show(ToastConfig::warning(message, duration_ms));
}

void Toast::showError(const std::string& message, int duration_ms) {
    show(ToastConfig::error(message, duration_ms));
}

void Toast::hide() {
    visible_ = false;
    fading_out_ = false;
}

void Toast::update() {
    if (!visible_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - show_time_);

    // 如果 duration_ms 为 0，不自动消失
    if (current_config_.duration_ms == 0) {
        return;
    }

    // 开始淡出（在持续时间到达时）
    if (!fading_out_ && elapsed.count() >= current_config_.duration_ms) {
        fading_out_ = true;
        fade_start_time_ = now;
    }

    // 完全隐藏（淡出动画完成后）
    if (fading_out_) {
        auto fade_elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - fade_start_time_);
        if (fade_elapsed.count() >= FADE_OUT_DURATION_MS) {
            hide();
        }
    }
}

const char* Toast::getIcon(ToastType type) const {
    switch (type) {
        case ToastType::SUCCESS:
            return icons::CHECK_CIRCLE;
        case ToastType::INFO:
            return icons::INFO_CIRCLE;
        case ToastType::WARNING:
            return icons::WARNING;
        case ToastType::ERROR:
            return icons::ERROR;
        default:
            return icons::INFO_CIRCLE;
    }
}

Color Toast::getIconColor(ToastType type, const ThemeColors& colors) const {
    switch (type) {
        case ToastType::SUCCESS:
            return colors.success;
        case ToastType::INFO:
            return colors.info;
        case ToastType::WARNING:
            return colors.warning;
        case ToastType::ERROR:
            return colors.error;
        default:
            return colors.info;
    }
}

Color Toast::getBorderColor(ToastType type, const ThemeColors& colors) const {
    switch (type) {
        case ToastType::SUCCESS:
            return colors.success;
        case ToastType::INFO:
            return colors.info;
        case ToastType::WARNING:
            return colors.warning;
        case ToastType::ERROR:
            return colors.error;
        default:
            return colors.dialog_border;
    }
}

Element Toast::render(const Theme& theme) const {
    if (!visible_) {
        return text("");
    }

    const auto& colors = theme.getColors();

    // 获取图标和颜色
    const char* icon = getIcon(current_config_.type);
    Color icon_color = getIconColor(current_config_.type, colors);
    Color border_color = getBorderColor(current_config_.type, colors);
    Color text_color = colors.foreground; // 文字使用前景色

    // 构建内容
    Elements content;

    if (current_config_.show_icon) {
        content.push_back(text(icon) | color(icon_color));
        content.push_back(text(" "));
    }

    // 消息文本，允许自动换行
    auto message_text = text(current_config_.message) | color(text_color) | flex;
    content.push_back(message_text);

    // 创建 Toast 元素（透明背景），设置最大宽度并允许换行
    auto toast_content = hbox(content) | bold | flex;

    // 使用 paragraph 实现自动换行，限制最大宽度
    auto toast_element = vbox({toast_content}) | border | color(border_color) |
                         size(WIDTH, LESS_THAN, 50); // 限制最大宽度为 50 字符

    // 应用透明度（如果正在淡出）
    if (fading_out_) {
        toast_element = toast_element | dim;
    }

    return toast_element;
}

} // namespace ui
} // namespace pnana
