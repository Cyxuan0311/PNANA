#include "ui/animation_menu.h"
#include "ui/icons.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

AnimationMenu::AnimationMenu(Theme& theme) : theme_(theme) {
    effects_ = {
        {"none", "None", "No animation (static logo)"},
        {"scanner", "Scanner", "Fast narrow scanning highlight"},
        {"shimmer", "Shimmer", "Soft flicker with wide sweep"},
        {"wave", "Wave", "Sinusoidal row shimmer effect"},
        {"strobe", "Strobe", "Sharp pulse with aggressive sweep"},
        {"horizontal", "Horizontal", "Flat horizontal band sweep"},
        {"rain", "Rain", "Raindrop-like jitter and streak"},
        {"ripple", "Ripple", "Breathing ripple-like oscillation"},
        {"spiral", "Spiral", "Spiral-moving sweep with rotating offset"},
        {"bounce", "Bounce", "Bouncing ball motion from bottom to top"},
        {"fade", "Fade", "Gentle fade in/out with subtle sweep"},
        {"glitch", "Glitch", "Random jitter and glitch effect"},
        {"neon", "Neon", "Fast blinking neon light effect"},
        {"comet", "Comet", "Comet-like sweep with trailing tail"},
        {"breath", "Breath", "Slow breathing rhythm like human breath"},
        {"disco", "Disco", "Upbeat strobe with fast scanning"},
        {"matrix", "Matrix", "Digital rain flowing downward"},
        {"pulse", "Pulse", "Strong rhythmic heartbeat-like pulse"},
        {"aurora", "Aurora", "Slow flowing aurora borealis effect"},
        {"flame", "Flame", "Flickering flame with heat distortion"},
        {"rainbow", "Rainbow", "Vibrant rainbow color cycling effect"},
    };
}

void AnimationMenu::setConfig(const core::AnimationConfig& config) {
    pending_config_ = config;
    clampConfig();
    syncSelectedEffect();
    selected_param_index_ = 0;
    focus_effects_ = true;
}

void AnimationMenu::clampConfig() {
    if (pending_config_.refresh_interval_ms < 16)
        pending_config_.refresh_interval_ms = 16;
    if (pending_config_.refresh_interval_ms > 300)
        pending_config_.refresh_interval_ms = 300;

    pending_config_.pulse_speed = std::clamp(pending_config_.pulse_speed, 0.2f, 24.0f);
}

void AnimationMenu::syncSelectedEffect() {
    selected_effect_index_ = 0;
    for (size_t i = 0; i < effects_.size(); ++i) {
        if (effects_[i].id == pending_config_.effect) {
            selected_effect_index_ = i;
            break;
        }
    }
    updateEffectScroll();
}

void AnimationMenu::cycleEffect(int delta) {
    if (effects_.empty())
        return;
    int next = static_cast<int>(selected_effect_index_) + delta;
    if (next < 0)
        next = static_cast<int>(effects_.size()) - 1;
    if (next >= static_cast<int>(effects_.size()))
        next = 0;
    selected_effect_index_ = static_cast<size_t>(next);
    pending_config_.effect = effects_[selected_effect_index_].id;
    updateEffectScroll();
}

void AnimationMenu::updateEffectScroll() {
    if (effects_.size() <= MAX_VISIBLE_EFFECTS) {
        effect_scroll_offset_ = 0;
        return;
    }

    // 确保选中项在可见区域内
    if (selected_effect_index_ < effect_scroll_offset_) {
        // 选中项在可见区域上方，向上滚动
        effect_scroll_offset_ = selected_effect_index_;
    } else if (selected_effect_index_ >= effect_scroll_offset_ + MAX_VISIBLE_EFFECTS) {
        // 选中项在可见区域下方，向下滚动
        effect_scroll_offset_ = selected_effect_index_ - MAX_VISIBLE_EFFECTS + 1;
    }

    // 确保滚动偏移不越界
    if (effect_scroll_offset_ + MAX_VISIBLE_EFFECTS > effects_.size()) {
        effect_scroll_offset_ = effects_.size() - MAX_VISIBLE_EFFECTS;
    }
}

void AnimationMenu::adjustCurrentParam(int delta) {
    const float fdelta = delta > 0 ? 1.0f : -1.0f;
    switch (selected_param_index_) {
        case 0:
            pending_config_.enabled = !pending_config_.enabled;
            break;
        case 1:
            pending_config_.refresh_interval_ms += (delta > 0 ? 5 : -5);
            break;
        case 2:
            pending_config_.pulse_speed += 0.2f * fdelta;
            break;
        default:
            break;
    }
    clampConfig();
}

