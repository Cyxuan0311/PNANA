#ifndef PNANA_PLUGINS_LUA_API_H
#define PNANA_PLUGINS_LUA_API_H

#ifdef BUILD_LUA_SUPPORT

#include "plugins/editor_api.h"
#include "plugins/file_api.h"
#include "plugins/lua_engine.h"
#include "plugins/system_api.h"
#include "plugins/theme_api.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

// 前向声明
namespace pnana {
namespace core {
class Editor;
}
} // namespace pnana

namespace pnana {
namespace plugins {

/**
 * @brief Lua API 主控制器类
 *
 * 使用组合模式将不同的API功能分离到专门的类中：
 * - EditorAPI: 编辑器操作（文档、光标等）
 * - FileAPI: 文件操作（打开、保存、读写等）
 * - ThemeAPI: 主题和外观管理
 * - SystemAPI: 系统工具和事件处理
 */
class LuaAPI {
  public:
    explicit LuaAPI(core::Editor* editor);
    ~LuaAPI();

    // 初始化 API（注册所有函数到 Lua）
    void initialize(LuaEngine* engine);

    // 触发事件
    void triggerEvent(const std::string& event, const std::vector<std::string>& args = {});

    // 注册事件监听器
    void registerEventListener(const std::string& event, const std::string& callback);
    void registerEventListenerFunction(const std::string& event);

    // 注册命令（旧API兼容）
    void registerCommand(const std::string& name, const std::string& callback);

    // 注册键位映射（旧API兼容）
    void registerKeymap(const std::string& mode, const std::string& keys,
                        const std::string& callback);

    // 注册命令（新API，支持选项）
    void registerUserCommand(const std::string& name, int callback_ref, const std::string& nargs,
                             const std::string& desc, bool force);

    // 删除命令
    bool delUserCommand(const std::string& name);

    // 执行命令
    bool executeCommand(const std::string& name, const std::string& args = "");

    // 注册键映射（新API，支持选项）
    void registerKeymap(const std::string& mode, const std::string& lhs, int rhs_ref, bool noremap,
                        bool silent, bool expr, bool nowait, const std::string& desc);

    // 注册键映射（新API，字符串rhs）
    void registerKeymap(const std::string& mode, const std::string& lhs,
                        const std::string& rhs_string, bool noremap, bool silent, bool expr,
                        bool nowait, const std::string& desc);

    // 删除键映射
    bool delKeymap(const std::string& mode, const std::string& lhs);

    // 执行键映射
    bool executeKeymap(const std::string& mode, const std::string& lhs);

    // 注册autocmd（新API，支持选项）
    void registerAutocmd(const std::string& event, int callback_ref, const std::string& pattern,
                         bool once, bool nested, const std::string& desc, const std::string& group);

    // 清除autocmd
    void clearAutocmds(const std::string& event, const std::string& pattern,
                       const std::string& group);

    // 获取编辑器实例
    core::Editor* getEditor() {
        return editor_;
    }

    // 获取Lua引擎
    LuaEngine* getEngine() {
        return engine_;
    }

    // 获取FileAPI实例（用于设置路径验证器）
    FileAPI* getFileAPI() {
        return file_api_.get();
    }

  private:
    core::Editor* editor_;
    LuaEngine* engine_;

    // 组合的API组件
    std::unique_ptr<EditorAPI> editor_api_;
    std::unique_ptr<FileAPI> file_api_;
    std::unique_ptr<ThemeAPI> theme_api_;
    std::unique_ptr<SystemAPI> system_api_;

    // 事件监听器映射: event -> [callbacks] (旧API兼容)
    std::map<std::string, std::vector<std::string>> event_listeners_;

    // 函数引用事件监听器: event -> [function_refs] (新API)
    std::map<std::string, std::vector<int>> event_function_listeners_;

    // Autocmd信息: event -> [{ref, pattern, once, nested, group}]
    struct AutocmdInfo {
        int callback_ref;
        std::string pattern;
        bool once;
        bool nested;
        std::string group;
    };
    std::map<std::string, std::vector<AutocmdInfo>> autocmds_;

    // 命令映射: name -> callback (旧API兼容)
    std::map<std::string, std::string> commands_;

    // 用户命令信息: name -> {callback_ref, nargs, desc}
    struct UserCommandInfo {
        int callback_ref;
        std::string nargs; // "0", "1", "*", "?", "+"
        std::string desc;
        bool force;
    };
    std::map<std::string, UserCommandInfo> user_commands_;

    // 键位映射: mode -> keys -> callback (旧API兼容)
    std::map<std::string, std::map<std::string, std::string>> keymaps_;

    // 键映射信息: mode -> lhs -> {rhs_ref, noremap, silent, expr, nowait, desc}
    struct KeymapInfo {
        int rhs_ref;            // Lua函数引用，如果是字符串则为-1
        std::string rhs_string; // 字符串形式的rhs
        bool noremap;
        bool silent;
        bool expr;
        bool nowait;
        std::string desc;
    };
    std::map<std::string, std::map<std::string, KeymapInfo>> keymaps_info_;

    // 注册 API 函数
    void registerAPIFunctions();

    // Lua API 函数（静态，供 Lua 调用）
    static int lua_api_notify(lua_State* L);
    static int lua_api_command(lua_State* L);
    static int lua_api_keymap(lua_State* L);
    static int lua_api_autocmd(lua_State* L);

    // 辅助函数：从 Lua 栈获取编辑器实例
    static core::Editor* getEditorFromLua(lua_State* L);
    static LuaAPI* getAPIFromLua(lua_State* L);
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_LUA_API_H
