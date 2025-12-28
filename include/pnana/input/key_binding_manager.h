#ifndef PNANA_INPUT_KEY_BINDING_MANAGER_H
#define PNANA_INPUT_KEY_BINDING_MANAGER_H

#include "input/key_action.h"
#include "input/event_parser.h"
#include <ftxui/component/event.hpp>
#include <map>
#include <string>
#include <vector>

namespace pnana {
namespace input {

// 快捷键绑定管理器
// 职责：管理快捷键到动作的映射，支持配置和查询
class KeyBindingManager {
public:
    KeyBindingManager();
    ~KeyBindingManager() = default;
    
    // 获取事件对应的动作
    KeyAction getAction(const ftxui::Event& event) const;
    
    // 绑定快捷键到动作
    void bindKey(const std::string& key, KeyAction action);
    
    // 绑定多个快捷键到同一个动作（别名）
    void bindKeyAliases(const std::vector<std::string>& keys, KeyAction action);
    
    // 解绑快捷键
    void unbindKey(const std::string& key);
    
    // 获取动作的所有快捷键
    std::vector<std::string> getKeysForAction(KeyAction action) const;
    
    // 获取快捷键对应的动作
    KeyAction getActionForKey(const std::string& key) const;
    
    // 检查是否是全局快捷键（任何模式下都有效）
    bool isGlobalKey(const ftxui::Event& event) const;
    
    // 重置为默认绑定
    void resetToDefaults();
    
    // 从配置加载绑定（未来扩展）
    // void loadFromConfig(const std::string& config_path);
    
    // 保存绑定到配置（未来扩展）
    // void saveToConfig(const std::string& config_path) const;

private:
    // 事件解析器
    EventParser parser_;
    
    // 键到动作的映射
    std::map<std::string, KeyAction> key_to_action_;
    
    // 动作到键的映射（支持一个动作多个快捷键）
    std::map<KeyAction, std::vector<std::string>> action_to_keys_;
    
    // 初始化默认绑定
    void initializeDefaultBindings();
    
    // 初始化文件操作快捷键
    void initializeFileOperationBindings();
    
    // 初始化编辑操作快捷键
    void initializeEditOperationBindings();
    
    // 初始化搜索和导航快捷键
    void initializeSearchNavigationBindings();
    
    // 初始化视图操作快捷键
    void initializeViewOperationBindings();
    
    // 初始化标签页操作快捷键
    void initializeTabOperationBindings();
};

} // namespace input
} // namespace pnana

#endif // PNANA_INPUT_KEY_BINDING_MANAGER_H

