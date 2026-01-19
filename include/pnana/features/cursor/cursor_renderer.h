#pragma once

#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <string>

// 需要包含 CursorStyle 定义
#include "ui/cursor_config_dialog.h"

namespace pnana {
namespace ui {

// 光标配置结构体
struct CursorConfig {
    CursorStyle style = CursorStyle::BLOCK;
    ftxui::Color color = ftxui::Color::White;
    bool smooth = false; // 是否启用流动效果
};

// 光标渲染器类 - 负责渲染各种样式的光标
class CursorRenderer {
  public:
    CursorRenderer() = default;
    ~CursorRenderer() = default;

    // 设置光标配置
    void setCursorStyle(CursorStyle style);
    void setCursorColor(ftxui::Color color);
    void setSmoothCursor(bool smooth);

    // 设置完整配置
    void setConfig(const CursorConfig& config);

    // 设置流动效果参数
    void setSmoothIntensity(float intensity); // 0.0-1.0，流动强度
    void setBlinkRate(int rate_ms);           // 闪烁间隔（毫秒）

    // 获取当前配置
    CursorStyle getCursorStyle() const;
    ftxui::Color getCursorColor() const;
    bool getSmoothCursor() const;
    const CursorConfig& getConfig() const;

    // 更新光标状态（用于动画效果）
    void updateCursorState();

    // 渲染光标元素
    // @param cursor_char: 光标位置的字符
    // @param cursor_pos: 光标在行中的位置
    // @param line_length: 行的总长度
    // @param foreground_color: 前景色（可选，默认白色）
    // @param background_color: 背景色（可选，默认黑色）
    // @return: 渲染后的光标元素
    ftxui::Element renderCursorElement(const std::string& cursor_char, size_t cursor_pos,
                                       size_t line_length,
                                       ftxui::Color foreground_color = ftxui::Color::White,
                                       ftxui::Color background_color = ftxui::Color::Black) const;

  private:
    CursorConfig config_;

    // 流动效果相关状态
    float smooth_intensity_ = 0.3f; // 流动强度 (0.0-1.0)
    int blink_rate_ms_ = 800;       // 闪烁间隔，默认800ms

    // 动画状态（轻量级，不会影响性能）
    std::chrono::steady_clock::time_point last_update_time_;
    float animation_phase_ = 0.0f; // 0-2π的动画相位

    // 辅助方法
    ftxui::Color calculateSmoothColor() const;
    float getCurrentAnimationPhase() const;
};

} // namespace ui
} // namespace pnana
