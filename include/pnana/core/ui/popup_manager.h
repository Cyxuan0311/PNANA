#ifndef PNANA_CORE_UI_POPUP_MANAGER_H
#define PNANA_CORE_UI_POPUP_MANAGER_H

#include "core/ui/layout_engine.h"
#include "core/ui/widget.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pnana {
namespace core {
namespace ui {

using PopupHandle = int;

struct PopupCallbacks {
    std::function<void(bool)> on_bool_result;
    std::function<void(bool, const std::string&)> on_input_result;
    std::function<void(bool, std::size_t)> on_select_result;
    std::function<void(const std::string& widget_id)> on_widget_action;
    std::function<bool(const std::string& event_name, const std::string& payload)>
        on_component_event;
};

struct PopupSpec {
    std::string title;
    std::string message;
    std::string prompt;
    std::string default_value;
    std::vector<std::string> items;
    bool modal = true;
    bool center = true;
    int width = 56;
    int height = 12;
    WidgetSpec root;

    // Component 模式：由外部（Lua）提供 lines 渲染与事件处理
    bool component_mode = false;
    std::vector<std::string> component_lines;

    // 结构化 component 数据（使用 FTXUI 原生 window/border 渲染）
    std::string component_input_line;
    std::string component_left_title = "Results";
    std::string component_right_title = "Preview";
    std::vector<std::string> component_left_lines;
    std::vector<std::string> component_right_lines;
    std::vector<std::string> component_help_lines;

    // 每行文本的颜色配置（新增）
    std::vector<std::map<std::string, std::string>> component_left_line_colors;
    std::vector<std::map<std::string, std::string>> component_right_line_colors;

    // 标题装饰器配置
    WidgetSpec::TitleDecorators window_title_decorators;
};

class PopupManager {
  public:
    explicit PopupManager(::pnana::ui::Theme* theme = nullptr);
    ~PopupManager() = default;

    PopupHandle openPopup(const PopupSpec& spec, PopupCallbacks callbacks = {});
    bool updatePopup(PopupHandle handle, const PopupSpec& patch);
    bool closePopup(PopupHandle handle, bool accepted = false, const std::string& value = "",
                    std::size_t selected = 0);

    bool isVisible() const;
    bool hasPopup(PopupHandle handle) const;

    bool handleInput(ftxui::Event event);
    ftxui::Element render(ftxui::Element base_ui, int screen_w, int screen_h);

    // 事件路由增强
    bool routeEventToAnyEligiblePopup(ftxui::Event event, bool allow_non_top_non_modal = false);
    bool bringToFront(PopupHandle handle);

    PopupHandle openInput(const std::string& title, const std::string& prompt,
                          const std::string& default_value,
                          std::function<void(bool, const std::string&)> on_result);

    PopupHandle openConfirm(const std::string& title, const std::string& message,
                            std::function<void(bool)> on_result);

    PopupHandle openSelect(const std::string& title, const std::string& prompt,
                           const std::vector<std::string>& items,
                           std::function<void(bool, std::size_t)> on_result);

  private:
    struct PopupState {
        PopupHandle handle = 0;
        PopupSpec spec;
        PopupCallbacks callbacks;
        int z_index = 0;
        int focus_index = 0;
        std::vector<std::string> focus_chain;

        // 简单状态（用于默认 Input/List）
        std::string live_input_value;
        int selected_index = 0;

        // 每窗独立 widget 状态（支持多 Input / 多 List）
        std::map<std::string, std::string> input_values;
        std::map<std::string, int> list_selected_indices;
        std::map<std::string, int> scroll_offsets;
    };

    std::optional<std::reference_wrapper<PopupState>> topPopup();
    std::optional<std::reference_wrapper<const PopupState>> topPopup() const;
    std::optional<std::reference_wrapper<PopupState>> getPopup(PopupHandle handle);
    std::vector<PopupHandle> eventDispatchOrder(bool allow_non_top_non_modal) const;

    void rebuildFocusChain(PopupState& state);
    void focusNext(PopupState& state, bool backwards);

    bool handleEventForPopup(PopupState& state, ftxui::Event event, bool is_top);
    bool dispatchAction(PopupState& state, const WidgetSpec* focused_widget);

    WidgetSpec buildDefaultWidgetTree(const PopupState& state) const;
    const WidgetSpec* findWidgetById(const WidgetSpec& root, const std::string& id) const;

    std::string getInputValue(const PopupState& state, const WidgetSpec& widget) const;
    int getListSelectedIndex(const PopupState& state, const WidgetSpec& widget) const;

    ftxui::Element renderWidgetTree(const PopupState& state, const LayoutNode& node) const;
    ftxui::Element renderPopupLayer(const PopupState& state, int screen_w, int screen_h) const;

    // 颜色解析辅助函数
    ftxui::Color parseColor(const std::string& color_str) const;
    ftxui::Element applyColorDecorators(
        ftxui::Element elem, const std::map<std::string, std::string>& color_config) const;

    ::pnana::ui::Theme* theme_;
    LayoutEngine layout_engine_;
    std::map<PopupHandle, PopupState> popups_;
    std::vector<PopupHandle> z_order_;
    PopupHandle next_handle_;
};

} // namespace ui
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_UI_POPUP_MANAGER_H
