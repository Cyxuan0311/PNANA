#ifdef BUILD_LUA_SUPPORT

#include "plugins/plugin_manager.h"
#include "core/editor.h"
#include "plugins/file_api.h"
#include "plugins/path_validator.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace pnana {
namespace plugins {

PluginManager::PluginManager(core::Editor* editor) : editor_(editor) {}

PluginManager::~PluginManager() {
    // 清理插件
    for (const auto& [name, info] : plugins_) {
        if (info.loaded) {
            unloadPlugin(name);
        }
    }
}

bool PluginManager::initialize() {
    // 初始化沙盒环境
    initializeSandbox();

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

    // 设置路径验证器到 FileAPI
    if (path_validator_ && lua_api_) {
        FileAPI* file_api = lua_api_->getFileAPI();
        if (file_api) {
            file_api->setPathValidator(path_validator_.get());
        }
    }

    // 设置插件路径
    setupPluginPaths();

    // 加载插件
    std::string plugin_dir = findPluginDirectory();
    if (!plugin_dir.empty()) {
        loadPlugins(plugin_dir);

        // 从配置中加载已启用的插件
        if (editor_) {
            auto& config = editor_->getConfigManager().getConfig();
            std::string saved_theme = config.current_theme; // 保存当前主题

            for (const auto& plugin_name : config.plugins.enabled_plugins) {
                if (plugin_paths_.find(plugin_name) != plugin_paths_.end()) {
                    enablePlugin(plugin_name);
                }
            }

            // 重新应用保存的主题（因为插件加载后可能有新的主题可用）
            if (!saved_theme.empty() && saved_theme != "monokai") {
                editor_->setTheme(saved_theme);
            }

            // 更新主题菜单以反映当前真正可用的主题
            updateThemeMenu();
        }
    }

    return true;
}

void PluginManager::setupPluginPaths() {
    if (!lua_engine_)
        return;

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

    std::vector<std::string> candidates = {std::string(home) + "/.config/pnana/plugins",
                                           "./plugins", "./lua", "./.pnana/plugins"};

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
    std::vector<std::string> config_files = {plugin_path + "/plugin.lua",
                                             plugin_path + "/init.lua"};

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
                    // 静默处理单个文件失败
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

void PluginManager::updateThemeMenu() {
    if (!editor_) {
        return;
    }

    // 获取当前真正可用的主题
    std::vector<std::string> available_themes;
    auto& config = editor_->getConfigManager().getConfig();
    if (!config.available_themes.empty()) {
        available_themes = config.available_themes;
    } else {
        // 使用 Theme 类提供的所有可用主题
        available_themes = ::pnana::ui::Theme::getAvailableThemes();
    }

    // 只有当前加载的插件提供的主题才应该被添加到可用主题列表中
    // 首先清除所有自定义主题，然后重新添加当前加载插件的主题
    editor_->getTheme().clearCustomThemes();

    // 获取当前启用的插件列表
    std::vector<std::string> enabled_plugins = config.plugins.enabled_plugins;

    // 重新加载已启用插件的主题
    for (const auto& plugin_name : enabled_plugins) {
        if (plugin_paths_.find(plugin_name) != plugin_paths_.end()) {
            // 先卸载插件（如果已加载）
            unloadPlugin(plugin_name);
            // 然后重新加载插件
            loadPlugin(plugin_paths_[plugin_name]);
        }
    }

    // 现在获取重新加载后的自定义主题
    std::vector<std::string> custom_themes = editor_->getTheme().getCustomThemeNames();
    available_themes.insert(available_themes.end(), custom_themes.begin(), custom_themes.end());

    // 更新主题菜单
    editor_->getThemeMenu().setAvailableThemes(available_themes);
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
    bool success = loadPlugin(it->second);
    if (success && editor_) {
        // 保存到配置
        auto& config_manager = editor_->getConfigManager();
        auto& config = config_manager.getConfig();
        // 检查是否已经在启用列表中
        auto& enabled_plugins = config.plugins.enabled_plugins;
        if (std::find(enabled_plugins.begin(), enabled_plugins.end(), plugin_name) ==
            enabled_plugins.end()) {
            enabled_plugins.push_back(plugin_name);
            // 保存配置
            config_manager.saveConfig();
        }

        // 更新主题菜单
        updateThemeMenu();
    }
    return success;
}

bool PluginManager::disablePlugin(const std::string& plugin_name) {
    bool success = unloadPlugin(plugin_name);
    if (success && editor_) {
        // 从配置中移除
        auto& config_manager = editor_->getConfigManager();
        auto& config = config_manager.getConfig();
        auto& enabled_plugins = config.plugins.enabled_plugins;
        auto it = std::find(enabled_plugins.begin(), enabled_plugins.end(), plugin_name);
        if (it != enabled_plugins.end()) {
            enabled_plugins.erase(it);
            // 保存配置
            config_manager.saveConfig();
        }

        // 更新主题菜单
        updateThemeMenu();
    }
    return success;
}

void PluginManager::triggerEvent(const std::string& event, const std::vector<std::string>& args) {
    LOG("PluginManager::triggerEvent: triggering event '" + event + "' with " +
        std::to_string(args.size()) + " args");
    for (size_t i = 0; i < args.size(); ++i) {
        LOG("PluginManager::triggerEvent: arg[" + std::to_string(i) + "] = '" + args[i] + "'");
    }

    if (lua_api_) {
        lua_api_->triggerEvent(event, args);
    } else {
        LOG("PluginManager::triggerEvent: lua_api_ is null");
    }
}

bool PluginManager::executeCommand(const std::string& command_name) {
    if (!lua_api_) {
        return false;
    }

    // 使用新的命令执行系统
    // 命令参数从command_name中提取（格式：CommandName args）
    std::string name = command_name;
    std::string args = "";

    size_t space_pos = command_name.find(' ');
    if (space_pos != std::string::npos) {
        name = command_name.substr(0, space_pos);
        args = command_name.substr(space_pos + 1);
    }

    return lua_api_->executeCommand(name, args);
}

bool PluginManager::handleKeymap(const std::string& mode, const std::string& keys) {
    if (!lua_api_) {
        return false;
    }

    // 使用新的键映射执行系统
    return lua_api_->executeKeymap(mode, keys);
}

void PluginManager::initializeSandbox() {
    // 创建路径验证器
    path_validator_ = std::make_unique<PathValidator>();

    // 查找插件目录
    std::string plugin_dir = findPluginDirectory();

    // 设置允许的路径
    std::vector<std::string> allowed_paths;
    if (!plugin_dir.empty()) {
        allowed_paths.push_back(plugin_dir);
    }

    // 添加当前工作目录（如果存在）
    try {
        std::string cwd = fs::current_path().string();
        allowed_paths.push_back(cwd);
        path_validator_->setWorkingDirectory(cwd);
    } catch (const std::exception& e) {
        LOG_WARNING("Failed to get current working directory: " + std::string(e.what()));
    }

    // 设置允许的路径
    path_validator_->setAllowedPaths(allowed_paths);

    // 配置沙盒设置
    sandbox_config_.allow_system_commands = false; // 禁用系统命令
    sandbox_config_.max_memory_mb = 512;           // 最大内存512MB
    sandbox_config_.max_execution_time_ms = 5000;  // 最大执行时间5秒

    LOG("Plugin sandbox initialized: " + std::to_string(allowed_paths.size()) + " allowed paths");
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
