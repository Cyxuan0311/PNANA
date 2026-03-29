#ifndef PNANA_CORE_UI_LUA_UI_PARSER_H
#define PNANA_CORE_UI_LUA_UI_PARSER_H

#ifdef BUILD_LUA_SUPPORT

#include "core/ui/widget.h"

#include <lua.hpp>

namespace pnana {
namespace core {
namespace ui {

// 将 Lua 声明式 UI 表结构解析为 WidgetSpec（核心解析模块）
class LuaUIParser {
  public:
    // 解析 WidgetSpec（递归解析 children / on / layout 等字段）
    static WidgetSpec parseWidgetSpecFromLua(lua_State* L, int index);

    // 解析 WidgetSpec 的布局选项（direction/align/flex/min_width/...）
    static void parseLayoutOptionsFromLua(lua_State* L, int index, WidgetSpec& spec);
};

} // namespace ui
} // namespace core
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_CORE_UI_LUA_UI_PARSER_H
