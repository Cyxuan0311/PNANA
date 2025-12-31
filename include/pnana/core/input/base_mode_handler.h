#ifndef PNANA_CORE_INPUT_BASE_MODE_HANDLER_H
#define PNANA_CORE_INPUT_BASE_MODE_HANDLER_H

#include "input/key_action.h"
#include <ftxui/component/event.hpp>
#include <vector>

namespace pnana {
namespace core {

// 前向声明
class Editor;
enum class EditorMode;

namespace input {

// 模式处理器基类
class BaseModeHandler {
public:
    virtual ~BaseModeHandler() = default;
    
    // 处理输入事件
    virtual bool handleInput(ftxui::Event event, Editor* editor) = 0;
    
    // 获取模式类型
    virtual EditorMode getModeType() const = 0;
    
    // 获取支持的快捷键列表
    virtual std::vector<pnana::input::KeyAction> getSupportedActions() const = 0;
};

} // namespace input
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_INPUT_BASE_MODE_HANDLER_H

