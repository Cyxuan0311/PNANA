#include "core/input/region_handlers/file_browser_handler.h"
#include "core/editor.h"
#include "input/event_parser.h"
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
        LOG("FileBrowserHandler: Current region is " + editor->getRegionManager().getRegionName() +
            ", switching to FILE_BROWSER");
        editor->getRegionManager().setRegion(EditorRegion::FILE_BROWSER);
    }

    std::string is_char_str = event.is_character() ? "true" : "false";
    LOG("FileBrowserHandler: Received event: " + event.input() + " (is_character=" + is_char_str +
        ")");

    // 处理文件浏览器宽度调整：+ 增加宽度，- 减少宽度
    if (event == Event::Character('+') || event == Event::Character('=')) {
        // + 或 = 键：增加文件浏览器宽度
        int current_width = editor->getFileBrowserWidth();
        int screen_width = editor->getScreenWidth();
        int new_width = current_width + 1;
        LOG("FileBrowserHandler: + key pressed, current_width=" + std::to_string(current_width) +
            ", screen_width=" + std::to_string(screen_width));
        // 限制最大宽度（保留至少20列给代码区）
        if (new_width < screen_width - 20) {
            editor->setFileBrowserWidth(new_width);
            editor->setStatusMessage("File browser width: " + std::to_string(new_width) +
                                     " columns (+: increase, -: decrease)");
            LOG("FileBrowserHandler: Increased file browser width to " + std::to_string(new_width));
        } else {
            LOG("FileBrowserHandler: Cannot increase width, would exceed limit (max=" +
                std::to_string(screen_width - 20) + ")");
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
            editor->setStatusMessage("File browser width: " + std::to_string(new_width) +
                                     " columns (+: increase, -: decrease)");
            LOG("FileBrowserHandler: Decreased file browser width to " + std::to_string(new_width));
        } else {
            LOG("FileBrowserHandler: Cannot decrease width, would be below minimum (min=10)");
        }
        return true;
    }

    // 切换显示/隐藏默认忽略的隐藏文件和文件夹（以 . 开头）
    if (event == Event::Character('.')) {
        bool current_show_hidden = editor->file_browser_.getShowHidden();
        bool new_show_hidden = !current_show_hidden;
        editor->file_browser_.setShowHidden(new_show_hidden);

        if (new_show_hidden) {
            editor->setStatusMessage(
                "Showing hidden files and folders (press . to hide them again)");
        } else {
            editor->setStatusMessage(
                "Hiding hidden files and folders (press . to show them again)");
        }

        LOG("FileBrowserHandler: Toggled show_hidden to " +
            std::string(new_show_hidden ? "true" : "false"));
        return true;
    }

    // ESC 键处理已移至 editor_input.cpp 的 handleFileBrowserInput 中
    // 这里不再处理，避免重复处理

    // F6: 移动文件/文件夹
    if (event == Event::F6) {
        LOG("FileBrowserHandler: F6 detected, opening move file dialog");
        if (editor->file_browser_.hasSelection()) {
            editor->startMoveFile();
            return true;
        } else {
            editor->setStatusMessage("No file or folder selected");
            LOG("FileBrowserHandler: No selection to move");
            return true;
        }
    }

    // Ctrl+P: 复制选中的文件/文件夹
    if (event == Event::CtrlP) {
        if (editor->file_browser_.getSelectedCount() > 0) {
            if (editor->file_browser_.copySelected()) {
                editor->setStatusMessage("Copied " +
                                         std::to_string(editor->file_browser_.getSelectedCount()) +
                                         " item(s)");
            } else {
                editor->setStatusMessage("Failed to copy files");
            }
        } else {
            editor->setStatusMessage("No files selected");
        }
        return true;
    }

    // Ctrl+X: 剪切选中的文件/文件夹
    if (event == Event::CtrlX) {
        if (editor->file_browser_.getSelectedCount() > 0) {
            if (editor->file_browser_.cutSelected()) {
                editor->setStatusMessage(
                    "Cut " + std::to_string(editor->file_browser_.getSelectedCount()) + " item(s)");
            } else {
                editor->setStatusMessage("Failed to cut files");
            }
        } else {
            editor->setStatusMessage("No files selected");
        }
        return true;
    }

    // Ctrl+V: 粘贴文件/文件夹
    if (event == Event::CtrlV) {
        if (editor->file_browser_.hasClipboardFiles()) {
            std::string target_dir = editor->file_browser_.getCurrentDirectory();
            if (editor->file_browser_.pasteFiles(target_dir)) {
                std::string op = editor->file_browser_.isCutOperation() ? "Moved" : "Copied";
                editor->setStatusMessage(op + " files successfully");
            } else {
                editor->setStatusMessage("Failed to paste files");
            }
        } else {
            editor->setStatusMessage("No files in clipboard");
        }
        return true;
    }

    // 其他输入事件不在这里处理，返回 false 让其他处理器处理
    LOG("FileBrowserHandler: Event not handled, returning false");
    return false;
}

