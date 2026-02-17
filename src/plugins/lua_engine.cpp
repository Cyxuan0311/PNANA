#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_engine.h"
#include "utils/logger.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

namespace pnana {
namespace plugins {

LuaEngine::LuaEngine() : L_(nullptr) {
    L_ = luaL_newstate();
    if (!L_) {
        LOG_ERROR("Failed to create Lua state");
        return;
    }

    // 加载标准库
    loadStandardLibs();
}

LuaEngine::~LuaEngine() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
}

void LuaEngine::loadStandardLibs() {
    if (!L_)
        return;

    // 沙盒模式：只加载安全的库，移除危险的库
    // 加载基础库（_G）
    luaL_requiref(L_, "_G", luaopen_base, 1);
    lua_pop(L_, 1);

    // 加载数学库
    luaL_requiref(L_, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L_, 1);

    // 加载字符串库
    luaL_requiref(L_, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(L_, 1);

    // 加载表库
    luaL_requiref(L_, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L_, 1);

    // 不加载以下危险的库：
    // - os: 可以执行系统命令、访问环境变量、删除文件等
    // - io: 可以读写任意文件
    // - package: 可以加载动态库（loadlib）
    // - debug: 可以访问和修改内部状态

    // 创建受限的 package 表（仅支持 require 和路径设置，不支持 loadlib）
    lua_newtable(L_);

    // 设置 package.path（用于 require 查找）
    lua_pushstring(L_, "?;?.lua");
    lua_setfield(L_, -2, "path");

    // 设置 package.loaded（用于缓存已加载的模块）
    lua_newtable(L_);
    lua_setfield(L_, -2, "loaded");

    // 实现受限的 require 函数（只允许加载 Lua 文件，不允许 loadlib）
    lua_pushcfunction(L_, [](lua_State* L) -> int {
        const char* modname = luaL_checkstring(L, 1);
        // 获取 package.loaded
        lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_getglobal(L, "package");
            lua_getfield(L, -1, "loaded");
            lua_remove(L, -2);
        }
        lua_getfield(L, -1, modname);
        if (!lua_isnil(L, -1)) {
            // 模块已加载
            lua_remove(L, -2);
            return 1;
        }
        lua_pop(L, 1);

        // 尝试加载模块（简化实现，使用 package.searchers）
        // 这里只实现基本的文件搜索
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        const char* path = lua_tostring(L, -1);
        lua_pop(L, 2);

        if (path) {
            // 简单的路径搜索实现
            std::string modpath = modname;
            std::replace(modpath.begin(), modpath.end(), '.', '/');
            std::string search_path = modpath + ".lua";

            if (luaL_loadfile(L, search_path.c_str()) == LUA_OK) {
                lua_call(L, 0, 1);
                // 将结果存储到 package.loaded
                lua_pushvalue(L, -1);
                lua_setfield(L, -3, modname);
                lua_remove(L, -2);
                return 1;
            }
        }

        lua_pushnil(L);
        lua_pushstring(L, ("module '" + std::string(modname) + "' not found").c_str());
        return 2;
    });
    lua_setfield(L_, -2, "loadlib"); // 禁用 loadlib
    lua_pushnil(L_);
    lua_setfield(L_, -2, "loadlib"); // 确保 loadlib 为 nil

    // 设置全局 require 函数
    lua_getfield(L_, -1, "loaded");
    lua_pushcfunction(L_, [](lua_State* L) -> int {
        const char* modname = luaL_checkstring(L, 1);
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "loaded");
        lua_getfield(L, -1, modname);
        if (!lua_isnil(L, -1)) {
            lua_remove(L, -2);
            lua_remove(L, -2);
            return 1;
        }
        lua_pop(L, 1);
        lua_getfield(L, -1, "path");
        const char* path = lua_tostring(L, -1);
        lua_pop(L, 2);

        if (path) {
            std::string modpath = modname;
            std::replace(modpath.begin(), modpath.end(), '.', '/');
            std::string search_path = modpath + ".lua";

            if (luaL_loadfile(L, search_path.c_str()) == LUA_OK) {
                lua_call(L, 0, 1);
                lua_pushvalue(L, -1);
                lua_getglobal(L, "package");
                lua_getfield(L, -1, "loaded");
                lua_setfield(L, -1, modname);
                lua_pop(L, 2);
                return 1;
            }
        }

        lua_pushnil(L);
        lua_pushstring(L, ("module '" + std::string(modname) + "' not found").c_str());
        return 2;
    });
    lua_setglobal(L_, "require");

