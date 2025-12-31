#include "core/input/region_handlers/file_browser_handler.h"
#include "core/editor.h"
#include "utils/logger.h"
#include <ftxui/component/event.hpp>

using namespace ftxui;

namespace pnana {
namespace core {
namespace input {

FileBrowserHandler::FileBrowserHandler() = default;

bool FileBrowserHandler::handleInput(Event event, Editor* editor) {
    // 文件浏览器区域：处理文件浏览器特定的输入
    if (!editor->isFileBrowserVisible()) {
        LOG("FileBrowserHandler: File browser not visible, ignoring input");
        return false;
    }
    
    // 确保当前区域是文件浏览器
    EditorRegion current_region = editor->getRegionManager().getCurrentRegion();
    if (current_region != EditorRegion::FILE_BROWSER) {
        LOG("FileBrowserHandler: Current region is " + editor->getRegionManager().getRegionName() + ", switching to FILE_BROWSER");
        editor->getRegionManager().setRegion(EditorRegion::FILE_BROWSER);
    }
    
    std::string is_char_str = event.is_character() ? "true" : "false";
    LOG("FileBrowserHandler: Received event: " + event.input() + " (is_character=" + is_char_str + ")");
    
    // 处理文件浏览器宽度调整：+ 增加宽度，- 减少宽度
    if (event == Event::Character('+') || event == Event::Character('=')) {
        // + 或 = 键：增加文件浏览器宽度
        int current_width = editor->getFileBrowserWidth();
        int screen_width = editor->getScreenWidth();
        int new_width = current_width + 1;
        LOG("FileBrowserHandler: + key pressed, current_width=" + std::to_string(current_width) + ", screen_width=" + std::to_string(screen_width));
        // 限制最大宽度（保留至少20列给代码区）
        if (new_width < screen_width - 20) {
            editor->setFileBrowserWidth(new_width);
            editor->setStatusMessage("File browser width: " + std::to_string(new_width) + " columns (+: increase, -: decrease)");
            LOG("FileBrowserHandler: Increased file browser width to " + std::to_string(new_width));
        } else {
            LOG("FileBrowserHandler: Cannot increase width, would exceed limit (max=" + std::to_string(screen_width - 20) + ")");
        }
        return true;
    } else if (event == Event::Character('-') || event == Event::Character('_')) {
        // - 或 _ 键：减少文件浏览器宽度
        int current_width = editor->getFileBrowserWidth();
        int new_width = current_width - 1;
        LOG("FileBrowserHandler: - key pressed, current_width=" + std::to_string(current_width));
        // 限制最小宽度（至少10列）
        if (new_width >= 10) {
            editor->setFileBrowserWidth(new_width);
            editor->setStatusMessage("File browser width: " + std::to_string(new_width) + " columns (+: increase, -: decrease)");
            LOG("FileBrowserHandler: Decreased file browser width to " + std::to_string(new_width));
        } else {
            LOG("FileBrowserHandler: Cannot decrease width, would be below minimum (min=10)");
        }
        return true;
    }
    
    // 其他输入事件不在这里处理，返回 false 让其他处理器处理
    LOG("FileBrowserHandler: Event not handled, returning false");
    return false;
}

bool FileBrowserHandler::handleNavigation(Event event, Editor* editor) {
    // 文件浏览器区域导航：左右键用于切换面板
    if (event == Event::ArrowRight) {
        // 右键：切换到代码区
        editor->getRegionManager().setRegion(EditorRegion::CODE_AREA);
        editor->setStatusMessage("Switched to code area | Press ← to return to file browser");
        return true;
    } else if (event == Event::ArrowLeft) {
        // 左键：文件浏览器已经在最左侧，无法再向左
        return false;
    }
    
    // 上下键在文件浏览器内处理（文件列表导航）
    if (event == Event::ArrowUp || event == Event::ArrowDown) {
        return false;  // 让文件浏览器内部处理
    }
    
    return false;
}

std::vector<pnana::input::KeyAction> FileBrowserHandler::getSupportedActions() const {
    // 文件浏览器区域支持的快捷键
    return {
        // 文件浏览器特定的操作可以在这里列出
    };
}

} // namespace input
} // namespace core
} // namespace pnana

