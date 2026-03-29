#ifdef BUILD_LUA_SUPPORT

#include "plugins/system_api.h"
#include "core/editor.h"
#include "plugins/lua_api.h"
#include "utils/logger.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <lua.hpp>
#include <memory>
#include <sstream>
#include <vector>

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

    // 注册到 vim.fn 表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "fn");

    lua_pushcfunction(L, lua_fn_system);
    lua_setfield(L, -2, "system");
    lua_pushcfunction(L, lua_fn_systemlist);
    lua_setfield(L, -2, "systemlist");
    lua_pushcfunction(L, lua_fn_systemlist_async);
    lua_setfield(L, -2, "systemlist_async");
    lua_pushcfunction(L, lua_fn_hrtime);
    lua_setfield(L, -2, "hrtime");

    lua_pop(L, 2); // 弹出 vim 和 fn 表

    // 注册全局函数
    lua_pushcfunction(L, lua_fn_notify);
    lua_setglobal(L, "pnana_notify");

    // 注册新 API 到 vim.api 表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "api");

    lua_pushcfunction(L, lua_fn_create_user_command);
    lua_setfield(L, -2, "create_user_command");

    lua_pushcfunction(L, lua_fn_del_user_command);
    lua_setfield(L, -2, "del_user_command");

    lua_pushcfunction(L, lua_fn_register_palette_command);
    lua_setfield(L, -2, "register_palette_command");

    lua_pushcfunction(L, lua_fn_create_autocmd);
    lua_setfield(L, -2, "create_autocmd");

    lua_pushcfunction(L, lua_fn_clear_autocmds);
    lua_setfield(L, -2, "clear_autocmds");

    lua_pop(L, 2); // 弹出vim和api表

    // 注册 vim.log（插件日志 API）
    lua_getglobal(L, "vim");
    lua_newtable(L);
    lua_pushcfunction(L, lua_log_info);
    lua_setfield(L, -2, "info");
    lua_pushcfunction(L, lua_log_warn);
    lua_setfield(L, -2, "warn");
    lua_pushcfunction(L, lua_log_error);
    lua_setfield(L, -2, "error");
    lua_pushcfunction(L, lua_log_debug);
    lua_setfield(L, -2, "debug");
    lua_setfield(L, -2, "log");

    lua_pushcfunction(L, lua_fn_defer_fn);
    lua_setfield(L, -2, "defer_fn");
    lua_pushcfunction(L, lua_fn_defer_cancel);
    lua_setfield(L, -2, "defer_cancel");

    lua_pop(L, 1); // 弹出 vim 表

    // 注册 vim.keymap 表
    lua_getglobal(L, "vim");
    lua_newtable(L);
    lua_pushcfunction(L, lua_fn_keymap_set);
    lua_setfield(L, -2, "set");
    lua_pushcfunction(L, lua_fn_keymap_del);
    lua_setfield(L, -2, "del");
    lua_setfield(L, -2, "keymap");
    lua_pop(L, 1); // 弹出 vim 表

    // 创建便捷别名（类似 Neovim）- 旧 API 兼容层
    // 直接在 Lua 栈上设置，更可靠
    lua_getglobal(L, "vim");
    lua_getglobal(L, "pnana_notify");
    lua_setfield(L, -2, "notify");
    lua_getglobal(L, "pnana_command");
    lua_setfield(L, -2, "cmd");
    // 注意：不覆盖 vim.keymap，保持上面的表结构
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
// 沙盒模式：禁用 system 字符串命令执行，建议使用 vim.fn.systemlist({"rg", ...}, opts)
int SystemAPI::lua_fn_system(lua_State* L) {
    const char* command = lua_tostring(L, 1);
    if (!command) {
        lua_pushnil(L);
        lua_pushstring(L, "System command execution is disabled in sandbox mode");
        return 2;
    }

    LOG_WARNING("Plugin attempted to execute system command: " + std::string(command) +
                " (blocked by sandbox, use vim.fn.systemlist)");

    lua_pushnil(L);
    lua_pushstring(L, "System command execution is disabled in sandbox mode. "
                      "Use vim.fn.systemlist({\"rg\", ...}, opts)");
    return 2;
}

