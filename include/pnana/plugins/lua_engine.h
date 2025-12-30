#ifndef PNANA_PLUGINS_LUA_ENGINE_H
#define PNANA_PLUGINS_LUA_ENGINE_H

#ifdef BUILD_LUA_SUPPORT

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace pnana {
namespace plugins {

// Lua 引擎包装类
class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();
    
    // 禁止拷贝
    LuaEngine(const LuaEngine&) = delete;
    LuaEngine& operator=(const LuaEngine&) = delete;
    
    // 获取 Lua 状态
    lua_State* getState() { return L_; }
    
    // 执行 Lua 代码
    bool executeString(const std::string& code);
    
    // 执行 Lua 文件
    bool executeFile(const std::string& filepath);
    
    // 注册 C++ 函数到 Lua
    void registerFunction(const std::string& name, lua_CFunction func);
    
    // 注册全局表
    void createTable(const std::string& name);
    
    // 创建嵌套表（支持点号分隔的路径，如 "vim.api"）
    void createNestedTable(const std::string& path);
    
    // 在表中注册函数（自动创建嵌套表）
    void registerTableFunction(const std::string& table, const std::string& name, lua_CFunction func);
    
    // 设置全局变量
    void setGlobal(const std::string& name, const std::string& value);
    void setGlobal(const std::string& name, int value);
    void setGlobal(const std::string& name, bool value);
    
    // 获取全局变量
    std::string getGlobalString(const std::string& name);
    int getGlobalInt(const std::string& name);
    bool getGlobalBool(const std::string& name);
    
    // 调用 Lua 函数
    bool callFunction(const std::string& funcName, int nargs = 0, int nresults = 0);
    
    // 检查错误
    bool checkError(int result);
    
    // 加载标准库
    void loadStandardLibs();
    
    // 设置路径
    void setPackagePath(const std::string& path);

private:
    lua_State* L_;
    
    // 错误处理
    void handleError(const std::string& context);
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_LUA_ENGINE_H