bool FileBrowserHandler::handleNavigation(Event event, Editor* editor) {
    // 如果在分屏模式下，让 InputRouter 处理分屏导航
    if (editor->getSplitViewManager().hasSplits()) {
        return false;
    }

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

    // PageUp/PageDown键直接在这里处理
    if (event == Event::PageUp) {
        editor->pageUp();
        return true;
    } else if (event == Event::PageDown) {
        editor->pageDown();
        return true;
    }

    // 检查 Alt+0 和 Alt+9 组合键用于页面滚动
    pnana::input::EventParser parser;
    std::string key_str = parser.eventToKey(event);
    if (key_str == "alt_0") {
        LOG("FileBrowserHandler: Alt+0 detected, calling pageUp()");
        editor->pageUp();
        return true;
    } else if (key_str == "alt_9") {
        LOG("FileBrowserHandler: Alt+9 detected, calling pageDown()");
        editor->pageDown();
        return true;
    }

    // 检查 Ctrl+Z 组合键用于撤销删除
    if (event == Event::CtrlZ) {
        LOG("FileBrowserHandler: Ctrl+Z detected, attempting to undo delete");
        if (editor->file_browser_.canUndoDelete()) {
            bool success = editor->file_browser_.undoDelete();
            if (success) {
                editor->setStatusMessage("Restored: " + editor->file_browser_.getSelectedName());
                LOG("FileBrowserHandler: Successfully restored deleted item");
            } else {
                editor->setStatusMessage("Failed to restore deleted item");
                LOG("FileBrowserHandler: Failed to restore deleted item");
            }
        } else {
            editor->setStatusMessage("Nothing to undo");
            LOG("FileBrowserHandler: No delete operation to undo");
        }
        return true;
    }

    // Ctrl+上下键：批量选中
    if (event == Event::ArrowUpCtrl) {
        LOG("FileBrowserHandler: Ctrl+Up detected, extending selection");
        size_t current_index = editor->file_browser_.getSelectedIndex();
        if (current_index > 0) {
            // 先确保当前项被选中（如果未选中则选中，已选中则保持）
            if (!editor->file_browser_.isSelected(current_index)) {
                editor->file_browser_.toggleSelection(current_index);
            }
            // 移动到上一个
            editor->file_browser_.selectPrevious();
            size_t new_index = editor->file_browser_.getSelectedIndex();
            // 选中从 new_index 到 current_index 的范围（包括两端）
            editor->file_browser_.selectRange(new_index, current_index);
            editor->setStatusMessage(std::to_string(editor->file_browser_.getSelectedCount()) +
                                     " item(s) selected");
        }
        return true;
    } else if (event == Event::ArrowDownCtrl) {
        LOG("FileBrowserHandler: Ctrl+Down detected, extending selection");
        size_t current_index = editor->file_browser_.getSelectedIndex();
        size_t item_count = editor->file_browser_.getItemCount();
        if (current_index < item_count - 1) {
            // 先确保当前项被选中（如果未选中则选中，已选中则保持）
            if (!editor->file_browser_.isSelected(current_index)) {
                editor->file_browser_.toggleSelection(current_index);
            }
            // 移动到下一个
            editor->file_browser_.selectNext();
            size_t new_index = editor->file_browser_.getSelectedIndex();
            // 选中从 current_index 到 new_index 的范围（包括两端）
            editor->file_browser_.selectRange(current_index, new_index);
            editor->setStatusMessage(std::to_string(editor->file_browser_.getSelectedCount()) +
                                     " item(s) selected");
        }
        return true;
    }

    // 上下键在文件浏览器内处理（文件列表导航）
    if (event == Event::ArrowUp || event == Event::ArrowDown) {
        return false; // 让文件浏览器内部处理
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
