#ifdef BUILD_LUA_SUPPORT

#ifndef PNANA_PLUGINS_UI_API_H
#define PNANA_PLUGINS_UI_API_H

#include "core/ui/widget.h"
#include <lua.hpp>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace pnana {
namespace core {
class Editor;
}
namespace plugins {

class LuaAPI;

/**
 * @brief UI 相关 Lua API
 * 提供 vim.ui.* 接口
 *
 * 注册到 vim.ui 命名空间
 */
class UIAPI {
  public:
    UIAPI();
    ~UIAPI();

    void setLuaAPI(LuaAPI* lua_api) {
        lua_api_ = lua_api;
    }

    void registerFunctions(lua_State* L);
    static void closeAllWindows(pnana::core::Editor* editor);

  private:
    LuaAPI* lua_api_;

    // 基础 UI API（统一使用 lua_fn_* 前缀）
    static int lua_fn_notify(lua_State* L);
    static int lua_fn_input(lua_State* L);
    static int lua_fn_select(lua_State* L);
    static int lua_fn_popup(lua_State* L);
    static int lua_fn_dialog(lua_State* L);
    static int lua_fn_list(lua_State* L);
    static int lua_fn_open_window(lua_State* L);
    static int lua_fn_update_window(lua_State* L);
    static int lua_fn_close_window(lua_State* L);
    static int lua_fn_open_component_window(lua_State* L);
    static int lua_fn_update_component_window(lua_State* L);

    // 高级 UI API
    static int lua_fn_progress(lua_State* L);
    static int lua_fn_update_progress(lua_State* L);
    static int lua_fn_close_progress(lua_State* L);
    static int lua_fn_multiselect(lua_State* L);
    static int lua_fn_hover(lua_State* L);
    static int lua_fn_close_hover(lua_State* L);
    static int lua_fn_list_windows(lua_State* L);
    static int lua_fn_window_is_valid(lua_State* L);
    static int lua_fn_get_window_info(lua_State* L);
    static int lua_fn_focus_window(lua_State* L);

    // 布局 API
    static int lua_fn_create_layout(lua_State* L);
    static int lua_fn_open_layout_window(lua_State* L);
    static int lua_fn_update_layout(lua_State* L);

    static LuaAPI* getLuaAPIFromLua(lua_State* L);

    // Lua window id -> PopupHandle 映射（真实内核句柄由 PopupManager 管理）
    // 使用 unordered_map 优化查找性能 O(1) vs O(log n)
    static std::unordered_map<int, int> window_handles_;
    static std::unordered_map<int, lua_State*> window_lua_states_;
    static std::unordered_map<int, std::unordered_map<std::string, int>> window_event_refs_;
    static int next_window_id_;

    // 进度条窗口管理
    static std::unordered_map<int, int> progress_handles_;
    static int next_progress_id_;

    // Hover 窗口管理
    static std::unordered_map<int, int> hover_handles_;
    static int next_hover_id_;
};

} // namespace plugins
} // namespace pnana

#endif // PNANA_PLUGINS_UI_API_H

#endif // BUILD_LUA_SUPPORT
