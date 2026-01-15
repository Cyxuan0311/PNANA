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

LuaAPI::LuaAPI(core::Editor* editor) : editor_(editor), engine_(nullptr) {}

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

    // 在注册表中存储编辑器指针
    lua_pushlightuserdata(L, editor_);
    lua_setfield(L, LUA_REGISTRYINDEX, EDITOR_REGISTRY_KEY);

    // 创建 vim 全局表
    engine_->createTable("vim");

    // 创建嵌套表（自动处理）
    engine_->createNestedTable("vim.api");
    engine_->createNestedTable("vim.fn");

    // 注册 vim.api 函数（自动创建嵌套表）
    engine_->registerTableFunction("vim.api", "get_current_line", lua_api_get_current_line);
    engine_->registerTableFunction("vim.api", "get_line_count", lua_api_get_line_count);
    engine_->registerTableFunction("vim.api", "get_cursor_pos", lua_api_get_cursor_pos);
    engine_->registerTableFunction("vim.api", "set_cursor_pos", lua_api_set_cursor_pos);
    engine_->registerTableFunction("vim.api", "get_line", lua_api_get_line);
    engine_->registerTableFunction("vim.api", "set_line", lua_api_set_line);
    engine_->registerTableFunction("vim.api", "insert_text", lua_api_insert_text);
    engine_->registerTableFunction("vim.api", "delete_line", lua_api_delete_line);
    engine_->registerTableFunction("vim.api", "get_filepath", lua_api_get_filepath);
    engine_->registerTableFunction("vim.api", "open_file", lua_api_open_file);
    engine_->registerTableFunction("vim.api", "save_file", lua_api_save_file);
    engine_->registerTableFunction("vim.api", "set_status_message", lua_api_set_status_message);
    engine_->registerTableFunction("vim.api", "get_theme", lua_api_get_theme);
    engine_->registerTableFunction("vim.api", "set_theme", lua_api_set_theme);

    // 注册 vim.fn 函数
    engine_->registerTableFunction("vim.fn", "system", lua_fn_system);
    engine_->registerTableFunction("vim.fn", "readfile", lua_fn_readfile);
    engine_->registerTableFunction("vim.fn", "writefile", lua_fn_writefile);

    // 注册全局函数
    engine_->registerFunction("pnana_notify", lua_api_notify);
    engine_->registerFunction("pnana_command", lua_api_command);
    engine_->registerFunction("pnana_keymap", lua_api_keymap);
    engine_->registerFunction("pnana_autocmd", lua_api_autocmd);

    // 创建便捷别名（类似 Neovim）
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

// vim.api.get_current_line() -> string
int LuaAPI::lua_api_get_current_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushnil(L);
        return 1;
    }

    core::Document* doc = editor->getCurrentDocument();
    if (!doc || doc->lineCount() == 0) {
        lua_pushstring(L, "");
        return 1;
    }

    // LuaAPI 是 Editor 的友元类，可以访问私有成员
    // 但由于这是静态函数，需要通过实例访问
    // 简化处理：获取第一行（实际应该获取光标所在行）
    size_t row = 0;
    if (row < doc->lineCount()) {
        lua_pushstring(L, doc->getLine(row).c_str());
    } else {
        lua_pushstring(L, "");
    }
    return 1;
}

// vim.api.get_line_count() -> number
int LuaAPI::lua_api_get_line_count(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushinteger(L, 0);
        return 1;
    }

    core::Document* doc = editor->getCurrentDocument();
    if (!doc) {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushinteger(L, static_cast<lua_Integer>(doc->lineCount()));
    return 1;
}

