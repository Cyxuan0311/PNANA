#ifndef PNANA_CORE_INPUT_BASE_REGION_HANDLER_H
#define PNANA_CORE_INPUT_BASE_REGION_HANDLER_H

#include "core/region_manager.h"
#include "input/key_action.h"
#include <ftxui/component/event.hpp>
#include <vector>

namespace pnana {
namespace core {

// 前向声明
class Editor;

namespace input {

// 区域输入处理器基类
class BaseRegionHandler {
public:
    virtual ~BaseRegionHandler() = default;
    
    // 处理输入事件
    virtual bool handleInput(ftxui::Event event, Editor* editor) = 0;
    
    // 处理区域导航（上下左右键）
    // 返回 true 表示已处理导航，false 表示继续处理其他输入
    virtual bool handleNavigation(ftxui::Event event, Editor* editor) = 0;
    
    // 获取支持的快捷键列表（用于帮助显示）
    virtual std::vector<pnana::input::KeyAction> getSupportedActions() const = 0;
    
    // 获取区域类型
    virtual EditorRegion getRegionType() const = 0;
    
protected:
    // 辅助方法：检查是否在区域边界
    bool isAtTop(Editor* editor) const;
    bool isAtBottom(Editor* editor) const;
    bool isAtLeft(Editor* editor) const;
    bool isAtRight(Editor* editor) const;
};

} // namespace input
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_INPUT_BASE_REGION_HANDLER_H

