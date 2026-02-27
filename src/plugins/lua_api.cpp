#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_api.h"
#include "core/document.h"
#include "core/editor.h"
#include "utils/logger.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

namespace pnana {
namespace plugins {

// 在 Lua 注册表中存储编辑器指针的键
static const char* EDITOR_REGISTRY_KEY = "pnana_editor";
static const char* API_REGISTRY_KEY = "pnana_api";
static const char* LUA_API_REGISTRY_KEY = "pnana_lua_api";

LuaAPI::LuaAPI(core::Editor* editor) : editor_(editor), engine_(nullptr) {
    // 初始化各个API组件
    editor_api_ = std::make_unique<EditorAPI>(editor);
    file_api_ = std::make_unique<FileAPI>(editor);
    theme_api_ = std::make_unique<ThemeAPI>(editor);
    system_api_ = std::make_unique<SystemAPI>();
}

LuaAPI::~LuaAPI() {}

void LuaAPI::initialize(LuaEngine* engine) {
    engine_ = engine;
    if (!engine_ || !engine_->getState()) {
        LOG_ERROR("LuaAPI: Engine not initialized");
        return;
    }

    lua_State* L = engine_->getState();

    // 在注册表中存储 API 实例
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, API_REGISTRY_KEY);

    // 在注册表中存储LuaAPI实例（供SystemAPI使用）
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, LUA_API_REGISTRY_KEY);

    // 在注册表中存储编辑器指针
    lua_pushlightuserdata(L, editor_);
    lua_setfield(L, LUA_REGISTRYINDEX, EDITOR_REGISTRY_KEY);

    // 创建 vim 全局表
    engine_->createTable("vim");

    // 创建嵌套表（自动处理）
    engine_->createNestedTable("vim.api");
    engine_->createNestedTable("vim.fn");

    // 设置SystemAPI的LuaAPI引用
    system_api_->setLuaAPI(this);

    // 初始化各个API组件
    editor_api_->registerFunctions(L);
    file_api_->registerFunctions(L);
    theme_api_->registerFunctions(L);
    system_api_->registerFunctions(L);
}

core::Editor* LuaAPI::getEditorFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, EDITOR_REGISTRY_KEY);
    core::Editor* editor = static_cast<core::Editor*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return editor;
}

LuaAPI* LuaAPI::getAPIFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, API_REGISTRY_KEY);
    LuaAPI* api = static_cast<LuaAPI*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return api;
}

