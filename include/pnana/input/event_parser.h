#ifndef PNANA_INPUT_EVENT_PARSER_H
#define PNANA_INPUT_EVENT_PARSER_H

#include <ftxui/component/event.hpp>
#include <string>

namespace pnana {
namespace input {

// 修饰键结构
struct Modifiers {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool meta = false;
    
    bool operator==(const Modifiers& other) const {
        return ctrl == other.ctrl && alt == other.alt && 
               shift == other.shift && meta == other.meta;
    }
};

// 事件解析器：将 FTXUI Event 转换为标准化的键字符串
// 职责：只负责事件解析，不涉及快捷键绑定
class EventParser {
public:
    EventParser() = default;
    ~EventParser() = default;
    
    // 将事件转换为标准化的键字符串
    // 例如：Ctrl+S -> "ctrl_s", Alt+A -> "alt_a", F3 -> "f3"
    std::string eventToKey(const ftxui::Event& event) const;
    
    // 解析修饰键
    Modifiers parseModifiers(const ftxui::Event& event) const;
    
    // 检查是否是 Ctrl+Shift 组合
    bool isCtrlShift(const ftxui::Event& event) const;
    
    // 检查是否是 Alt 组合
    bool isAlt(const ftxui::Event& event) const;
    
    // 检查是否是 Ctrl 组合
    bool isCtrl(const ftxui::Event& event) const;
    
    // 检查是否是 Shift 组合
    bool isShift(const ftxui::Event& event) const;

private:
    // 解析 Ctrl+字符组合
    std::string parseCtrlKey(const ftxui::Event& event) const;
    
    // 解析功能键
    std::string parseFunctionKey(const ftxui::Event& event) const;
    
    // 解析导航键
    std::string parseNavigationKey(const ftxui::Event& event) const;
    
    // 解析方向键
    std::string parseArrowKey(const ftxui::Event& event) const;
    
    // 解析特殊键
    std::string parseSpecialKey(const ftxui::Event& event) const;
    
    // 解析 Alt+字符组合
    std::string parseAltKey(const ftxui::Event& event) const;
    
    // 解析 Ctrl+特殊字符组合
    std::string parseCtrlSpecialChar(const ftxui::Event& event) const;
};

} // namespace input
} // namespace pnana

#endif // PNANA_INPUT_EVENT_PARSER_H

