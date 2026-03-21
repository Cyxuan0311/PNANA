#ifndef PNANA_UI_ANIMATION_MENU_H
#define PNANA_UI_ANIMATION_MENU_H

#include "core/config_manager.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

class AnimationMenu {
  public:
    explicit AnimationMenu(Theme& theme);

    void setConfig(const core::AnimationConfig& config);
    const core::AnimationConfig& getPendingConfig() const {
        return pending_config_;
    }

    bool handleInput(ftxui::Event event);
    ftxui::Element render();

  private:
    struct EffectItem {
        std::string id;
        std::string name;
        std::string desc;
    };

    Theme& theme_;
    core::AnimationConfig pending_config_;
    std::vector<EffectItem> effects_;
    size_t selected_effect_index_ = 0;
    size_t selected_param_index_ = 0;
    bool focus_effects_ = true;
    size_t effect_scroll_offset_ = 0;                // 滚动偏移量
    static constexpr size_t MAX_VISIBLE_EFFECTS = 8; // 最大可见项数

    void clampConfig();
    void syncSelectedEffect();
    void cycleEffect(int delta);
    void adjustCurrentParam(int delta);
    void updateEffectScroll(); // 更新滚动偏移

    ftxui::Element renderEffects() const;
    ftxui::Element renderParams() const;
    ftxui::Element renderPreview() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_ANIMATION_MENU_H
