#ifndef PNANA_UI_TABBAR_H
#define PNANA_UI_TABBAR_H

#include "ui/theme.h"
#include "core/document_manager.h"
#include <ftxui/dom/elements.hpp>
#include <vector>
#include <string>

namespace pnana {
namespace ui {

// 标签栏组件
class Tabbar {
public:
    explicit Tabbar(Theme& theme);
    
    // 渲染标签栏
    ftxui::Element render(const std::vector<core::DocumentManager::TabInfo>& tabs);
    
private:
    Theme& theme_;
    
    // 渲染单个标签
    ftxui::Element renderTab(const core::DocumentManager::TabInfo& tab, size_t index);
    
    // 获取文件类型图标
    std::string getFileIcon(const std::string& filename) const;
    std::string getFileExtension(const std::string& filename) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_TABBAR_H

