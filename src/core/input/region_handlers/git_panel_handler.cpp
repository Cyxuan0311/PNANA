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

    bool handled = false;

    // 如果diff viewer打开，优先处理diff viewer的事件
    if (editor->getGitPanel().isDiffViewerVisible()) {
        if (event == Event::Escape) {
            editor->getGitPanel().handleDiffViewerEscape();
            handled = true;
        } else if (event == Event::PageUp) {
            auto& git_panel = editor->getGitPanel();
            const size_t VISIBLE_LINES = 25; // Must match renderDiffViewer
            size_t current_offset = git_panel.getDiffScrollOffset();
            if (current_offset > 0) {
                current_offset =
                    (current_offset > VISIBLE_LINES) ? current_offset - VISIBLE_LINES : 0;
                git_panel.setDiffScrollOffset(current_offset);
            }
            handled = true;
        } else if (event == Event::PageDown) {
            auto& git_panel = editor->getGitPanel();
            const size_t VISIBLE_LINES = 25; // Must match renderDiffViewer
            const auto& diff_content = git_panel.getDiffContent();
            size_t max_offset =
                diff_content.size() > VISIBLE_LINES ? diff_content.size() - VISIBLE_LINES : 0;
            size_t current_offset = git_panel.getDiffScrollOffset();
            if (current_offset < max_offset) {
                current_offset += VISIBLE_LINES;
                if (current_offset > max_offset) {
                    current_offset = max_offset;
                }
                git_panel.setDiffScrollOffset(current_offset);
            }
            handled = true;
        } else {
            // 对于diff viewer打开时的其他事件，直接消费掉，不传递给GitPanel
            handled = true;
        }
    } else {
        // 将事件传递给GitPanel处理
        handled = editor->getGitPanel().onKeyPress(event);
    }

    // 如果GitPanel没有处理Tab键，则将其作为区域切换
    if (!handled && event == Event::Tab) {
        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleInput - Tab not handled by GitPanel, switching to code area");
        // 从Git面板切换到代码区域
        editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
        editor->setStatusMessage("Switched to Code Area | Ctrl+G: Git Panel");
        handled = true;
    }

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

    // Git面板的导航：Esc关闭面板，但如果diff viewer打开，则让GitPanel处理
    if (event == Event::Escape) {
        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleNavigation - ESC pressed, diff viewer visible: " +
            std::string(editor->getGitPanel().isDiffViewerVisible() ? "true" : "false"));

        // 如果diff viewer打开，让GitPanel处理ESC
        if (editor->getGitPanel().isDiffViewerVisible()) {
            pnana::utils::Logger::getInstance().log(
                "GitPanelHandler::handleNavigation - Diff viewer is visible, letting GitPanel "
                "handle ESC");
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            pnana::utils::Logger::getInstance().log("GitPanelHandler::handleNavigation - END (diff "
                                                    "viewer open, let GitPanel handle) - " +
                                                    std::to_string(duration.count()) + "ms");
            return false; // 不处理，让GitPanel处理
        }

        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleNavigation - No diff viewer, closing git panel");
        editor->toggleGitPanel();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitPanelHandler::handleNavigation - END (escape) - " +
            std::to_string(duration.count()) + "ms");
        return true;
    }

    // 注意：Tab键现在由GitPanel的onKeyPress方法处理，用于标签切换
    // 不在这里拦截Tab键，让它传递到GitPanel进行内部标签切换

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
