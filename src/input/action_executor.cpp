#include "input/action_executor.h"
#include "core/editor.h"
#include "utils/logger.h"
#include <iostream>

namespace pnana {
namespace input {

ActionExecutor::ActionExecutor(core::Editor* editor) : editor_(editor) {}

bool ActionExecutor::execute(KeyAction action) {
    if (!editor_ || !canExecute(action)) {
        return false;
    }

    switch (action) {
        case KeyAction::SAVE_FILE:
        case KeyAction::SAVE_AS:
        case KeyAction::QUIT:
        case KeyAction::NEW_FILE:
        case KeyAction::OPEN_FILE:
        case KeyAction::CLOSE_TAB:
        case KeyAction::CREATE_FOLDER:
        case KeyAction::FILE_PICKER:
            return executeFileOperation(action);

        case KeyAction::UNDO:
        case KeyAction::REDO:
        case KeyAction::CUT:
        case KeyAction::COPY:
        case KeyAction::PASTE:
        case KeyAction::SELECT_ALL:
        case KeyAction::SELECT_WORD:
        case KeyAction::SELECT_EXTEND_UP:
        case KeyAction::SELECT_EXTEND_DOWN:
        case KeyAction::SELECT_EXTEND_LEFT:
        case KeyAction::SELECT_EXTEND_RIGHT:
        case KeyAction::DUPLICATE_LINE:
        case KeyAction::DELETE_LINE:
        case KeyAction::DELETE_WORD:
        case KeyAction::MOVE_LINE_UP:
        case KeyAction::MOVE_LINE_DOWN:
        case KeyAction::INDENT_LINE:
        case KeyAction::UNINDENT_LINE:
        case KeyAction::TOGGLE_COMMENT:
            return executeEditOperation(action);
#ifdef BUILD_LSP_SUPPORT
        case KeyAction::TRIGGER_COMPLETION:
        case KeyAction::SHOW_DIAGNOSTICS:
            return executeEditOperation(action);
#endif

        case KeyAction::SEARCH:
        case KeyAction::REPLACE:
        case KeyAction::GOTO_LINE:
        case KeyAction::SEARCH_NEXT:
        case KeyAction::SEARCH_PREV:
        case KeyAction::GOTO_FILE_START:
        case KeyAction::GOTO_FILE_END:
        case KeyAction::GOTO_LINE_START:
        case KeyAction::GOTO_LINE_END:
            return executeSearchNavigation(action);

        case KeyAction::TOGGLE_THEME_MENU:
        case KeyAction::TOGGLE_FILE_BROWSER:
        case KeyAction::TOGGLE_HELP:
        case KeyAction::TOGGLE_LINE_NUMBERS:
        case KeyAction::COMMAND_PALETTE:
        case KeyAction::SPLIT_VIEW:
        case KeyAction::SSH_CONNECT:
#ifdef BUILD_LUA_SUPPORT
        case KeyAction::OPEN_PLUGIN_MANAGER:
#endif
            return executeViewOperation(action);

        case KeyAction::NEXT_TAB:
        case KeyAction::PREV_TAB:
            return executeTabOperation(action);

        case KeyAction::FOCUS_LEFT_REGION:
        case KeyAction::FOCUS_RIGHT_REGION:
        case KeyAction::FOCUS_UP_REGION:
        case KeyAction::FOCUS_DOWN_REGION:
            return executeSplitNavigation(action);

        case KeyAction::UNKNOWN:
        default:
            return false;
    }
}

bool ActionExecutor::canExecute(KeyAction action) const {
    // 所有动作都可以执行（未来可以添加权限检查）
    return action != KeyAction::UNKNOWN;
}

std::string ActionExecutor::getActionDescription(KeyAction action) const {
    auto info = getActionInfo(action);
    return info.description;
}

bool ActionExecutor::executeFileOperation(KeyAction action) {
    switch (action) {
        case KeyAction::SAVE_FILE:
            editor_->saveFile();
            return true;
        case KeyAction::SAVE_AS:
            editor_->startSaveAs();
            return true;
        case KeyAction::QUIT:
            editor_->quit();
            return true;
        case KeyAction::NEW_FILE:
            editor_->newFile();
            return true;
        case KeyAction::OPEN_FILE:
            editor_->toggleFileBrowser();
            return true;
        case KeyAction::CLOSE_TAB:
            editor_->closeCurrentTab();
            return true;
        case KeyAction::CREATE_FOLDER:
            editor_->createFolder();
            return true;
        case KeyAction::FILE_PICKER:
            editor_->openFilePicker();
            return true;
        default:
            return false;
    }
}

bool ActionExecutor::executeEditOperation(KeyAction action) {
    switch (action) {
        case KeyAction::UNDO:
            editor_->undo();
            return true;
        case KeyAction::REDO:
            editor_->redo();
            return true;
        case KeyAction::CUT:
            editor_->cut();
            return true;
        case KeyAction::COPY:
            LOG("[DEBUG COPY] ActionExecutor: COPY action executing");
            editor_->copy();
            LOG("[DEBUG COPY] ActionExecutor: COPY action completed");
            return true;
        case KeyAction::PASTE:
            editor_->paste();
            return true;
        case KeyAction::SELECT_ALL:
            editor_->selectAll();
            return true;
        case KeyAction::SELECT_WORD:
            editor_->selectWord();
            return true;
        case KeyAction::SELECT_EXTEND_UP:
            editor_->extendSelectionUp();
            return true;
        case KeyAction::SELECT_EXTEND_DOWN:
            editor_->extendSelectionDown();
            return true;
        case KeyAction::SELECT_EXTEND_LEFT:
            editor_->extendSelectionLeft();
            return true;
        case KeyAction::SELECT_EXTEND_RIGHT:
            editor_->extendSelectionRight();
            return true;
        case KeyAction::DUPLICATE_LINE:
            editor_->duplicateLine();
            return true;
        case KeyAction::DELETE_LINE:
            editor_->deleteLine();
            return true;
        case KeyAction::DELETE_WORD:
            editor_->deleteWord();
            return true;
        case KeyAction::MOVE_LINE_UP:
            editor_->moveLineUp();
            return true;
        case KeyAction::MOVE_LINE_DOWN:
            editor_->moveLineDown();
            return true;
        case KeyAction::INDENT_LINE:
            LOG("ActionExecutor: Executing INDENT_LINE");
            editor_->indentLine();
            LOG("ActionExecutor: indentLine() completed");
            return true;
        case KeyAction::UNINDENT_LINE:
            editor_->unindentLine();
            return true;
        case KeyAction::TOGGLE_COMMENT:
            editor_->toggleComment();
            return true;
#ifdef BUILD_LSP_SUPPORT
        case KeyAction::TRIGGER_COMPLETION:
            editor_->triggerCompletion();
            return true;
        case KeyAction::SHOW_DIAGNOSTICS:
            editor_->showDiagnosticsPopup();
            return true;
#endif
        default:
            return false;
    }
}

bool ActionExecutor::executeSearchNavigation(KeyAction action) {
    switch (action) {
        case KeyAction::SEARCH:
            editor_->startSearch();
            return true;
        case KeyAction::REPLACE:
            editor_->startReplace();
            return true;
        case KeyAction::GOTO_LINE:
            editor_->startGotoLineMode();
            return true;
        case KeyAction::SEARCH_NEXT:
            editor_->searchNext();
            return true;
        case KeyAction::SEARCH_PREV:
            editor_->searchPrevious();
            return true;
        case KeyAction::GOTO_FILE_START:
            editor_->moveCursorFileStart();
            return true;
        case KeyAction::GOTO_FILE_END:
            editor_->moveCursorFileEnd();
            return true;
        case KeyAction::GOTO_LINE_START:
            editor_->moveCursorLineStart();
            return true;
        case KeyAction::GOTO_LINE_END:
            editor_->moveCursorLineEnd();
            return true;
        case KeyAction::PAGE_UP:
            if (editor_->isFileBrowserVisible()) {
                editor_->pageUp();
                return true;
            }
            // 如果文件浏览器不可见，则执行普通的光标翻页
            editor_->moveCursorPageUp();
            return true;
        case KeyAction::PAGE_DOWN:
            if (editor_->isFileBrowserVisible()) {
                editor_->pageDown();
                return true;
            }
            // 如果文件浏览器不可见，则执行普通的光标翻页
            editor_->moveCursorPageDown();
            return true;
        default:
            return false;
    }
}

bool ActionExecutor::executeViewOperation(KeyAction action) {
    switch (action) {
        case KeyAction::TOGGLE_THEME_MENU:
            editor_->toggleThemeMenu();
            return true;
        case KeyAction::TOGGLE_FILE_BROWSER:
            editor_->toggleFileBrowser();
            return true;
        case KeyAction::TOGGLE_HELP:
            editor_->toggleHelp();
            return true;
        case KeyAction::TOGGLE_LINE_NUMBERS:
            editor_->toggleLineNumbers();
            return true;
        case KeyAction::COMMAND_PALETTE:
            editor_->openCommandPalette();
            return true;
        case KeyAction::SPLIT_VIEW:
            editor_->showSplitDialog();
            return true;
        case KeyAction::SSH_CONNECT:
            editor_->showSSHDialog();
            return true;
#ifdef BUILD_LUA_SUPPORT
        case KeyAction::OPEN_PLUGIN_MANAGER:
            editor_->openPluginManager();
            return true;
#endif
        default:
            return false;
    }
}

bool ActionExecutor::executeSplitNavigation(KeyAction action) {
    switch (action) {
        case KeyAction::FOCUS_LEFT_REGION:
            editor_->focusLeftRegion();
            return true;
        case KeyAction::FOCUS_RIGHT_REGION:
            editor_->focusRightRegion();
            return true;
        case KeyAction::FOCUS_UP_REGION:
            editor_->focusUpRegion();
            return true;
        case KeyAction::FOCUS_DOWN_REGION:
            editor_->focusDownRegion();
            return true;
        default:
            return false;
    }
}

bool ActionExecutor::executeTabOperation(KeyAction action) {
    switch (action) {
        case KeyAction::NEXT_TAB:
            editor_->switchToNextTab();
            return true;
        case KeyAction::PREV_TAB:
            editor_->switchToPreviousTab();
            return true;
        default:
            return false;
    }
}

} // namespace input
} // namespace pnana
