#ifndef PNANA_CORE_INPUT_INPUT_ROUTER_H
#define PNANA_CORE_INPUT_INPUT_ROUTER_H

#include "core/input/base_mode_handler.h"
#include "core/input/base_region_handler.h"
#include <ftxui/component/event.hpp>
#include <map>
#include <memory>

namespace pnana {
namespace core {

// 前向声明
class Editor;
enum class EditorMode;

namespace input {

// 输入路由器：统一的事件分发系统
class InputRouter {
  public:
    InputRouter();
    ~InputRouter();

    // 主路由方法（替代 Editor::handleInput 的核心逻辑）
    bool route(ftxui::Event event, Editor* editor);

  private:
    // 检查全局快捷键（在任何情况下都有效）
    bool handleGlobalShortcuts(ftxui::Event event, Editor* editor);

    // 检查对话框优先级（按优先级顺序）
    bool handleDialogs(ftxui::Event event, Editor* editor);

    // 检查分屏导航（在分屏模式下优先级较高）
    bool handleSplitNavigation(ftxui::Event event, Editor* editor);

    // 根据区域分发到对应的 RegionHandler
    bool routeByRegion(ftxui::Event event, Editor* editor);

    // 根据模式分发（在代码区内部）
    bool routeByMode(ftxui::Event event, Editor* editor);

    // 初始化区域处理器
    void initializeRegionHandlers();

    // 初始化模式处理器
    void initializeModeHandlers();

    // 区域处理器映射
    std::map<EditorRegion, std::unique_ptr<BaseRegionHandler>> region_handlers_;

    // 模式处理器映射（用于代码区）
    std::map<EditorMode, std::unique_ptr<BaseModeHandler>> mode_handlers_;

    // 是否已初始化
    bool initialized_;
};

} // namespace input
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_INPUT_INPUT_ROUTER_H
