#ifndef PNANA_UI_HELP_H
#define PNANA_UI_HELP_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 帮助条目
struct HelpEntry {
    std::string category;
    std::string key;
    std::string description;
};

// 帮助系统
class Help {
  public:
    explicit Help(Theme& theme);

    // 渲染帮助窗口
    ftxui::Element render(int width, int height);

    // 处理输入事件（翻页等）
    bool handleInput(ftxui::Event event);

    // 获取所有帮助条目
    static std::vector<HelpEntry> getAllHelp();

    // 重置滚动位置
    void reset();

  private:
    Theme& theme_;
    size_t scroll_offset_; // 滚动偏移量

    // 渲染帮助分类
    ftxui::Element renderCategory(const std::string& category,
                                  const std::vector<HelpEntry>& entries);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_HELP_H
