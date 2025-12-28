#ifndef PNANA_UI_HELP_H
#define PNANA_UI_HELP_H

#include "ui/theme.h"
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
    
    // 获取所有帮助条目
    static std::vector<HelpEntry> getAllHelp();
    
private:
    Theme& theme_;
    
    // 渲染帮助分类
    ftxui::Element renderCategory(const std::string& category, 
                                   const std::vector<HelpEntry>& entries);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_HELP_H

