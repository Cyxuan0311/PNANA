#ifndef PNANA_UI_STATUSBAR_STYLE_MENU_H
#define PNANA_UI_STATUSBAR_STYLE_MENU_H

#include "ui/icons.h"
#include "ui/statusbar.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

using namespace ftxui;

namespace pnana {
namespace ui {

class StatusbarStyleMenu {
  public:
    StatusbarStyleMenu(Statusbar& statusbar, Theme& theme);

    void setAvailableStyles(const std::vector<std::string>& styles);
    void setSelectedIndex(size_t index);
    /// 用户按 Enter 确认样式时调用，参数为当前选中的样式名，用于持久化
    void setOnStyleConfirmed(std::function<void(const std::string&)> cb) {
        on_style_confirmed_ = std::move(cb);
    }

    bool handleInput(ftxui::Event event);
    Element render();

    std::string getSelectedStyle() const;
    void applySelectedStyle();

  private:
    Statusbar& statusbar_;
    Theme& theme_;
    size_t selected_index_ = 0;
    std::vector<std::string> available_styles_; // 由 statusbar_theme 提供，构造函数中初始化
    std::string search_input_;
    size_t search_cursor_pos_ = 0;
    std::vector<std::string> filtered_styles_;
    std::vector<size_t> filtered_indices_;
    std::function<void(const std::string&)> on_style_confirmed_;

    void updateFilteredStyles();
    Element renderSearchBox() const;
    Element renderStyleList() const;
    Element renderStylePreview() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_STATUSBAR_STYLE_MENU_H