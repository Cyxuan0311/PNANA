#include "features/welcome_logo_animation.h"
#include "core/config_manager.h"
#include <algorithm>
#include <cmath>

namespace pnana {
namespace features {

WelcomeLogoAnimation::WelcomeLogoAnimation() : animation_start_(std::chrono::steady_clock::now()) {}

void WelcomeLogoAnimation::setConfig(const core::AnimationConfig& config) {
    enabled_ = config.enabled;
    effect_ = config.effect;
    pulse_speed_ = std::clamp(config.pulse_speed, 0.2f, 24.0f);
}

WelcomeLogoAnimation::Frame WelcomeLogoAnimation::currentFrame() const {
    Frame frame;
    if (!enabled_) {
        frame.pulse_wave = 1.0f;
        // None 模式：完全静态，无动画
        frame.glow_intensity = 0.5f;
        frame.chroma_shift = 0.0f;
        frame.sharpness = 1.0f;
        frame.jitter = 0.0f;
        frame.hue_shift = 0.0f;
        frame.saturation_boost = 1.0f;
        frame.value_mod = 1.0f;
        frame.color_speed = 0.0f;
        return frame;
    }

    const auto now = std::chrono::steady_clock::now();
    const float elapsed_s =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - animation_start_).count() /
        1000.0f;

