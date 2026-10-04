#include "ui/border_config_popup.h"

#include "core/ui/border_manager.h"
#include "ui/icons.h"
#include "ui/responsive_size.h"
#include <utility>

using namespace ftxui;

static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        auto style = pnana::core::ui::BorderManager::getCurrentStyle();
        return std::move(child) | ftxui::borderStyled(style) | ftxui::color(border_color);
    };
}

static inline Element padded(Element e) {
    return hbox({text(" "), std::move(e), text(" ")});
}

namespace pnana {
namespace ui {

BorderConfigPopup::BorderConfigPopup(Theme& theme)
    : theme_(theme), visible_(false), selected_option_(0), global_index_(0), active_index_(0),
      inactive_index_(2) {
    global_styles_ = {"rounded", "light", "double", "heavy", "dashed", "empty"};
    override_options_ = {"Inherit", "rounded", "light", "double", "heavy", "dashed", "empty"};
}

void BorderConfigPopup::open(const core::BorderConfig& config) {
    config_ = config;
    visible_ = true;
    selected_option_ = 0;

    auto it = std::find(global_styles_.begin(), global_styles_.end(), config_.global_style);
    global_index_ =
        (it != global_styles_.end()) ? static_cast<int>(it - global_styles_.begin()) : 0;

    if (config_.active_style.empty()) {
        active_index_ = 0;
    } else {
        auto ait = std::find(global_styles_.begin(), global_styles_.end(), config_.active_style);
        active_index_ =
            (ait != global_styles_.end()) ? static_cast<int>(ait - global_styles_.begin()) + 1 : 0;
    }

    if (config_.inactive_style.empty()) {
        inactive_index_ = 0;
    } else {
        auto iit = std::find(global_styles_.begin(), global_styles_.end(), config_.inactive_style);
        inactive_index_ =
            (iit != global_styles_.end()) ? static_cast<int>(iit - global_styles_.begin()) + 1 : 0;
    }
}

void BorderConfigPopup::close() {
    visible_ = false;
}

bool BorderConfigPopup::handleInput(const ftxui::Event& event) {
    if (!visible_)
        return false;

    if (event == Event::Escape) {
        close();
        return true;
    }

    if (event == Event::Return) {
        apply();
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        selectPrevious();
        return true;
    }

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        selectNext();
        return true;
    }

    if (event == Event::ArrowLeft || event == Event::Character('h')) {
        if (selected_option_ == 0 && global_index_ > 0)
            global_index_--;
        else if (selected_option_ == 1 && active_index_ > 0)
            active_index_--;
        else if (selected_option_ == 2 && inactive_index_ > 0)
            inactive_index_--;
        return true;
    }

    if (event == Event::ArrowRight || event == Event::Character('l')) {
        if (selected_option_ == 0 && global_index_ < static_cast<int>(global_styles_.size()) - 1)
            global_index_++;
        else if (selected_option_ == 1 &&
                 active_index_ < static_cast<int>(override_options_.size()) - 1)
            active_index_++;
        else if (selected_option_ == 2 &&
                 inactive_index_ < static_cast<int>(override_options_.size()) - 1)
            inactive_index_++;
        return true;
    }

    return false;
}

void BorderConfigPopup::selectNext() {
    if (selected_option_ < 2)
        selected_option_++;
    else
        selected_option_ = 0;
}

void BorderConfigPopup::selectPrevious() {
    if (selected_option_ > 0)
        selected_option_--;
    else
        selected_option_ = 2;
}

void BorderConfigPopup::apply() {
    config_.global_style = global_styles_[global_index_];
    config_.active_style = (active_index_ == 0) ? "" : global_styles_[active_index_ - 1];
    config_.inactive_style = (inactive_index_ == 0) ? "" : global_styles_[inactive_index_ - 1];

    pnana::core::ui::BorderManager::setBorderConfig(config_);

    if (on_apply_)
        on_apply_(config_);

    close();
}

Element BorderConfigPopup::render() {
    if (!visible_)
        return text("");

    auto& colors = theme_.getColors();

    Element header = hbox({
                         text(" "),
                         text(icons::SETTINGS) | color(colors.keyword) | bold,
                         text(" Border Style ") | color(colors.menubar_fg) | bold,
                         filler(),
                         text("Enter") | color(colors.helpbar_key) | bold,
                         text(" Apply  ") | color(colors.menubar_fg) | dim,
                         text("Esc") | color(colors.helpbar_key) | bold,
                         text(" Cancel ") | color(colors.menubar_fg) | dim,
                     }) |
                     bgcolor(colors.menubar_bg);

    Element body =
        vbox({
            padded(renderRow("Global Style", global_styles_[global_index_], 0, false)),
            separatorEmpty(),
            padded(renderRow("Active Style", override_options_[active_index_], 1, true)),
            separatorEmpty(),
            padded(renderRow("Inactive Style", override_options_[inactive_index_], 2, true)),
        }) |
        flex;

    Element footer = hbox({
                         text(" "),
                         text("↑↓") | color(colors.helpbar_key) | bold,
                         text(": Navigate  ") | color(colors.helpbar_fg) | dim,
                         text("←→") | color(colors.helpbar_key) | bold,
                         text(": Change") | color(colors.helpbar_fg) | dim,
                         filler(),
                     }) |
                     bgcolor(colors.helpbar_bg);

    return vbox({
               hbox({text(" Border ") | color(colors.keyword) | bold, filler()}) |
                   bgcolor(colors.menubar_bg),
               separator(),
               header,
               separator(),
               body,
               separator(),
               footer,
           }) |
           size(WIDTH, GREATER_THAN, responsiveMinWidth(50)) |
           size(HEIGHT, GREATER_THAN, responsiveMinHeight(10)) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border) | center;
}

Element BorderConfigPopup::renderRow(const std::string& label, const std::string& value,
                                     size_t option_index, bool is_override) {
    auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == option_index);

    std::string display_value = value;
    Color display_color = colors.comment;
    if (is_override && option_index > 0) {
        display_color = colors.success;
    } else if (is_override && option_index == 0) {
        display_color = colors.comment;
    } else {
        display_color = colors.function;
    }

    return hbox({(is_selected ? text("► ") | color(colors.function) : text("  ")),
                 text(label + ": ") | color(is_selected ? colors.foreground : colors.comment),
                 text("[" + display_value + "]") | bold | color(display_color) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 text("  ") | color(colors.comment) | dim, filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

} // namespace ui
} // namespace pnana
