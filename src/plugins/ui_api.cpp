#ifdef BUILD_LUA_SUPPORT

#include "plugins/ui_api.h"
#include "core/editor.h"
#include "plugins/lua_api.h"
#include "utils/logger.h"
#include <lua.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace plugins {

static const char* LUA_API_REGISTRY_KEY = "pnana_lua_api";

UIAPI::UIAPI() : lua_api_(nullptr) {}

UIAPI::~UIAPI() {}

std::map<int, int> UIAPI::window_handles_;
int UIAPI::next_window_id_ = 1;

std::map<int, int> UIAPI::progress_handles_;
int UIAPI::next_progress_id_ = 1;

std::map<int, int> UIAPI::hover_handles_;
int UIAPI::next_hover_id_ = 1;

void UIAPI::registerFunctions(lua_State* L) {
    lua_getglobal(L, "vim");
    lua_newtable(L);
    lua_pushcfunction(L, lua_ui_notify);
    lua_setfield(L, -2, "notify");
    lua_pushcfunction(L, lua_ui_input);
    lua_setfield(L, -2, "input");
    lua_pushcfunction(L, lua_ui_select);
    lua_setfield(L, -2, "select");
    lua_pushcfunction(L, lua_ui_popup);
    lua_setfield(L, -2, "popup");
    lua_pushcfunction(L, lua_ui_dialog);
    lua_setfield(L, -2, "dialog");
    lua_pushcfunction(L, lua_ui_list);
    lua_setfield(L, -2, "list");
    lua_pushcfunction(L, lua_ui_open_window);
    lua_setfield(L, -2, "open_window");
    lua_pushcfunction(L, lua_ui_update_window);
    lua_setfield(L, -2, "update_window");
    lua_pushcfunction(L, lua_ui_close_window);
    lua_setfield(L, -2, "close_window");

    // 新增 UI API 注册
    lua_pushcfunction(L, lua_ui_progress);
    lua_setfield(L, -2, "progress");
    lua_pushcfunction(L, lua_ui_update_progress);
    lua_setfield(L, -2, "update_progress");
    lua_pushcfunction(L, lua_ui_close_progress);
    lua_setfield(L, -2, "close_progress");
    lua_pushcfunction(L, lua_ui_multiselect);
    lua_setfield(L, -2, "multiselect");
    lua_pushcfunction(L, lua_ui_hover);
    lua_setfield(L, -2, "hover");
    lua_pushcfunction(L, lua_ui_close_hover);
    lua_setfield(L, -2, "close_hover");
    lua_pushcfunction(L, lua_ui_list_windows);
    lua_setfield(L, -2, "list_windows");
    lua_pushcfunction(L, lua_ui_window_is_valid);
    lua_setfield(L, -2, "window_is_valid");
    lua_pushcfunction(L, lua_ui_get_window_info);
    lua_setfield(L, -2, "get_window_info");
    lua_pushcfunction(L, lua_ui_focus_window);
    lua_setfield(L, -2, "focus_window");

    // 新增高级布局 API
    lua_pushcfunction(L, lua_ui_create_layout);
    lua_setfield(L, -2, "create_layout");
    lua_pushcfunction(L, lua_ui_open_layout_window);
    lua_setfield(L, -2, "open_layout_window");
    lua_pushcfunction(L, lua_ui_update_layout);
    lua_setfield(L, -2, "update_layout");

    lua_setfield(L, -2, "ui");
    lua_pop(L, 1);
}

LuaAPI* UIAPI::getLuaAPIFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, LUA_API_REGISTRY_KEY);
    LuaAPI* api = static_cast<LuaAPI*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return api;
}

int UIAPI::lua_ui_notify(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    const char* level = lua_tostring(L, 2);

    std::string message = msg ? std::string(msg) : "";
    std::string lv = level ? std::string(level) : "info";

    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (lua_api && lua_api->getEditor()) {
        lua_api->getEditor()->setStatusMessageForLua(message);
    }

    if (lv == "error") {
        LOG_ERROR("Plugin notify: " + message);
    } else if (lv == "warn") {
        LOG_WARNING("Plugin notify: " + message);
    } else {
        LOG("Plugin notify: " + message);
    }
    return 0;
}

