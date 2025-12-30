#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_engine.h"
#include "utils/logger.h"
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
    if (!L_) return;
    
    luaL_openlibs(L_);
    
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
    if (!L_) return;
    
    lua_register(L_, name.c_str(), func);
}

void LuaEngine::createTable(const std::string& name) {
    if (!L_) return;
    
    lua_newtable(L_);
    lua_setglobal(L_, name.c_str());
}

void LuaEngine::createNestedTable(const std::string& path) {
    if (!L_) return;
    
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
            lua_pushvalue(L_, -1);  // 复制新表
            lua_setfield(L_, -3, parts[i].c_str());  // 设置到父表
        }
        // 现在栈顶是当前层级的表
    }
    
    // 清理栈（只保留最后一个表在栈上，但这里我们不需要）
    lua_pop(L_, 1);
}

void LuaEngine::registerTableFunction(const std::string& table, const std::string& name, lua_CFunction func) {
    if (!L_) return;
    
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
            lua_pop(L_, 2);  // 清理栈
            return;
        }
        lua_remove(L_, -2);  // 移除父表，保留当前表
    }
    
    // 现在栈顶是目标表，注册函数
    lua_pushstring(L_, name.c_str());
    lua_pushcfunction(L_, func);
    lua_settable(L_, -3);
    
    lua_pop(L_, 1);
}

void LuaEngine::setGlobal(const std::string& name, const std::string& value) {
    if (!L_) return;
    
    lua_pushstring(L_, value.c_str());
    lua_setglobal(L_, name.c_str());
}

void LuaEngine::setGlobal(const std::string& name, int value) {
    if (!L_) return;
    
    lua_pushinteger(L_, value);
    lua_setglobal(L_, name.c_str());
}

void LuaEngine::setGlobal(const std::string& name, bool value) {
    if (!L_) return;
    
    lua_pushboolean(L_, value ? 1 : 0);
    lua_setglobal(L_, name.c_str());
}

std::string LuaEngine::getGlobalString(const std::string& name) {
    if (!L_) return "";
    
    lua_getglobal(L_, name.c_str());
    if (lua_isstring(L_, -1)) {
        std::string result = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return result;
    }
    lua_pop(L_, 1);
    return "";
}

int LuaEngine::getGlobalInt(const std::string& name) {
    if (!L_) return 0;
    
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
    if (!L_) return false;
    
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
    if (!L_) return false;
    
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
    if (!L_) return;
    
    // 获取 package.path
    lua_getglobal(L_, "package");
    lua_getfield(L_, -1, "path");
    
    std::string current_path = lua_tostring(L_, -1);
    lua_pop(L_, 1);
    
    // 添加新路径
    std::string new_path = current_path + ";" + path;
    lua_pushstring(L_, new_path.c_str());
    lua_setfield(L_, -2, "path");
    
    lua_pop(L_, 1);
}

void LuaEngine::handleError(const std::string& context) {
    if (!L_) return;
    
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