// vim.fn.systemlist(argv, opts) -> {lines} | nil, error
// 安全执行：仅允许 rg，并限制 cwd/超时/输出大小
int SystemAPI::lua_fn_systemlist(lua_State* L) {
    auto start_time = std::chrono::high_resolution_clock::now();

    if (!lua_istable(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "argv must be a table");
        return 2;
    }

    std::vector<std::string> argv;
    int argc = static_cast<int>(luaL_len(L, 1));
    for (int i = 1; i <= argc; ++i) {
        lua_rawgeti(L, 1, i);
        if (lua_isstring(L, -1)) {
            argv.emplace_back(lua_tostring(L, -1));
        }
        lua_pop(L, 1);
    }

    if (argv.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, "argv is empty");
        return 2;
    }

    const std::string exe = argv.front();

    if (!(exe == "rg" || exe == "ripgrep")) {
        LOG_WARNING("[vim.fn.systemlist] Blocked executable: " + exe);
        lua_pushnil(L);
        lua_pushstring(L, "only rg/ripgrep is allowed");
        return 2;
    }

    std::string cwd = ".";
    int timeout_ms = 800;
    size_t max_output_bytes = 1024 * 1024; // 1MB

    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "cwd");
        if (lua_isstring(L, -1)) {
            cwd = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "timeout_ms");
        if (lua_isnumber(L, -1)) {
            timeout_ms = static_cast<int>(lua_tointeger(L, -1));
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "max_output_bytes");
        if (lua_isnumber(L, -1)) {
            long long v = static_cast<long long>(lua_tointeger(L, -1));
            if (v > 0) {
                max_output_bytes = static_cast<size_t>(v);
            }
        }
        lua_pop(L, 1);
    }

    if (timeout_ms < 50)
        timeout_ms = 50;
    if (timeout_ms > 10000)
        timeout_ms = 10000;
    if (max_output_bytes < 1024)
        max_output_bytes = 1024;
    if (max_output_bytes > 4 * 1024 * 1024)
        max_output_bytes = 4 * 1024 * 1024;

    // 构建命令行字符串用于日志
    std::string cmd_str;
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i > 0)
            cmd_str += " ";
        cmd_str += argv[i];
    }

    LOG_DEBUG("[vim.fn.systemlist] EXEC: " + cmd_str + " cwd=" + cwd +
              " timeout=" + std::to_string(timeout_ms) + "ms");
    auto exec_start = std::chrono::high_resolution_clock::now();

    namespace fs = std::filesystem;
    std::string effective_cwd = cwd;
    try {
        fs::path p(cwd);
        if (!fs::exists(p) || !fs::is_directory(p)) {
            lua_pushnil(L);
            lua_pushstring(L, "cwd does not exist or is not a directory");
            return 2;
        }
        effective_cwd = fs::weakly_canonical(p).string();
    } catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
    }

    // 将参数安全拼接为 shell 命令（单引号转义）
    auto shell_escape = [](const std::string& s) {
        std::string out;
        out.reserve(s.size() + 8);
        out.push_back('\'');
        for (char c : s) {
            if (c == '\'') {
                out += "'\\''";
            } else {
                out.push_back(c);
            }
        }
        out.push_back('\'');
        return out;
    };

    std::string full_cmd = "cd " + shell_escape(effective_cwd) + " && ";
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i > 0)
            full_cmd += " ";
        full_cmd += shell_escape(argv[i]);
    }

    // 限制输出大小：重定向到临时文件再读取限定字节
    std::string tmp_file =
        "/tmp/pnana_rg_" + std::to_string(std::rand()) + "_" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".out";
    std::string cmd_with_redirect = full_cmd + " > " + shell_escape(tmp_file) + " 2>&1";

    auto runner = [cmd_with_redirect]() {
        return std::system(cmd_with_redirect.c_str());
    };

    auto fut = std::async(std::launch::async, runner);
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status::ready) {
        std::remove(tmp_file.c_str());
        LOG_DEBUG("[vim.fn.systemlist] TIMEOUT after " + std::to_string(timeout_ms) + "ms");
        lua_pushnil(L);
        lua_pushstring(L, "systemlist timeout");
        return 2;
    }

    auto exec_end = std::chrono::high_resolution_clock::now();
    auto exec_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(exec_end - exec_start).count();
    LOG_DEBUG("[vim.fn.systemlist] EXEC COMPLETED in " + std::to_string(exec_ms) + "ms");

    std::ifstream in(tmp_file, std::ios::binary);
    if (!in.is_open()) {
        lua_pushnil(L);
        lua_pushstring(L, "failed to read command output");
        return 2;
    }

    std::string output;
    output.resize(max_output_bytes);
    in.read(&output[0], static_cast<std::streamsize>(max_output_bytes));
    std::streamsize got = in.gcount();
    output.resize(static_cast<size_t>(got));
    in.close();
    std::remove(tmp_file.c_str());

    // 计算行数
    int line_count = 0;
    std::stringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        line_count++;
    }
    LOG_DEBUG("[vim.fn.systemlist] Read " + std::to_string(line_count) + " lines, " +
              std::to_string(output.size()) + " bytes");

    lua_newtable(L);
    std::stringstream ss2(output);
    std::string line2;
    int idx = 1;
    while (std::getline(ss2, line2)) {
        lua_pushstring(L, line2.c_str());
        lua_rawseti(L, -2, idx++);
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(total_end - start_time).count();
    LOG_DEBUG("[vim.fn.systemlist] RETURN: " + std::to_string(idx - 1) +
              " lines, total=" + std::to_string(total_ms) + "ms");

    return 1;
}