    // 将 package 表设置为全局
    lua_setglobal(L_, "package");

    // 设置 package.loaded 到注册表（供后续使用）
    lua_getglobal(L_, "package");
    lua_getfield(L_, -1, "loaded");
    lua_setfield(L_, LUA_REGISTRYINDEX, "_LOADED");
    lua_pop(L_, 1);

    // 重写 print 函数，禁用输出（避免插件输出干扰用户）
    lua_pushcfunction(L_, [](lua_State* L) -> int {
        // 不输出任何内容，直接返回
        // 移除所有参数（print 可能有多个参数）
        int n = lua_gettop(L);
        lua_pop(L, n);
        return 0;
    });
    lua_setglobal(L_, "print");
}

bool LuaEngine::executeString(const std::string& code) {
    if (!L_) {
        LOG_ERROR("Lua state not initialized");
        return false;
    }

    int result = luaL_loadstring(L_, code.c_str());
    if (result != LUA_OK) {
        handleError("loadstring");
        return false;
    }

    result = lua_pcall(L_, 0, LUA_MULTRET, 0);
    return checkError(result);
}

bool LuaEngine::executeFile(const std::string& filepath) {
    if (!L_) {
        LOG_ERROR("Lua state not initialized");
        return false;
    }

    int result = luaL_loadfile(L_, filepath.c_str());
    if (result != LUA_OK) {
        handleError("loadfile: " + filepath);
        return false;
    }

    result = lua_pcall(L_, 0, LUA_MULTRET, 0);
    return checkError(result);
}

void LuaEngine::registerFunction(const std::string& name, lua_CFunction func) {
    if (!L_)
        return;

    lua_register(L_, name.c_str(), func);
}

void LuaEngine::createTable(const std::string& name) {
    if (!L_)
        return;

    lua_newtable(L_);
    lua_setglobal(L_, name.c_str());
}

void LuaEngine::createNestedTable(const std::string& path) {
    if (!L_)
        return;

    // 支持点号分隔的路径，如 "vim.api.fn"
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '.') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }

    if (parts.empty()) {
        return;
    }

    // 创建或获取根表
    lua_getglobal(L_, parts[0].c_str());
    if (lua_isnil(L_, -1)) {
        lua_pop(L_, 1);
        lua_newtable(L_);
        lua_setglobal(L_, parts[0].c_str());
        lua_getglobal(L_, parts[0].c_str());
    }

    // 遍历路径，创建嵌套表
    for (size_t i = 1; i < parts.size(); ++i) {
        lua_getfield(L_, -1, parts[i].c_str());
        if (lua_isnil(L_, -1)) {
            lua_pop(L_, 1);
            lua_newtable(L_);
            lua_pushvalue(L_, -1);                  // 复制新表
            lua_setfield(L_, -3, parts[i].c_str()); // 设置到父表
        }
        // 现在栈顶是当前层级的表
    }

    // 清理栈（只保留最后一个表在栈上，但这里我们不需要）
    lua_pop(L_, 1);
}

