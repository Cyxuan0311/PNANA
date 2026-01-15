#include "core/input/region_handlers/git_panel_handler.h"
#include "core/editor.h"
#include "input/key_action.h"
#include "utils/logger.h"
#include <chrono>
#include <ftxui/component/event.hpp>

using namespace ftxui;

namespace pnana {
namespace core {
namespace input {

GitPanelHandler::GitPanelHandler() = default;

bool GitPanelHandler::handleInput(ftxui::Event event, Editor* editor) {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanelHandler::handleInput - START");

    if (!isGitPanelActive(editor)) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleInput - END (not active) - " +
            std::to_string(duration.count()) + "ms");
        return false;
    }

    // 将事件传递给GitPanel处理
    bool handled = editor->getGitPanel().onKeyPress(event);

    // 如果事件被处理，强制UI更新
    if (handled) {
        auto ui_update_start = std::chrono::high_resolution_clock::now();
        pnana::utils::Logger::getInstance().log("GitPanelHandler::handleInput - UI update START");

        // 使用PostTask for immediate UI refresh
        editor->screen_.Post([editor]() {
            // Force UI update
            editor->screen_.PostEvent(Event::Custom);
        });

        auto ui_update_end = std::chrono::high_resolution_clock::now();
        auto ui_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(ui_update_end - ui_update_start);
        pnana::utils::Logger::getInstance().log("GitPanelHandler::handleInput - UI update END - " +
                                                std::to_string(ui_duration.count()) + "ms");
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log(
        "GitPanelHandler::handleInput - END (handled: " + std::string(handled ? "true" : "false") +
        ") - " + std::to_string(duration.count()) + "ms");

    return handled;
}

bool GitPanelHandler::handleNavigation(ftxui::Event event, Editor* editor) {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanelHandler::handleNavigation - START");

    if (!isGitPanelActive(editor)) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleNavigation - END (not active) - " +
            std::to_string(duration.count()) + "ms");
        return false;
    }

    // Git面板的导航：Esc关闭面板，Tab切换区域
    if (event == Event::Escape) {
        editor->toggleGitPanel();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleNavigation - END (escape) - " +
            std::to_string(duration.count()) + "ms");
        return true;
    }

    if (event == Event::Tab) {
        // 从Git面板切换到代码区域
        editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
        editor->setStatusMessage("Switched to Code Area | Ctrl+G: Git Panel");
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleNavigation - END (tab switch) - " +
            std::to_string(duration.count()) + "ms");
        return true;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log(
        "GitPanelHandler::handleNavigation - END (not handled) - " +
        std::to_string(duration.count()) + "ms");

    return false;
}

std::vector<pnana::input::KeyAction> GitPanelHandler::getSupportedActions() const {
    return {}; // Git面板主要使用键盘导航，不需要特殊的快捷键
}

bool GitPanelHandler::isGitPanelActive(Editor* editor) const {
    return editor->isGitPanelVisible() &&
           editor->getRegionManager().getCurrentRegion() == EditorRegion::GIT_PANEL;
}

} // namespace input
} // namespace core
} // namespace pnana
