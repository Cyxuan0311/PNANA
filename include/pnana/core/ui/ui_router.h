#ifndef PNANA_CORE_UI_UI_ROUTER_H
#define PNANA_CORE_UI_UI_ROUTER_H

#include "core/ui/base_region_renderer.h"
#include "core/ui/border_manager.h"
#include <ftxui/dom/elements.hpp>
#include <map>
#include <memory>

namespace pnana {
namespace core {

// 前向声明
class Editor;

namespace ui {

// UI渲染路由器：统一的UI渲染系统
class UIRouter {
  public:
    UIRouter();
    ~UIRouter();

    // 主渲染方法（替代 Editor::renderUI 的核心逻辑）
    ftxui::Element render(Editor* editor);

  private:
    // 根据区域渲染对应的面板
    ftxui::Element renderByRegion(Editor* editor);

    // 渲染标签栏（带边框）
    ftxui::Element renderTabbar(Editor* editor);

    // 渲染主内容区域
    ftxui::Element renderMainContent(Editor* editor);

    // 渲染状态栏、输入框、帮助栏
    ftxui::Element renderStatusAndHelp(Editor* editor);

    // 叠加对话框（如果打开）
    ftxui::Element overlayDialogs(ftxui::Element main_ui, Editor* editor);

    // 初始化区域渲染器
    void initializeRegionRenderers();

    // 区域渲染器映射
    std::map<EditorRegion, std::unique_ptr<BaseRegionRenderer>> region_renderers_;

    // 边框管理器
    BorderManager border_manager_;

    // 是否已初始化
    bool initialized_;
};

} // namespace ui
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_UI_UI_ROUTER_H
