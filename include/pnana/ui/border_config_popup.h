#ifndef PNANA_UI_BORDER_CONFIG_POPUP_H
#define PNANA_UI_BORDER_CONFIG_POPUP_H

#include "core/config_manager.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace pnana {
namespace ui {

class BorderConfigPopup {
  public:
    explicit BorderConfigPopup(Theme& theme);

    void open(const core::BorderConfig& config);
    void close();
    bool isVisible() const {
        return visible_;
    }

    bool handleInput(const ftxui::Event& event);
    ftxui::Element render();

    void setOnApply(std::function<void(const core::BorderConfig&)> callback) {
        on_apply_ = std::move(callback);
    }

  private:
    Theme& theme_;
    bool visible_;

    core::BorderConfig config_;

    size_t selected_option_;

    std::vector<std::string> global_styles_;
    std::vector<std::string> override_options_;

    int global_index_;
    int active_index_;
    int inactive_index_;

    std::function<void(const core::BorderConfig&)> on_apply_;

    void apply();
    void selectNext();
    void selectPrevious();

    ftxui::Element renderRow(const std::string& label, const std::string& value,
                             size_t option_index, bool is_override);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_BORDER_CONFIG_POPUP_H