// vim.fn.systemlist_async(argv, opts, callback) -> request_id
// 异步非阻塞执行，回调接收 (lines, error)
int SystemAPI::lua_fn_systemlist_async(lua_State* L) {
    if (!lua_istable(L, 1) || !lua_isfunction(L, 3)) {
        lua_pushnil(L);
        lua_pushstring(L, "argv must be a table, callback must be a function");
        return 2;
    }

    // 获取 LuaAPI 实例
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        lua_pushnil(L);
        lua_pushstring(L, "failed to get LuaAPI instance");
        return 2;
    }

    // 解析 argv
    std::vector<std::string> argv;
    int argc = static_cast<int>(luaL_len(L, 1));
    for (int i = 1; i <= argc; ++i) {
        lua_rawgeti(L, 1, i);
        if (lua_isstring(L, -1)) {
            argv.emplace_back(lua_tostring(L, -1));
        }
        lua_pop(L, 1);
    }

    if (argv.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, "argv is empty");
        return 2;
    }

    const std::string exe = argv.front();
    if (!(exe == "rg" || exe == "ripgrep")) {
        LOG_WARNING("[vim.fn.systemlist_async] Blocked executable: " + exe);
        lua_pushnil(L);
        lua_pushstring(L, "only rg/ripgrep is allowed");
        return 2;
    }

    // 解析 opts
    std::string cwd = ".";
    int timeout_ms = 800;
    size_t max_output_bytes = 1024 * 1024;

    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "cwd");
        if (lua_isstring(L, -1)) {
            cwd = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "timeout_ms");
        if (lua_isnumber(L, -1)) {
            timeout_ms = static_cast<int>(lua_tointeger(L, -1));
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "max_output_bytes");
        if (lua_isnumber(L, -1)) {
            long long v = static_cast<long long>(lua_tointeger(L, -1));
            if (v > 0) {
                max_output_bytes = static_cast<size_t>(v);
            }
        }
        lua_pop(L, 1);
    }

    // 创建 callback 引用
    lua_pushvalue(L, 3);
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // 生成 request_id
    static int s_request_counter = 0;
    int request_id = ++s_request_counter;

    // 在后台线程执行
    std::thread([argv, cwd, timeout_ms, max_output_bytes, callback_ref, request_id, lua_api]() {
        // 构建命令
        auto shell_escape = [](const std::string& s) {
            std::string out;
            out.reserve(s.size() + 8);
            out.push_back('\'');
            for (char c : s) {
                if (c == '\'') {
                    out += "'\\''";
                } else {
                    out.push_back(c);
                }
            }
            out.push_back('\'');
            return out;
        };

        std::string full_cmd;
        for (size_t i = 0; i < argv.size(); ++i) {
            if (i > 0)
                full_cmd += " ";
            full_cmd += shell_escape(argv[i]);
        }

        std::string tmp_file =
            "/tmp/pnana_rg_async_" + std::to_string(request_id) + "_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".out";
        std::string cmd_with_redirect = "cd " + shell_escape(cwd) + " && " + full_cmd + " > " +
                                        shell_escape(tmp_file) + " 2>&1";

        [[maybe_unused]] int ret = std::system(cmd_with_redirect.c_str());

        // 读取结果
        std::ifstream in(tmp_file, std::ios::binary);
        std::string output;
        if (in.is_open()) {
            output.resize(max_output_bytes);
            in.read(&output[0], static_cast<std::streamsize>(max_output_bytes));
            std::streamsize got = in.gcount();
            output.resize(static_cast<size_t>(got));
            in.close();
        }
        std::remove(tmp_file.c_str());

        // 使用 Editor::postToMainThread 在主线程调用 Lua 回调
        auto* editor = lua_api->getEditor();
        if (!editor)
            return;

        editor->postToMainThread([callback_ref, output, request_id, lua_api]() {
            lua_State* L = lua_api->getEngine()->getState();
            if (!L)
                return;

            lua_rawgeti(L, LUA_REGISTRYINDEX, callback_ref);
            if (!lua_isfunction(L, -1)) {
                lua_pop(L, 1);
                luaL_unref(L, LUA_REGISTRYINDEX, callback_ref);
                return;
            }

            // 构建 lines table
            lua_newtable(L);
            std::stringstream ss(output);
            std::string line;
            int idx = 1;
            while (std::getline(ss, line)) {
                lua_pushstring(L, line.c_str());
                lua_rawseti(L, -2, idx++);
            }

            // 调用回调：callback(lines, nil)
            if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                const char* error = lua_tostring(L, -1);
                LOG_ERROR("[vim.fn.systemlist_async] Callback error: " +
                          std::string(error ? error : "unknown"));
                lua_pop(L, 1);
            }

            luaL_unref(L, LUA_REGISTRYINDEX, callback_ref);
        });
    }).detach();

    lua_pushinteger(L, static_cast<lua_Integer>(request_id));
    return 1;
}