int UIAPI::lua_ui_input(lua_State* L) {
    LOG_INFO("[vim.ui.input] Called");

    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor()) {
        LOG_ERROR("[vim.ui.input] Lua API or Editor not available");
        lua_pushboolean(L, false);
        return 1;
    }

    std::string title = "Input";
    std::string prompt = "Input:";
    std::string default_value;
    bool has_callback = false;

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "prompt");
        if (lua_isstring(L, -1)) {
            prompt = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "default");
        if (lua_isstring(L, -1)) {
            default_value = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        if (lua_isfunction(L, 2)) {
            has_callback = true;
            lua_pushvalue(L, 2);
        }
    } else {
        if (lua_isstring(L, 1)) {
            prompt = lua_tostring(L, 1);
        }
        if (lua_isfunction(L, 2)) {
            has_callback = true;
            lua_pushvalue(L, 2);
        }
    }

    LOG_INFO("[vim.ui.input] Title: " + title + ", Prompt: " + prompt);

    if (!has_callback) {
        LOG_ERROR("[vim.ui.input] No callback provided");
        lua_pushboolean(L, false);
        return 1;
    }

    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_State* lua_state = L;
    LOG_INFO("[vim.ui.input] Showing input dialog");
    lua_api->getEditor()->showInputDialogForLua(
        title, prompt, default_value,
        [lua_state, callback_ref](bool confirmed, const std::string& value) {
            LOG_INFO("[vim.ui.input] Callback called, confirmed: " +
                     std::to_string(confirmed ? 1 : 0));

            lua_rawgeti(lua_state, LUA_REGISTRYINDEX, callback_ref);
            if (!lua_isfunction(lua_state, -1)) {
                lua_pop(lua_state, 1);
                luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
                return;
            }

            if (confirmed) {
                lua_pushstring(lua_state, value.c_str());
            } else {
                lua_pushnil(lua_state);
            }

            if (lua_pcall(lua_state, 1, 0, 0) != LUA_OK) {
                const char* error = lua_tostring(lua_state, -1);
                LOG_ERROR("[vim.ui.input] Callback error: " +
                          std::string(error ? error : "unknown"));
                lua_pop(lua_state, 1);
            }
            luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
        });

    lua_pushboolean(L, true);
    return 1;
}

int UIAPI::lua_ui_select(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_istable(L, 1) || !lua_isfunction(L, 3)) {
        lua_pushboolean(L, false);
        return 1;
    }

    std::string title = "Select";
    std::string prompt = "Select:";
    std::vector<std::string> item_labels;

    int item_count = static_cast<int>(luaL_len(L, 1));
    for (int i = 1; i <= item_count; ++i) {
        lua_rawgeti(L, 1, i);
        if (lua_isstring(L, -1)) {
            item_labels.push_back(lua_tostring(L, -1));
        } else {
            item_labels.push_back("<item " + std::to_string(i) + ">");
        }
        lua_pop(L, 1);
    }

    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "prompt");
        if (lua_isstring(L, -1)) {
            prompt = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }

    lua_pushvalue(L, 3);
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_State* lua_state = L;

    lua_api->getEditor()->showSelectDialogForLua(
        title, prompt, item_labels,
        [lua_state, callback_ref](bool confirmed, size_t one_based_idx) {
            lua_rawgeti(lua_state, LUA_REGISTRYINDEX, callback_ref);
            if (!lua_isfunction(lua_state, -1)) {
                lua_pop(lua_state, 1);
                luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
                return;
            }

            if (confirmed && one_based_idx > 0) {
                lua_pushinteger(lua_state, static_cast<lua_Integer>(one_based_idx));
            } else {
                lua_pushnil(lua_state);
                lua_pushnil(lua_state);
            }

            if (lua_pcall(lua_state, 2, 0, 0) != LUA_OK) {
                const char* error = lua_tostring(lua_state, -1);
                LOG_ERROR("vim.ui.select callback error: " +
                          std::string(error ? error : "unknown"));
                lua_pop(lua_state, 1);
            }
            luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
        });

    lua_pushboolean(L, true);
    return 1;
}

int UIAPI::lua_ui_popup(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* title = lua_tostring(L, 1);
    std::string title_text = title ? std::string(title) : "Popup";
    std::string body_text;
    if (lua_istable(L, 2)) {
        int len = static_cast<int>(luaL_len(L, 2));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, 2, i);
            if (lua_isstring(L, -1)) {
                if (!body_text.empty()) {
                    body_text += "\n";
                }
                body_text += lua_tostring(L, -1);
            }
            lua_pop(L, 1);
        }
    } else if (lua_isstring(L, 2)) {
        body_text = lua_tostring(L, 2);
    }

    lua_api->getEditor()->getPopupManagerForLua()->openConfirm(title_text, body_text, nullptr);
    lua_pushboolean(L, true);
    return 1;
}

