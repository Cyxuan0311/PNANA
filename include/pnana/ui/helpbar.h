#ifndef PNANA_UI_HELPBAR_H
#define PNANA_UI_HELPBAR_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 帮助项
struct HelpItem {
    std::string key;
    std::string description;

    HelpItem(const std::string& k, const std::string& d) : key(k), description(d) {}
};

// 帮助栏组件 - 显示快捷键提示
class Helpbar {
  public:
    explicit Helpbar(Theme& theme);

    // 渲染帮助栏
    ftxui::Element render(const std::vector<HelpItem>& items);

    // 预设帮助项集合
    static std::vector<HelpItem> getDefaultHelp();
    static std::vector<HelpItem> getEditModeHelp();
    static std::vector<HelpItem> getSearchModeHelp();

  private:
    Theme& theme_;

    // 渲染单个帮助项
    ftxui::Element renderItem(const HelpItem& item);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_HELPBAR_H