// pnana_notify(message, level)
int SystemAPI::lua_fn_notify(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (message) {
        LOG("Plugin: " + std::string(message));
    }
    return 0;
}

// vim.api.create_user_command(name, callback, opts) - 新 API
int SystemAPI::lua_fn_create_user_command(lua_State* L) {
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

// vim.api.del_user_command(name) - 新 API
int SystemAPI::lua_fn_del_user_command(lua_State* L) {
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

// vim.api.register_palette_command(opts, callback)
int SystemAPI::lua_fn_register_palette_command(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api) {
        LOG_ERROR("[lua_fn_register_palette_command] lua_api is null");
        lua_pushboolean(L, false);
        return 1;
    }

    if (!lua_istable(L, 1)) {
        LOG_ERROR("[lua_fn_register_palette_command] Argument 1 is not a table");
        lua_pushboolean(L, false);
        return 1;
    }

    if (!lua_isfunction(L, 2)) {
        LOG_ERROR("[lua_fn_register_palette_command] Argument 2 is not a function");
        lua_pushboolean(L, false);
        return 1;
    }

    std::string id;
    std::string name;
    std::string desc;
    bool force = false;
    std::vector<std::string> keywords;

    lua_getfield(L, 1, "id");
    if (lua_isstring(L, -1)) {
        id = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "name");
    if (lua_isstring(L, -1)) {
        name = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "desc");
    if (lua_isstring(L, -1)) {
        desc = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "force");
    if (lua_isboolean(L, -1)) {
        force = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "keywords");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_isstring(L, -1)) {
                keywords.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    if (id.empty()) {
        LOG_ERROR("[lua_fn_register_palette_command] id is empty");
        lua_pushboolean(L, false);
        return 1;
    }

    lua_pushvalue(L, 2);
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_api->registerPaletteCommand(id, name, desc, keywords, callback_ref, force);

    lua_pushboolean(L, true);
    return 1;
}

// vim.keymap.set(mode, lhs, rhs, opts) - 新 API
int SystemAPI::lua_fn_keymap_set(lua_State* L) {
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

// vim.keymap.del(mode, lhs) - 新 API
int SystemAPI::lua_fn_keymap_del(lua_State* L) {
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

// vim.api.create_autocmd(event, opts, callback) - 新 API
int SystemAPI::lua_fn_create_autocmd(lua_State* L) {
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

// vim.api.clear_autocmds(opts) - 新 API
int SystemAPI::lua_fn_clear_autocmds(lua_State* L) {
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

int SystemAPI::lua_fn_defer_fn(lua_State* L) {
    LOG_INFO("[vim.defer_fn] Called");

    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_isfunction(L, 1) || !lua_isnumber(L, 2)) {
        LOG_ERROR("[vim.defer_fn] Invalid arguments");
        lua_pushnil(L);
        lua_pushstring(L, "invalid arguments");
        return 2;
    }

    lua_pushvalue(L, 1);
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    int delay_ms = static_cast<int>(lua_tointeger(L, 2));

    LOG_INFO("[vim.defer_fn] Timer created with delay: " + std::to_string(delay_ms) + "ms");

    int timer_id = lua_api->deferFunction(callback_ref, delay_ms);
    lua_pushinteger(L, static_cast<lua_Integer>(timer_id));
    return 1;
}

int SystemAPI::lua_fn_defer_cancel(lua_State* L) {
    LOG_INFO("[vim.defer_cancel] Called");

    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_isnumber(L, 1)) {
        lua_pushboolean(L, false);
        return 1;
    }

    int timer_id = static_cast<int>(lua_tointeger(L, 1));
    lua_api->cancelDeferred(timer_id);
    LOG_INFO("[vim.defer_cancel] Timer cancelled: " + std::to_string(timer_id));
    lua_pushboolean(L, true);
    return 1;
}

// vim.fn.hrtime() -> nanoseconds (number)
// 返回高精度时间（纳秒），用于性能分析
int SystemAPI::lua_fn_hrtime(lua_State* L) {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    lua_pushinteger(L, static_cast<lua_Integer>(nanoseconds));
    return 1;
}

int SystemAPI::lua_log_info(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (!message) {
        lua_pushboolean(L, false);
        return 1;
    }
    LOG("Plugin: " + std::string(message));
    lua_pushboolean(L, true);
    return 1;
}

int SystemAPI::lua_log_warn(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (!message) {
        lua_pushboolean(L, false);
        return 1;
    }
    LOG_WARNING("Plugin: " + std::string(message));
    lua_pushboolean(L, true);
    return 1;
}

int SystemAPI::lua_log_error(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (!message) {
        lua_pushboolean(L, false);
        return 1;
    }
    LOG_ERROR("Plugin: " + std::string(message));
    lua_pushboolean(L, true);
    return 1;
}

int SystemAPI::lua_log_debug(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (!message) {
        lua_pushboolean(L, false);
        return 1;
    }
    LOG_DEBUG("Plugin: " + std::string(message));
    lua_pushboolean(L, true);
    return 1;
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
