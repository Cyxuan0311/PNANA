#ifdef BUILD_LUA_SUPPORT

#include "plugins/editor_api.h"
#include "core/document.h"
#include "core/editor.h"
#include <lua.hpp>

namespace pnana {
namespace plugins {

// 在 Lua 注册表中存储编辑器指针的键
static const char* EDITOR_REGISTRY_KEY = "pnana_editor";

EditorAPI::EditorAPI(core::Editor* editor) : editor_(editor) {}

EditorAPI::~EditorAPI() {}

void EditorAPI::registerFunctions(lua_State* L) {
    // 在注册表中存储编辑器指针
    lua_pushlightuserdata(L, editor_);
    lua_setfield(L, LUA_REGISTRYINDEX, EDITOR_REGISTRY_KEY);

    // 注册API函数到vim.api表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "api");

    lua_pushcfunction(L, lua_api_get_current_line);
    lua_setfield(L, -2, "get_current_line");

    lua_pushcfunction(L, lua_api_get_line_count);
    lua_setfield(L, -2, "get_line_count");

    lua_pushcfunction(L, lua_api_get_cursor_pos);
    lua_setfield(L, -2, "get_cursor_pos");

    lua_pushcfunction(L, lua_api_set_cursor_pos);
    lua_setfield(L, -2, "set_cursor_pos");

    lua_pushcfunction(L, lua_api_get_line);
    lua_setfield(L, -2, "get_line");

    lua_pushcfunction(L, lua_api_set_line);
    lua_setfield(L, -2, "set_line");

    lua_pushcfunction(L, lua_api_insert_text);
    lua_setfield(L, -2, "insert_text");

    lua_pushcfunction(L, lua_api_delete_line);
    lua_setfield(L, -2, "delete_line");

    lua_pop(L, 2); // 弹出vim和api表
}

core::Editor* EditorAPI::getEditorFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, EDITOR_REGISTRY_KEY);
    core::Editor* editor = static_cast<core::Editor*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return editor;
}

// vim.api.get_current_line() -> string
int EditorAPI::lua_api_get_current_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushnil(L);
        return 1;
    }

    core::Document* doc = editor->getCurrentDocumentForLua();
    if (!doc || doc->lineCount() == 0) {
        lua_pushstring(L, "");
        return 1;
    }

    // 获取光标所在行
    size_t row = editor->getCursorRowForLua();
    if (row < doc->lineCount()) {
        lua_pushstring(L, doc->getLine(row).c_str());
    } else {
        lua_pushstring(L, "");
    }
    return 1;
}

// vim.api.get_line_count() -> number
int EditorAPI::lua_api_get_line_count(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushinteger(L, 0);
        return 1;
    }

    core::Document* doc = editor->getCurrentDocumentForLua();
    if (!doc) {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushinteger(L, static_cast<lua_Integer>(doc->lineCount()));
    return 1;
}

// vim.api.get_cursor_pos() -> {row, col}
int EditorAPI::lua_api_get_cursor_pos(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    lua_newtable(L);

    if (editor) {
        size_t row = editor->getCursorRowForLua();
        size_t col = editor->getCursorColForLua();
        lua_pushinteger(L, static_cast<lua_Integer>(row));
            lua_setfield(L, -2, "row");
        lua_pushinteger(L, static_cast<lua_Integer>(col));
            lua_setfield(L, -2, "col");
    } else {
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "row");
        lua_pushinteger(L, 0);
        lua_setfield(L, -2, "col");
    }
    return 1;
}

// vim.api.set_cursor_pos({row, col})
int EditorAPI::lua_api_set_cursor_pos(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor || !lua_istable(L, 1)) {
        return 0;
    }

    lua_getfield(L, 1, "row");
    lua_getfield(L, 1, "col");

    int row = static_cast<int>(lua_tointeger(L, -2));
    int col = static_cast<int>(lua_tointeger(L, -1));
    
    // 确保非负
    if (row < 0) row = 0;
    if (col < 0) col = 0;
    
    editor->setCursorPosForLua(static_cast<size_t>(row), static_cast<size_t>(col));

    lua_pop(L, 2);

    return 0;
}

// vim.api.get_line(row) -> string
int EditorAPI::lua_api_get_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushnil(L);
        return 1;
    }

    int row = static_cast<int>(lua_tointeger(L, 1));
    core::Document* doc = editor->getCurrentDocumentForLua();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount()) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, doc->getLine(static_cast<size_t>(row)).c_str());
    return 1;
}

// vim.api.set_line(row, text)
int EditorAPI::lua_api_set_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    int row = static_cast<int>(lua_tointeger(L, 1));
    const char* text = lua_tostring(L, 2);

    core::Document* doc = editor->getCurrentDocumentForLua();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount() || !text) {
        return 0;
    }

    doc->replaceLine(static_cast<size_t>(row), std::string(text));
    return 0;
}

// vim.api.insert_text(row, col, text)
int EditorAPI::lua_api_insert_text(lua_State* L) {
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

    core::Document* doc = editor->getCurrentDocumentForLua();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount()) {
        return 0;
    }

    doc->insertText(static_cast<size_t>(row), static_cast<size_t>(col), std::string(text));
    return 0;
}

// vim.api.delete_line(row)
int EditorAPI::lua_api_delete_line(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    int row = static_cast<int>(lua_tointeger(L, 1));
    core::Document* doc = editor->getCurrentDocumentForLua();
    if (!doc || row < 0 || static_cast<size_t>(row) >= doc->lineCount()) {
        return 0;
    }

    doc->deleteLine(static_cast<size_t>(row));
    return 0;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