    if (effect_ == "none") {
        frame.pulse_wave = 1.0f;
        // None 模式：完全静态，无动画
        frame.glow_intensity = 0.5f;
        frame.chroma_shift = 0.0f;
        frame.sharpness = 1.0f;
        frame.jitter = 0.0f;
        frame.hue_shift = 0.0f;
        frame.saturation_boost = 1.0f;
        frame.value_mod = 1.0f;
        frame.color_speed = 0.0f;
        return frame;
    } else if (effect_ == "scanner") {
        // 扫描器效果：极快的色彩扫描，类似激光扫描
        const float scan_cycle = std::fmod(elapsed_s * 2.0f, 1.0f);
        frame.pulse_wave = std::sin(scan_cycle * 3.14159f * 2.0f);
        // Scanner 特有参数：极快的色彩旋转，高对比度
        frame.glow_intensity = scan_cycle > 0.9f ? 1.0f : 0.3f;
        frame.chroma_shift = scan_cycle;
        frame.sharpness = 2.0f;
        frame.jitter = scan_cycle > 0.85f ? 0.5f : 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 360.0f, 360.0f);
        frame.saturation_boost = scan_cycle > 0.8f ? 2.0f : 0.5f;
        frame.value_mod = scan_cycle > 0.9f ? 1.5f : 0.5f;
        frame.color_speed = 15.0f;
    } else if (effect_ == "shimmer") {
        // 微光效果：非常柔和的星光闪烁，细微的色彩变化
        const float shimmer1 = std::sin(elapsed_s * 0.8f);
        const float shimmer2 = std::sin(elapsed_s * 1.3f + 1.0f);
        const float shimmer_mix = (shimmer1 + shimmer2) * 0.5f;
        frame.pulse_wave = shimmer_mix * 0.5f;
        // Shimmer 特有参数：低饱和度，柔和变化
        frame.glow_intensity = 0.5f + shimmer_mix * 0.2f;
        frame.chroma_shift = std::fmod(elapsed_s * 0.05f, 1.0f);
        frame.sharpness = 0.9f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 8.0f, 360.0f);
        frame.saturation_boost = 0.8f + shimmer_mix * 0.1f;
        frame.value_mod = 0.9f + shimmer_mix * 0.15f;
        frame.color_speed = 0.5f;
    } else if (effect_ == "wave") {
        // 波浪效果：平滑的正弦波色彩流动，类似海浪
        const float wave1 = std::sin(elapsed_s * 0.6f);
        const float wave2 = std::sin(elapsed_s * 0.9f + 1.5f);
        const float wave_mix = (wave1 + wave2 * 0.7f) * 0.5f;
        frame.pulse_wave = wave_mix * 0.8f;
        // Wave 特有参数：中等速度，平滑过渡
        frame.glow_intensity = 0.6f + wave_mix * 0.4f;
        frame.chroma_shift = std::fmod((elapsed_s * 0.15f + wave_mix * 0.3f), 1.0f);
        frame.sharpness = 1.1f + wave_mix * 0.2f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 20.0f + wave_mix * 30.0f, 360.0f);
        frame.saturation_boost = 1.0f + wave_mix * 0.4f;
        frame.value_mod = 0.9f + wave_mix * 0.2f;
        frame.color_speed = 1.5f;
    } else if (effect_ == "strobe") {
        // 频闪效果：极端的开/关闪烁，类似迪厅频闪灯
        const float strobe_freq = pulse_speed_ * 1.5f;
        const float strobe_state = std::sin(elapsed_s * strobe_freq * 3.14159f);
        const float strobe_binary = strobe_state > 0.95f ? 1.0f : 0.0f;
        frame.pulse_wave = strobe_binary;
        // Strobe 特有参数：极端对比，瞬间切换
        frame.glow_intensity = strobe_binary;
        frame.chroma_shift = strobe_binary;
        frame.sharpness = strobe_binary > 0.5f ? 2.0f : 0.5f;
        frame.jitter = strobe_binary > 0.5f ? 0.4f : 0.0f;
        frame.hue_shift = strobe_binary > 0.5f ? std::fmod(elapsed_s * 720.0f, 360.0f) : 0.0f;
        frame.saturation_boost = strobe_binary > 0.5f ? 2.0f : 0.3f;
        frame.value_mod = strobe_binary > 0.5f ? 1.5f : 0.3f;
        frame.color_speed = 20.0f;
    } else if (effect_ == "horizontal") {
        // 横向效果：所有行完全同步的缓慢呼吸灯
        const float horiz_breath = std::sin(elapsed_s * 0.5f);
        frame.pulse_wave = horiz_breath * 0.9f;
        // Horizontal 特有参数：完全同步，缓慢变化
        frame.glow_intensity = 0.5f + horiz_breath * 0.5f;
        frame.chroma_shift = std::fmod(elapsed_s * 0.08f, 1.0f);
        frame.sharpness = 1.0f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 12.0f, 360.0f);
        frame.saturation_boost = 0.9f + horiz_breath * 0.2f;
        frame.value_mod = 0.85f + horiz_breath * 0.25f;
        frame.color_speed = 0.8f;
    } else if (effect_ == "rain") {
        // 雨点效果：高频不规则抖动，类似雨滴落下
        const float rain_drop1 = std::sin(elapsed_s * 12.0f);
        const float rain_drop2 = std::sin(elapsed_s * 17.0f + 0.5f);
        const float rain_drop3 = std::sin(elapsed_s * 23.0f + 1.2f);
        const float rain_mix = (rain_drop1 + rain_drop2 * 0.7f + rain_drop3 * 0.5f) / 2.2f;
        frame.pulse_wave = std::clamp(rain_mix * 0.8f, -1.0f, 1.0f);
        // Rain 特有参数：高频抖动，冷色调
        frame.glow_intensity = std::clamp(0.3f + std::abs(rain_mix) * 0.7f, 0.2f, 1.0f);
        frame.chroma_shift = std::fmod(elapsed_s * 0.8f, 1.0f);
        frame.sharpness = 1.6f;
        frame.jitter = std::clamp(std::abs(rain_mix) * 0.7f, 0.2f, 0.8f);
        frame.hue_shift = std::fmod(elapsed_s * 120.0f, 360.0f);
        frame.saturation_boost = 1.4f + std::abs(rain_mix) * 0.6f;
        frame.value_mod = 0.7f + std::abs(rain_mix) * 0.5f;
        frame.color_speed = 8.0f;
    } else if (effect_ == "ripple") {
        // 波纹效果：类似水波纹的扩散效果，平滑衰减
        const float ripple_cycle = std::fmod(elapsed_s * 0.7f, 2.0f);
        const float ripple_exp = ripple_cycle < 1.0f ? ripple_cycle : (2.0f - ripple_cycle);
        const float ripple_wave = std::sin(ripple_exp * 3.14159f);
        frame.pulse_wave = ripple_wave * 0.7f;
        // Ripple 特有参数：周期性扩散，柔和衰减
        frame.glow_intensity = 0.4f + ripple_exp * 0.6f;
        frame.chroma_shift = std::fmod(ripple_exp * 0.5f, 1.0f);
        frame.sharpness = 0.9f + ripple_exp * 0.3f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(ripple_exp * 90.0f, 360.0f);
        frame.saturation_boost = 0.85f + ripple_exp * 0.35f;
        frame.value_mod = 0.8f + ripple_exp * 0.3f;
        frame.color_speed = 1.2f;
    } else if (effect_ == "spiral") {
        // 螺旋效果：旋转的色彩螺旋，类似漩涡
        const float spiral_angle = elapsed_s * 1.5f;
        const float spiral_radius = std::sin(elapsed_s * 0.8f) * 0.5f + 0.5f;
        frame.pulse_wave = std::sin(spiral_angle) * spiral_radius;
        // Spiral 特有参数：旋转色相，中等速度
        frame.glow_intensity = 0.5f + spiral_radius * 0.5f;
        frame.chroma_shift = std::fmod(spiral_angle / (3.14159f * 2.0f), 1.0f);
        frame.sharpness = 1.3f + spiral_radius * 0.4f;
        frame.jitter = spiral_radius > 0.8f ? 0.15f : 0.0f;
        frame.hue_shift = std::fmod(spiral_angle * 120.0f, 360.0f);
        frame.saturation_boost = 1.1f + spiral_radius * 0.5f;
        frame.value_mod = 0.85f + spiral_radius * 0.35f;
        frame.color_speed = 3.0f;
    } else if (effect_ == "bounce") {
        // 弹跳效果：像球一样弹跳，快速上升缓慢下落
        const float bounce_period = 2.5f;
        const float bounce_phase = std::fmod(elapsed_s, bounce_period) / bounce_period;
        const float bounce_wave = std::abs(std::sin(bounce_phase * 3.14159f * 2.0f));
        frame.pulse_wave = bounce_wave * 0.9f;
        // Bounce 特有参数：弹跳时强烈，落地时柔和
        frame.glow_intensity = bounce_wave > 0.8f ? 1.0f : 0.4f + bounce_wave * 0.4f;
        frame.sharpness = bounce_wave > 0.8f ? 1.8f : 1.0f;
        frame.jitter = bounce_wave > 0.9f ? 0.4f : 0.0f;
        frame.hue_shift = bounce_wave > 0.8f ? std::fmod(elapsed_s * 180.0f, 360.0f)
                                             : std::fmod(elapsed_s * 30.0f, 360.0f);
        frame.saturation_boost = bounce_wave > 0.8f ? 1.8f : 0.9f;
        frame.value_mod = bounce_wave > 0.8f ? 1.4f : 0.75f;
        frame.color_speed = bounce_wave > 0.8f ? 10.0f : 2.0f;
    } else if (effect_ == "fade") {
        // 淡入淡出效果：非常缓慢的淡入淡出，类似渐变
        const float fade_slow = std::sin(elapsed_s * 0.3f);
        frame.pulse_wave = fade_slow * 0.6f;
        // Fade 特有参数：极慢变化，低对比度
        frame.glow_intensity = 0.5f + fade_slow * 0.3f;
        frame.chroma_shift = std::fmod(elapsed_s * 0.03f, 1.0f);
        frame.sharpness = 0.95f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 5.0f, 360.0f);
        frame.saturation_boost = 0.85f + fade_slow * 0.15f;
        frame.value_mod = 0.8f + fade_slow * 0.3f;
        frame.color_speed = 0.3f;
    } else if (effect_ == "glitch") {
        // 故障效果：随机跳变，类似信号故障
        const float glitch_freq = 3.0f;
        const float glitch_trigger =
            std::floor(elapsed_s * glitch_freq) - std::floor((elapsed_s - 0.05f) * glitch_freq);
        const float glitch_noise = std::sin(elapsed_s * 37.0f) * 0.5f + 0.5f;
        const float glitch_intensity = glitch_trigger > 0.5f ? glitch_noise : 0.0f;
        frame.pulse_wave = glitch_intensity > 0.5f ? 1.0f : -1.0f;
        // Glitch 特有参数：瞬间跳变，高对比度
        frame.glow_intensity = glitch_intensity > 0.5f ? 1.0f : 0.2f;
        frame.chroma_shift = glitch_intensity > 0.5f ? std::fmod(elapsed_s * 2.0f, 1.0f)
                                                     : std::fmod(elapsed_s * 0.1f, 1.0f);
        frame.sharpness = glitch_intensity > 0.5f ? 2.0f : 0.8f;
        frame.jitter = glitch_intensity > 0.5f ? 0.8f : 0.0f;
        frame.hue_shift = glitch_intensity > 0.5f ? std::fmod(elapsed_s * 540.0f, 360.0f)
                                                  : std::fmod(elapsed_s * 20.0f, 360.0f);
        frame.saturation_boost = glitch_intensity > 0.5f ? 2.0f : 0.5f;
        frame.value_mod = glitch_intensity > 0.5f ? 1.5f : 0.4f;
        frame.color_speed = glitch_intensity > 0.5f ? 15.0f : 1.0f;
    } else if (effect_ == "neon") {
        // 霓虹灯效果：快速的霓虹灯闪烁，类似霓虹招牌
        const float neon_cycle = std::fmod(elapsed_s * 0.8f, 1.0f);
        const float neon_flicker = std::sin(elapsed_s * 23.0f) * 0.5f + 0.5f;
        const float neon_on = neon_cycle < 0.3f ? 1.0f : (neon_flicker > 0.7f ? 0.3f : 0.1f);
        frame.pulse_wave = neon_on > 0.5f ? 1.0f : -0.5f;
        // Neon 特有参数：暖色调，高频闪烁
        frame.glow_intensity = neon_on;
        frame.chroma_shift = std::fmod(elapsed_s * 0.4f, 1.0f);
        frame.sharpness = neon_on > 0.8f ? 1.9f : 0.7f;
        frame.jitter = neon_on > 0.9f ? 0.2f : 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 45.0f + neon_on * 90.0f, 360.0f);
        frame.saturation_boost = neon_on > 0.8f ? 1.9f : 0.6f;
        frame.value_mod = neon_on > 0.8f ? 1.5f : 0.5f;
        frame.color_speed = 6.0f;
    } else if (effect_ == "comet") {
        // 彗星效果：周期性快速划过，然后缓慢恢复
        const float comet_period = 4.0f;
        const float comet_phase = std::fmod(elapsed_s, comet_period) / comet_period;
        const float comet_streak = comet_phase < 0.15f ? std::sin(comet_phase / 0.15f * 1.5708f)
                                                       : (1.0f - (comet_phase - 0.15f) / 0.85f);
        frame.pulse_wave = comet_streak * 0.9f;
        // Comet 特有参数：快速划过，长尾迹
        frame.glow_intensity = comet_streak > 0.8f ? 1.0f : 0.3f + comet_streak * 0.4f;
        frame.chroma_shift = std::fmod(comet_phase * 0.6f, 1.0f);
        frame.sharpness = comet_streak > 0.8f ? 1.9f : 0.9f;
        frame.jitter = comet_streak > 0.9f ? 0.35f : 0.0f;
        frame.hue_shift = std::fmod(comet_phase * 300.0f + elapsed_s * 30.0f, 360.0f);
        frame.saturation_boost = comet_streak > 0.8f ? 1.9f : 0.8f;
        frame.value_mod = comet_streak > 0.8f ? 1.5f : 0.6f;
        frame.color_speed = comet_streak > 0.8f ? 12.0f : 1.5f;
    } else if (effect_ == "breath") {
        // 呼吸灯效果：非常缓慢的呼吸节奏，类似睡眠呼吸
        const float breath_rate = 0.25f;
        const float breath_wave = std::sin(elapsed_s * breath_rate);
        const float breath_smooth = std::pow(breath_wave, 3) * 0.5f;
        frame.pulse_wave = breath_smooth * 0.7f;
        // Breath 特有参数：极慢节奏，柔和变化
        frame.glow_intensity = 0.5f + breath_smooth * 0.3f;
        frame.chroma_shift = std::fmod(elapsed_s * 0.02f, 1.0f);
        frame.sharpness = 0.95f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 3.0f, 360.0f);
        frame.saturation_boost = 0.9f + breath_smooth * 0.15f;
        frame.value_mod = 0.85f + breath_smooth * 0.2f;
        frame.color_speed = 0.2f;
    } else if (effect_ == "disco") {
        // 迪斯科效果：强烈的节奏感，类似迪厅舞池
        const float disco_kick = std::fmod(elapsed_s * 2.0f, 1.0f);
        const float disco_hit = disco_kick < 0.15f ? std::sin(disco_kick / 0.15f * 1.5708f) : 0.0f;
        const float disco_color = std::floor(elapsed_s * 1.5f);
        frame.pulse_wave = disco_hit;
        // Disco 特有参数：强节奏，多彩色，高对比
        frame.glow_intensity = disco_hit > 0.8f ? 1.0f : 0.3f;
        frame.chroma_shift = std::fmod(disco_color * 0.125f, 1.0f);
        frame.sharpness = disco_hit > 0.8f ? 2.0f : 0.7f;
        frame.jitter = disco_hit > 0.9f ? 0.3f : 0.0f;
        frame.hue_shift = std::fmod(disco_color * 45.0f, 360.0f);
        frame.saturation_boost = disco_hit > 0.8f ? 2.0f : 0.5f;
        frame.value_mod = disco_hit > 0.8f ? 1.5f : 0.4f;
        frame.color_speed = 18.0f;
    } else if (effect_ == "matrix") {
        // 矩阵效果：绿色数字雨，类似黑客帝国
        const float matrix_drop = std::fmod(elapsed_s * 0.7f, 1.0f);
        const float matrix_char = std::floor(elapsed_s * 15.0f);
        const float matrix_stream = std::sin(matrix_drop * 3.14159f);
        frame.pulse_wave = matrix_stream * 0.8f;
        // Matrix 特有参数：绿色调，向下流动感
        frame.glow_intensity = 0.6f + matrix_stream * 0.4f;
        frame.chroma_shift = std::fmod(matrix_drop * 0.3f, 1.0f);
        frame.sharpness = 1.5f;
        frame.jitter = std::fmod(matrix_char, 10.0f) / 20.0f;
        frame.hue_shift = 90.0f + std::fmod(matrix_drop * 30.0f, 60.0f); // 绿色范围
        frame.saturation_boost = 1.3f + matrix_stream * 0.4f;
        frame.value_mod = 0.8f + matrix_stream * 0.4f;
        frame.color_speed = 2.5f;
    } else if (effect_ == "pulse") {
        // 脉冲效果：强烈的心跳节奏，收缩 - 舒张
        const float pulse_rate = 1.2f;
        const float pulse_phase = std::fmod(elapsed_s * pulse_rate, 1.0f);
        const float pulse_systole =
            pulse_phase < 0.15f ? std::sin(pulse_phase / 0.15f * 1.5708f) : 0.0f;
        const float pulse_diastole = (pulse_phase >= 0.5f && pulse_phase < 0.65f)
                                         ? std::sin((pulse_phase - 0.5f) / 0.15f * 1.5708f) * 0.6f
                                         : 0.0f;
        const float pulse_beat = pulse_systole + pulse_diastole;
        frame.pulse_wave = pulse_beat;
        // Pulse 特有参数：双峰脉冲，强节奏
        frame.glow_intensity = pulse_beat > 0.7f ? 1.0f : 0.4f + pulse_beat * 0.4f;
        frame.sharpness = pulse_beat > 0.7f ? 1.9f : 1.0f;
        frame.jitter = pulse_beat > 0.9f ? 0.25f : 0.0f;
        frame.hue_shift = pulse_beat > 0.7f ? std::fmod(elapsed_s * 120.0f, 360.0f)
                                            : std::fmod(elapsed_s * 15.0f, 360.0f);
        frame.saturation_boost = pulse_beat > 0.7f ? 1.8f : 0.9f;
        frame.value_mod = pulse_beat > 0.7f ? 1.4f : 0.7f;
        frame.color_speed = pulse_beat > 0.7f ? 8.0f : 1.5f;
    } else if (effect_ == "aurora") {
        // 极光效果：缓慢飘逸的彩色光带，类似北极光
        const float aurora_slow1 = std::sin(elapsed_s * 0.15f);
        const float aurora_slow2 = std::sin(elapsed_s * 0.22f + 2.0f);
        const float aurora_slow3 = std::sin(elapsed_s * 0.33f + 4.0f);
        const float aurora_mix = (aurora_slow1 + aurora_slow2 * 0.7f + aurora_slow3 * 0.5f) / 2.2f;
        frame.pulse_wave = aurora_mix * 0.6f;
        // Aurora 特有参数：极慢速，柔和色彩过渡
        frame.glow_intensity = 0.5f + aurora_mix * 0.4f;
        frame.chroma_shift = std::fmod(elapsed_s * 0.04f, 1.0f);
        frame.sharpness = 0.85f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(elapsed_s * 6.0f + aurora_mix * 45.0f, 360.0f);
        frame.saturation_boost = 0.85f + aurora_mix * 0.25f;
        frame.value_mod = 0.85f + aurora_mix * 0.25f;
        frame.color_speed = 0.4f;
    } else if (effect_ == "flame") {
        // 火焰效果：跳动的火焰，橙红色调，热浪扭曲
        const float flame_flicker1 = std::sin(elapsed_s * 8.0f);
        const float flame_flicker2 = std::sin(elapsed_s * 13.0f + 1.5f);
        const float flame_flicker3 = std::sin(elapsed_s * 19.0f + 2.3f);
        const float flame_mix =
            (flame_flicker1 + flame_flicker2 * 0.7f + flame_flicker3 * 0.5f) / 2.2f;
        const float flame_intensity = std::clamp((flame_mix + 1.0f) * 0.5f, 0.0f, 1.0f);
        frame.pulse_wave = flame_mix * 0.9f;
        // Flame 特有参数：暖色调，高频抖动
        frame.glow_intensity = 0.5f + flame_intensity * 0.5f;
        frame.chroma_shift = std::fmod(elapsed_s * 0.3f + flame_intensity * 0.3f, 1.0f);
        frame.sharpness = 1.4f + flame_intensity * 0.6f;
        frame.jitter = std::clamp(flame_intensity * 0.5f, 0.1f, 0.6f);
        frame.hue_shift = 20.0f + std::fmod(flame_intensity * 40.0f, 60.0f); // 橙红范围
        frame.saturation_boost = 1.4f + flame_intensity * 0.6f;
        frame.value_mod = 0.9f + flame_intensity * 0.4f;
        frame.color_speed = 5.0f;
    } else if (effect_ == "rainbow") {
        // 彩虹效果：快速旋转变换的七彩色彩，类似 RGB 循环
        const float rainbow_fast = std::fmod(elapsed_s * 0.8f, 1.0f);
        const float rainbow_wave = std::sin(elapsed_s * 2.5f) * 0.5f + 0.5f;
        frame.pulse_wave = rainbow_wave * 0.8f;
        // Rainbow 特有参数：全色相快速旋转，高饱和度
        frame.glow_intensity = 0.7f + rainbow_wave * 0.3f;
        frame.chroma_shift = rainbow_fast;
        frame.sharpness = 1.3f;
        frame.jitter = 0.0f;
        frame.hue_shift = std::fmod(rainbow_fast * 360.0f, 360.0f);
        frame.saturation_boost = 1.6f + rainbow_wave * 0.4f;
        frame.value_mod = 1.1f + rainbow_wave * 0.3f;
        frame.color_speed = 20.0f;
    } else {
        frame.pulse_wave = std::sin(elapsed_s * pulse_speed_);
    }

    // 为所有效果添加基础精细化参数（如果未设置）
    if (frame.glow_intensity == 0.0f) {
        frame.glow_intensity = std::clamp(std::abs(frame.pulse_wave), 0.0f, 1.0f);
    }
    if (frame.chroma_shift == 0.0f) {
        frame.chroma_shift = std::fmod(elapsed_s * 0.1f, 1.0f);
    }
    if (frame.sharpness == 1.0f) {
        frame.sharpness = 1.0f + frame.pulse_wave * 0.2f;
    }

    return frame;
}

} // namespace features
} // namespace pnana
