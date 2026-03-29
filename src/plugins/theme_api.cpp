#ifdef BUILD_LUA_SUPPORT

#include "plugins/theme_api.h"
#include "core/editor.h"
#include "ui/statusbar.h"
#include "ui/theme.h"
#include <ftxui/screen/color.hpp>
#include <lua.hpp>

namespace pnana {
namespace plugins {

// 在 Lua 注册表中存储编辑器指针的键
static const char* kEditorRegistryKey = "pnana_editor";

ThemeAPI::ThemeAPI(core::Editor* editor) : editor_(editor) {}

ThemeAPI::~ThemeAPI() {}

void ThemeAPI::registerFunctions(lua_State* L) {
    // 在注册表中存储编辑器指针
    lua_pushlightuserdata(L, editor_);
    lua_setfield(L, LUA_REGISTRYINDEX, kEditorRegistryKey);

    // 注册 API 函数到 vim.api 表
    lua_getglobal(L, "vim");
    lua_getfield(L, -1, "api");

    lua_pushcfunction(L, lua_fn_get_theme);
    lua_setfield(L, -2, "get_theme");

    lua_pushcfunction(L, lua_fn_get_current_theme);
    lua_setfield(L, -2, "get_current_theme");

    lua_pushcfunction(L, lua_fn_set_theme);
    lua_setfield(L, -2, "set_theme");

    lua_pushcfunction(L, lua_fn_add_theme);
    lua_setfield(L, -2, "add_theme");

    lua_pushcfunction(L, lua_fn_remove_theme);
    lua_setfield(L, -2, "remove_theme");

    lua_pushcfunction(L, lua_fn_set_status_message);
    lua_setfield(L, -2, "set_status_message");

    lua_pushcfunction(L, lua_fn_set_statusbar_beautify);
    lua_setfield(L, -2, "set_statusbar_beautify");

    lua_pop(L, 2); // 弹出 vim 和 api 表
}

core::Editor* ThemeAPI::getEditorFromLua(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, kEditorRegistryKey);
    core::Editor* editor = static_cast<core::Editor*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return editor;
}

ui::ThemeColors ThemeAPI::parseThemeColorsFromLua(lua_State* L, int tableIndex) {
    ui::ThemeColors colors;

    // 优化：批量解析颜色，减少 Lua API 调用
    // 使用单次遍历解析所有颜色字段
    static const char* color_keys[] = {"background",   "foreground",     "current_line",
                                       "selection",    "line_number",    "line_number_current",
                                       "statusbar_bg", "statusbar_fg",   "menubar_bg",
                                       "menubar_fg",   "helpbar_bg",     "helpbar_fg",
                                       "helpbar_key",  "keyword",        "string",
                                       "comment",      "number",         "function",
                                       "type",         "operator_color", "error",
                                       "warning",      "info",           "success"};

    // 优化：预分配 RGB 数组
    std::vector<int> rgb;
    rgb.reserve(3);

    // 优化 Lambda：解析单个颜色值
    auto parse_color = [&](const char* key) -> ftxui::Color {
        lua_getfield(L, tableIndex, key);
        if (lua_istable(L, -1)) {
            rgb.clear();
            for (int i = 1; i <= 3; ++i) {
                lua_rawgeti(L, -1, i);
                if (lua_isnumber(L, -1)) {
                    rgb.push_back(static_cast<int>(lua_tonumber(L, -1)));
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
            if (rgb.size() == 3) {
                return ftxui::Color::RGB(rgb[0], rgb[1], rgb[2]);
            }
        } else {
            lua_pop(L, 1);
        }
        return ftxui::Color::Default;
    };

    // 批量解析所有颜色
    colors.background = parse_color(color_keys[0]);
    colors.foreground = parse_color(color_keys[1]);
    colors.current_line = parse_color(color_keys[2]);
    colors.selection = parse_color(color_keys[3]);
    colors.line_number = parse_color(color_keys[4]);
    colors.line_number_current = parse_color(color_keys[5]);
    colors.statusbar_bg = parse_color(color_keys[6]);
    colors.statusbar_fg = parse_color(color_keys[7]);
    colors.menubar_bg = parse_color(color_keys[8]);
    colors.menubar_fg = parse_color(color_keys[9]);
    colors.helpbar_bg = parse_color(color_keys[10]);
    colors.helpbar_fg = parse_color(color_keys[11]);
    colors.helpbar_key = parse_color(color_keys[12]);
    colors.keyword = parse_color(color_keys[13]);
    colors.string = parse_color(color_keys[14]);
    colors.comment = parse_color(color_keys[15]);
    colors.number = parse_color(color_keys[16]);
    colors.function = parse_color(color_keys[17]);
    colors.type = parse_color(color_keys[18]);
    colors.operator_color = parse_color(color_keys[19]);
    colors.error = parse_color(color_keys[20]);
    colors.warning = parse_color(color_keys[21]);
    colors.info = parse_color(color_keys[22]);
    colors.success = parse_color(color_keys[23]);

    return colors;
}

ui::StatusbarBeautifyConfig ThemeAPI::parseStatusbarConfigFromLua(lua_State* L, int tableIndex) {
    ui::StatusbarBeautifyConfig config;

    // 优化：预分配 RGB 数组
    std::vector<int> rgb;
    rgb.reserve(3);

    // 优化：批量解析配置，减少 Lua API 调用
    lua_pushnil(L); // 第一次调用 lua_next 的 key
    while (lua_next(L, tableIndex) != 0) {
        if (lua_isstring(L, -2)) {
            const char* key = lua_tostring(L, -2);

            if (strcmp(key, "enabled") == 0 && lua_isboolean(L, -1)) {
                config.enabled = lua_toboolean(L, -1);
            } else if ((strcmp(key, "bg_color") == 0 || strcmp(key, "fg_color") == 0) &&
                       lua_istable(L, -1)) {
                rgb.clear();
                for (int i = 1; i <= 3; ++i) {
                    lua_rawgeti(L, -1, i);
                    if (lua_isnumber(L, -1)) {
                        rgb.push_back(static_cast<int>(lua_tonumber(L, -1)));
                    }
                    lua_pop(L, 1);
                }
                if (rgb.size() == 3) {
                    if (strcmp(key, "bg_color") == 0) {
                        config.bg_color = rgb;
                    } else {
                        config.fg_color = rgb;
                    }
                }
            }
        }
        lua_pop(L, 1); // 移除 value，保留 key 用于下一次迭代
    }

    return config;
}

// vim.api.get_theme() -> string
int ThemeAPI::lua_fn_get_theme(lua_State* L) {
    // TODO: 实现获取主题
    lua_pushstring(L, "monokai");
    return 1;
}

// vim.api.get_current_theme() -> string
int ThemeAPI::lua_fn_get_current_theme(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushstring(L, "monokai");
        return 1;
    }

    std::string current_theme = editor->getTheme().getCurrentThemeName();
    lua_pushstring(L, current_theme.c_str());
    return 1;
}

// vim.api.set_theme(theme_name)
int ThemeAPI::lua_fn_set_theme(lua_State* L) {
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

// vim.api.add_theme(theme_name, theme_config)
int ThemeAPI::lua_fn_add_theme(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* theme_name = lua_tostring(L, 1);
    if (!theme_name || !lua_istable(L, 2)) {
        lua_pushboolean(L, false);
        return 1;
    }

    ui::ThemeColors colors = parseThemeColorsFromLua(L, 2);

    // 添加到主题系统
    bool success = editor->getTheme().loadCustomTheme(std::string(theme_name), colors);
    if (success) {
        // 通知编辑器刷新主题菜单
        std::vector<std::string> available_themes;
        auto& config = editor->getConfigManager().getConfig();
        if (!config.available_themes.empty()) {
            available_themes = config.available_themes;
        } else {
            // 使用 Theme 类提供的所有可用主题
            available_themes = ::pnana::ui::Theme::getAvailableThemes();
        }

        // 添加自定义主题（包括插件添加的主题）
        std::vector<std::string> custom_themes = editor->getTheme().getCustomThemeNames();
        available_themes.insert(available_themes.end(), custom_themes.begin(), custom_themes.end());

        editor->getThemeMenu().setAvailableThemes(available_themes);
    }
    lua_pushboolean(L, success);
    return 1;
}

// vim.api.remove_theme(theme_name) -> boolean
int ThemeAPI::lua_fn_remove_theme(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushboolean(L, false);
        return 1;
    }

    const char* theme_name = lua_tostring(L, 1);
    if (!theme_name) {
        lua_pushboolean(L, false);
        return 1;
    }

    // 移除自定义主题
    bool success = editor->getTheme().removeCustomTheme(std::string(theme_name));
    if (success) {
        // 通知编辑器刷新主题菜单
        std::vector<std::string> available_themes;
        auto& config = editor->getConfigManager().getConfig();
        if (!config.available_themes.empty()) {
            available_themes = config.available_themes;
        } else {
            // 使用 Theme 类提供的所有可用主题
            available_themes = ::pnana::ui::Theme::getAvailableThemes();
        }

        // 添加自定义主题（包括插件添加的主题）
        std::vector<std::string> custom_themes = editor->getTheme().getCustomThemeNames();
        available_themes.insert(available_themes.end(), custom_themes.begin(), custom_themes.end());

        editor->getThemeMenu().setAvailableThemes(available_themes);
    }
    lua_pushboolean(L, success);
    return 1;
}

// vim.api.set_status_message(message)
int ThemeAPI::lua_fn_set_status_message(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        return 0;
    }

    const char* message = lua_tostring(L, 1);
    if (message) {
        // LuaAPI 是 Editor 的友元类，可以访问私有方法
        editor->setStatusMessageForLua(std::string(message));
    }
    return 0;
}

// vim.api.set_statusbar_beautify(config)
int ThemeAPI::lua_fn_set_statusbar_beautify(lua_State* L) {
    core::Editor* editor = getEditorFromLua(L);
    if (!editor) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (!lua_istable(L, 1)) {
        lua_pushboolean(L, false);
        return 1;
    }

    ui::StatusbarBeautifyConfig config = parseStatusbarConfigFromLua(L, 1);

    // 应用配置到状态栏
    editor->setStatusbarBeautify(config);

    lua_pushboolean(L, true);
    return 1;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