void LuaAPI::triggerEvent(const std::string& event, const std::vector<std::string>& args) {
    LOG("LuaAPI::triggerEvent: triggering event '" + event + "' with " +
        std::to_string(args.size()) + " args");

    if (!engine_) {
        LOG("LuaAPI::triggerEvent: engine is null");
        return;
    }

    lua_State* L = engine_->getState();

    // 提取文件路径（如果有）
    std::string filepath = args.empty() ? "" : args[0];

    // 处理新API的autocmd
    auto autocmd_it = autocmds_.find(event);
    if (autocmd_it != autocmds_.end()) {
        std::vector<int> to_remove; // 标记需要删除的once事件

        for (size_t i = 0; i < autocmd_it->second.size(); ++i) {
            const auto& info = autocmd_it->second[i];

            // 检查pattern匹配（简化实现，支持通配符*）
            bool pattern_match = true;
            if (!info.pattern.empty() && !filepath.empty()) {
                // 简单的通配符匹配
                std::string pattern = info.pattern;
                if (pattern.find("*") != std::string::npos) {
                    // 提取扩展名
                    size_t dot_pos = filepath.find_last_of(".");
                    if (dot_pos != std::string::npos) {
                        std::string ext = filepath.substr(dot_pos);
                        pattern_match = (pattern == "*" + ext || pattern == ext);
                    } else {
                        pattern_match = false;
                    }
                } else {
                    pattern_match = (filepath.find(pattern) != std::string::npos);
                }
            }

            if (!pattern_match) {
                continue;
            }

            // 调用回调函数
            lua_rawgeti(L, LUA_REGISTRYINDEX, info.callback_ref);
            if (lua_isfunction(L, -1)) {
                // 创建event表（新API格式）
                lua_newtable(L);
                lua_pushstring(L, event.c_str());
                lua_setfield(L, -2, "event");
                if (!filepath.empty()) {
                    lua_pushstring(L, filepath.c_str());
                    lua_setfield(L, -2, "file");
                }

                // 调用函数
                int result = lua_pcall(L, 1, 0, 0);
                if (result != LUA_OK) {
                    const char* error = lua_tostring(L, -1);
                    LOG_ERROR("Autocmd callback error: " + std::string(error));
                    lua_pop(L, 1);
                }

                // 如果是once事件，标记删除
                if (info.once) {
                    to_remove.push_back(static_cast<int>(i));
                }
            } else {
                lua_pop(L, 1);
            }
        }

        // 删除once事件（从后往前删除，避免索引问题）
        for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
            int index = *it;
            if (index < static_cast<int>(autocmd_it->second.size())) {
                if (engine_ && engine_->getState()) {
                    luaL_unref(engine_->getState(), LUA_REGISTRYINDEX,
                               autocmd_it->second[index].callback_ref);
                }
                autocmd_it->second.erase(autocmd_it->second.begin() + index);
            }
        }

        // 如果列表为空，删除事件
        if (autocmd_it->second.empty()) {
            autocmds_.erase(autocmd_it);
        }
    }

    // 处理旧API的字符串回调（兼容性）
    auto it = event_listeners_.find(event);
    if (it != event_listeners_.end()) {
        for (const auto& callback : it->second) {
            lua_getglobal(L, callback.c_str());
            if (lua_isfunction(L, -1)) {
                // 推送参数（旧格式：直接传字符串）
                for (const auto& arg : args) {
                    lua_pushstring(L, arg.c_str());
                }

                int result = lua_pcall(L, static_cast<int>(args.size()), 0, 0);
                if (result != LUA_OK) {
                    const char* error = lua_tostring(L, -1);
                    LOG_ERROR("Event callback error: " + std::string(error));
                    lua_pop(L, 1);
                }
            } else {
                lua_pop(L, 1);
            }
        }
    }

    // 处理旧API的函数引用回调（兼容性）
    auto func_it = event_function_listeners_.find(event);
    if (func_it != event_function_listeners_.end()) {
        for (int ref : func_it->second) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
            if (lua_isfunction(L, -1)) {
                // 推送参数（旧格式：直接传字符串）
                for (const auto& arg : args) {
                    lua_pushstring(L, arg.c_str());
                }

                int result = lua_pcall(L, static_cast<int>(args.size()), 0, 0);
                if (result != LUA_OK) {
                    const char* error = lua_tostring(L, -1);
                    LOG_ERROR("Event function callback error: " + std::string(error));
                    lua_pop(L, 1);
                }
            } else {
                lua_pop(L, 1);
            }
        }
    }
}

void LuaAPI::registerEventListener(const std::string& event, const std::string& callback) {
    event_listeners_[event].push_back(callback);
}

void LuaAPI::registerEventListenerFunction(const std::string& event) {
    if (!engine_ || !engine_->getState()) {
        return;
    }

    lua_State* L = engine_->getState();
    // 函数在栈顶（位置 2），复制到栈顶
    lua_pushvalue(L, 2);
    // 创建引用
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);
    event_function_listeners_[event].push_back(ref);
}

void LuaAPI::registerCommand(const std::string& name, const std::string& callback) {
    commands_[name] = callback;
}

void LuaAPI::registerKeymap(const std::string& mode, const std::string& keys,
                            const std::string& callback) {
    keymaps_[mode][keys] = callback;
}

// 新API实现

void LuaAPI::registerUserCommand(const std::string& name, int callback_ref,
                                 const std::string& nargs, const std::string& desc, bool force) {
    // 检查是否已存在且force=false
    if (user_commands_.find(name) != user_commands_.end() && !force) {
        LOG_ERROR("Command '" + name + "' already exists. Use force=true to override.");
        return;
    }

    UserCommandInfo info;
    info.callback_ref = callback_ref;
    info.nargs = nargs;
    info.desc = desc;
    info.force = force;
    user_commands_[name] = info;
}

