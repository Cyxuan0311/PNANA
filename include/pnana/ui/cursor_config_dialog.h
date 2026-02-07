#ifndef PNANA_UI_CURSOR_CONFIG_DIALOG_H
#define PNANA_UI_CURSOR_CONFIG_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace ui {

// 光标样式枚举
enum class CursorStyle {
    BLOCK,     // 块状光标
    UNDERLINE, // 下划线光标
    BAR,       // 竖线光标
    HOLLOW     // 空心块光标
};

// 光标配置对话框
class CursorConfigDialog {
  public:
    explicit CursorConfigDialog(Theme& theme);

    // 打开对话框
    void open();

    // 关闭对话框
    void close();

    // 是否可见
    bool isVisible() const {
        return visible_;
    }

    // 处理输入
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render();

    // 获取当前配置
    CursorStyle getCursorStyle() const {
        return cursor_style_;
    }
    std::string getCursorColor() const {
        return cursor_color_;
    }
    int getBlinkRate() const {
        return blink_rate_;
    }
    bool getSmoothCursor() const {
        return smooth_cursor_;
    }
    bool getBlinkEnabled() const {
        return blink_enabled_;
    }

    // 设置配置
    void setCursorStyle(CursorStyle style) {
        cursor_style_ = style;
    }
    void setCursorColor(const std::string& color) {
        cursor_color_ = color;
    }
    void setBlinkRate(int rate) {
        blink_rate_ = rate;
    }
    void setSmoothCursor(bool smooth) {
        smooth_cursor_ = smooth;
    }
    void setBlinkEnabled(bool enabled) {
        blink_enabled_ = enabled;
    }

    // 应用配置回调
    void setOnApply(std::function<void()> callback) {
        on_apply_ = callback;
    }

    // 重置为默认值
    void resetToDefaults();

  private:
    Theme& theme_;
    bool visible_;

    CursorStyle cursor_style_;
    std::string cursor_color_;
    int blink_rate_;
    bool smooth_cursor_; // 流动光标效果
    bool blink_enabled_; // 是否启用闪烁

    size_t selected_option_;     // 当前选中的选项索引
    size_t color_input_index_;   // 颜色输入框索引
    size_t rate_input_index_;    // 频率输入框索引
    size_t blink_enabled_index_; // 闪烁开关索引
    size_t smooth_cursor_index_; // 流动光标选项索引

    std::string color_input_; // 颜色输入值（RGB格式，如 "255,255,255"）
    std::string rate_input_;  // 频率输入值

    std::function<void()> on_apply_;

    // 光标样式选项
    std::vector<std::string> style_options_;

    // 渲染样式选择
    ftxui::Element renderStyleSelector();

    // 渲染颜色选择
    ftxui::Element renderColorSelector();

    // 渲染频率选择
    ftxui::Element renderRateSelector();

    // 渲染闪烁开关
    ftxui::Element renderBlinkEnabledSelector();

    // 渲染流动光标选项
    ftxui::Element renderSmoothCursorSelector();

    // 解析颜色字符串
    bool parseColor(const std::string& color_str, int& r, int& g, int& b);

    // 格式化颜色字符串
    std::string formatColor(int r, int g, int b);

    // 移动选择
    void selectNext();
    void selectPrevious();

    // 应用配置
    void apply();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_CURSOR_CONFIG_DIALOG_H
