#ifdef BUILD_LUA_SUPPORT

#ifndef PNANA_PLUGINS_UI_API_H
#define PNANA_PLUGINS_UI_API_H

#include "core/ui/widget.h"
#include <lua.hpp>
#include <map>
#include <string>
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

    static int lua_ui_notify(lua_State* L);
    static int lua_ui_input(lua_State* L);
    static int lua_ui_select(lua_State* L);
    static int lua_ui_popup(lua_State* L);
    static int lua_ui_dialog(lua_State* L);
    static int lua_ui_list(lua_State* L);
    static int lua_ui_open_window(lua_State* L);
    static int lua_ui_update_window(lua_State* L);
    static int lua_ui_close_window(lua_State* L);

    // 新增 UI API
    static int lua_ui_progress(lua_State* L);
    static int lua_ui_update_progress(lua_State* L);
    static int lua_ui_close_progress(lua_State* L);
    static int lua_ui_multiselect(lua_State* L);
    static int lua_ui_hover(lua_State* L);
    static int lua_ui_close_hover(lua_State* L);
    static int lua_ui_list_windows(lua_State* L);
    static int lua_ui_window_is_valid(lua_State* L);
    static int lua_ui_get_window_info(lua_State* L);
    static int lua_ui_focus_window(lua_State* L);

    // 新增高级布局 API
    static int lua_ui_create_layout(lua_State* L);
    static int lua_ui_open_layout_window(lua_State* L);
    static int lua_ui_update_layout(lua_State* L);

    static LuaAPI* getLuaAPIFromLua(lua_State* L);

    // 辅助函数：从 Lua 表解析 WidgetSpec
    static pnana::core::ui::WidgetSpec parseWidgetSpecFromLua(lua_State* L, int index);
    static void parseLayoutOptions(lua_State* L, int index, pnana::core::ui::WidgetSpec& spec);

    // Lua window id -> PopupHandle 映射（真实内核句柄由 PopupManager 管理）
    static std::map<int, int> window_handles_;
    static int next_window_id_;

    // 进度条窗口管理
    static std::map<int, int> progress_handles_;
    static int next_progress_id_;

    // Hover 窗口管理
    static std::map<int, int> hover_handles_;
    static int next_hover_id_;
};

} // namespace plugins
} // namespace pnana

#endif // PNANA_PLUGINS_UI_API_H

#endif // BUILD_LUA_SUPPORT