bool LuaAPI::delUserCommand(const std::string& name) {
    auto it = user_commands_.find(name);
    if (it != user_commands_.end()) {
        // 释放Lua函数引用
        if (engine_ && engine_->getState()) {
            luaL_unref(engine_->getState(), LUA_REGISTRYINDEX, it->second.callback_ref);
        }
        user_commands_.erase(it);
        return true;
    }

    // 也检查旧API的命令
    auto old_it = commands_.find(name);
    if (old_it != commands_.end()) {
        commands_.erase(old_it);
        return true;
    }

    return false;
}

bool LuaAPI::executeCommand(const std::string& name, const std::string& args) {
    if (!engine_ || !engine_->getState()) {
        return false;
    }

    lua_State* L = engine_->getState();

    // 首先检查新API的命令
    auto it = user_commands_.find(name);
    if (it != user_commands_.end()) {
        // 获取函数引用
        lua_rawgeti(L, LUA_REGISTRYINDEX, it->second.callback_ref);
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            return false;
        }

        // 创建opts表
        lua_newtable(L);
        lua_pushstring(L, args.c_str());
        lua_setfield(L, -2, "args");

        // 解析参数到fargs数组（简化实现）
        lua_newtable(L);
        if (!args.empty()) {
            std::istringstream iss(args);
            std::string arg;
            int index = 1;
            while (iss >> arg) {
                lua_pushstring(L, arg.c_str());
                lua_rawseti(L, -2, index++);
            }
        }
        lua_setfield(L, -2, "fargs");

        // 调用函数
        int result = lua_pcall(L, 1, 0, 0);
        if (result != LUA_OK) {
            const char* error = lua_tostring(L, -1);
            LOG_ERROR("Command execution error: " + std::string(error));
            lua_pop(L, 1);
            return false;
        }
        return true;
    }

    // 检查旧API的命令
    auto old_it = commands_.find(name);
    if (old_it != commands_.end()) {
        lua_getglobal(L, old_it->second.c_str());
        if (lua_isfunction(L, -1)) {
            int result = lua_pcall(L, 0, 0, 0);
            if (result != LUA_OK) {
                const char* error = lua_tostring(L, -1);
                LOG_ERROR("Command execution error: " + std::string(error));
                lua_pop(L, 1);
                return false;
            }
            return true;
        } else {
            lua_pop(L, 1);
        }
    }

    return false;
}

void LuaAPI::registerKeymap(const std::string& mode, const std::string& lhs, int rhs_ref,
                            bool noremap, bool silent, bool expr, bool nowait,
                            const std::string& desc) {
    KeymapInfo info;
    info.rhs_ref = rhs_ref;
    info.rhs_string = "";
    info.noremap = noremap;
    info.silent = silent;
    info.expr = expr;
    info.nowait = nowait;
    info.desc = desc;
    keymaps_info_[mode][lhs] = info;
}

void LuaAPI::registerKeymap(const std::string& mode, const std::string& lhs,
                            const std::string& rhs_string, bool noremap, bool silent, bool expr,
                            bool nowait, const std::string& desc) {
    KeymapInfo info;
    info.rhs_ref = -1;
    info.rhs_string = rhs_string;
    info.noremap = noremap;
    info.silent = silent;
    info.expr = expr;
    info.nowait = nowait;
    info.desc = desc;
    keymaps_info_[mode][lhs] = info;
}

bool LuaAPI::delKeymap(const std::string& mode, const std::string& lhs) {
    auto mode_it = keymaps_info_.find(mode);
    if (mode_it != keymaps_info_.end()) {
        auto lhs_it = mode_it->second.find(lhs);
        if (lhs_it != mode_it->second.end()) {
            // 释放Lua函数引用
            if (lhs_it->second.rhs_ref != -1 && engine_ && engine_->getState()) {
                luaL_unref(engine_->getState(), LUA_REGISTRYINDEX, lhs_it->second.rhs_ref);
            }
            mode_it->second.erase(lhs_it);
            return true;
        }
    }

    // 也检查旧API的键映射
    auto old_mode_it = keymaps_.find(mode);
    if (old_mode_it != keymaps_.end()) {
        auto old_lhs_it = old_mode_it->second.find(lhs);
        if (old_lhs_it != old_mode_it->second.end()) {
            old_mode_it->second.erase(old_lhs_it);
            return true;
        }
    }

    return false;
}

