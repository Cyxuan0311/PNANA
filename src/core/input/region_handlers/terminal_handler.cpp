#include "core/input/region_handlers/terminal_handler.h"
#include "core/editor.h"
#include "features/terminal.h"
#include "input/key_binding_manager.h"
#include "utils/logger.h"
#include <ftxui/component/event.hpp>

using namespace ftxui;

namespace pnana {
namespace core {
namespace input {

TerminalHandler::TerminalHandler() = default;

bool TerminalHandler::handleInput(Event event, Editor* editor) {
    if (!editor->isTerminalVisible()) {
        return false;
    }

    std::string key_str = event.input();

#ifdef BUILD_LIBVTERM_SUPPORT
    for (int i = 1; i <= 9; i++) {
        if (key_str == "alt_" + std::to_string(i)) {
            editor->getTerminal().setActiveSession(i - 1);
            editor->setStatusMessage("Terminal: Switched to tab " + std::to_string(i));
            return true;
        }
    }
#endif

    EditorRegion current_region = editor->getRegionManager().getCurrentRegion();
    if (current_region != EditorRegion::TERMINAL) {
        editor->getRegionManager().setRegion(EditorRegion::TERMINAL);
    }

#ifdef BUILD_LIBVTERM_SUPPORT
    pnana::input::KeyAction action = editor->getKeyBindingManager().getAction(event);
    if (action == pnana::input::KeyAction::NEW_FILE) {
        editor->terminal_session_dialog_.show(
            [editor](const pnana::ui::TerminalSessionChoice& choice) {
                int idx = -1;
                if (choice.shell_path.empty()) {
                    idx = editor->getTerminal().newLocalShellSession();
                } else {
                    idx = editor->getTerminal().newLocalShellSession("", choice.shell_path);
                }
                if (idx >= 0) {
                    editor->setStatusMessage("Terminal: New tab opened");
                } else {
                    editor->setStatusMessage("Terminal: Failed to open new session");
                }
                editor->getRegionManager().setRegion(EditorRegion::TERMINAL);
            },
            [editor]() {
                editor->setStatusMessage("Terminal: New session cancelled");
            });
        return true;
    }

    if (action == pnana::input::KeyAction::FOCUS_TAB_BAR) {
        int count = editor->getTerminal().sessionCount();
        if (count > 1) {
            int next = (editor->getTerminal().activeSessionIndex() + 1) % count;
            editor->getTerminal().setActiveSession(next);
            editor->setStatusMessage("Terminal: Switched to tab " + std::to_string(next + 1));
        }
        return true;
    }

    if (action == pnana::input::KeyAction::CLOSE_TAB) {
        if (editor->getTerminal().sessionCount() > 1) {
            editor->getTerminal().closeSession(editor->getTerminal().activeSessionIndex());
            editor->setStatusMessage("Terminal: Tab closed");
        }
        return true;
    }
#endif

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
        if (editor->getTerminal().sessionCount() <= 1) {
            editor->getTerminal().handleKeyEvent("ctrl_w");
        }
        return true;
    }
    if (event == Event::CtrlX) {
        editor->getTerminal().handleKeyEvent("ctrl_x");
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

    // 终端区域导航：左右键在不同 panel 间切换，方向由 file_browser_side 决定
    const auto& display_cfg = editor->getConfigManager().getConfig().display;
    bool browser_on_left = display_cfg.file_browser_side != "right";

    if (event == Event::ArrowLeft) {
        if (editor->isFileBrowserVisible() && browser_on_left) {
            // 文件列表在左侧：← 进入文件列表
            editor->getRegionManager().setRegion(EditorRegion::FILE_BROWSER);
            editor->setStatusMessage("Switched to file browser | Press → to return to terminal");
        } else {
            // 否则：← 进入代码区
            editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
            editor->setStatusMessage("Switched to code area | Press → to return to terminal");
        }
        return true;
    } else if (event == Event::ArrowRight) {
        if (editor->isFileBrowserVisible() && !browser_on_left) {
            // 文件列表在右侧：→ 进入文件列表
            editor->getRegionManager().setRegion(EditorRegion::FILE_BROWSER);
            editor->setStatusMessage("Switched to file browser | Press ← to return to terminal");
        } else {
            // 否则：→ 进入代码区
            editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
            editor->setStatusMessage("Switched to code area | Press ← to return to terminal");
        }
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
