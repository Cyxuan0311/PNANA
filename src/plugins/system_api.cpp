#ifdef BUILD_LUA_SUPPORT

#include "plugins/system_api.h"
#include "plugins/lua_api.h"
#include "utils/logger.h"
#include <cstdio>
#include <lua.hpp>
#include <memory>
#include <sstream>

namespace pnana {
namespace plugins {

// 在 Lua 注册表中存储API实例的键
static const char* API_REGISTRY_KEY = "pnana_system_api";
static const char* LUA_API_REGISTRY_KEY = "pnana_lua_api";

SystemAPI::SystemAPI() : lua_api_(nullptr) {}

SystemAPI::~SystemAPI() {}

void SystemAPI::registerFunctions(lua_State* L) {
    // 在注册表中存储API实例
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, API_REGISTRY_KEY);

    // 注册到vim.fn表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "fn");

    lua_pushcfunction(L, lua_fn_system);
    lua_setfield(L, -2, "system");

    lua_pop(L, 2); // 弹出vim和fn表

    // 注册全局函数
    lua_pushcfunction(L, lua_api_notify);
    lua_setglobal(L, "pnana_notify");

    lua_pushcfunction(L, lua_api_command);
    lua_setglobal(L, "pnana_command");

    lua_pushcfunction(L, lua_api_keymap);
    lua_setglobal(L, "pnana_keymap");

    lua_pushcfunction(L, lua_api_autocmd);
    lua_setglobal(L, "pnana_autocmd");

    // 注册新API到vim.api表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "api");

    lua_pushcfunction(L, lua_api_create_user_command);
    lua_setfield(L, -2, "create_user_command");

    lua_pushcfunction(L, lua_api_del_user_command);
    lua_setfield(L, -2, "del_user_command");

    lua_pushcfunction(L, lua_api_create_autocmd);
    lua_setfield(L, -2, "create_autocmd");

    lua_pushcfunction(L, lua_api_clear_autocmds);
    lua_setfield(L, -2, "clear_autocmds");

    lua_pop(L, 2); // 弹出vim和api表

    // 注册vim.keymap表
    lua_getglobal(L, "vim");
    lua_newtable(L);
    lua_pushcfunction(L, lua_keymap_set);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, lua_keymap_del);
    lua_setfield(L, -2, "del");
    lua_setfield(L, -2, "keymap");
    lua_pop(L, 1); // 弹出vim表

    // 创建便捷别名（类似 Neovim）- 旧API兼容层
    // 直接在 Lua 栈上设置，更可靠
    lua_getglobal(L, "vim");
    lua_getglobal(L, "pnana_notify");
    lua_setfield(L, -2, "notify");
    lua_getglobal(L, "pnana_command");
    lua_setfield(L, -2, "cmd");
    lua_getglobal(L, "pnana_keymap");
    lua_setfield(L, -2, "keymap");
    lua_getglobal(L, "pnana_autocmd");
    lua_setfield(L, -2, "autocmd");
    lua_pop(L, 1);
}

SystemAPI* SystemAPI::getAPIFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, API_REGISTRY_KEY);
    SystemAPI* api = static_cast<SystemAPI*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return api;
}

LuaAPI* SystemAPI::getLuaAPIFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_API_REGISTRY_KEY);
    LuaAPI* api = static_cast<LuaAPI*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return api;
}

// vim.fn.system(command) -> string
// 沙盒模式：禁用系统命令执行
int SystemAPI::lua_fn_system(lua_State* L) {
    const char* command = lua_tostring(L, 1);
    if (!command) {
        lua_pushnil(L);
        lua_pushstring(L, "System command execution is disabled in sandbox mode");
        return 2; // 返回 nil, error_message
    }

    // 记录尝试执行的命令（用于安全审计）
    LOG_WARNING("Plugin attempted to execute system command: " + std::string(command) +
                " (blocked by sandbox)");

    // 返回 nil 和错误消息
    lua_pushnil(L);
    lua_pushstring(L, "System command execution is disabled in sandbox mode");
    return 2; // 返回 nil, error_message
}

// pnana_notify(message, level)
int SystemAPI::lua_api_notify(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (message) {
        LOG("Plugin: " + std::string(message));
    }
    return 0;
}