int UIAPI::lua_ui_dialog(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor()) {
        lua_pushboolean(L, false);
        return 1;
    }

    std::string title = "Dialog";
    std::string message;
    bool has_callback = false;

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "message");
        if (lua_isstring(L, -1)) {
            message = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "callback");
        if (lua_isfunction(L, -1)) {
            has_callback = true;
            lua_pushvalue(L, -1);
        }
        lua_pop(L, 1);
    } else {
        const char* title_c = lua_tostring(L, 1);
        const char* msg_c = lua_tostring(L, 2);
        if (title_c) {
            title = title_c;
        }
        if (msg_c) {
            message = msg_c;
        }
        if (lua_isfunction(L, 3)) {
            has_callback = true;
            lua_pushvalue(L, 3);
        }
    }

    if (!has_callback) {
        lua_pushboolean(L, false);
        return 1;
    }

    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_State* lua_state = L;
    lua_api->getEditor()->showConfirmDialogForLua(
        title, message, [lua_state, callback_ref](bool ok) {
            lua_rawgeti(lua_state, LUA_REGISTRYINDEX, callback_ref);
            if (!lua_isfunction(lua_state, -1)) {
                lua_pop(lua_state, 1);
                luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
                return;
            }

            lua_pushboolean(lua_state, ok ? 1 : 0);
            if (lua_pcall(lua_state, 1, 0, 0) != LUA_OK) {
                const char* error = lua_tostring(lua_state, -1);
                LOG_ERROR("vim.ui.dialog callback error: " +
                          std::string(error ? error : "unknown"));
                lua_pop(lua_state, 1);
            }
            luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
        });

    lua_pushboolean(L, true);
    return 1;
}

int UIAPI::lua_ui_list(lua_State* L) {
    // 通用可交互列表：当前基于 showSelectDialogForLua 实现
    return lua_ui_select(L);
}

int UIAPI::lua_ui_open_window(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushnil(L);
        return 1;
    }

    std::string title = "Window";
    std::vector<std::string> lines;

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "lines");
        if (lua_istable(L, -1)) {
            int len = static_cast<int>(luaL_len(L, -1));
            for (int i = 1; i <= len; ++i) {
                lua_rawgeti(L, -1, i);
                if (lua_isstring(L, -1)) {
                    lines.emplace_back(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }

    pnana::core::ui::PopupSpec spec;
    spec.title = title;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            spec.message += "\n";
        }
        spec.message += lines[i];
    }

    int lua_win_id = next_window_id_++;
    int handle = lua_api->getEditor()->getPopupManagerForLua()->openPopup(spec, {});
    window_handles_[lua_win_id] = handle;

    lua_pushinteger(L, static_cast<lua_Integer>(lua_win_id));
    return 1;
}

