#ifndef PNANA_UI_PLUGIN_MANAGER_DIALOG_H
#define PNANA_UI_PLUGIN_MANAGER_DIALOG_H

#include "ui/theme.h"
#include "plugins/plugin_manager.h"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 插件管理对话框
class PluginManagerDialog {
public:
    explicit PluginManagerDialog(Theme& theme, plugins::PluginManager* plugin_manager);
    
    // 打开对话框
    void open();
    
    // 关闭对话框
    void close();
    
    // 是否可见
    bool isVisible() const { return visible_; }
    
    // 处理输入
    bool handleInput(ftxui::Event event);
    
    // 渲染对话框
    ftxui::Element render();
    
    // 刷新插件列表
    void refreshPlugins();
    
    // 设置插件管理器（用于延迟初始化）
    void setPluginManager(plugins::PluginManager* plugin_manager);

private:
    Theme& theme_;
    plugins::PluginManager* plugin_manager_;
    bool visible_;
    
    std::vector<plugins::PluginInfo> plugins_;
    size_t selected_index_;
    
    // 渲染插件列表
    ftxui::Element renderPluginList();
    
    // 渲染单个插件项
    ftxui::Element renderPluginItem(const plugins::PluginInfo& plugin, size_t index, bool is_selected);
    
    // 移动选择
    void selectNext();
    void selectPrevious();
    
    // 切换插件状态
    void togglePlugin(size_t index);
    
    // 关闭对话框
    void apply();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_PLUGIN_MANAGER_DIALOG_H

