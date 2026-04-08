#ifndef PNANA_UI_TOAST_H
#define PNANA_UI_TOAST_H

#include "ui/icons.h"
#include "ui/theme.h"
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace ui {

// Toast 类型枚举
enum class ToastType {
    SUCCESS, // 成功
    INFO,    // 信息
    WARNING, // 警告
    ERROR    // 错误
};

// Toast 样式枚举
// AUTO: 使用全局默认样式
// CLASSIC: 经典边框样式
// MINIMAL: 极简样式（无边框）
// SOLID: 实色背景样式
// ACCENT: 左侧强调条样式
// OUTLINE: 线框样式（强调边框）
enum class ToastStyle {
    AUTO,
    CLASSIC,
    MINIMAL,
    SOLID,
    ACCENT,
    OUTLINE,
};

// Toast 配置结构
struct ToastConfig {
    std::string message;              // 消息内容
    ToastType type = ToastType::INFO; // Toast 类型
    int duration_ms = -1; // 显示持续时间（毫秒），0 表示不自动消失，-1 使用全局默认
    bool show_icon = true;               // 是否显示图标
    ToastStyle style = ToastStyle::AUTO; // 样式（AUTO 使用全局默认）
    int max_width = 0;                   // 最大宽度（0 使用全局默认）
    bool bold_text = false;              // 是否粗体文字（false 时由样式决定）

    // 便捷构造方法
    static ToastConfig success(const std::string& msg, int duration = 3000) {
        ToastConfig c;
        c.message = msg;
        c.type = ToastType::SUCCESS;
        c.duration_ms = duration;
        c.show_icon = true;
        return c;
    }

    static ToastConfig info(const std::string& msg, int duration = 3000) {
        ToastConfig c;
        c.message = msg;
        c.type = ToastType::INFO;
        c.duration_ms = duration;
        c.show_icon = true;
        return c;
    }

    static ToastConfig warning(const std::string& msg, int duration = 3000) {
        ToastConfig c;
        c.message = msg;
        c.type = ToastType::WARNING;
        c.duration_ms = duration;
        c.show_icon = true;
        return c;
    }

    static ToastConfig error(const std::string& msg, int duration = 0) {
        ToastConfig c;
        c.message = msg;
        c.type = ToastType::ERROR;
        c.duration_ms = duration;
        c.show_icon = true;
        return c;
    }
};

// Toast 组件类
class Toast {
  public:
    Toast();
    ~Toast();

    // 全局配置
    static void setEnabled(bool enabled) {
        s_enabled_ = enabled;
    }
    static bool isEnabled() {
        return s_enabled_;
    }
    static void setDefaultStyle(ToastStyle style) {
        s_default_style_ = style;
    }
    static ToastStyle getDefaultStyle() {
        return s_default_style_;
    }
    static void setDefaultDurationMs(int duration_ms) {
        s_default_duration_ms_ = duration_ms;
    }
    static int getDefaultDurationMs() {
        return s_default_duration_ms_;
    }
    static void setDefaultMaxWidth(int max_width) {
        s_default_max_width_ = max_width;
    }
    static int getDefaultMaxWidth() {
        return s_default_max_width_;
    }
    static void setDefaultShowIcon(bool show_icon) {
        s_default_show_icon_ = show_icon;
    }
    static bool getDefaultShowIcon() {
        return s_default_show_icon_;
    }
    static void setDefaultBoldText(bool bold_text) {
        s_default_bold_text_ = bold_text;
    }
    static bool getDefaultBoldText() {
        return s_default_bold_text_;
    }

    // 显示 Toast（一行代码调用）
    void show(const ToastConfig& config);

    // 便捷显示方法（一行代码调用）
    void showSuccess(const std::string& message, int duration_ms = 3000);
    void showInfo(const std::string& message, int duration_ms = 3000);
    void showWarning(const std::string& message, int duration_ms = 3000);
    void showError(const std::string& message, int duration_ms = 0);

    // 隐藏 Toast
    void hide();

    // 检查是否可见
    bool isVisible() const {
        return visible_;
    }

    // 检查是否正在淡出
    bool isFadingOut() const {
        return fading_out_;
    }

    // 获取渲染元素（使用主题色）
    ftxui::Element render(const Theme& theme) const;

    // 更新状态（检查是否应该自动隐藏）
    void update();

  private:
    // 获取图标
    const char* getIcon(ToastType type) const;

    // 获取颜色
    ftxui::Color getIconColor(ToastType type, const ThemeColors& colors) const;
    ftxui::Color getBorderColor(ToastType type, const ThemeColors& colors) const;
    ftxui::Color getTypeColor(ToastType type, const ThemeColors& colors) const;

    ToastStyle resolveStyle() const;
    int resolveDurationMs() const;
    int resolveMaxWidth() const;

    static constexpr int FADE_OUT_DURATION_MS = 300; // 淡出动画时长
    static bool s_enabled_;                          // 全局启用标志
    static ToastStyle s_default_style_;              // 全局默认样式
    static int s_default_duration_ms_;               // 全局默认时长
    static int s_default_max_width_;                 // 全局默认最大宽度
    static bool s_default_show_icon_;                // 全局默认是否显示图标
    static bool s_default_bold_text_;                // 全局默认是否粗体

    bool visible_ = false;
    bool fading_out_ = false; // 是否正在淡出
    ToastConfig current_config_;
    std::chrono::steady_clock::time_point show_time_;
    std::chrono::steady_clock::time_point fade_start_time_; // 淡出开始时间
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_TOAST_H
