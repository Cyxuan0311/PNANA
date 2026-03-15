#ifndef PNANA_UI_STATUSBAR_THEME_H
#define PNANA_UI_STATUSBAR_THEME_H

#include "ui/statusbar.h"
#include <string>
#include <vector>

namespace pnana {
namespace ui {

/// 状态栏主题：样式名与配置的单一来源，供状态栏、样式菜单、配置加载使用

/// 返回所有可用的状态栏样式名称（顺序与菜单一致）
std::vector<std::string> getAvailableStatusbarStyleNames();

/// 根据样式名称生成对应的 StatusbarBeautifyConfig；未知名称时仅设置 style_name
StatusbarBeautifyConfig getStatusbarConfigForStyle(const std::string& style_name);

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_STATUSBAR_THEME_H
