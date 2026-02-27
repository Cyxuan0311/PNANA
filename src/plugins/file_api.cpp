#ifdef BUILD_LUA_SUPPORT

#include "plugins/file_api.h"
#include "core/document.h"
#include "core/editor.h"
#include "plugins/path_validator.h"
#include "utils/logger.h"
#include <fstream>
#include <lua.hpp>

namespace pnana {
namespace plugins {

// 在 Lua 注册表中存储编辑器指针的键
static const char* EDITOR_REGISTRY_KEY = "pnana_editor";
static const char* FILE_API_REGISTRY_KEY = "pnana_file_api";

FileAPI::FileAPI(core::Editor* editor) : editor_(editor), path_validator_(nullptr) {}

FileAPI::~FileAPI() {}

void FileAPI::setPathValidator(PathValidator* validator) {
    path_validator_ = validator;
}

void FileAPI::registerFunctions(lua_State* L) {
    // 在注册表中存储编辑器指针
    lua_pushlightuserdata(L, editor_);
    lua_setfield(L, LUA_REGISTRYINDEX, EDITOR_REGISTRY_KEY);

    // 在注册表中存储 FileAPI 实例
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, FILE_API_REGISTRY_KEY);

    // 注册API函数到vim.api表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "api");

    lua_pushcfunction(L, lua_api_get_filepath);
    lua_setfield(L, -2, "get_filepath");

    lua_pushcfunction(L, lua_api_open_file);
    lua_setfield(L, -2, "open_file");

    lua_pushcfunction(L, lua_api_save_file);
    lua_setfield(L, -2, "save_file");

    lua_pop(L, 2); // 弹出vim和api表

    // 注册到vim.fn表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "fn");

    lua_pushcfunction(L, lua_fn_readfile);
    lua_setfield(L, -2, "readfile");

    lua_pushcfunction(L, lua_fn_writefile);
    lua_setfield(L, -2, "writefile");

    lua_pop(L, 2); // 弹出vim和fn表
}

core::Editor* FileAPI::getEditorFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, EDITOR_REGISTRY_KEY);
    core::Editor* editor = static_cast<core::Editor*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return editor;
}

FileAPI* FileAPI::getAPIFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, FILE_API_REGISTRY_KEY);
    FileAPI* api = static_cast<FileAPI*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return api;
}

// vim.api.get_filepath() -> string
int FileAPI::lua_api_get_filepath(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushnil(L);
        return 1;
    }

    core::Document* doc = editor->getCurrentDocumentForLua();
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
int FileAPI::lua_api_open_file(lua_State* L) {
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
int FileAPI::lua_api_save_file(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushboolean(L, 0);
        return 1;
    }

    bool result = editor->saveFile();
    lua_pushboolean(L, result ? 1 : 0);
    return 1;
}

// vim.fn.readfile(filepath) -> {lines}
int FileAPI::lua_fn_readfile(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    if (!filepath) {
        lua_pushnil(L);
        return 1;
    }

    // 沙盒检查：验证路径是否允许访问
    FileAPI* api = getAPIFromLua(L);
    if (api && api->path_validator_) {
        if (!api->path_validator_->isPathAllowed(filepath)) {
            LOG_WARNING("Plugin attempted to read restricted path: " + std::string(filepath) +
                        " (blocked by sandbox)");
            lua_pushnil(L);
            return 1;
        }
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
int FileAPI::lua_fn_writefile(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    if (!filepath || !lua_istable(L, 2)) {
        lua_pushboolean(L, 0);
        return 1;
    }

    // 沙盒检查：验证路径是否允许访问
    FileAPI* api = getAPIFromLua(L);
    if (api && api->path_validator_) {
        if (!api->path_validator_->isPathAllowed(filepath)) {
            LOG_WARNING("Plugin attempted to write to restricted path: " + std::string(filepath) +
                        " (blocked by sandbox)");
            lua_pushboolean(L, 0);
            return 1;
        }
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

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
