#ifdef BUILD_LUA_SUPPORT

#include "plugins/file_api.h"
#include "core/document.h"
#include "core/editor.h"
#include "plugins/path_validator.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <lua.hpp>
#include <sstream>

namespace pnana {
namespace plugins {

namespace {
constexpr std::size_t kMaxSecureIOBytes = 1024 * 1024; // 1MB
}

// 在 Lua 注册表中存储编辑器指针的键
static const char* kEditorRegistryKey = "pnana_editor";
static const char* kFileAPIRegistryKey = "pnana_file_api";

FileAPI::FileAPI(core::Editor* editor) : editor_(editor), path_validator_(nullptr) {}

FileAPI::~FileAPI() {}

void FileAPI::setPathValidator(PathValidator* validator) {
    path_validator_ = validator;
}

void FileAPI::registerFunctions(lua_State* L) {
    // 在注册表中存储编辑器指针
    lua_pushlightuserdata(L, editor_);
    lua_setfield(L, LUA_REGISTRYINDEX, kEditorRegistryKey);

    // 在注册表中存储 FileAPI 实例
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, kFileAPIRegistryKey);

    // 注册 API 函数到 vim.api 表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "api");

    lua_pushcfunction(L, lua_fn_get_filepath);
    lua_setfield(L, -2, "get_filepath");

    lua_pushcfunction(L, lua_fn_open_file);
    lua_setfield(L, -2, "open_file");

    lua_pushcfunction(L, lua_fn_save_file);
    lua_setfield(L, -2, "save_file");

    lua_pop(L, 2); // 弹出 vim 和 api 表

    // 注册到 vim.fn 表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "fn");

    lua_pushcfunction(L, lua_fn_readfile);
    lua_setfield(L, -2, "readfile");

    lua_pushcfunction(L, lua_fn_writefile);
    lua_setfield(L, -2, "writefile");

    lua_pop(L, 2); // 弹出 vim 和 fn 表

    // 注册 vim.secure_io 表（安全 IO API）
    lua_getglobal(L, "vim");
    lua_newtable(L);
    lua_pushcfunction(L, lua_fn_secure_io_read_text);
    lua_setfield(L, -2, "read_text");
    lua_pushcfunction(L, lua_fn_secure_io_write_text);
    lua_setfield(L, -2, "write_text");
    lua_pushcfunction(L, lua_fn_secure_io_append_text);
    lua_setfield(L, -2, "append_text");
    lua_pushcfunction(L, lua_fn_secure_io_exists);
    lua_setfield(L, -2, "exists");
    lua_setfield(L, -2, "secure_io");
    lua_pop(L, 1); // 弹出 vim
}

core::Editor* FileAPI::getEditorFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, kEditorRegistryKey);
    core::Editor* editor = static_cast<core::Editor*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return editor;
}

FileAPI* FileAPI::getAPIFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, kFileAPIRegistryKey);
    FileAPI* api = static_cast<FileAPI*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return api;
}

