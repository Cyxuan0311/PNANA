#ifndef PNANA_CORE_INPUT_REGION_HANDLERS_TERMINAL_HANDLER_H
#define PNANA_CORE_INPUT_REGION_HANDLERS_TERMINAL_HANDLER_H

#include "core/input/base_region_handler.h"
#include "core/region_manager.h"
#include "input/key_action.h"
#include <ftxui/component/event.hpp>
#include <vector>

namespace pnana {
namespace core {

// 前向声明
class Editor;

namespace input {

// 终端区域输入处理器
class TerminalHandler : public BaseRegionHandler {
  public:
    TerminalHandler();
    ~TerminalHandler() override = default;

    // 处理输入事件
    bool handleInput(ftxui::Event event, Editor* editor) override;

    // 处理区域导航（上下左右键）
    bool handleNavigation(ftxui::Event event, Editor* editor) override;

    // 获取支持的快捷键列表
    std::vector<pnana::input::KeyAction> getSupportedActions() const override;

    // 获取区域类型
    EditorRegion getRegionType() const override {
        return EditorRegion::TERMINAL;
    }
};

} // namespace input
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_INPUT_REGION_HANDLERS_TERMINAL_HANDLER_H
