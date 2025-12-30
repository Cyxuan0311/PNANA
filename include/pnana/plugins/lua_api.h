#ifndef PNANA_PLUGINS_LUA_API_H
#define PNANA_PLUGINS_LUA_API_H

#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_engine.h"
#include <string>
#include <functional>
#include <vector>
#include <map>

// 前向声明
namespace pnana {
namespace core {
    class Editor;
}
}

namespace pnana {
namespace plugins {

// Lua API 绑定类
class LuaAPI {
public:
    explicit LuaAPI(core::Editor* editor);
    ~LuaAPI();
    
    // 初始化 API（注册所有函数到 Lua）
    void initialize(LuaEngine* engine);
    
    // 触发事件
    void triggerEvent(const std::string& event, const std::vector<std::string>& args = {});
    
    // 注册事件监听器
    void registerEventListener(const std::string& event, const std::string& callback);
    
    // 注册命令
    void registerCommand(const std::string& name, const std::string& callback);
    
    // 注册键位映射
    void registerKeymap(const std::string& mode, const std::string& keys, const std::string& callback);
    
    // 获取编辑器实例
    core::Editor* getEditor() { return editor_; }

private:
    core::Editor* editor_;
    LuaEngine* engine_;
    
    // 事件监听器映射: event -> [callbacks]
    std::map<std::string, std::vector<std::string>> event_listeners_;
    
    // 命令映射: name -> callback
    std::map<std::string, std::string> commands_;
    
    // 键位映射: mode -> keys -> callback
    std::map<std::string, std::map<std::string, std::string>> keymaps_;
    
    // 注册 API 函数
    void registerAPIFunctions();
    
    // Lua API 函数（静态，供 Lua 调用）
    static int lua_api_notify(lua_State* L);
    static int lua_api_command(lua_State* L);
    static int lua_api_keymap(lua_State* L);
    static int lua_api_autocmd(lua_State* L);
    
    // vim.api 命名空间函数
    static int lua_api_get_current_line(lua_State* L);
    static int lua_api_get_line_count(lua_State* L);
    static int lua_api_get_cursor_pos(lua_State* L);
    static int lua_api_set_cursor_pos(lua_State* L);
    static int lua_api_get_line(lua_State* L);
    static int lua_api_set_line(lua_State* L);
    static int lua_api_insert_text(lua_State* L);
    static int lua_api_delete_line(lua_State* L);
    static int lua_api_get_filepath(lua_State* L);
    static int lua_api_open_file(lua_State* L);
    static int lua_api_save_file(lua_State* L);
    static int lua_api_set_status_message(lua_State* L);
    static int lua_api_get_theme(lua_State* L);
    static int lua_api_set_theme(lua_State* L);
    
    // vim.fn 命名空间函数（工具函数）
    static int lua_fn_system(lua_State* L);
    static int lua_fn_readfile(lua_State* L);
    static int lua_fn_writefile(lua_State* L);
    
    // 辅助函数：从 Lua 栈获取编辑器实例
    static core::Editor* getEditorFromLua(lua_State* L);
    static LuaAPI* getAPIFromLua(lua_State* L);
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_LUA_API_H

