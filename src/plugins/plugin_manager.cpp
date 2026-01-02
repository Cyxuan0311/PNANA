#ifdef BUILD_LUA_SUPPORT

#include "plugins/plugin_manager.h"
#include "core/editor.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace fs = std::filesystem;

namespace pnana {
namespace plugins {

PluginManager::PluginManager(core::Editor* editor)
    : editor_(editor) {
}

PluginManager::~PluginManager() {
    // 清理插件
    for (const auto& [name, info] : plugins_) {
        if (info.loaded) {
            unloadPlugin(name);
        }
    }
}

bool PluginManager::initialize() {
    // 创建 Lua 引擎
    lua_engine_ = std::make_unique<LuaEngine>();
    if (!lua_engine_ || !lua_engine_->getState()) {
        LOG_ERROR("Failed to create Lua engine");
        return false;
    }
    
    // 创建 Lua API
    lua_api_ = std::make_unique<LuaAPI>(editor_);
    if (!lua_api_) {
        LOG_ERROR("Failed to create LuaAPI");
        return false;
    }
    lua_api_->initialize(lua_engine_.get());
    
    // 设置插件路径
    setupPluginPaths();
    
    // 加载插件
    std::string plugin_dir = findPluginDirectory();
    if (!plugin_dir.empty()) {
        loadPlugins(plugin_dir);
    }
    
    return true;
}

void PluginManager::setupPluginPaths() {
    if (!lua_engine_) return;
    
    // 设置 Lua package.path
    std::string plugin_dir = findPluginDirectory();
    if (!plugin_dir.empty()) {
        std::string lua_path = plugin_dir + "/?.lua;" + plugin_dir + "/?/init.lua";
        lua_engine_->setPackagePath(lua_path);
    }
    
    // 设置插件运行时路径
    std::string runtime_path = plugin_dir + "/runtime";
    if (fs::exists(runtime_path)) {
        std::string runtime_lua_path = runtime_path + "/?.lua;" + runtime_path + "/?/init.lua";
        lua_engine_->setPackagePath(runtime_lua_path);
    }
}

std::string PluginManager::findPluginDirectory() {
    // 查找插件目录的优先级：
    // 1. ~/.config/pnana/plugins
    // 2. ./plugins
    // 3. ./lua
    
    const char* home = getenv("HOME");
    if (!home) {
        home = ".";
    }
    
    std::vector<std::string> candidates = {
        std::string(home) + "/.config/pnana/plugins",
        "./plugins",
        "./lua",
        "./.pnana/plugins"
    };
    
    for (const auto& dir : candidates) {
        if (fs::exists(dir) && fs::is_directory(dir)) {
            return dir;
        }
    }
    
    // 如果都不存在，创建默认目录
    std::string default_dir = std::string(home) + "/.config/pnana/plugins";
    try {
        fs::create_directories(default_dir);
        return default_dir;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create plugin directory: " + std::string(e.what()));
        return "./plugins";
    }
}

void PluginManager::loadPlugins(const std::string& plugin_dir) {
    if (!fs::exists(plugin_dir) || !fs::is_directory(plugin_dir)) {
        return;
    }
    
    // 遍历插件目录，扫描所有插件
    try {
        for (const auto& entry : fs::directory_iterator(plugin_dir)) {
            if (!entry.is_directory()) {
                continue;
            }
            
            std::string plugin_path = entry.path().string();
            std::string plugin_name = entry.path().filename().string();
            
            // 跳过以 . 开头的目录
            if (plugin_name[0] == '.') {
                continue;
            }
            
            // 注册插件（扫描配置但不加载）
            PluginInfo info;
            info.path = plugin_path;
            info.name = plugin_name;
            info.loaded = false;
            
            // 加载插件配置（只有在 Lua 引擎已初始化时才能读取配置）
            if (lua_engine_ && lua_engine_->getState()) {
                if (!loadPluginConfig(plugin_path, info)) {
                    // 如果没有配置文件或读取失败，使用默认值
                    info.version = "1.0.0";
                    info.description = "No description";
                    info.author = "Unknown";
                }
            } else {
                // Lua 引擎未初始化，使用默认值
                info.version = "1.0.0";
                info.description = "No description";
                info.author = "Unknown";
            }
            
            // 注册插件
            plugins_[info.name] = info;
            plugin_paths_[info.name] = plugin_path;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while scanning plugins: " + std::string(e.what()));
    }
}

bool PluginManager::loadPlugin(const std::string& plugin_path) {
    if (!lua_engine_ || !lua_api_) {
        return false;
    }
    
    PluginInfo info;
    info.path = plugin_path;
    info.name = fs::path(plugin_path).filename().string();
    
    // 如果插件已经注册，使用已有信息
    auto it = plugins_.find(info.name);
    if (it != plugins_.end()) {
        info = it->second;
    } else {
    // 加载插件配置
    if (!loadPluginConfig(plugin_path, info)) {
        // 如果没有配置文件，使用默认值
        info.version = "1.0.0";
        info.description = "No description";
        info.author = "Unknown";
        }
        
        // 注册插件（但不加载）
        plugins_[info.name] = info;
        plugin_paths_[info.name] = plugin_path;
    }
    
    // 如果已经加载，直接返回
    if (info.loaded) {
        return true;
    }
    
    // 执行插件初始化
    if (!executePluginInit(plugin_path)) {
        return false;
    }
    
    // 更新加载状态
    plugins_[info.name].loaded = true;
    
    return true;
}

bool PluginManager::loadPluginConfig(const std::string& plugin_path, PluginInfo& info) {
    if (!lua_engine_ || !lua_engine_->getState()) {
        return false;
    }
    
    // 查找 plugin.lua 或 init.lua
    std::vector<std::string> config_files = {
        plugin_path + "/plugin.lua",
        plugin_path + "/init.lua"
    };
    
    for (const auto& config_file : config_files) {
        if (fs::exists(config_file)) {
            // 执行插件文件
            if (lua_engine_->executeFile(config_file)) {
                // 尝试从全局变量获取插件信息
                info.name = lua_engine_->getGlobalString("plugin_name");
                if (info.name.empty()) {
                    info.name = fs::path(plugin_path).filename().string();
                }
                
                info.version = lua_engine_->getGlobalString("plugin_version");
                if (info.version.empty()) {
                    info.version = "1.0.0";
                }
                
                info.description = lua_engine_->getGlobalString("plugin_description");
                info.author = lua_engine_->getGlobalString("plugin_author");
                
                return true;
            }
        }
    }
    
    return false;
}

bool PluginManager::executePluginInit(const std::string& plugin_path) {
    // 查找 init.lua
    std::string init_file = plugin_path + "/init.lua";
    if (fs::exists(init_file)) {
        return lua_engine_->executeFile(init_file);
    }
    
    // 如果没有 init.lua，查找 plugin.lua
    std::string plugin_file = plugin_path + "/plugin.lua";
    if (fs::exists(plugin_file)) {
        return lua_engine_->executeFile(plugin_file);
    }
    
    // 查找 lua/ 子目录
    std::string lua_dir = plugin_path + "/lua";
    if (fs::exists(lua_dir) && fs::is_directory(lua_dir)) {
        int file_count = 0;
        // 加载 lua 目录下的所有 .lua 文件
        for (const auto& entry : fs::recursive_directory_iterator(lua_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                std::string file_path = entry.path().string();
                file_count++;
                if (!lua_engine_->executeFile(file_path)) {
                    LOG_WARNING("Failed to load plugin file: " + file_path);
                }
            }
        }
        return file_count > 0;
    }
    
    return false;
}

bool PluginManager::unloadPlugin(const std::string& plugin_name) {
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end() || !it->second.loaded) {
        return false;
    }
    
    // 触发卸载事件
    triggerEvent("PluginUnload", {plugin_name});
    
    it->second.loaded = false;
    return true;
}

bool PluginManager::reloadPlugin(const std::string& plugin_name) {
    auto it = plugin_paths_.find(plugin_name);
    if (it == plugin_paths_.end()) {
        return false;
    }
    
    // 先卸载
    unloadPlugin(plugin_name);
    
    // 再加载
    return loadPlugin(it->second);
}

std::vector<PluginInfo> PluginManager::getLoadedPlugins() const {
    std::vector<PluginInfo> result;
    for (const auto& [name, info] : plugins_) {
        if (info.loaded) {
            result.push_back(info);
        }
    }
    return result;
}

std::vector<PluginInfo> PluginManager::getAllPlugins() const {
    std::vector<PluginInfo> result;
    for (const auto& [name, info] : plugins_) {
        result.push_back(info);
    }
    return result;
}

PluginInfo PluginManager::getPluginInfo(const std::string& plugin_name) const {
    auto it = plugins_.find(plugin_name);
    if (it != plugins_.end()) {
        return it->second;
    }
    return PluginInfo();
}

bool PluginManager::enablePlugin(const std::string& plugin_name) {
    auto it = plugin_paths_.find(plugin_name);
    if (it == plugin_paths_.end()) {
        return false;
    }
    
    // 如果已经加载，直接返回成功
    auto plugin_it = plugins_.find(plugin_name);
    if (plugin_it != plugins_.end() && plugin_it->second.loaded) {
        return true;
    }
    
    // 加载插件
    return loadPlugin(it->second);
}

bool PluginManager::disablePlugin(const std::string& plugin_name) {
    return unloadPlugin(plugin_name);
}

void PluginManager::triggerEvent(const std::string& event, const std::vector<std::string>& args) {
    if (lua_api_) {
        lua_api_->triggerEvent(event, args);
    }
}

bool PluginManager::executeCommand(const std::string& command_name) {
    if (!lua_api_) {
        return false;
    }
    
    // 查找命令
    // 这里需要通过 Lua API 查找并执行
    // 简化实现：直接调用 Lua 函数
    if (lua_engine_) {
        std::string code = "if pnana_commands and pnana_commands['" + command_name + "'] then "
                          "pnana_commands['" + command_name + "']() "
                          "end";
        return lua_engine_->executeString(code);
    }
    
    return false;
}

bool PluginManager::handleKeymap(const std::string& mode, const std::string& keys) {
    if (!lua_api_ || !lua_engine_) {
        return false;
    }
    
    // 查找键位映射
    std::string code = "if pnana_keymaps and pnana_keymaps['" + mode + "'] and "
                      "pnana_keymaps['" + mode + "']['" + keys + "'] then "
                      "pnana_keymaps['" + mode + "']['" + keys + "']() "
                      "return true "
                      "end "
                      "return false";
    
    lua_engine_->executeString(code);
    bool result = lua_engine_->getGlobalBool("keymap_result");
    return result;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

