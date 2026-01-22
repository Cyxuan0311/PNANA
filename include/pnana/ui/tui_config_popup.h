#ifndef PNANA_UI_TUI_CONFIG_POPUP_H
#define PNANA_UI_TUI_CONFIG_POPUP_H

#include "features/tui_config_manager.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// TUI配置文件选择弹窗类
class TUIConfigPopup {
  public:
    explicit TUIConfigPopup(Theme& theme);

    // 设置数据
    void setData(bool is_open, const std::vector<features::TUIConfig>& tui_configs,
                 size_t selected_index);

    // 渲染弹窗
    ftxui::Element render();

    // 事件处理
    bool handleInput(ftxui::Event event);

    // 获取当前选中的索引
    size_t getSelectedIndex() const {
        return selected_index_;
    }

    // 检查是否打开
    bool isOpen() const {
        return is_open_;
    }

    // 打开/关闭弹窗
    void open();
    void close();

    // 设置配置打开回调
    void setConfigOpenCallback(std::function<void(const features::TUIConfig&)> callback);

  private:
    Theme& theme_;
    bool is_open_;
    std::vector<features::TUIConfig> tui_configs_;
    size_t selected_index_;

    // 配置打开回调
    std::function<void(const features::TUIConfig&)> config_open_callback_;

    // 渲染各个组件
    ftxui::Element renderTitle() const;
    ftxui::Element renderConfigList() const;
    ftxui::Element renderHelpBar() const;

    // 获取配置文件的显示路径
    std::string getConfigPathDisplay(const features::TUIConfig& config) const;

  private:
    // 获取类别显示名称
    std::string getCategoryDisplayName(const std::string& category) const;

    // 展开路径（处理~和环境变量）
    std::filesystem::path expandPath(const std::string& path) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_TUI_CONFIG_POPUP_H