int UIAPI::lua_ui_update_window(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_win_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = window_handles_.find(lua_win_id);
    if (it == window_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    pnana::core::ui::PopupSpec patch;
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "title");
        if (lua_isstring(L, -1)) {
            patch.title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "lines");
        if (lua_istable(L, -1)) {
            int len = static_cast<int>(luaL_len(L, -1));
            for (int i = 1; i <= len; ++i) {
                lua_rawgeti(L, -1, i);
                if (lua_isstring(L, -1)) {
                    if (!patch.message.empty()) {
                        patch.message += "\n";
                    }
                    patch.message += lua_tostring(L, -1);
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }

    bool ok = lua_api->getEditor()->getPopupManagerForLua()->updatePopup(it->second, patch);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

int UIAPI::lua_ui_close_window(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_win_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = window_handles_.find(lua_win_id);
    if (it == window_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool ok = lua_api->getEditor()->getPopupManagerForLua()->closePopup(it->second, false);
    window_handles_.erase(it);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

void UIAPI::closeAllWindows(pnana::core::Editor* editor) {
    if (!editor || !editor->getPopupManagerForLua()) {
        window_handles_.clear();
        return;
    }

    for (const auto& entry : window_handles_) {
        editor->getPopupManagerForLua()->closePopup(entry.second, false);
    }
    window_handles_.clear();
}

// vim.ui.progress(opts) -> progress_id
// 显示进度条窗口
int UIAPI::lua_ui_progress(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushnil(L);
        return 1;
    }

    std::string title = "Progress";
    std::string message = "Processing...";
    int percent = 0;
    bool indeterminate = false;

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "message");
        if (lua_isstring(L, -1)) {
            message = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "percent");
        if (lua_isnumber(L, -1)) {
            percent = static_cast<int>(lua_tonumber(L, -1));
            if (percent < 0)
                percent = 0;
            if (percent > 100)
                percent = 100;
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "indeterminate");
        if (lua_isboolean(L, -1)) {
            indeterminate = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
    }

    // 构建进度条显示内容
    std::string progress_bar;
    if (indeterminate) {
        progress_bar = "[    处理中...    ]";
    } else {
        int filled = percent / 5;
        int empty = 20 - filled;
        progress_bar = "[" + std::string(filled, '=') + std::string(empty, ' ') + "] " +
                       std::to_string(percent) + "%";
    }

    pnana::core::ui::PopupSpec spec;
    spec.title = title;
    spec.message = message + "\n\n" + progress_bar;
    spec.width = 60;
    spec.height = 8;

    int lua_progress_id = next_progress_id_++;
    int handle = lua_api->getEditor()->getPopupManagerForLua()->openPopup(spec, {});
    progress_handles_[lua_progress_id] = handle;

    lua_pushinteger(L, static_cast<lua_Integer>(lua_progress_id));
    return 1;
}

// vim.ui.update_progress(progress_id, opts) -> boolean
// 更新进度条
int UIAPI::lua_ui_update_progress(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_progress_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = progress_handles_.find(lua_progress_id);
    if (it == progress_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    std::string title;
    std::string message;
    int percent = -1;
    bool indeterminate = false;

    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "message");
        if (lua_isstring(L, -1)) {
            message = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "percent");
        if (lua_isnumber(L, -1)) {
            percent = static_cast<int>(lua_tonumber(L, -1));
            if (percent < 0)
                percent = 0;
            if (percent > 100)
                percent = 100;
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "indeterminate");
        if (lua_isboolean(L, -1)) {
            indeterminate = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
    }

    // 获取当前弹窗信息以保留未更改的字段
    pnana::core::ui::PopupSpec patch;
    if (!title.empty()) {
        patch.title = title;
    }

    // 构建进度条显示内容
    if (!message.empty() || percent >= 0) {
        std::string progress_bar;
        if (indeterminate) {
            progress_bar = "[    处理中...    ]";
        } else if (percent >= 0) {
            int filled = percent / 5;
            int empty = 20 - filled;
            progress_bar = "[" + std::string(filled, '=') + std::string(empty, ' ') + "] " +
                           std::to_string(percent) + "%";
        }
        patch.message = message + "\n\n" + progress_bar;
    }

    bool ok = lua_api->getEditor()->getPopupManagerForLua()->updatePopup(it->second, patch);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// vim.ui.close_progress(progress_id) -> boolean
// 关闭进度条窗口
int UIAPI::lua_ui_close_progress(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_progress_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = progress_handles_.find(lua_progress_id);
    if (it == progress_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool ok = lua_api->getEditor()->getPopupManagerForLua()->closePopup(it->second, false);
    progress_handles_.erase(it);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// vim.ui.multiselect(items, opts, callback) -> boolean
// 多选列表对话框
int UIAPI::lua_ui_multiselect(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_istable(L, 1) || !lua_isfunction(L, 3)) {
        lua_pushboolean(L, false);
        return 1;
    }

    std::string title = "Multi Select";
    std::string prompt = "Select items:";
    std::vector<std::string> item_labels;
    std::vector<bool> default_selected;

    // 解析项目列表
    int item_count = static_cast<int>(luaL_len(L, 1));
    for (int i = 1; i <= item_count; ++i) {
        lua_rawgeti(L, 1, i);
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "text");
            if (lua_isstring(L, -1)) {
                item_labels.push_back(lua_tostring(L, -1));
            } else {
                item_labels.push_back("<item " + std::to_string(i) + ">");
            }
            lua_pop(L, 1);

            lua_getfield(L, -1, "selected");
            default_selected.push_back(lua_isboolean(L, -1) && lua_toboolean(L, -1));
            lua_pop(L, 1);
        } else if (lua_isstring(L, -1)) {
            item_labels.push_back(lua_tostring(L, -1));
            default_selected.push_back(false);
        } else {
            item_labels.push_back("<item " + std::to_string(i) + ">");
            default_selected.push_back(false);
        }
        lua_pop(L, 1);
    }

    // 解析选项
    if (lua_istable(L, 2)) {
        lua_getfield(L, 2, "prompt");
        if (lua_isstring(L, -1)) {
            prompt = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }

    // 构建显示内容（带复选框）
    std::string message = prompt + "\n\n";
    for (size_t i = 0; i < item_labels.size(); ++i) {
        message += "[ ] " + item_labels[i] + "\n";
    }
    message += "\n(使用 vim.ui.select 单选模式，多选版本需要 PopupManager 支持)";

    lua_pushvalue(L, 3);
    int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_State* lua_state = L;

    // 暂时使用确认对话框，实际应该使用支持多选的 UI
    lua_api->getEditor()->showConfirmDialogForLua(
        title, message, [lua_state, callback_ref](bool ok) {
            lua_rawgeti(lua_state, LUA_REGISTRYINDEX, callback_ref);
            if (!lua_isfunction(lua_state, -1)) {
                lua_pop(lua_state, 1);
                luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
                return;
            }

            if (ok) {
                // 返回空表作为选中的项目（实际实现需要更复杂的逻辑）
                lua_newtable(lua_state);
            } else {
                lua_pushnil(lua_state);
            }

            if (lua_pcall(lua_state, 1, 0, 0) != LUA_OK) {
                const char* error = lua_tostring(lua_state, -1);
                LOG_ERROR("vim.ui.multiselect callback error: " +
                          std::string(error ? error : "unknown"));
                lua_pop(lua_state, 1);
            }
            luaL_unref(lua_state, LUA_REGISTRYINDEX, callback_ref);
        });

    lua_pushboolean(L, true);
    return 1;
}

// vim.ui.hover(opts) -> hover_id
// 显示悬浮提示
int UIAPI::lua_ui_hover(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushnil(L);
        return 1;
    }

    std::vector<std::string> content_lines;
    std::string anchor = "NW";

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "content");
        if (lua_istable(L, -1)) {
            int len = static_cast<int>(luaL_len(L, -1));
            for (int i = 1; i <= len; ++i) {
                lua_rawgeti(L, -1, i);
                if (lua_isstring(L, -1)) {
                    content_lines.emplace_back(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
            }
        } else if (lua_isstring(L, -1)) {
            content_lines.push_back(lua_tostring(L, -1));
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "anchor");
        if (lua_isstring(L, -1)) {
            anchor = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        (void)anchor; // TODO: 使用 anchor 参数定位悬浮提示
    }

    if (content_lines.empty()) {
        content_lines.push_back("(no content)");
    }

    // 构建消息内容
    std::string message;
    for (size_t i = 0; i < content_lines.size(); ++i) {
        if (i > 0)
            message += "\n";
        message += content_lines[i];
    }

    pnana::core::ui::PopupSpec spec;
    spec.title = "";
    spec.message = message;
    spec.width = 40;
    spec.height = static_cast<int>(content_lines.size()) + 2;
    spec.modal = false;

    int lua_hover_id = next_hover_id_++;
    int handle = lua_api->getEditor()->getPopupManagerForLua()->openPopup(spec, {});
    hover_handles_[lua_hover_id] = handle;

    lua_pushinteger(L, static_cast<lua_Integer>(lua_hover_id));
    return 1;
}

// vim.ui.close_hover(hover_id) -> boolean
// 关闭悬浮提示
int UIAPI::lua_ui_close_hover(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_hover_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = hover_handles_.find(lua_hover_id);
    if (it == hover_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool ok = lua_api->getEditor()->getPopupManagerForLua()->closePopup(it->second, false);
    hover_handles_.erase(it);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// vim.ui.list_windows() -> { {id=..., title=...}, ... }
// 列出所有打开的窗口
int UIAPI::lua_ui_list_windows(lua_State* L) {
    lua_newtable(L);

    int index = 1;
    for (const auto& entry : window_handles_) {
        lua_newtable(L);
        lua_pushinteger(L, static_cast<lua_Integer>(entry.first));
        lua_setfield(L, -2, "id");
        // 注意：这里无法直接获取标题，需要存储更多信息
        lua_pushstring(L, "window");
        lua_setfield(L, -2, "title");
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

// vim.ui.window_is_valid(win_id) -> boolean
// 检查窗口是否有效
int UIAPI::lua_ui_window_is_valid(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_win_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = window_handles_.find(lua_win_id);
    if (it == window_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool valid = lua_api->getEditor()->getPopupManagerForLua()->hasPopup(it->second);
    lua_pushboolean(L, valid ? 1 : 0);
    return 1;
}

// vim.ui.get_window_info(win_id) -> { title=..., width=..., height=... } | nil
// 获取窗口信息
int UIAPI::lua_ui_get_window_info(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushnil(L);
        return 1;
    }

    int lua_win_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = window_handles_.find(lua_win_id);
    if (it == window_handles_.end()) {
        lua_pushnil(L);
        return 1;
    }

    bool valid = lua_api->getEditor()->getPopupManagerForLua()->hasPopup(it->second);
    if (!valid) {
        lua_pushnil(L);
        return 1;
    }

    // 由于 PopupManager 没有提供获取窗口信息的接口，返回基本信息
    lua_newtable(L);
    lua_pushinteger(L, lua_win_id);
    lua_setfield(L, -2, "id");
    lua_pushstring(L, "window");
    lua_setfield(L, -2, "type");
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "valid");

    return 1;
}

// vim.ui.focus_window(win_id) -> boolean
// 聚焦窗口
int UIAPI::lua_ui_focus_window(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_win_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = window_handles_.find(lua_win_id);
    if (it == window_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    bool ok = lua_api->getEditor()->getPopupManagerForLua()->bringToFront(it->second);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// 辅助函数：解析布局选项
void UIAPI::parseLayoutOptions(lua_State* L, int index, pnana::core::ui::WidgetSpec& spec) {
    if (!lua_istable(L, index))
        return;

    lua_getfield(L, index, "direction");
    if (lua_isstring(L, -1)) {
        const char* dir = lua_tostring(L, -1);
        if (strcmp(dir, "horizontal") == 0 || strcmp(dir, "h") == 0) {
            spec.layout_direction = pnana::core::ui::LayoutDirection::HORIZONTAL;
        } else {
            spec.layout_direction = pnana::core::ui::LayoutDirection::VERTICAL;
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "align");
    if (lua_isstring(L, -1)) {
        const char* align = lua_tostring(L, -1);
        if (strcmp(align, "start") == 0) {
            spec.alignment = pnana::core::ui::Alignment::START;
        } else if (strcmp(align, "center") == 0) {
            spec.alignment = pnana::core::ui::Alignment::CENTER;
        } else if (strcmp(align, "end") == 0) {
            spec.alignment = pnana::core::ui::Alignment::END;
        } else if (strcmp(align, "stretch") == 0) {
            spec.alignment = pnana::core::ui::Alignment::STRETCH;
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "flex");
    if (lua_isnumber(L, -1)) {
        spec.flex = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "min_width");
    if (lua_isnumber(L, -1)) {
        spec.min_width = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "min_height");
    if (lua_isnumber(L, -1)) {
        spec.min_height = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "padding");
    if (lua_isnumber(L, -1)) {
        spec.padding = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "spacing");
    if (lua_isnumber(L, -1)) {
        spec.spacing = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "border");
    if (lua_isstring(L, -1)) {
        spec.border_style = lua_tostring(L, -1);
    }
    lua_pop(L, 1);
}

// 辅助函数：从 Lua 表解析 WidgetSpec
pnana::core::ui::WidgetSpec UIAPI::parseWidgetSpecFromLua(lua_State* L, int index) {
    pnana::core::ui::WidgetSpec spec;

    if (!lua_istable(L, index)) {
        spec.type = pnana::core::ui::WidgetType::TEXT;
        spec.label = "(invalid widget)";
        return spec;
    }

    // 解析类型
    lua_getfield(L, index, "type");
    if (lua_isstring(L, -1)) {
        const char* type_str = lua_tostring(L, -1);
        // 基础显示组件
        if (strcmp(type_str, "text") == 0) {
            spec.type = pnana::core::ui::WidgetType::TEXT;
        } else if (strcmp(type_str, "paragraph") == 0) {
            spec.type = pnana::core::ui::WidgetType::PARAGRAPH;
        } else if (strcmp(type_str, "separator") == 0) {
            spec.type = pnana::core::ui::WidgetType::SEPARATOR;
        } else if (strcmp(type_str, "canvas") == 0) {
            spec.type = pnana::core::ui::WidgetType::CANVAS;
        } else if (strcmp(type_str, "spinner") == 0) {
            spec.type = pnana::core::ui::WidgetType::SPINNER;
        } else if (strcmp(type_str, "image") == 0) {
            spec.type = pnana::core::ui::WidgetType::IMAGE;
        } else if (strcmp(type_str, "animation") == 0) {
            spec.type = pnana::core::ui::WidgetType::ANIMATION;
        } else if (strcmp(type_str, "bullet") == 0) {
            spec.type = pnana::core::ui::WidgetType::BULLET;
        } else if (strcmp(type_str, "link") == 0) {
            spec.type = pnana::core::ui::WidgetType::LINK;
        }
        // 基础交互组件
        else if (strcmp(type_str, "input") == 0) {
            spec.type = pnana::core::ui::WidgetType::INPUT;
            spec.focusable = true;
        } else if (strcmp(type_str, "textarea") == 0) {
            spec.type = pnana::core::ui::WidgetType::TEXTAREA;
            spec.focusable = true;
        } else if (strcmp(type_str, "button") == 0) {
            spec.type = pnana::core::ui::WidgetType::BUTTON;
            spec.focusable = true;
        } else if (strcmp(type_str, "checkbox") == 0) {
            spec.type = pnana::core::ui::WidgetType::CHECKBOX;
            spec.focusable = true;
        } else if (strcmp(type_str, "radiobox") == 0) {
            spec.type = pnana::core::ui::WidgetType::RADIOBOX;
            spec.focusable = true;
        } else if (strcmp(type_str, "toggle") == 0) {
            spec.type = pnana::core::ui::WidgetType::TOGGLE;
            spec.focusable = true;
        } else if (strcmp(type_str, "slider") == 0) {
            spec.type = pnana::core::ui::WidgetType::SLIDER;
            spec.focusable = true;
        } else if (strcmp(type_str, "dropdown") == 0) {
            spec.type = pnana::core::ui::WidgetType::DROPDOWN;
            spec.focusable = true;
        } else if (strcmp(type_str, "menu") == 0) {
            spec.type = pnana::core::ui::WidgetType::MENU;
            spec.focusable = true;
        } else if (strcmp(type_str, "color_picker") == 0) {
            spec.type = pnana::core::ui::WidgetType::COLOR_PICKER;
            spec.focusable = true;
        } else if (strcmp(type_str, "file_picker") == 0) {
            spec.type = pnana::core::ui::WidgetType::FILE_PICKER;
            spec.focusable = true;
        } else if (strcmp(type_str, "gauge") == 0) {
            spec.type = pnana::core::ui::WidgetType::GAUGE;
        } else if (strcmp(type_str, "list") == 0) {
            spec.type = pnana::core::ui::WidgetType::LIST;
            spec.focusable = true;
        }
        // 容器组件
        else if (strcmp(type_str, "window") == 0) {
            spec.type = pnana::core::ui::WidgetType::WINDOW;
        } else if (strcmp(type_str, "container") == 0) {
            spec.type = pnana::core::ui::WidgetType::CONTAINER;
        } else if (strcmp(type_str, "group") == 0) {
            spec.type = pnana::core::ui::WidgetType::GROUP;
        } else if (strcmp(type_str, "hbox") == 0) {
            spec.type = pnana::core::ui::WidgetType::HBOX;
        } else if (strcmp(type_str, "vbox") == 0) {
            spec.type = pnana::core::ui::WidgetType::VBOX;
        } else if (strcmp(type_str, "dbox") == 0) {
            spec.type = pnana::core::ui::WidgetType::DBOX;
        } else if (strcmp(type_str, "split") == 0) {
            spec.type = pnana::core::ui::WidgetType::SPLIT;
        } else if (strcmp(type_str, "resizable_split") == 0) {
            spec.type = pnana::core::ui::WidgetType::RESIZABLE_SPLIT;
        } else if (strcmp(type_str, "tabs") == 0) {
            spec.type = pnana::core::ui::WidgetType::TABS;
        } else if (strcmp(type_str, "grid") == 0) {
            spec.type = pnana::core::ui::WidgetType::GRID;
        } else if (strcmp(type_str, "frame") == 0) {
            spec.type = pnana::core::ui::WidgetType::FRAME;
        } else if (strcmp(type_str, "yframe") == 0) {
            spec.type = pnana::core::ui::WidgetType::YFRAME;
        } else if (strcmp(type_str, "xframe") == 0) {
            spec.type = pnana::core::ui::WidgetType::XFRAME;
        } else if (strcmp(type_str, "vscroll") == 0) {
            spec.type = pnana::core::ui::WidgetType::VSCROLL;
        } else if (strcmp(type_str, "hscroll") == 0) {
            spec.type = pnana::core::ui::WidgetType::HSCROLL;
        }
        // 弹窗/模态组件
        else if (strcmp(type_str, "modal") == 0) {
            spec.type = pnana::core::ui::WidgetType::MODAL;
        } else if (strcmp(type_str, "popup") == 0) {
            spec.type = pnana::core::ui::WidgetType::POPUP;
        } else if (strcmp(type_str, "notification") == 0) {
            spec.type = pnana::core::ui::WidgetType::NOTIFICATION;
        }
    }
    lua_pop(L, 1);

    // 解析 ID
    lua_getfield(L, index, "id");
    if (lua_isstring(L, -1)) {
        spec.id = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析标签/文本
    lua_getfield(L, index, "text");
    if (lua_isstring(L, -1)) {
        spec.label = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "label");
    if (lua_isstring(L, -1)) {
        spec.label = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析值
    lua_getfield(L, index, "value");
    if (lua_isstring(L, -1)) {
        spec.value = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析列表项
    lua_getfield(L, index, "items");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_isstring(L, -1)) {
                spec.items.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    // 解析布局选项
    parseLayoutOptions(L, index, spec);

    // 递归解析子元素
    lua_getfield(L, index, "children");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            spec.children.push_back(parseWidgetSpecFromLua(L, -1));
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    return spec;
}

// vim.ui.create_layout(opts) -> layout_spec
// 创建布局规范（用于复杂的自定义布局）
int UIAPI::lua_ui_create_layout(lua_State* L) {
    // 解析传入的布局定义
    pnana::core::ui::WidgetSpec spec = parseWidgetSpecFromLua(L, 1);

    // 将 spec 存储在 Lua 注册表中，返回引用
    // 这里简化处理，直接返回一个表表示布局
    lua_newtable(L);

    lua_pushstring(L, "layout");
    lua_setfield(L, -2, "_type");

    // 存储类型
    switch (spec.type) {
        case pnana::core::ui::WidgetType::HBOX:
            lua_pushstring(L, "hbox");
            break;
        case pnana::core::ui::WidgetType::VBOX:
            lua_pushstring(L, "vbox");
            break;
        case pnana::core::ui::WidgetType::CONTAINER:
            lua_pushstring(L, "container");
            break;
        default:
            lua_pushstring(L, "container");
            break;
    }
    lua_setfield(L, -2, "layout_type");

    // 存储原始定义供后续使用
    lua_pushvalue(L, 1);
    lua_setfield(L, -2, "_definition");

    return 1;
}

// vim.ui.open_layout_window(opts) -> win_id
// 使用复杂布局打开窗口
int UIAPI::lua_ui_open_layout_window(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushnil(L);
        return 1;
    }

    std::string title = "Layout Window";
    int width = 80;
    int height = 24;
    pnana::core::ui::WidgetSpec root_spec;

    if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "title");
        if (lua_isstring(L, -1)) {
            title = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "width");
        if (lua_isnumber(L, -1)) {
            width = static_cast<int>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);

        lua_getfield(L, 1, "height");
        if (lua_isnumber(L, -1)) {
            height = static_cast<int>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);

        // 解析布局
        lua_getfield(L, 1, "layout");
        if (lua_istable(L, -1)) {
            root_spec = parseWidgetSpecFromLua(L, -1);
        }
        lua_pop(L, 1);
    }

    // 创建 PopupSpec
    pnana::core::ui::PopupSpec spec;
    spec.title = title;
    spec.width = width;
    spec.height = height;
    spec.modal = true;

    // 设置自定义布局（如果有）
    if (root_spec.type != pnana::core::ui::WidgetType::TEXT || !root_spec.children.empty()) {
        spec.root = root_spec;
        // 如果 root 没有标签，使用标题
        if (spec.root.label.empty()) {
            spec.root.label = title;
        }
    }

    int lua_win_id = next_window_id_++;
    int handle = lua_api->getEditor()->getPopupManagerForLua()->openPopup(spec, {});
    window_handles_[lua_win_id] = handle;

    lua_pushinteger(L, static_cast<lua_Integer>(lua_win_id));
    return 1;
}

// vim.ui.update_layout(win_id, layout) -> boolean
// 更新窗口布局
int UIAPI::lua_ui_update_layout(lua_State* L) {
    LuaAPI* lua_api = getLuaAPIFromLua(L);
    if (!lua_api || !lua_api->getEditor() || !lua_api->getEditor()->getPopupManagerForLua()) {
        lua_pushboolean(L, false);
        return 1;
    }

    int lua_win_id = static_cast<int>(lua_tointeger(L, 1));
    auto it = window_handles_.find(lua_win_id);
    if (it == window_handles_.end()) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 解析新布局
    pnana::core::ui::WidgetSpec new_root = parseWidgetSpecFromLua(L, 2);

    // 构建更新
    pnana::core::ui::PopupSpec patch;
    if (new_root.type != pnana::core::ui::WidgetType::TEXT || !new_root.children.empty()) {
        patch.root = new_root;
    }

    bool ok = lua_api->getEditor()->getPopupManagerForLua()->updatePopup(it->second, patch);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
