#ifndef PNANA_PLUGINS_PLUGIN_MANAGER_H
#define PNANA_PLUGINS_PLUGIN_MANAGER_H

#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_api.h"
#include "plugins/lua_engine.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace core {
class Editor;
}
namespace plugins {

class PathValidator;

// 沙盒配置
struct SandboxConfig {
    std::vector<std::string> allowed_paths;
    bool allow_system_commands;
    size_t max_memory_mb;
    int max_execution_time_ms;

    SandboxConfig()
        : allow_system_commands(false), max_memory_mb(512), max_execution_time_ms(5000) {}
};

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

    // 更新主题菜单以反映当前可用主题
    void updateThemeMenu();

    // 获取已加载的插件列表
    std::vector<PluginInfo> getLoadedPlugins() const;

    // 获取所有插件列表（包括未加载的）
    std::vector<PluginInfo> getAllPlugins() const;

    // 获取插件信息
    PluginInfo getPluginInfo(const std::string& plugin_name) const;

    // 启用插件
    // save_config: true=保存配置到文件（用户操作时）, false=不保存配置（启动时加载）
    bool enablePlugin(const std::string& plugin_name, bool save_config = true);

    // 禁用插件
    bool disablePlugin(const std::string& plugin_name);

    // 触发事件
    void triggerEvent(const std::string& event, const std::vector<std::string>& args = {});

    // 执行命令（由插件注册）
    bool executeCommand(const std::string& command_name);

    // 处理键位映射（由插件注册）
    bool handleKeymap(const std::string& mode, const std::string& keys);

    // 获取 Lua API
    LuaAPI* getAPI() {
        return lua_api_.get();
    }

  private:
    core::Editor* editor_;
    std::unique_ptr<LuaEngine> lua_engine_;
    std::unique_ptr<LuaAPI> lua_api_;
    std::unique_ptr<PathValidator> path_validator_;
    SandboxConfig sandbox_config_;

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

    // 初始化沙盒环境
    void initializeSandbox();
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_PLUGIN_MANAGER_H
