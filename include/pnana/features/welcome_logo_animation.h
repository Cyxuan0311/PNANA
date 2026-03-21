#ifndef PNANA_FEATURES_WELCOME_LOGO_ANIMATION_H
#define PNANA_FEATURES_WELCOME_LOGO_ANIMATION_H

#include <chrono>
#include <cstddef>
#include <string>

namespace pnana {
namespace core {
struct AnimationConfig;
}
namespace features {

class WelcomeLogoAnimation {
  public:
    struct Frame {
        float pulse_wave = 0.0f; // 脉冲波（-1 到 1）

        // 精细化效果参数
        float glow_intensity = 0.0f; // 发光强度（0-1）
        float chroma_shift = 0.0f;   // 色相偏移（0-1）
        float sharpness = 1.0f;      // 锐度（0.5-2.0）
        float jitter = 0.0f;         // 抖动强度（0-1）

        // 颜色动画参数
        float hue_shift = 0.0f;        // 色相旋转（0-360 度）
        float saturation_boost = 1.0f; // 饱和度增强（0.5-2.0）
        float value_mod = 1.0f;        // 明度调制（0.5-1.5）
        float color_speed = 1.0f;      // 颜色变化速度倍率

        bool useBrightStyle() const {
            return pulse_wave > 0.0f;
        }

        // 获取颜色偏移量（用于渐变动画）
        float getColorShift() const {
            return chroma_shift;
        }

        // 获取发光强度（0-1）
        float getGlowIntensity() const {
            return glow_intensity;
        }

        // 获取锐度（0.5-2.0）
        float getSharpness() const {
            return sharpness;
        }

        // 获取色相偏移（0-360）
        float getHueShift() const {
            return hue_shift;
        }

        // 获取饱和度增强
        float getSaturationBoost() const {
            return saturation_boost;
        }

        // 获取明度调制
        float getValueModulation() const {
            return value_mod;
        }
    };

    WelcomeLogoAnimation();

    void setConfig(const core::AnimationConfig& config);
    Frame currentFrame() const;

  private:
    std::chrono::steady_clock::time_point animation_start_;
    bool enabled_ = true;
    std::string effect_ = "pulse";
    float pulse_speed_ = 4.8f;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_WELCOME_LOGO_ANIMATION_H