bool AnimationMenu::handleInput(ftxui::Event event) {
    if (event == Event::Escape || event == Event::Return) {
        return false;
    }

    if (event == Event::Tab) {
        focus_effects_ = !focus_effects_;
        return true;
    }

    if (focus_effects_) {
        if (event == Event::ArrowUp) {
            cycleEffect(-1);
            return true;
        }
        if (event == Event::ArrowDown) {
            cycleEffect(1);
            return true;
        }
        if (event == Event::ArrowLeft || event == Event::ArrowRight) {
            focus_effects_ = false;
            return true;
        }
    } else {
        if (event == Event::ArrowUp) {
            if (selected_param_index_ > 0)
                selected_param_index_--;
            return true;
        }
        if (event == Event::ArrowDown) {
            if (selected_param_index_ < 6)
                selected_param_index_++;
            return true;
        }
        if (event == Event::ArrowLeft) {
            adjustCurrentParam(-1);
            return true;
        }
        if (event == Event::ArrowRight) {
            adjustCurrentParam(1);
            return true;
        }
    }

    return false;
}

Element AnimationMenu::renderEffects() const {
    auto& colors = theme_.getColors();
    Elements rows;
    rows.push_back(text(" Effects") | bold | color(colors.keyword));
    rows.push_back(separator());

    // 计算可见范围
    size_t start = effect_scroll_offset_;
    size_t end = std::min(start + MAX_VISIBLE_EFFECTS, effects_.size());

    // 添加滚动指示器（如果需要）
    if (effects_.size() > MAX_VISIBLE_EFFECTS) {
        rows.push_back(text("  ▲") | color(colors.comment) | dim);
    }

    for (size_t i = start; i < end; ++i) {
        bool is_selected = i == selected_effect_index_;
        std::string name = effects_[i].name;
        if (pending_config_.effect == effects_[i].id) {
            name += " ";
            name += icons::SUCCESS;
        }
        Element row = hbox({text("  "),
                            (is_selected ? text("► ") | color(colors.function) | bold : text("  ")),
                            text(name) | color(is_selected ? colors.function : colors.foreground) |
                                (is_selected ? bold : nothing)});
        if (is_selected && focus_effects_) {
            row = row | bgcolor(colors.selection);
        }
        rows.push_back(row);
        rows.push_back(text("    " + effects_[i].desc) | color(colors.comment) | dim);
    }

    // 添加滚动指示器（如果需要）
    if (effects_.size() > MAX_VISIBLE_EFFECTS && end < effects_.size()) {
        rows.push_back(text("  ▼") | color(colors.comment) | dim);
    }

    return vbox(rows) | size(WIDTH, EQUAL, 34) | borderWithColor(colors.dialog_border);
}

Element AnimationMenu::renderParams() const {
    auto& c = theme_.getColors();
    Elements rows;
    rows.push_back(text(" Parameters") | bold | color(c.keyword));
    rows.push_back(separator());

    auto line = [&](size_t idx, const std::string& name, const std::string& value) {
        bool selected = (!focus_effects_ && selected_param_index_ == idx);
        Element row = hbox({text("  "), text(name) | color(c.comment), filler(),
                            text(value) | color(c.foreground) | bold, text("  ")});
        if (selected)
            row = row | bgcolor(c.selection);
        rows.push_back(row);
    };

    line(0, "Enabled", pending_config_.enabled ? "ON" : "OFF");
    line(1, "Refresh(ms)", std::to_string(pending_config_.refresh_interval_ms));
    line(2, "Pulse Speed", std::to_string(pending_config_.pulse_speed));

    return vbox(rows) | size(WIDTH, EQUAL, 42) | borderWithColor(c.dialog_border);
}

Element AnimationMenu::renderPreview() const {
    auto& c = theme_.getColors();
    return hbox({text(" Preview: effect=") | color(c.comment),
                 text(pending_config_.effect) | color(c.foreground) | bold,
                 text(" | interval=") | color(c.comment),
                 text(std::to_string(pending_config_.refresh_interval_ms) + "ms") |
                     color(c.foreground) | bold}) |
           bgcolor(c.dialog_bg);
}

Element AnimationMenu::render() {
    auto& c = theme_.getColors();
    Element title_bar = hbox({text(" "), text(icons::SETTINGS) | color(Color::Cyan),
                              text(" Animation Panel ") | bold | color(c.foreground), filler()}) |
                        bgcolor(c.menubar_bg);

    Element main = hbox({renderEffects(), text(" "), renderParams()}) | flex;

    Element help =
        hbox({text(" "), text("Tab") | color(c.helpbar_key) | bold, text(": switch panel  "),
              text("↑↓") | color(c.helpbar_key) | bold, text(": select  "),
              text("←→") | color(c.helpbar_key) | bold, text(": adjust  "),
              text("Enter") | color(c.helpbar_key) | bold, text(": apply  "),
              text("Esc") | color(c.helpbar_key) | bold, text(": cancel"), filler()}) |
        bgcolor(c.helpbar_bg) | color(c.helpbar_fg) | dim;

    return vbox({title_bar, separator(), main, separator(), renderPreview(), separator(), help}) |
           borderWithColor(c.dialog_border) | bgcolor(c.background) |
           size(WIDTH, GREATER_THAN, 84) | size(HEIGHT, GREATER_THAN, 24);
}

} // namespace ui
} // namespace pnana
