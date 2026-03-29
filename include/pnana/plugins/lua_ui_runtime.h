#ifndef PNANA_PLUGINS_LUA_UI_RUNTIME_H
#define PNANA_PLUGINS_LUA_UI_RUNTIME_H

#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_engine.h"
#include <string>

namespace pnana {
namespace core {
class Editor;
}
namespace plugins {

/**
 * @brief 基于 sol2 的 Lua -> FTXUI 运行时绑定层
 *
 * 负责向 Lua 暴露 vim.ftxui.* DSL，生成与 C++ FTXUI API 风格一致的声明式 UI 表达。
 */
class LuaUIRuntime {
  public:
    LuaUIRuntime(core::Editor* editor, LuaEngine* engine);

    // 注册 vim.ftxui 命名空间与 DSL
    bool initialize();

  private:
    core::Editor* editor_;
    LuaEngine* engine_;

    static std::string normalizeWidgetType(const std::string& type);
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_LUA_UI_RUNTIME_H