void LuaEngine::registerTableFunction(const std::string& table, const std::string& name,
                                      lua_CFunction func) {
    if (!L_)
        return;

    // 如果表路径包含点号，使用嵌套表创建
    if (table.find('.') != std::string::npos) {
        createNestedTable(table);
    }

    // 获取表（支持嵌套路径）
    std::vector<std::string> parts;
    std::string current;
    for (char c : table) {
        if (c == '.') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }

    if (parts.empty()) {
        return;
    }

    // 获取根表
    lua_getglobal(L_, parts[0].c_str());
    if (lua_isnil(L_, -1)) {
        lua_pop(L_, 1);
        createNestedTable(table);
        lua_getglobal(L_, parts[0].c_str());
    }

    // 遍历路径获取目标表
    for (size_t i = 1; i < parts.size(); ++i) {
        lua_getfield(L_, -1, parts[i].c_str());
        if (lua_isnil(L_, -1)) {
            lua_pop(L_, 2); // 清理栈
            return;
        }
        lua_remove(L_, -2); // 移除父表，保留当前表
    }

    // 现在栈顶是目标表，注册函数
    lua_pushstring(L_, name.c_str());
    lua_pushcfunction(L_, func);
    lua_settable(L_, -3);

    lua_pop(L_, 1);
}

void LuaEngine::setGlobal(const std::string& name, const std::string& value) {
    if (!L_)
        return;

    lua_pushstring(L_, value.c_str());
    lua_setglobal(L_, name.c_str());
}

void LuaEngine::setGlobal(const std::string& name, int value) {
    if (!L_)
        return;

    lua_pushinteger(L_, value);
    lua_setglobal(L_, name.c_str());
}

void LuaEngine::setGlobal(const std::string& name, bool value) {
    if (!L_)
        return;

    lua_pushboolean(L_, value ? 1 : 0);
    lua_setglobal(L_, name.c_str());
}

std::string LuaEngine::getGlobalString(const std::string& name) {
    if (!L_)
        return "";

    lua_getglobal(L_, name.c_str());
    if (lua_isstring(L_, -1)) {
        const char* str = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return str ? std::string(str) : "";
    }
    lua_pop(L_, 1);
    return "";
}

int LuaEngine::getGlobalInt(const std::string& name) {
    if (!L_)
        return 0;

    lua_getglobal(L_, name.c_str());
    if (lua_isnumber(L_, -1)) {
        int result = static_cast<int>(lua_tointeger(L_, -1));
        lua_pop(L_, 1);
        return result;
    }
    lua_pop(L_, 1);
    return 0;
}

bool LuaEngine::getGlobalBool(const std::string& name) {
    if (!L_)
        return false;

    lua_getglobal(L_, name.c_str());
    if (lua_isboolean(L_, -1)) {
        bool result = lua_toboolean(L_, -1) != 0;
        lua_pop(L_, 1);
        return result;
    }
    lua_pop(L_, 1);
    return false;
}

bool LuaEngine::callFunction(const std::string& funcName, int nargs, int nresults) {
    if (!L_)
        return false;

    lua_getglobal(L_, funcName.c_str());
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return false;
    }

    // 函数已经在栈上，参数应该在函数下面
    int result = lua_pcall(L_, nargs, nresults, 0);
    return checkError(result);
}

bool LuaEngine::checkError(int result) {
    if (result != LUA_OK) {
        handleError("pcall");
        return false;
    }
    return true;
}

void LuaEngine::setPackagePath(const std::string& path) {
    if (!L_)
        return;

    // 获取 package.path
    lua_getglobal(L_, "package");
    lua_getfield(L_, -1, "path");

    const char* current_path_str = lua_tostring(L_, -1);
    std::string current_path = current_path_str ? std::string(current_path_str) : "";
    lua_pop(L_, 1);

    // 添加新路径
    std::string new_path = current_path + ";" + path;
    lua_pushstring(L_, new_path.c_str());
    lua_setfield(L_, -2, "path");

    lua_pop(L_, 1);
}

void LuaEngine::handleError(const std::string& context) {
    if (!L_)
        return;

    const char* error = lua_tostring(L_, -1);
    if (error) {
        std::string error_msg = "Lua error in " + context + ": " + std::string(error);
        LOG_ERROR(error_msg);
        lua_pop(L_, 1);
    }
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