// vim.api.get_filepath() -> string
int FileAPI::lua_fn_get_filepath(lua_State* L) {
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
int FileAPI::lua_fn_open_file(lua_State* L) {
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
int FileAPI::lua_fn_save_file(lua_State* L) {
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

    // 使用二进制模式并预先获取文件大小
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        lua_pushnil(L);
        return 1;
    }

    // 预分配缓冲区，避免多次重新分配
    auto size = file.tellg();
    if (size > 100 * 1024 * 1024) { // 限制 100MB
        lua_pushnil(L);
        return 1;
    }

    std::string content(static_cast<size_t>(size), '\0');
    file.seekg(0, std::ios::beg);
    file.read(&content[0], size);

    if (!file) {
        lua_pushnil(L);
        return 1;
    }

    // 按行分割并推入 Lua 表
    lua_newtable(L);
    int index = 1;
    size_t start = 0;
    for (size_t i = 0; i <= content.size(); ++i) {
        if (i == content.size() || content[i] == '\n') {
            // 移除行尾的 \r
            size_t len = (i > start && content[i - 1] == '\r') ? i - start - 1 : i - start;
            lua_pushlstring(L, content.c_str() + start, len);
            lua_rawseti(L, -2, index++);
            start = i + 1;
        }
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

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        lua_pushboolean(L, 0);
        return 1;
    }

    int len = static_cast<int>(luaL_len(L, 2));
    // 预先计算总大小，减少重新分配
    std::string buffer;
    buffer.reserve(len * 64); // 预估每行 64 字节

    for (int i = 1; i <= len; ++i) {
        lua_rawgeti(L, 2, i);
        size_t line_len = 0;
        const char* line = lua_tolstring(L, -1, &line_len);
        if (line) {
            buffer.append(line, line_len);
            buffer.push_back('\n');
        }
        lua_pop(L, 1);
    }

    file.write(buffer.c_str(), static_cast<std::streamsize>(buffer.size()));

    lua_pushboolean(L, file.good() ? 1 : 0);
    return 1;
}

int FileAPI::lua_fn_secure_io_read_text(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    if (!filepath) {
        lua_pushnil(L);
        lua_pushstring(L, "invalid path");
        return 2;
    }

    FileAPI* api = getAPIFromLua(L);
    if (!api || !api->path_validator_ || !api->path_validator_->isPathAllowed(filepath)) {
        LOG_WARNING("secure_io.read_text blocked path: " + std::string(filepath));
        lua_pushnil(L);
        lua_pushstring(L, "path is not allowed");
        return 2;
    }

    // 使用更高效的读取方式
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        lua_pushnil(L);
        lua_pushstring(L, "failed to open file");
        return 2;
    }

    auto size = file.tellg();
    if (size > static_cast<std::streamoff>(kMaxSecureIOBytes)) {
        lua_pushnil(L);
        lua_pushstring(L, "file is too large");
        return 2;
    }

    // 预分配缓冲区
    std::string content(static_cast<size_t>(size), '\0');
    file.seekg(0, std::ios::beg);
    file.read(&content[0], size);

    if (!file) {
        lua_pushnil(L);
        lua_pushstring(L, "failed to read file");
        return 2;
    }

    lua_pushlstring(L, content.c_str(), content.size());
    return 1;
}

int FileAPI::lua_fn_secure_io_write_text(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    size_t content_len = 0;
    const char* content = luaL_checklstring(L, 2, &content_len);
    if (!filepath || !content) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "invalid arguments");
        return 2;
    }
    if (content_len > kMaxSecureIOBytes) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "content too large");
        return 2;
    }

    FileAPI* api = getAPIFromLua(L);
    if (!api || !api->path_validator_ || !api->path_validator_->isPathAllowed(filepath)) {
        LOG_WARNING("secure_io.write_text blocked path: " + std::string(filepath));
        lua_pushboolean(L, 0);
        lua_pushstring(L, "path is not allowed");
        return 2;
    }

    std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "failed to open file");
        return 2;
    }
    file.write(content, static_cast<std::streamsize>(content_len));
    if (!file.good()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "write failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int FileAPI::lua_fn_secure_io_append_text(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    size_t content_len = 0;
    const char* content = luaL_checklstring(L, 2, &content_len);
    if (!filepath || !content) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "invalid arguments");
        return 2;
    }
    if (content_len > kMaxSecureIOBytes) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "content too large");
        return 2;
    }

    FileAPI* api = getAPIFromLua(L);
    if (!api || !api->path_validator_ || !api->path_validator_->isPathAllowed(filepath)) {
        LOG_WARNING("secure_io.append_text blocked path: " + std::string(filepath));
        lua_pushboolean(L, 0);
        lua_pushstring(L, "path is not allowed");
        return 2;
    }

    try {
        if (std::filesystem::exists(filepath)) {
            auto size = std::filesystem::file_size(filepath);
            if (size + content_len > kMaxSecureIOBytes) {
                lua_pushboolean(L, 0);
                lua_pushstring(L, "result file is too large");
                return 2;
            }
        }
    } catch (const std::exception& e) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, e.what());
        return 2;
    }

    std::ofstream file(filepath, std::ios::out | std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "failed to open file");
        return 2;
    }
    file.write(content, static_cast<std::streamsize>(content_len));
    if (!file.good()) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "append failed");
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

int FileAPI::lua_fn_secure_io_exists(lua_State* L) {
    const char* filepath = lua_tostring(L, 1);
    if (!filepath) {
        lua_pushboolean(L, 0);
        return 1;
    }

    FileAPI* api = getAPIFromLua(L);
    if (!api || !api->path_validator_ || !api->path_validator_->isPathAllowed(filepath)) {
        lua_pushboolean(L, 0);
        return 1;
    }

    bool exists = false;
    try {
        exists = std::filesystem::exists(filepath);
    } catch (...) {
        exists = false;
    }
    lua_pushboolean(L, exists ? 1 : 0);
    return 1;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