// vim.api.get_cursor_pos() -> {row, col}
int LuaAPI::lua_api_get_cursor_pos(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    lua_newtable(L);

    if (editor) {
        core::Document* doc = editor->getCurrentDocument();
        if (doc && doc->lineCount() > 0) {
            // 简化：返回 (0, 0)，实际应该通过 API 获取
            lua_pushinteger(L, 0);
            lua_setfield(L, -2, "row");
            lua_pushinteger(L, 0);
            lua_setfield(L, -2, "col");
        } else {
            lua_pushinteger(L, 0);
            lua_setfield(L, -2, "row");
            lua_pushinteger(L, 0);
            lua_setfield(L, -2, "col");
        }
    } else {
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "row");
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "col");
    }
    return 1;
}

// vim.api.set_cursor_pos({row, col})
int LuaAPI::lua_api_set_cursor_pos(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor || !lua_istable(L, 1)) {
        return 0;
    }

    lua_getfield(L, 1, "row");
    lua_getfield(L, 1, "col");

    // TODO: 通过公共 API 设置光标位置
    // int row = static_cast<int>(lua_tointeger(L, -2));
    // int col = static_cast<int>(lua_tointeger(L, -1));
    // editor->gotoLine(row);

    lua_pop(L, 2);

    return 0;
}

// vim.api.get_line(row) -> string
int LuaAPI::lua_api_get_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushnil(L);
        return 1;
    }

    int row = static_cast<int>(lua_tointeger(L, 1));
    core::Document* doc = editor->getCurrentDocument();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount()) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, doc->getLine(static_cast<size_t>(row)).c_str());
    return 1;
}

// vim.api.set_line(row, text)
int LuaAPI::lua_api_set_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    int row = static_cast<int>(lua_tointeger(L, 1));
    const char* text = lua_tostring(L, 2);

    core::Document* doc = editor->getCurrentDocument();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount() || !text) {
        return 0;
    }

    doc->replaceLine(static_cast<size_t>(row), std::string(text));
    return 0;
}

// vim.api.insert_text(row, col, text)
int LuaAPI::lua_api_insert_text(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    int row = static_cast<int>(lua_tointeger(L, 1));
    int col = static_cast<int>(lua_tointeger(L, 2));
    const char* text = lua_tostring(L, 3);

    if (!text) {
        return 0;
    }

    core::Document* doc = editor->getCurrentDocument();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount()) {
        return 0;
    }

    doc->insertText(static_cast<size_t>(row), static_cast<size_t>(col), std::string(text));
    return 0;
}

// vim.api.delete_line(row)
int LuaAPI::lua_api_delete_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    int row = static_cast<int>(lua_tointeger(L, 1));
    core::Document* doc = editor->getCurrentDocument();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount()) {
        return 0;
    }

    doc->deleteLine(static_cast<size_t>(row));
    return 0;
}

// vim.api.get_filepath() -> string
int LuaAPI::lua_api_get_filepath(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushnil(L);
        return 1;
    }

    core::Document* doc = editor->getCurrentDocument();
    if (!doc) {
        lua_pushnil(L);
        return 1;
    }

    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, filepath.c_str());
    }
    return 1;
}

// vim.api.open_file(filepath)
int LuaAPI::lua_api_open_file(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushboolean(L, 0);
        return 1;
    }

    const char* filepath = lua_tostring(L, 1);
    if (!filepath) {
        lua_pushboolean(L, 0);
        return 1;
    }

    bool result = editor->openFile(std::string(filepath));
    lua_pushboolean(L, result ? 1 : 0);
    return 1;
}

// vim.api.save_file()
int LuaAPI::lua_api_save_file(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushboolean(L, 0);
        return 1;
    }

    bool result = editor->saveFile();
    lua_pushboolean(L, result ? 1 : 0);
    return 1;
}

// vim.api.set_status_message(message)
int LuaAPI::lua_api_set_status_message(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    const char* message = lua_tostring(L, 1);
    if (message) {
        // LuaAPI 是 Editor 的友元类，可以访问私有方法
        editor->setStatusMessage(std::string(message));
    }
    return 0;
}

// vim.api.get_theme() -> string
int LuaAPI::lua_api_get_theme(lua_State* L) {
    // TODO: 实现获取主题
    lua_pushstring(L, "monokai");
    return 1;
}

