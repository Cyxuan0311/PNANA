#ifdef BUILD_LUA_SUPPORT

#ifndef PNANA_PLUGINS_ICON_API_H
#define PNANA_PLUGINS_ICON_API_H

#include "ui/icons.h"
#include <lua.hpp>
#include <string>

namespace pnana {
namespace plugins {

class LuaAPI;

/**
 * @brief 图标相关 Lua API
 * 提供 vim.icon.* 接口，允许插件访问和使用 Nerd Font 图标
 */
class IconAPI {
  public:
    IconAPI();
    ~IconAPI();

    void setLuaAPI(LuaAPI* lua_api) {
        lua_api_ = lua_api;
    }

    void registerFunctions(lua_State* L);

  private:
    LuaAPI* lua_api_;

    // 获取图标
    static int lua_icon_get(lua_State* L);

    // 获取所有图标名称列表
    static int lua_icon_list(lua_State* L);

    // 检查图标是否存在
    static int lua_icon_has(lua_State* L);

    // 获取图标类别
    static int lua_icon_category(lua_State* L);

    // 辅助函数：获取图标值
    static const char* getIconByName(const std::string& name);
};

} // namespace plugins
} // namespace pnana

#endif // PNANA_PLUGINS_ICON_API_H

#endif // BUILD_LUA_SUPPORT
