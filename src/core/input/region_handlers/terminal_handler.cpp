#include "core/input/region_handlers/terminal_handler.h"
#include "core/editor.h"
#include "features/terminal.h"
#include "utils/logger.h"
#include <ftxui/component/event.hpp>

using namespace ftxui;

namespace pnana {
namespace core {
namespace input {

TerminalHandler::TerminalHandler() = default;

bool TerminalHandler::handleInput(Event event, Editor* editor) {
    // 终端区域：直接传递输入给终端
    if (!editor->isTerminalVisible()) {
        LOG("TerminalHandler: Terminal not visible, ignoring input");
        return false;
    }
    
    // 确保当前区域是终端
    EditorRegion current_region = editor->getRegionManager().getCurrentRegion();
    if (current_region != EditorRegion::TERMINAL) {
        LOG("TerminalHandler: Current region is " + editor->getRegionManager().getRegionName() + ", switching to TERMINAL");
        editor->getRegionManager().setRegion(EditorRegion::TERMINAL);
    }
    
    std::string is_char_str = event.is_character() ? "true" : "false";
    LOG("TerminalHandler: Received event: " + event.input() + " (is_character=" + is_char_str + ")");
    
    // 处理终端高度调整：F1 增加高度，F2 减少高度
    if (event == Event::F1) {
        // F1 键：增加终端高度
        int current_height = editor->getTerminalHeight();
        int screen_height = editor->getScreenHeight();
        int new_height = current_height > 0 ? current_height + 1 : screen_height / 3 + 1;
        if (new_height < screen_height - 4) {  // 保留最小空间给其他UI元素
            editor->setTerminalHeight(new_height);
            editor->setStatusMessage("Terminal height: " + std::to_string(new_height) + " lines (F1: increase, F2: decrease)");
            LOG("TerminalHandler: Increased terminal height to " + std::to_string(new_height));
        }
        return true;
    } else if (event == Event::F2) {
        // F2 键：减少终端高度
        int current_height = editor->getTerminalHeight();
        int new_height = current_height > 0 ? current_height - 1 : editor->getScreenHeight() / 3 - 1;
        if (new_height >= 3) {  // 最小高度3行
            editor->setTerminalHeight(new_height);
            editor->setStatusMessage("Terminal height: " + std::to_string(new_height) + " lines (F1: increase, F2: decrease)");
            LOG("TerminalHandler: Decreased terminal height to " + std::to_string(new_height));
        }
        return true;
    }
    
    // 处理特殊键（在字符输入之前）
    if (event == Event::Escape) {
        // Escape 键：关闭终端
        editor->getTerminal().setVisible(false);
        editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
        return true;
    } else if (event == Event::Return) {
        // Enter 键：执行命令
        std::string command = editor->getTerminal().getCurrentInput();
        if (command == "exit" || command == "quit") {
            editor->getTerminal().setVisible(false);
            editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
            return true;
        }
        editor->getTerminal().executeCommand(command);
        editor->getTerminal().handleInput("");  // 清空输入
        return true;
    } else if (event == Event::Tab) {
        // Tab 键：命令补全
        return editor->getTerminal().handleTabCompletion();
    }
    
    // 上下方向键：处理命令历史
    if (event == Event::ArrowUp) {
        editor->getTerminal().handleKeyEvent("ArrowUp");
        return true;
    } else if (event == Event::ArrowDown) {
        editor->getTerminal().handleKeyEvent("ArrowDown");
        return true;
    }

    // PageUp/PageDown：滚动查看终端历史输出
    if (event == Event::PageUp) {
        // PageUp：向上滚动历史输出
        editor->getTerminal().scrollUp();
        editor->setStatusMessage("Terminal: Scrolled up (PageUp: scroll up, PageDown: scroll down)");
        return true;
    } else if (event == Event::PageDown) {
        // PageDown：向下滚动历史输出
        editor->getTerminal().scrollDown();
        editor->setStatusMessage("Terminal: Scrolled down (PageUp: scroll up, PageDown: scroll down)");
        return true;
    }
    
    // 左右键在 handleNavigation 中处理，用于切换面板
    // 这里不处理左右键，让它们传递给 handleNavigation
    if (event == Event::ArrowLeft || event == Event::ArrowRight) {
        return false;  // 让 handleNavigation 处理
    }
    
    // 处理普通字符输入（包括 + 和 -，因为它们也是常用字符）
    if (event.is_character()) {
        std::string input = event.input();
        LOG("TerminalHandler: Character input detected: '" + input + "' (size=" + std::to_string(input.size()) + ")");
        // 只处理可打印字符（排除控制字符）
        // 注意：+ 和 - 现在作为普通字符处理，不再用于高度调整
        if (!input.empty() && input[0] >= 32 && input[0] != 127) {
            std::string current_input = editor->getTerminal().getCurrentInput();
            size_t cursor_pos = editor->getTerminal().getCursorPosition();
            
            LOG("TerminalHandler: Before insert - current_input='" + current_input + "', cursor_pos=" + std::to_string(cursor_pos));
            
            // 在光标位置插入字符
            current_input.insert(cursor_pos, input);
            editor->getTerminal().handleInput(current_input);
            editor->getTerminal().setCursorPosition(cursor_pos + 1);
            
            LOG("TerminalHandler: After insert - new_input='" + editor->getTerminal().getCurrentInput() + "', new_cursor_pos=" + std::to_string(editor->getTerminal().getCursorPosition()));
            return true;
        } else {
            LOG("TerminalHandler: Character filtered out (control character or empty)");
        }
    }
    
    // 处理其他特殊键
    if (event == Event::Backspace) {
        LOG("TerminalHandler: Backspace key detected");
        editor->getTerminal().handleKeyEvent("Backspace");
        return true;
    } else if (event == Event::Home || event == Event::End || event == Event::Delete) {
        LOG("TerminalHandler: Special key: " + event.input());
        editor->getTerminal().handleKeyEvent(event.input());
        return true;
    }
    
    LOG("TerminalHandler: Event not handled, returning false");
    return false;
}

bool TerminalHandler::handleNavigation(Event event, Editor* editor) {
    // 终端区域导航：左右键用于切换面板
    if (event == Event::ArrowLeft) {
        // 左键：切换到左侧面板（文件浏览器或代码区）
        if (editor->isFileBrowserVisible()) {
            editor->getRegionManager().setRegion(EditorRegion::FILE_BROWSER);
            editor->setStatusMessage("Switched to file browser | Press → to return to terminal");
            return true;
        } else {
            editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
            editor->setStatusMessage("Switched to code area | Press → to return to terminal");
            return true;
        }
    } else if (event == Event::ArrowRight) {
        // 右键：切换到代码区
        editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
        editor->setStatusMessage("Switched to code area | Press ← to return to terminal");
        return true;
    }
    
    // 上下键在终端内处理（命令历史）
    if (event == Event::ArrowUp || event == Event::ArrowDown) {
        return false;  // 让终端内部处理
    }
    
    return false;
}

std::vector<pnana::input::KeyAction> TerminalHandler::getSupportedActions() const {
    // 终端区域支持的快捷键
    return {
        // 终端特定的操作可以在这里列出
    };
}

} // namespace input
} // namespace core
} // namespace pnana