// vim.api.set_theme(theme_name)
int LuaAPI::lua_api_set_theme(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    const char* theme = lua_tostring(L, 1);
    if (theme) {
        editor->setTheme(std::string(theme));
    }
    return 0;
}

// vim.fn.system(command) -> string
int LuaAPI::lua_fn_system(lua_State* L) {
    const char* command = lua_tostring(L, 1);
    if (!command) {
        lua_pushnil(L);
        return 1;
    }

    FILE* pipe = popen(command, "r");
    if (!pipe) {
        lua_pushnil(L);
        return 1;
    }

    std::string result;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    lua_pushstring(L, result.c_str());
    return 1;
}

// vim.fn.readfile(filepath) -> {lines}
int LuaAPI::lua_fn_readfile(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    if (!filepath) {
        lua_pushnil(L);
        return 1;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    std::string line;
    int index = 1;
    while (std::getline(file, line)) {
        lua_pushstring(L, line.c_str());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

// vim.fn.writefile(filepath, {lines})
int LuaAPI::lua_fn_writefile(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    if (!filepath || !lua_istable(L, 2)) {
        lua_pushboolean(L, 0);
        return 1;
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        lua_pushboolean(L, 0);
        return 1;
    }

    int len = static_cast<int>(luaL_len(L, 2));
    for (int i = 1; i <= len; ++i) {
        lua_rawgeti(L, 2, i);
        const char* line = lua_tostring(L, -1);
        if (line) {
            file << line << "\n";
        }
        lua_pop(L, 1);
    }

    lua_pushboolean(L, 1);
    return 1;
}

// pnana_notify(message, level)
int LuaAPI::lua_api_notify(lua_State* L) {
    const char* message = lua_tostring(L, 1);
    if (message) {
        LOG("Plugin: " + std::string(message));
    }
    return 0;
}

// pnana_command(name, callback)
int LuaAPI::lua_api_command(lua_State* L) {
    LuaAPI* api = getAPIFromLua(L);
    if (!api) {
        return 0;
    }

    const char* name = lua_tostring(L, 1);
    const char* callback = lua_tostring(L, 2);

    if (name && callback) {
        api->registerCommand(std::string(name), std::string(callback));
    }

    return 0;
}

// pnana_keymap(mode, keys, callback)
int LuaAPI::lua_api_keymap(lua_State* L) {
    LuaAPI* api = getAPIFromLua(L);
    if (!api) {
        return 0;
    }

    const char* mode = lua_tostring(L, 1);
    const char* keys = lua_tostring(L, 2);
    const char* callback = lua_tostring(L, 3);

    if (mode && keys && callback) {
        api->registerKeymap(std::string(mode), std::string(keys), std::string(callback));
    }

    return 0;
}

// pnana_autocmd(event, callback)
int LuaAPI::lua_api_autocmd(lua_State* L) {
    LuaAPI* api = getAPIFromLua(L);
    if (!api) {
        return 0;
    }

    const char* event = lua_tostring(L, 1);
    const char* callback = lua_tostring(L, 2);

    if (event && callback) {
        api->registerEventListener(std::string(event), std::string(callback));
    }

    return 0;
}

void LuaAPI::triggerEvent(const std::string& event, const std::vector<std::string>& args) {
    if (!engine_)
        return;

    auto it = event_listeners_.find(event);
    if (it == event_listeners_.end()) {
        return;
    }

    lua_State* L = engine_->getState();
    for (const auto& callback : it->second) {
        lua_getglobal(L, callback.c_str());
        if (lua_isfunction(L, -1)) {
            // 推送参数
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

void LuaAPI::registerEventListener(const std::string& event, const std::string& callback) {
    event_listeners_[event].push_back(callback);
}

void LuaAPI::registerCommand(const std::string& name, const std::string& callback) {
    commands_[name] = callback;
}

void LuaAPI::registerKeymap(const std::string& mode, const std::string& keys,
                            const std::string& callback) {
    keymaps_[mode][keys] = callback;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
