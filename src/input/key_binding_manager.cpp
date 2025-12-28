#include "input/key_binding_manager.h"
#include "input/key_action.h"
#include <algorithm>

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
    bindKey("ctrl_c", KeyAction::COPY);
    bindKey("ctrl_v", KeyAction::PASTE);
    bindKey("ctrl_a", KeyAction::SELECT_ALL);
    bindKey("ctrl_d", KeyAction::DUPLICATE_LINE);
    bindKey("ctrl_l", KeyAction::DELETE_LINE);
    bindKeyAliases({"ctrl_shift_k"}, KeyAction::DELETE_LINE);
    bindKey("ctrl_backspace", KeyAction::DELETE_WORD);
    bindKey("alt_arrow_up", KeyAction::MOVE_LINE_UP);
    bindKey("alt_arrow_down", KeyAction::MOVE_LINE_DOWN);
    bindKey("tab", KeyAction::INDENT_LINE);
    bindKey("shift_tab", KeyAction::UNINDENT_LINE);
    bindKey("ctrl_slash", KeyAction::TOGGLE_COMMENT);
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
}

void KeyBindingManager::initializeViewOperationBindings() {
    bindKey("ctrl_t", KeyAction::TOGGLE_THEME_MENU);
    bindKey("f1", KeyAction::TOGGLE_HELP);
    bindKey("ctrl_shift_l", KeyAction::TOGGLE_LINE_NUMBERS);
    bindKey("f3", KeyAction::COMMAND_PALETTE);
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
    
    // 更新反向映射
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
        
        // 从反向映射中移除
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

