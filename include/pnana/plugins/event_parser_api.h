#ifndef PNANA_PLUGINS_EVENT_PARSER_API_H
#define PNANA_PLUGINS_EVENT_PARSER_API_H

#ifdef BUILD_LUA_SUPPORT

#include "input/event_parser.h"
#include "plugins/lua_engine.h"
#include <ftxui/component/event.hpp>
#include <memory>
#include <string>

namespace pnana {
namespace plugins {

/**
 * @brief Lua 侧事件解析 API
 *
 * 将 C++ 侧的 ftxui::Event 转换为 Lua 侧可解析的结构化数据
 * 允许 Lua 脚本自定义事件解析逻辑
 */
class EventParserAPI {
  public:
    EventParserAPI();
    ~EventParserAPI();

    // 注册到 Lua 引擎
    void registerAPI(LuaEngine* engine);

    // 将 FTXUI 事件转换为 Lua 表格数据
    // 返回：{event_type = "pageup", modifiers = {ctrl = false, alt = false, shift = false},
    // character = ""}
    lua_State* parseEvent(lua_State* L);

  private:
    input::EventParser parser_;

    // Lua C 函数绑定
    static int lua_parse_event(lua_State* L);
    static int lua_event_to_string(lua_State* L);
    static int lua_get_modifiers(lua_State* L);
    static int lua_is_arrow_key(lua_State* L);
    static int lua_is_function_key(lua_State* L);
    static int lua_is_navigation_key(lua_State* L);
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_EVENT_PARSER_API_H