// pnana_command(name, callback) - 旧API兼容层
int SystemAPI::lua_api_command(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        return 0;
    }

    const char* name = lua_tostring(L, 1);
    if (!name) {
        return 0;
    }

    // 检查第二个参数是函数还是字符串
    if (lua_isfunction(L, 2)) {
        // 函数回调 - 创建函数引用
        lua_pushvalue(L, 2);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        // 使用新API注册，但使用简化的选项
        lua_api->registerUserCommand(std::string(name), ref, "*", "", false);
    } else if (lua_isstring(L, 2)) {
        // 字符串回调 - 旧API兼容
        const char* callback = lua_tostring(L, 2);
        if (callback) {
            lua_api->registerCommand(std::string(name), std::string(callback));
        }
    }

    return 0;
}

// pnana_keymap(mode, keys, callback) - 旧API兼容层
int SystemAPI::lua_api_keymap(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        return 0;
    }

    const char* mode = lua_tostring(L, 1);
    const char* keys = lua_tostring(L, 2);
    if (!mode || !keys) {
        return 0;
    }

    // 检查第三个参数是函数还是字符串
    if (lua_isfunction(L, 3)) {
        // 函数回调 - 创建函数引用
        lua_pushvalue(L, 3);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        // 使用新API注册，默认noremap=true（旧行为）
        lua_api->registerKeymap(std::string(mode), std::string(keys), ref, true, false, false,
                                false, "");
    } else if (lua_isstring(L, 3)) {
        // 字符串回调 - 旧API兼容
        const char* callback = lua_tostring(L, 3);
        if (callback) {
            lua_api->registerKeymap(std::string(mode), std::string(keys), std::string(callback));
        }
    }

    return 0;
}

// pnana_autocmd(event, callback) - 旧API兼容层
int SystemAPI::lua_api_autocmd(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        return 0;
    }

    const char* event = lua_tostring(L, 1);
    if (!event) {
        return 0;
    }

    // 检查第二个参数是字符串还是函数
    if (lua_isfunction(L, 2)) {
        // 函数回调 - 创建函数引用
        lua_pushvalue(L, 2);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        // 使用新API注册，使用默认选项
        lua_api->registerAutocmd(std::string(event), ref, "", false, false, "", "");
    } else if (lua_isstring(L, 2)) {
        // 字符串回调 - 旧API兼容
        const char* callback = lua_tostring(L, 2);
        if (callback) {
            lua_api->registerEventListener(std::string(event), std::string(callback));
        }
    }

    return 0;
}

// vim.api.create_user_command(name, callback, opts) - 新API
int SystemAPI::lua_api_create_user_command(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* name = lua_tostring(L, 1);
    if (!name || !lua_isfunction(L, 2)) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 创建函数引用
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // 解析选项（第三个参数，可选）
    std::string nargs = "*";
    std::string desc = "";
    bool force = false;

    if (lua_istable(L, 3)) {
        parseCommandOptions(L, 3, nargs, desc, force);
    }

    lua_api->registerUserCommand(std::string(name), ref, nargs, desc, force);
    lua_pushboolean(L, true);
    return 1;
}

// vim.api.del_user_command(name) - 新API
int SystemAPI::lua_api_del_user_command(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* name = lua_tostring(L, 1);
    if (!name) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool result = lua_api->delUserCommand(std::string(name));
    lua_pushboolean(L, result);
    return 1;
}

