#ifndef PNANA_PLUGINS_PLUGIN_MANAGER_H
#define PNANA_PLUGINS_PLUGIN_MANAGER_H

#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_engine.h"
#include "plugins/lua_api.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace pnana {
namespace core {
    class Editor;
}
namespace plugins {

// 插件信息
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string path;
    bool loaded;
    
    PluginInfo() : loaded(false) {}
};

// 插件管理器
class PluginManager {
public:
    explicit PluginManager(core::Editor* editor);
    ~PluginManager();
    
    // 初始化插件系统
    bool initialize();
    
    // 加载插件目录中的所有插件
    void loadPlugins(const std::string& plugin_dir);
    
    // 加载单个插件
    bool loadPlugin(const std::string& plugin_path);
    
    // 卸载插件
    bool unloadPlugin(const std::string& plugin_name);
    
    // 重新加载插件
    bool reloadPlugin(const std::string& plugin_name);
    
    // 获取已加载的插件列表
    std::vector<PluginInfo> getLoadedPlugins() const;
    
    // 获取插件信息
    PluginInfo getPluginInfo(const std::string& plugin_name) const;
    
    // 触发事件
    void triggerEvent(const std::string& event, const std::vector<std::string>& args = {});
    
    // 执行命令（由插件注册）
    bool executeCommand(const std::string& command_name);
    
    // 处理键位映射（由插件注册）
    bool handleKeymap(const std::string& mode, const std::string& keys);
    
    // 获取 Lua API
    LuaAPI* getAPI() { return lua_api_.get(); }

private:
    core::Editor* editor_;
    std::unique_ptr<LuaEngine> lua_engine_;
    std::unique_ptr<LuaAPI> lua_api_;
    
    // 插件映射: name -> info
    std::map<std::string, PluginInfo> plugins_;
    
    // 插件路径映射: name -> path
    std::map<std::string, std::string> plugin_paths_;
    
    // 加载插件配置
    bool loadPluginConfig(const std::string& plugin_path, PluginInfo& info);
    
    // 执行插件初始化脚本
    bool executePluginInit(const std::string& plugin_path);
    
    // 查找插件目录
    std::string findPluginDirectory();
    
    // 加载内置插件路径
    void setupPluginPaths();
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_PLUGIN_MANAGER_H

