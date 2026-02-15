#ifdef BUILD_LUA_SUPPORT

#ifndef PNANA_PLUGINS_SYSTEM_API_H
#define PNANA_PLUGINS_SYSTEM_API_H

#include <lua.hpp>
#include <string>

// 前向声明
namespace pnana {
namespace plugins {
class LuaAPI;
}
} // namespace pnana

namespace pnana {
namespace plugins {

/**
 * @brief 系统工具相关的Lua API
 * 处理系统命令执行、通知、命令注册等
 */
class SystemAPI {
  public:
    SystemAPI();
    ~SystemAPI();

    // 设置LuaAPI引用（用于访问注册表）
    void setLuaAPI(LuaAPI* lua_api) {
        lua_api_ = lua_api;
    }

    // 注册所有系统相关的API函数
    void registerFunctions(lua_State* L);

  private:
    LuaAPI* lua_api_;

    // 系统工具API函数
    static int lua_fn_system(lua_State* L);
    static int lua_api_notify(lua_State* L);
    
    // 旧API（兼容层）
    static int lua_api_command(lua_State* L);
    static int lua_api_keymap(lua_State* L);
    static int lua_api_autocmd(lua_State* L);
    
    // 新API（Neovim风格）
    static int lua_api_create_user_command(lua_State* L);
    static int lua_api_del_user_command(lua_State* L);
    static int lua_keymap_set(lua_State* L);
    static int lua_keymap_del(lua_State* L);
    static int lua_api_create_autocmd(lua_State* L);
    static int lua_api_clear_autocmds(lua_State* L);

    // 辅助函数
    static SystemAPI* getAPIFromLua(lua_State* L);
    static LuaAPI* getLuaAPIFromLua(lua_State* L);
    
    // 解析命令选项
    static void parseCommandOptions(lua_State* L, int opts_index, std::string& nargs, std::string& desc, bool& force);
    
    // 解析键映射选项
    static void parseKeymapOptions(lua_State* L, int opts_index, bool& noremap, bool& silent, bool& expr, bool& nowait, std::string& desc);
    
    // 解析autocmd选项
    static void parseAutocmdOptions(lua_State* L, int opts_index, std::string& pattern, bool& once, bool& nested, std::string& desc, std::string& group);
};

} // namespace plugins
} // namespace pnana

#endif // PNANA_PLUGINS_SYSTEM_API_H
#endif // BUILD_LUA_SUPPORT