// vim.keymap.set(mode, lhs, rhs, opts) - 新API
int SystemAPI::lua_keymap_set(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* mode = lua_tostring(L, 1);
    const char* lhs = lua_tostring(L, 2);
    if (!mode || !lhs) {
        lua_pushboolean(L, false);
        return 1;
    }

    int rhs_ref = -1;
    std::string rhs_string = "";

    // 检查rhs是函数还是字符串
    if (lua_isfunction(L, 3)) {
        lua_pushvalue(L, 3);
        rhs_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    } else if (lua_isstring(L, 3)) {
        rhs_string = lua_tostring(L, 3);
    } else {
        lua_pushboolean(L, false);
        return 1;
    }

    // 解析选项（第四个参数，可选）
    bool noremap = true;
    bool silent = false;
    bool expr = false;
    bool nowait = false;
    std::string desc = "";

    if (lua_istable(L, 4)) {
        parseKeymapOptions(L, 4, noremap, silent, expr, nowait, desc);
    }

    if (rhs_ref != -1) {
        lua_api->registerKeymap(std::string(mode), std::string(lhs), rhs_ref, noremap, silent, expr,
                                nowait, desc);
    } else if (!rhs_string.empty()) {
        // 对于字符串rhs，使用字符串重载
        lua_api->registerKeymap(std::string(mode), std::string(lhs), rhs_string, noremap, silent,
                                expr, nowait, desc);
    } else {
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushboolean(L, true);
    return 1;
}

// vim.keymap.del(mode, lhs) - 新API
int SystemAPI::lua_keymap_del(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* mode = lua_tostring(L, 1);
    const char* lhs = lua_tostring(L, 2);
    if (!mode || !lhs) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool result = lua_api->delKeymap(std::string(mode), std::string(lhs));
    lua_pushboolean(L, result);
    return 1;
}

// vim.api.create_autocmd(event, opts, callback) - 新API
int SystemAPI::lua_api_create_autocmd(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 事件可以是字符串或表（多个事件）
    std::vector<std::string> events;
    if (lua_isstring(L, 1)) {
        events.push_back(lua_tostring(L, 1));
    } else if (lua_istable(L, 1)) {
        int len = static_cast<int>(luaL_len(L, 1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, 1, i);
            if (lua_isstring(L, -1)) {
                events.push_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    } else {
        lua_pushboolean(L, false);
        return 1;
    }

    if (events.empty()) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 第二个参数是选项表，第三个参数是回调函数
    int opts_index = 2;
    int callback_index = 3;

    // 检查参数顺序：可能是 (event, callback) 或 (event, opts, callback)
    if (!lua_istable(L, 2) && lua_isfunction(L, 2)) {
        // 只有两个参数：event和callback
        callback_index = 2;
        opts_index = -1;
    }

    if (!lua_isfunction(L, callback_index)) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 创建函数引用
    lua_pushvalue(L, callback_index);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // 解析选项
    std::string pattern = "";
    bool once = false;
    bool nested = false;
    std::string desc = "";
    std::string group = "";

    if (opts_index > 0 && lua_istable(L, opts_index)) {
        parseAutocmdOptions(L, opts_index, pattern, once, nested, desc, group);
    }

    // 为每个事件注册
    for (const auto& event : events) {
        lua_api->registerAutocmd(event, ref, pattern, once, nested, desc, group);
    }

    lua_pushboolean(L, true);
    return 1;
}

// vim.api.clear_autocmds(opts) - 新API
int SystemAPI::lua_api_clear_autocmds(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        return 0;
    }

    std::string event = "";
    std::string pattern = "";
    std::string group = "";

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "event");
        if (lua_isstring(L, -1)) {
            event = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "pattern");
        if (lua_isstring(L, -1)) {
            pattern = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "group");
        if (lua_isstring(L, -1)) {
            group = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }

    lua_api->clearAutocmds(event, pattern, group);
    return 0;
}

// 解析命令选项
void SystemAPI::parseCommandOptions(lua_State* L, int opts_index, std::string& nargs,
                                    std::string& desc, bool& force) {
    lua_getfield(L, opts_index, "nargs");
    if (lua_isstring(L, -1)) {
        nargs = lua_tostring(L, -1);
    } else if (lua_isnumber(L, -1)) {
        int n = static_cast<int>(lua_tointeger(L, -1));
        nargs = std::to_string(n);
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "desc");
    if (lua_isstring(L, -1)) {
        desc = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "force");
    if (lua_isboolean(L, -1)) {
        force = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);
}

// 解析键映射选项
void SystemAPI::parseKeymapOptions(lua_State* L, int opts_index, bool& noremap, bool& silent,
                                   bool& expr, bool& nowait, std::string& desc) {
    lua_getfield(L, opts_index, "noremap");
    if (lua_isboolean(L, -1)) {
        noremap = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "silent");
    if (lua_isboolean(L, -1)) {
        silent = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "expr");
    if (lua_isboolean(L, -1)) {
        expr = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "nowait");
    if (lua_isboolean(L, -1)) {
        nowait = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "desc");
    if (lua_isstring(L, -1)) {
        desc = lua_tostring(L, -1);
    }
    lua_pop(L, 1);
}

// 解析autocmd选项
void SystemAPI::parseAutocmdOptions(lua_State* L, int opts_index, std::string& pattern, bool& once,
                                    bool& nested, std::string& desc, std::string& group) {
    lua_getfield(L, opts_index, "pattern");
    if (lua_isstring(L, -1)) {
        pattern = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "once");
    if (lua_isboolean(L, -1)) {
        once = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "nested");
    if (lua_isboolean(L, -1)) {
        nested = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "desc");
    if (lua_isstring(L, -1)) {
        desc = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, opts_index, "group");
    if (lua_isstring(L, -1)) {
        group = lua_tostring(L, -1);
    }
    lua_pop(L, 1);
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
