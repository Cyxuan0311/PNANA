#include "core/input/input_router.h"
#include "core/editor.h"
#include "core/input/region_handlers/file_browser_handler.h"
#include "core/input/region_handlers/terminal_handler.h"
#include "input/action_executor.h"
#include "input/key_binding_manager.h"
#include "utils/logger.h"
#include <ftxui/component/event.hpp>

namespace pnana {
namespace core {
namespace input {

InputRouter::InputRouter() : initialized_(false) {
    initializeRegionHandlers();
    initializeModeHandlers();
    initialized_ = true;
}

InputRouter::~InputRouter() = default;

void InputRouter::initializeRegionHandlers() {
    // 初始化区域处理器
    region_handlers_[EditorRegion::TERMINAL] = std::make_unique<TerminalHandler>();
    region_handlers_[EditorRegion::FILE_BROWSER] = std::make_unique<FileBrowserHandler>();
    // 其他区域处理器将在后续阶段添加
}

void InputRouter::initializeModeHandlers() {
    // 模式处理器将在后续阶段实现
    // 这里先留空，后续添加各个模式处理器的初始化
}

bool InputRouter::route(ftxui::Event event, Editor* editor) {
    // 1. 检查全局快捷键（优先级最高）
    if (handleGlobalShortcuts(event, editor)) {
        return true;
    }

    // 2. 检查对话框优先级
    if (handleDialogs(event, editor)) {
        return true;
    }

    // 3. 根据区域分发
    return routeByRegion(event, editor);
}

bool InputRouter::handleGlobalShortcuts(ftxui::Event event, Editor* editor) {
    // 使用现有的 KeyBindingManager 解析事件
    pnana::input::KeyAction action = editor->getKeyBindingManager().getAction(event);

    // 全局快捷键：Alt+A (另存为)、Alt+F (创建文件夹)、Alt+M (文件选择器)
    if (action == pnana::input::KeyAction::SAVE_AS ||
        action == pnana::input::KeyAction::CREATE_FOLDER ||
        action == pnana::input::KeyAction::FILE_PICKER) {
        return editor->getActionExecutor().execute(action);
    }

    return false;
}

bool InputRouter::handleDialogs(ftxui::Event event, Editor* editor) {
    // 对话框优先级：命令面板 > SSH对话框 > 其他对话框
    // 这里需要访问 Editor 的对话框状态
    // 暂时返回 false，具体实现将在后续完善
    (void)event;
    (void)editor;
    return false;
}

bool InputRouter::routeByRegion(ftxui::Event event, Editor* editor) {
    // 获取当前区域
    EditorRegion current_region = editor->getRegionManager().getCurrentRegion();
    std::string region_name = editor->getRegionManager().getRegionName();

    LOG("InputRouter::routeByRegion: Current region=" + region_name + ", event=" + event.input());

    // 查找对应的区域处理器
    auto it = region_handlers_.find(current_region);
    if (it != region_handlers_.end() && it->second) {
        LOG("InputRouter::routeByRegion: Found handler for region " + region_name);

        // 先检查区域导航（左右键切换面板）
        if (it->second->handleNavigation(event, editor)) {
            LOG("InputRouter::routeByRegion: Navigation handled");
            return true;
        }

        // 处理区域特定的输入
        bool handled = it->second->handleInput(event, editor);
        std::string handled_str = handled ? "true" : "false";
        LOG("InputRouter::routeByRegion: Input handled=" + handled_str);

        // 如果是代码区，进一步根据模式分发（如果需要）
        if (handled && current_region == EditorRegion::CODE_AREA) {
            // 代码区的输入可能还需要模式处理器处理
            // 这里暂时返回 handled，后续可以扩展
        }

        return handled;
    }

    // 如果没有找到对应的处理器，返回 false
    LOG("InputRouter::routeByRegion: No handler found for region " + region_name +
        " (handlers registered: " + std::to_string(region_handlers_.size()) + ")");
    return false;
}

bool InputRouter::routeByMode(ftxui::Event event, Editor* editor) {
    // 获取当前模式
    EditorMode current_mode = editor->getMode();

    // 查找对应的模式处理器
    auto it = mode_handlers_.find(current_mode);
    if (it != mode_handlers_.end() && it->second) {
        return it->second->handleInput(event, editor);
    }

    // 如果没有找到对应的处理器，返回 false
    return false;
}

} // namespace input
} // namespace core
} // namespace pnana
