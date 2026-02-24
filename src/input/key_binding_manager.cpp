#include "input/key_binding_manager.h"
#include "input/key_action.h"
#include "utils/logger.h"
#include <algorithm>
#include <iostream>

namespace pnana {
namespace input {

KeyBindingManager::KeyBindingManager() {
    initializeDefaultBindings();
}

void KeyBindingManager::initializeDefaultBindings() {
    initializeFileOperationBindings();
    initializeEditOperationBindings();
    initializeSearchNavigationBindings();
    initializeViewOperationBindings();
    initializeTabOperationBindings();
}

void KeyBindingManager::initializeFileOperationBindings() {
    bindKey("ctrl_s", KeyAction::SAVE_FILE);
    bindKey("alt_a", KeyAction::SAVE_AS);
    bindKey("ctrl_q", KeyAction::QUIT);
    bindKey("ctrl_n", KeyAction::NEW_FILE);
    bindKey("ctrl_o", KeyAction::OPEN_FILE);
    bindKey("ctrl_w", KeyAction::CLOSE_TAB);
    bindKey("alt_f", KeyAction::CREATE_FOLDER);
    bindKey("alt_m", KeyAction::FILE_PICKER);
}

void KeyBindingManager::initializeEditOperationBindings() {
    bindKey("ctrl_z", KeyAction::UNDO);
    bindKey("ctrl_y", KeyAction::REDO);
    bindKeyAliases({"ctrl_shift_z"}, KeyAction::REDO);
    bindKey("ctrl_x", KeyAction::CUT);
    bindKey("ctrl_p", KeyAction::COPY);
    bindKey("ctrl_v", KeyAction::PASTE);
    bindKey("ctrl_a", KeyAction::SELECT_ALL);
    bindKey("alt_d", KeyAction::SELECT_WORD);
    bindKey("alt_shift_arrow_up", KeyAction::SELECT_EXTEND_UP);
    bindKey("alt_shift_arrow_down", KeyAction::SELECT_EXTEND_DOWN);
    bindKey("alt_shift_arrow_left", KeyAction::SELECT_EXTEND_LEFT);
    bindKey("alt_shift_arrow_right", KeyAction::SELECT_EXTEND_RIGHT);
    bindKey("ctrl_d", KeyAction::DUPLICATE_LINE);
    bindKey("ctrl_shift_k", KeyAction::DELETE_LINE);
    bindKey("ctrl_backspace", KeyAction::DELETE_WORD);
    bindKey("alt_arrow_up", KeyAction::MOVE_LINE_UP);
    bindKey("alt_arrow_down", KeyAction::MOVE_LINE_DOWN);
    bindKey("ctrl_u", KeyAction::TOGGLE_FOLD);
    bindKey("ctrl_shift_u", KeyAction::FOLD_ALL);
    bindKey("ctrl_alt_u", KeyAction::UNFOLD_ALL);
    bindKey("tab", KeyAction::INDENT_LINE);
    bindKey("shift_tab", KeyAction::UNINDENT_LINE);
    bindKey("ctrl_slash", KeyAction::TOGGLE_COMMENT);
#ifdef BUILD_LSP_SUPPORT
    bindKey("ctrl_space", KeyAction::TRIGGER_COMPLETION);
    bindKey("alt_e", KeyAction::SHOW_DIAGNOSTICS);
#endif
}

void KeyBindingManager::initializeSearchNavigationBindings() {
    bindKey("ctrl_f", KeyAction::SEARCH);
    bindKey("ctrl_h", KeyAction::REPLACE);
    bindKey("ctrl_g", KeyAction::GOTO_LINE);
    bindKey("ctrl_f3", KeyAction::SEARCH_NEXT);
    bindKey("ctrl_shift_f3", KeyAction::SEARCH_PREV);
    bindKey("ctrl_home", KeyAction::GOTO_FILE_START);
    bindKey("ctrl_end", KeyAction::GOTO_FILE_END);
    bindKey("home", KeyAction::GOTO_LINE_START);
    bindKey("end", KeyAction::GOTO_LINE_END);
    bindKey("pageup", KeyAction::PAGE_UP);
    bindKey("pagedown", KeyAction::PAGE_DOWN);
}

void KeyBindingManager::initializeViewOperationBindings() {
    bindKey("ctrl_t", KeyAction::TOGGLE_THEME_MENU);
    bindKey("f1", KeyAction::TOGGLE_HELP);
    bindKey("ctrl_shift_l", KeyAction::TOGGLE_LINE_NUMBERS);
    bindKey("f3", KeyAction::COMMAND_PALETTE);
    bindKey("ctrl_shift_a", KeyAction::AI_ASSISTANT);
    bindKey("f4", KeyAction::SSH_CONNECT);
    bindKey("alt_w", KeyAction::TOGGLE_MARKDOWN_PREVIEW);
#ifdef BUILD_LUA_SUPPORT
    bindKey("alt_p", KeyAction::OPEN_PLUGIN_MANAGER);
#endif
    // Ctrl+L ???????????????????

    // ??????? Ctrl+?????? tmux?
    bindKey("ctrl_left", KeyAction::FOCUS_LEFT_REGION);
    bindKey("ctrl_right", KeyAction::FOCUS_RIGHT_REGION);
    bindKey("ctrl_up", KeyAction::FOCUS_UP_REGION);
    bindKey("ctrl_down", KeyAction::FOCUS_DOWN_REGION);
}

void KeyBindingManager::initializeTabOperationBindings() {
    bindKey("alt_tab", KeyAction::NEXT_TAB);
    bindKeyAliases({"ctrl_pagedown"}, KeyAction::NEXT_TAB);
    bindKey("alt_shift_tab", KeyAction::PREV_TAB);
    bindKeyAliases({"ctrl_pageup"}, KeyAction::PREV_TAB);
}

KeyAction KeyBindingManager::getAction(const ftxui::Event& event) const {
    std::string key = parser_.eventToKey(event);

    if (key.empty()) {
        return KeyAction::UNKNOWN;
    }

    auto it = key_to_action_.find(key);
    if (it != key_to_action_.end()) {
        return it->second;
    }

    return KeyAction::UNKNOWN;
}

void KeyBindingManager::bindKey(const std::string& key, KeyAction action) {
    key_to_action_[key] = action;

    // ??????
    auto& keys = action_to_keys_[action];
    if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
        keys.push_back(key);
    }
}

void KeyBindingManager::bindKeyAliases(const std::vector<std::string>& keys, KeyAction action) {
    for (const auto& key : keys) {
        bindKey(key, action);
    }
}

void KeyBindingManager::unbindKey(const std::string& key) {
    auto it = key_to_action_.find(key);
    if (it != key_to_action_.end()) {
        KeyAction action = it->second;
        key_to_action_.erase(it);

        // ????????
        auto& keys = action_to_keys_[action];
        keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
    }
}

std::vector<std::string> KeyBindingManager::getKeysForAction(KeyAction action) const {
    auto it = action_to_keys_.find(action);
    if (it != action_to_keys_.end()) {
        return it->second;
    }
    return {};
}

KeyAction KeyBindingManager::getActionForKey(const std::string& key) const {
    auto it = key_to_action_.find(key);
    if (it != key_to_action_.end()) {
        return it->second;
    }
    return KeyAction::UNKNOWN;
}

bool KeyBindingManager::isGlobalKey(const ftxui::Event& event) const {
    KeyAction action = getAction(event);
    return action != KeyAction::UNKNOWN;
}

void KeyBindingManager::resetToDefaults() {
    key_to_action_.clear();
    action_to_keys_.clear();
    initializeDefaultBindings();
}

} // namespace input
} // namespace pnana