bool LuaAPI::executeKeymap(const std::string& mode, const std::string& lhs) {
    if (!engine_ || !engine_->getState()) {
        return false;
    }

    lua_State* L = engine_->getState();

    // 首先检查新API的键映射
    auto mode_it = keymaps_info_.find(mode);
    if (mode_it != keymaps_info_.end()) {
        auto lhs_it = mode_it->second.find(lhs);
        if (lhs_it != mode_it->second.end()) {
            const auto& info = lhs_it->second;

            if (info.rhs_ref != -1) {
                // 函数引用
                lua_rawgeti(L, LUA_REGISTRYINDEX, info.rhs_ref);
                if (lua_isfunction(L, -1)) {
                    int result = lua_pcall(L, 0, 0, 0);
                    if (result != LUA_OK) {
                        const char* error = lua_tostring(L, -1);
                        LOG_ERROR("Keymap execution error: " + std::string(error));
                        lua_pop(L, 1);
                        return false;
                    }
                    return true;
                } else {
                    lua_pop(L, 1);
                }
            } else if (!info.rhs_string.empty()) {
                // 字符串rhs - 可能是命令或键序列
                // 如果以:开头，是命令
                if (info.rhs_string[0] == ':') {
                    // 提取命令（去掉:和<CR>）
                    std::string cmd = info.rhs_string.substr(1);
                    size_t cr_pos = cmd.find("<CR>");
                    if (cr_pos != std::string::npos) {
                        cmd = cmd.substr(0, cr_pos);
                    }
                    // 执行命令
                    return executeCommand(cmd, "");
                } else {
                    // 键序列 - 需要模拟按键（简化实现：通过Lua执行）
                    std::string code = "vim.api.keymap_execute('" + info.rhs_string + "')";
                    if (engine_ && engine_->executeString(code)) {
                        return true;
                    }
                }
            }
        }
    }

    // 检查旧API的键映射
    auto old_mode_it = keymaps_.find(mode);
    if (old_mode_it != keymaps_.end()) {
        auto old_lhs_it = old_mode_it->second.find(lhs);
        if (old_lhs_it != old_mode_it->second.end()) {
            lua_getglobal(L, old_lhs_it->second.c_str());
            if (lua_isfunction(L, -1)) {
                int result = lua_pcall(L, 0, 0, 0);
                if (result != LUA_OK) {
                    const char* error = lua_tostring(L, -1);
                    LOG_ERROR("Keymap execution error: " + std::string(error));
                    lua_pop(L, 1);
                    return false;
                }
                return true;
            } else {
                lua_pop(L, 1);
            }
        }
    }

    return false;
}

void LuaAPI::registerAutocmd(const std::string& event, int callback_ref, const std::string& pattern,
                             bool once, bool nested, const std::string& desc,
                             const std::string& group) {
    AutocmdInfo info;
    info.callback_ref = callback_ref;
    info.pattern = pattern;
    info.once = once;
    info.nested = nested;
    info.group = group;
    // desc is stored for future use (e.g., debugging, documentation)
    (void)desc; // Suppress unused parameter warning
    autocmds_[event].push_back(info);
}

void LuaAPI::clearAutocmds(const std::string& event, const std::string& pattern,
                           const std::string& group) {
    if (event.empty()) {
        // 清除所有事件
        for (auto& [evt, infos] : autocmds_) {
            for (auto& info : infos) {
                if (engine_ && engine_->getState()) {
                    luaL_unref(engine_->getState(), LUA_REGISTRYINDEX, info.callback_ref);
                }
            }
        }
        autocmds_.clear();
        return;
    }

    auto it = autocmds_.find(event);
    if (it != autocmds_.end()) {
        auto& infos = it->second;
        auto new_end = std::remove_if(infos.begin(), infos.end(), [&](const AutocmdInfo& info) {
            bool match = true;
            if (!pattern.empty() && info.pattern != pattern) {
                match = false;
            }
            if (!group.empty() && info.group != group) {
                match = false;
            }
            if (match && engine_ && engine_->getState()) {
                luaL_unref(engine_->getState(), LUA_REGISTRYINDEX, info.callback_ref);
            }
            return match;
        });
        infos.erase(new_end, infos.end());

        if (infos.empty()) {
            autocmds_.erase(it);
        }
    }
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
