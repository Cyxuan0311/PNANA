#ifdef BUILD_LUA_SUPPORT

#include "plugins/event_parser_api.h"
#include "utils/logger.h"
#include <cstring>
#include <string>

namespace pnana {
namespace plugins {

EventParserAPI::EventParserAPI() : parser_() {}

EventParserAPI::~EventParserAPI() {}

// 注册 API 到 Lua 引擎
void EventParserAPI::registerAPI(LuaEngine* engine) {
    lua_State* L = engine->getState();

    // 创建 vim.event 表
    lua_newtable(L);

    // 注册 parse_event 函数
    lua_pushcfunction(L, lua_parse_event);
    lua_setfield(L, -2, "parse");

    // 注册 event_to_string 函数
    lua_pushcfunction(L, lua_event_to_string);
    lua_setfield(L, -2, "to_string");

    // 注册 get_modifiers 函数
    lua_pushcfunction(L, lua_get_modifiers);
    lua_setfield(L, -2, "get_modifiers");

    // 注册 is_arrow_key 函数
    lua_pushcfunction(L, lua_is_arrow_key);
    lua_setfield(L, -2, "is_arrow_key");

    // 注册 is_function_key 函数
    lua_pushcfunction(L, lua_is_function_key);
    lua_setfield(L, -2, "is_function_key");

    // 注册 is_navigation_key 函数
    lua_pushcfunction(L, lua_is_navigation_key);
    lua_setfield(L, -2, "is_navigation_key");

    // 将 vim.event 表设置为全局
    lua_setglobal(L, "event_parser");

    LOG_DEBUG("[EventParserAPI] Registered event parser API to Lua");
}

// Lua C 函数绑定 - 静态包装函数
int EventParserAPI::lua_parse_event(lua_State* L) {
    // 直接调用实例方法
    const char* event_type = luaL_checkstring(L, 1);
    const char* character = luaL_optstring(L, 2, "");

    // 创建返回表格
    lua_newtable(L);

    // event_type 字段
    lua_pushstring(L, "event_type");
    lua_pushstring(L, event_type);
    lua_settable(L, -3);

    // character 字段
    lua_pushstring(L, "character");
    lua_pushstring(L, character);
    lua_settable(L, -3);

    // modifiers 表格
    lua_pushstring(L, "modifiers");
    lua_newtable(L);

    // 默认 modifiers 都是 false
    lua_pushstring(L, "ctrl");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    lua_pushstring(L, "alt");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    lua_pushstring(L, "shift");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    lua_pushstring(L, "meta");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    lua_settable(L, -3); // 将 modifiers 表格添加到主表格

    return 1;
}

// 将事件转换为字符串
int EventParserAPI::lua_event_to_string(lua_State* L) {
    const char* event_type = luaL_checkstring(L, 1);
    const char* character = luaL_optstring(L, 2, "");

    std::string result = event_type;
    if (character && strlen(character) > 0) {
        result += ":";
        result += character;
    }

    lua_pushstring(L, result.c_str());
    return 1;
}

// 获取修饰键信息
int EventParserAPI::lua_get_modifiers(lua_State* L) {
    // 创建 modifiers 表格
    lua_newtable(L);

    lua_pushstring(L, "ctrl");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    lua_pushstring(L, "alt");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    lua_pushstring(L, "shift");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    lua_pushstring(L, "meta");
    lua_pushboolean(L, false);
    lua_settable(L, -3);

    return 1;
}

// 检查是否是方向键
int EventParserAPI::lua_is_arrow_key(lua_State* L) {
    const char* event_type = luaL_checkstring(L, 1);
    std::string type(event_type);

    bool is_arrow = (type == "arrow_up" || type == "arrow_down" || type == "arrow_left" ||
                     type == "arrow_right");

    lua_pushboolean(L, is_arrow);
    return 1;
}

// 检查是否是功能键
int EventParserAPI::lua_is_function_key(lua_State* L) {
    const char* event_type = luaL_checkstring(L, 1);
    std::string type(event_type);

    bool is_function = (type == "f1" || type == "f2" || type == "f3" || type == "f4" ||
                        type == "f5" || type == "f6" || type == "f7" || type == "f8" ||
                        type == "f9" || type == "f10" || type == "f11" || type == "f12");

    lua_pushboolean(L, is_function);
    return 1;
}

// 检查是否是导航键
int EventParserAPI::lua_is_navigation_key(lua_State* L) {
    const char* event_type = luaL_checkstring(L, 1);
    std::string type(event_type);

    bool is_navigation = (type == "pageup" || type == "pagedown" || type == "home" ||
                          type == "end" || type == "insert" || type == "delete");

    lua_pushboolean(L, is_navigation);
    return 1;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
