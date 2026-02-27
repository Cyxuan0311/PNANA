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
    if (!editor->isTerminalVisible()) {
        LOG("TerminalHandler: Terminal not visible, ignoring input");
        return false;
    }

    EditorRegion current_region = editor->getRegionManager().getCurrentRegion();
    if (current_region != EditorRegion::TERMINAL) {
        LOG("TerminalHandler: Current region is " + editor->getRegionManager().getRegionName() +
            ", switching to TERMINAL");
        editor->getRegionManager().setRegion(EditorRegion::TERMINAL);
    }

    // 终端高度调整
    if (event == Event::F1) {
        int current_height = editor->getTerminalHeight();
        int screen_height = editor->getScreenHeight();
        int new_height = current_height > 0 ? current_height + 1 : screen_height / 3 + 1;
        if (new_height < screen_height - 4) {
            editor->setTerminalHeight(new_height);
            editor->setStatusMessage("Terminal height: " + std::to_string(new_height) +
                                     " lines (F1: increase, F2: decrease)");
        }
        return true;
    } else if (event == Event::F2) {
        int current_height = editor->getTerminalHeight();
        int new_height =
            current_height > 0 ? current_height - 1 : editor->getScreenHeight() / 3 - 1;
        if (new_height >= 3) {
            editor->setTerminalHeight(new_height);
            editor->setStatusMessage("Terminal height: " + std::to_string(new_height) +
                                     " lines (F1: increase, F2: decrease)");
        }
        return true;
    }

    // Escape: 关闭终端
    if (event == Event::Escape) {
        editor->getTerminal().setVisible(false);
        editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
        return true;
    }

    // 所有按键透传到 shell（含 Return/Enter 及字符形式的换行）
    if (event == Event::Return) {
        editor->getTerminal().handleKeyEvent("return");
        return true;
    }
    if (event.is_character()) {
        std::string ch = event.character();
        if (ch == "\n" || ch == "\r") {
            editor->getTerminal().handleKeyEvent("return");
            return true;
        }
    }
    if (event == Event::CtrlM) {
        editor->getTerminal().handleKeyEvent("return");
        return true;
    }
    if (event == Event::Tab) {
        editor->getTerminal().handleKeyEvent("tab");
        return true;
    }
    if (event == Event::ArrowUp) {
        editor->getTerminal().handleKeyEvent("ArrowUp");
        return true;
    }
    if (event == Event::ArrowDown) {
        editor->getTerminal().handleKeyEvent("ArrowDown");
        return true;
    }
    if (event == Event::ArrowLeft || event == Event::ArrowRight) {
        editor->getTerminal().handleKeyEvent(event == Event::ArrowLeft ? "ArrowLeft"
                                                                       : "ArrowRight");
        return true;
    }
    if (event == Event::PageUp) {
        editor->getTerminal().scrollUp();
        editor->setStatusMessage(
            "Terminal: Scrolled up (PageUp: scroll up, PageDown: scroll down)");
        return true;
    }
    if (event == Event::PageDown) {
        editor->getTerminal().scrollDown();
        editor->setStatusMessage(
            "Terminal: Scrolled down (PageUp: scroll up, PageDown: scroll down)");
        return true;
    }

    // 字符和特殊键透传
    if (event.is_character()) {
        std::string input = event.input();
        if (!input.empty()) {
            editor->getTerminal().handleKeyEvent(input);
            return true;
        }
    }

    if (event == Event::Backspace) {
        editor->getTerminal().handleKeyEvent("Backspace");
        return true;
    }
    if (event == Event::Home || event == Event::End || event == Event::Delete) {
        editor->getTerminal().handleKeyEvent(event.input());
        return true;
    }

    // Ctrl+组合键：透传
    if (event == Event::CtrlC) {
        editor->getTerminal().handleKeyEvent("ctrl_c");
        return true;
    }
    if (event == Event::CtrlD) {
        editor->getTerminal().handleKeyEvent("ctrl_d");
        return true;
    }
    if (event == Event::CtrlZ) {
        editor->getTerminal().handleKeyEvent("ctrl_z");
        return true;
    }
    if (event == Event::CtrlL) {
        editor->getTerminal().handleKeyEvent("ctrl_l");
        return true;
    }
    if (event == Event::CtrlU) {
        editor->getTerminal().handleKeyEvent("ctrl_u");
        return true;
    }
    if (event == Event::CtrlK) {
        editor->getTerminal().handleKeyEvent("ctrl_k");
        return true;
    }
    if (event == Event::CtrlA) {
        editor->getTerminal().handleKeyEvent("ctrl_a");
        return true;
    }
    if (event == Event::CtrlE) {
        editor->getTerminal().handleKeyEvent("ctrl_e");
        return true;
    }
    if (event == Event::CtrlW) {
        editor->getTerminal().handleKeyEvent("ctrl_w");
        return true;
    }
    if (event == Event::CtrlH) {
        editor->getTerminal().handleKeyEvent("ctrl_h"); // Ctrl+H = Backspace（部分终端）
        return true;
    }

    return false;
}

bool TerminalHandler::handleNavigation(Event event, Editor* editor) {
    // 终端内：左右键用于 readline 光标移动，不切换区域
    if (editor->getRegionManager().getCurrentRegion() == EditorRegion::TERMINAL &&
        (event == Event::ArrowLeft || event == Event::ArrowRight)) {
        return false; // 交给 handleInput 透传到 shell
    }

    if (editor->getSplitViewManager().hasSplits()) {
        return false;
    }

    if (event == Event::ArrowLeft) {
        if (editor->isFileBrowserVisible()) {
            editor->getRegionManager().setRegion(EditorRegion::FILE_BROWSER);
            editor->setStatusMessage("Switched to file browser | Press → to return to terminal");
        } else {
            editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
            editor->setStatusMessage("Switched to code area | Press → to return to terminal");
        }
        return true;
    } else if (event == Event::ArrowRight) {
        editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
        editor->setStatusMessage("Switched to code area | Press ← to return to terminal");
        return true;
    }

    if (event == Event::ArrowUp || event == Event::ArrowDown) {
        return false;
    }

    return false;
}

std::vector<pnana::input::KeyAction> TerminalHandler::getSupportedActions() const {
    return {};
}

} // namespace input
} // namespace core
} // namespace pnana
