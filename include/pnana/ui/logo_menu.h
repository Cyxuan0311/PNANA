#ifndef PNANA_UI_LOGO_MENU_H
#define PNANA_UI_LOGO_MENU_H

#include "features/logo_manager.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// Logo 样式选择弹窗（参考 ThemeMenu：左侧列表，右侧预览，Enter 确认，Esc 取消）
class LogoMenu {
  public:
    explicit LogoMenu(Theme& theme);

    // 设置当前选中的样式 id（打开菜单时同步）
    void setCurrentStyle(const std::string& style_id);

    bool handleInput(ftxui::Event event);
    ftxui::Element render();

    std::string getSelectedStyleId() const;

  private:
    Theme& theme_;
    std::vector<features::LogoStyleEntry> styles_;
    size_t selected_index_;
    std::string current_style_id_; // 当前已应用的样式，列表中显示勾选
    std::string search_input_;
    size_t search_cursor_pos_;
    std::vector<size_t> filtered_indices_;

    void updateFilteredStyles();
    ftxui::Element renderSearchBox() const;
    ftxui::Element renderStyleList() const;
    ftxui::Element renderLogoPreview() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_LOGO_MENU_H
