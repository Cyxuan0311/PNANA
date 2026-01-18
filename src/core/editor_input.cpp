// 输入处理相关实现
#include "core/editor.h"
#include "core/input/input_router.h"
#include "input/event_parser.h"
#include "input/key_action.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <filesystem>
#include <ftxui/component/event.hpp>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

using namespace ftxui;

namespace pnana {
namespace core {

// 事件处理
void Editor::handleInput(Event event) {
    // 特殊处理Event::Custom（我们的渲染触发事件）
    if (event == Event::Custom) {
        LOG("[DEBUG EVENT] Received Event::Custom - this should trigger a render update");
        // Event::Custom是我们手动触发的渲染更新事件，不需要额外处理
        // 直接返回，让FTXUI重新渲染
        return;
    }

    // 记录事件处理开始（仅对关键事件）
    if (event == Event::Return || event == Event::Escape || event == Event::ArrowUp ||
        event == Event::ArrowDown || event == Event::ArrowLeft || event == Event::ArrowRight) {
        LOG("handleInput() called for event: " + event.input());
    }

    // GOTO_LINE 模式：完全参考搜索模式的实现，不做任何特殊处理
    // 搜索模式在 handleInput() 中没有任何特殊处理，直接路由到 handleSearchMode()
    // 所以 GOTO_LINE 模式也应该一样，不做任何特殊处理

    // 更新区域可用性
    region_manager_.setTabAreaEnabled(document_manager_.getDocumentCount() > 1);
    region_manager_.setFileBrowserEnabled(file_browser_.isVisible());
    region_manager_.setTerminalEnabled(terminal_.isVisible());
    region_manager_.setHelpWindowEnabled(show_help_);

    // 使用 InputRouter 处理输入（如果已初始化）
    // 支持终端、文件浏览器和Git面板区域
    if (input_router_) {
        // 如果当前在终端、文件浏览器或Git面板区域，使用 InputRouter
        EditorRegion current_region = region_manager_.getCurrentRegion();
        if (current_region == EditorRegion::TERMINAL ||
            current_region == EditorRegion::FILE_BROWSER ||
            current_region == EditorRegion::GIT_PANEL) {
            if (input_router_->route(event, this)) {
                LOG("InputRouter handled event for region: " + region_manager_.getRegionName());
                return;
            }
        }
    }

    // 如果 InputRouter 未初始化且终端可见，使用原有逻辑
    if (terminal_.isVisible() && !input_router_) {
        handleTerminalInput(event);
        return;
    }

    // 首先检查全局快捷键（在任何模式下都有效，包括对话框打开时）
    // 使用新的输入处理系统
    using namespace pnana::input;

    // 如果文件选择器可见且按下Tab键，直接交给文件选择器处理
    if (file_picker_.isVisible() && (event == Event::Tab || event == Event::Character('\t'))) {
        if (file_picker_.handleInput(event)) {
            return;
        }
    }

    // 调试信息：检查 Ctrl+P 事件
    if (event == ftxui::Event::CtrlP) {
        LOG("[DEBUG COPY] Ctrl+P event detected at start of handleInput!");
    }

    KeyAction action = key_binding_manager_.getAction(event);

    // 调试信息：检查 Ctrl+P 事件解析结果
    if (event == ftxui::Event::CtrlP) {
        LOG("[DEBUG COPY] After getAction, action: " + std::to_string(static_cast<int>(action)) +
            " (COPY=" + std::to_string(static_cast<int>(KeyAction::COPY)) +
            ", UNKNOWN=" + std::to_string(static_cast<int>(KeyAction::UNKNOWN)) + ")");
    }

    // Alt+A (另存为)、Alt+F (创建文件夹) 和 Alt+M (文件选择器) 应该能够在任何情况下工作
    // 包括在对话框中或文件浏览器打开时
    if (action == KeyAction::SAVE_AS || action == KeyAction::CREATE_FOLDER ||
        action == KeyAction::FILE_PICKER) {
        if (action_executor_.execute(action)) {
            return;
        }
    }

    // 如果命令面板打开，优先处理
    if (command_palette_.isOpen()) {
        handleCommandPaletteInput(event);
        return;
    }

    // 如果搜索对话框打开，优先处理
    if (search_dialog_.isVisible()) {
        if (search_dialog_.handleInput(event)) {
            return;
        }
        // 如果搜索对话框打开但没有处理输入，屏蔽其他输入
        return;
    }

    // 如果 SSH 传输对话框打开，优先处理
    if (ssh_transfer_dialog_.isVisible()) {
        if (ssh_transfer_dialog_.handleInput(event)) {
            return;
        }
    }

    // 如果 SSH 对话框打开，优先处理（类似帮助窗口）
    if (ssh_dialog_.isVisible()) {
        if (ssh_dialog_.handleInput(event)) {
            // 对话框处理了输入（可能是关闭或确认）
            return;
        }
        // 如果对话框仍然打开，继续处理其他输入
        return;
    }

    // 如果当前在对话框中，其他快捷键不处理（让对话框处理输入）
    // 但文件选择器可以在任何情况下打开
    bool in_dialog = show_save_as_ || show_create_folder_ || show_theme_menu_ || show_help_ ||
                     split_dialog_.isVisible() || ssh_dialog_.isVisible() ||
                     search_dialog_.isVisible() || cursor_config_dialog_.isVisible()
#ifdef BUILD_LUA_SUPPORT
                     || plugin_manager_dialog_.isVisible()
#endif
        ;

    // 如果当前在搜索模式，优先处理输入（除了 Escape 和 Return）
    bool in_search_mode = (mode_ == EditorMode::SEARCH);
    bool should_skip_shortcuts =
        in_search_mode && (event != Event::Escape && event != Event::Return);

    if (in_dialog) {
        // 对话框内的输入处理在下面
        // 但文件选择器仍然可以打开
        if (action == KeyAction::FILE_PICKER) {
            if (action_executor_.execute(action)) {
                return;
            }
        }
    } else if (action != KeyAction::UNKNOWN && action != KeyAction::SPLIT_VIEW &&
               !should_skip_shortcuts) {
        // 不在对话框中，处理其他全局快捷键（除了 SPLIT_VIEW，它在文件浏览器中处理）
        // 搜索模式下跳过（除了 Escape）
        if (action_executor_.execute(action)) {
            return;
        }
    }

    // 如果另存为对话框打开，优先处理
    if (show_save_as_) {
        if (event == Event::Escape) {
            show_save_as_ = false;
            save_as_dialog_.setInput("");
            setStatusMessage("Save as cancelled");
        } else if (event == Event::Return) {
            std::string input = save_as_dialog_.getInput();
            if (!input.empty()) {
                // 如果输入的不是完整路径（不包含目录分隔符），则使用当前目录
                std::string filepath = input;
                if (input.find('/') == std::string::npos && input.find('\\') == std::string::npos) {
                    // 只输入了文件名，添加当前目录
                    Document* doc = getCurrentDocument();
                    if (doc && !doc->getFilePath().empty()) {
                        // 使用当前文件的目录
                        std::filesystem::path current_path(doc->getFilePath());
                        filepath = (current_path.parent_path() / input).string();
                    } else {
                        // 使用文件浏览器的当前目录
                        filepath =
                            (std::filesystem::path(file_browser_.getCurrentDirectory()) / input)
                                .string();
                    }
                }
                // 保存文件
                if (saveFileAs(filepath)) {
                    show_save_as_ = false;
                    save_as_dialog_.setInput("");
                }
            }
        } else if (event == Event::Backspace) {
            std::string input = save_as_dialog_.getInput();
            if (!input.empty()) {
                input.pop_back();
                save_as_dialog_.setInput(input);
            }
        } else if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                // 接受所有可打印字符（文件路径可以包含各种字符）
                if (c >= 32 && c < 127) {
                    std::string input = save_as_dialog_.getInput();
                    input += c;
                    save_as_dialog_.setInput(input);
                }
            }
        }
        return;
    }

    // 如果创建文件夹对话框打开，优先处理
    if (show_create_folder_) {
        if (event == Event::Escape) {
            show_create_folder_ = false;
            create_folder_dialog_.setInput("");
            setStatusMessage("Folder creation cancelled");
        } else if (event == Event::Return) {
            std::string input = create_folder_dialog_.getInput();
            if (!input.empty()) {
                // 创建文件夹 - 使用C++ filesystem
                try {
                    fs::path folder_path = fs::path(file_browser_.getCurrentDirectory()) / input;
                    if (fs::create_directory(folder_path)) {
                        show_create_folder_ = false;
                        create_folder_dialog_.setInput("");
                        file_browser_.refresh();
                        // 自动选中新创建的文件夹
                        file_browser_.selectItemByName(input);
                        setStatusMessage(std::string(pnana::ui::icons::FOLDER) +
                                         " Folder created: " + input);
                    } else {
                        setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                                         " Failed to create folder (may already exist): " + input);
                    }
                } catch (const std::exception& e) {
                    setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                                     " Error: " + std::string(e.what()));
                }
            }
        } else if (event == Event::Backspace) {
            std::string input = create_folder_dialog_.getInput();
            if (!input.empty()) {
                input.pop_back();
                create_folder_dialog_.setInput(input);
            }
        } else if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                // 只接受可打印ASCII字符，排除文件名禁用字符
                if ((c >= 32 && c < 127) && c != '/' && c != '\\' && c != ':' && c != '*' &&
                    c != '?' && c != '"' && c != '<' && c != '>' && c != '|') {
                    std::string input = create_folder_dialog_.getInput();
                    input += c;
                    create_folder_dialog_.setInput(input);
                }
            }
        }
        return;
    }

    // 如果帮助窗口打开，优先处理
    if (show_help_) {
        region_manager_.setRegion(EditorRegion::HELP_WINDOW);
        // 处理翻页等操作
        if (help_.handleInput(event)) {
            return;
        }
        // 关闭帮助
        if (event == Event::Escape || event == Event::F1) {
            show_help_ = false;
            help_.reset();
            region_manager_.setRegion(EditorRegion::CODE_AREA);
            setStatusMessage("Help closed | Region: " + region_manager_.getRegionName());
        }
        return;
    }

    // 如果主题菜单打开，优先处理
    if (show_theme_menu_) {
        if (event == Event::Escape) {
            show_theme_menu_ = false;
            setStatusMessage("Theme selection cancelled | Region: " +
                             region_manager_.getRegionName());
        } else if (event == Event::ArrowUp || event == Event::Character('k')) {
            selectPreviousTheme();
        } else if (event == Event::ArrowDown || event == Event::Character('j')) {
            selectNextTheme();
        } else if (event == Event::Return) {
            applySelectedTheme();
            show_theme_menu_ = false;
        }
        return;
    }

    // 优先处理光标配置对话框输入
    if (cursor_config_dialog_.isVisible()) {
        if (cursor_config_dialog_.handleInput(event)) {
            return;
        }
    }

#ifdef BUILD_LUA_SUPPORT
    // 优先处理插件管理对话框输入
    if (plugin_manager_dialog_.isVisible()) {
        if (plugin_manager_dialog_.handleInput(event)) {
            return;
        }
    }
#endif

    // 优先处理对话框输入
    if (dialog_.isVisible()) {
        if (dialog_.handleInput(event)) {
            return;
        }
    }

    // 优先处理文件选择器输入（除了Tab键，Tab键已在前面处理）
    if (file_picker_.isVisible()) {
        if (file_picker_.handleInput(event)) {
            return;
        }
    }

    // 编码对话框输入处理
    if (encoding_dialog_.isVisible()) {
        handleEncodingDialogInput(event);
        return;
    }

    // 优先处理分屏对话框输入
    if (split_dialog_.isVisible()) {
        if (split_dialog_.handleInput(event)) {
            return;
        }
    }

    // 优先处理格式化对话框输入
    if (format_dialog_.isOpen()) {
        if (format_dialog_.handleInput(event)) {
            return;
        }
    }

    // 处理鼠标事件（用于拖动分屏线）
    if (event.is_mouse() && split_view_manager_.hasSplits()) {
        int screen_width = screen_.dimx();
        int screen_height = screen_.dimy();

        // 计算编辑器区域的偏移（考虑文件浏览器、标签栏等）
        int editor_x_offset = 0;
        int editor_y_offset = 1; // 标签栏

        if (file_browser_.isVisible()) {
            editor_x_offset = file_browser_width_ + 1; // 文件浏览器宽度 + 分隔符
        }

        // 计算编辑器区域尺寸（偏移在 handleMouseEvent 中处理）
        int editor_width = screen_width - editor_x_offset;
        int editor_height = screen_height - 6; // 减去标签栏、状态栏等

        if (split_view_manager_.handleMouseEvent(event, editor_width, editor_height,
                                                 editor_x_offset, editor_y_offset)) {
            return;
        }
    }

    // Ctrl+L: 如果已经有分屏，显示关闭分屏对话框（在代码区也可以使用）
    if (event == Event::CtrlL && split_view_manager_.hasSplits()) {
        showSplitDialog();
        return;
    }

    // 如果有分屏，优先处理分屏导航快捷键（Ctrl+方向键）
    if (split_view_manager_.hasSplits()) {
        using namespace pnana::input;
        KeyAction nav_action = key_binding_manager_.getAction(event);
        if (nav_action == KeyAction::FOCUS_LEFT_REGION ||
            nav_action == KeyAction::FOCUS_RIGHT_REGION ||
            nav_action == KeyAction::FOCUS_UP_REGION ||
            nav_action == KeyAction::FOCUS_DOWN_REGION) {
            if (action_executor_.execute(nav_action)) {
                return;
            }
        }
    }

    // 如果文件浏览器打开，处理文件浏览器输入
    // 但全局快捷键（如 Alt+A, Alt+F）应该优先处理
    if (file_browser_.isVisible()) {
        // 先检查是否是全局快捷键（Alt+A, Alt+F, Alt+M 等）
        if (action == KeyAction::SAVE_AS || action == KeyAction::CREATE_FOLDER ||
            action == KeyAction::FILE_PICKER) {
            if (action_executor_.execute(action)) {
                return;
            }
        }
        // 其他快捷键在文件浏览器中不处理，交给文件浏览器处理
        region_manager_.setRegion(EditorRegion::FILE_BROWSER);
        handleFileBrowserInput(event);
        return;
    }

    // 再次检查全局快捷键（如果之前没有处理）
    // 这确保在非对话框模式下，所有快捷键都能正常工作
    // 但搜索模式下不处理（除了 Escape）
    // 剪贴板操作和选择操作（Ctrl+C/V/X, Ctrl+A, Alt+D, Alt+Shift+方向键）只在代码区生效
    if (action != KeyAction::UNKNOWN && !should_skip_shortcuts) {
        EditorRegion current_region = region_manager_.getCurrentRegion();

        // 剪贴板操作和选择操作只在代码区生效
        if (action == KeyAction::COPY || action == KeyAction::PASTE || action == KeyAction::CUT ||
            action == KeyAction::SELECT_ALL || action == KeyAction::SELECT_WORD ||
            action == KeyAction::SELECT_EXTEND_UP || action == KeyAction::SELECT_EXTEND_DOWN ||
            action == KeyAction::SELECT_EXTEND_LEFT || action == KeyAction::SELECT_EXTEND_RIGHT) {
            LOG("[DEBUG COPY] Action detected: " + std::to_string(static_cast<int>(action)) +
                " (COPY=" + std::to_string(static_cast<int>(KeyAction::COPY)) + ")");
            LOG("[DEBUG COPY] Current region: " + std::to_string(static_cast<int>(current_region)) +
                " (CODE_AREA=" + std::to_string(static_cast<int>(EditorRegion::CODE_AREA)) + ")");

            if (current_region != EditorRegion::CODE_AREA) {
                // 不在代码区，忽略这些操作
                LOG("[DEBUG COPY] Not in CODE_AREA, ignoring copy action");
                return;
            }
            // 确保有文档
            if (!getCurrentDocument()) {
                LOG("[DEBUG COPY] No document available, ignoring copy action");
                return;
            }
            LOG("[DEBUG COPY] Region check passed, proceeding with copy");
        }

        LOG("[DEBUG COPY] About to execute action: " + std::to_string(static_cast<int>(action)));

        if (action_executor_.execute(action)) {
            LOG("[DEBUG COPY] ActionExecutor returned true");
            return;
        } else {
            LOG("[DEBUG COPY] ActionExecutor returned false");
        }
    } else {
        if (event == ftxui::Event::CtrlP) {
            LOG("[DEBUG COPY] Ctrl+P event but action is UNKNOWN or shortcuts skipped");
            LOG("[DEBUG COPY] action: " + std::to_string(static_cast<int>(action)) +
                " (UNKNOWN=" + std::to_string(static_cast<int>(KeyAction::UNKNOWN)) + ")");
            LOG("[DEBUG COPY] should_skip_shortcuts: " +
                std::string(should_skip_shortcuts ? "true" : "false"));
        }
    }

    // 根据模式处理其他输入
    switch (mode_) {
        case EditorMode::NORMAL:
            handleNormalMode(event);
            break;
        case EditorMode::SEARCH:
            handleSearchMode(event);
            break;
        case EditorMode::REPLACE:
            handleReplaceMode(event);
            break;
    }

    // 检查是否有待处理的光标更新需要触发
    auto now = std::chrono::steady_clock::now();
    if (pending_cursor_update_ && (now - last_render_time_) >= CURSOR_UPDATE_DELAY) {
        LOG("[DEBUG INCREMENTAL] Auto-triggering pending cursor update after delay");
        triggerPendingCursorUpdate();
    }

    // 移除全局的adjustViewOffset调用，避免与特定操作中的调用重复
    // 各个操作（如moveCursorUp等）已经负责调用adjustViewOffset

    // 注意：退出逻辑现在在 quit() 方法中直接处理
    // 这里保留检查是为了兼容性，但通常不会到达这里
    if (should_quit_) {
        screen_.ExitLoopClosure()();
    }
}

void Editor::handleNormalMode(Event event) {
    // If no document and 'i' key is pressed, create new document
    if (getCurrentDocument() == nullptr && event.is_character()) {
        std::string ch = event.character();
        if (ch == "i" || ch == "I") {
            newFile();
            setStatusMessage(std::string(pnana::ui::icons::NEW) +
                             " New document created | Region: " + region_manager_.getRegionName());
            return;
        }
    }

    // If no document, ignore other inputs
    if (getCurrentDocument() == nullptr) {
        return;
    }

#ifdef BUILD_LSP_SUPPORT
    // 优先处理补全弹窗的导航键，避免影响代码区光标
    // 必须在处理其他按键之前检查，确保补全导航优先
    if (completion_popup_.isVisible()) {
        if (event == Event::ArrowUp || event == Event::ArrowDown || event == Event::Return ||
            event == Event::Tab || event == Event::Escape) {
            handleCompletionInput(event);
            return; // 补全弹窗打开时，这些键只用于补全导航，不继续处理
        }
    }

    // 处理诊断弹窗的输入
    if (show_diagnostics_popup_) {
        // diagnostics popup is visible, processing input
        // 首先检查是否是关闭弹窗的快捷键（Alt+E）
        pnana::input::EventParser parser;
        std::string key_str = parser.eventToKey(event);
        // parsed key: key_str
        if (key_str == "alt_e") {
            // Alt+E detected, hiding diagnostics popup
            hideDiagnosticsPopup();
            return;
        }

        // 处理弹窗内的导航和操作
        bool handled = diagnostics_popup_.handleInput(event);

        // 如果弹窗内部处理了隐藏（如按 Esc），同步更新编辑器的显示标志
        if (handled && !diagnostics_popup_.isVisible()) {
            show_diagnostics_popup_ = false;
            // ensure fully hidden
            diagnostics_popup_.hide();
        }

        return; // 诊断弹窗打开时，优先处理其输入
    }
#endif

    // Normal mode = editing mode, can input directly
    // Arrow keys - 智能区域导航系统

    // 标签区域的特殊处理（总是有效）
    if (region_manager_.getCurrentRegion() == EditorRegion::TAB_AREA) {
        if (event == Event::ArrowLeft || event == Event::ArrowRight) {
            // 标签区域：左右键切换标签
            int old_index = region_manager_.getTabIndex();
            if (event == Event::ArrowLeft) {
                region_manager_.previousTab();
            } else {
                region_manager_.nextTab();
            }
            int new_index = region_manager_.getTabIndex();
            if (new_index != old_index && new_index >= 0 &&
                new_index < static_cast<int>(document_manager_.getDocumentCount())) {
                document_manager_.switchToDocument(new_index);

                // 如果在分屏模式下，更新激活区域的文档索引
                if (split_view_manager_.hasSplits()) {
                    split_view_manager_.setCurrentDocumentIndex(new_index);
                }

                cursor_row_ = 0;
                cursor_col_ = 0;
                view_offset_row_ = 0;
                view_offset_col_ = 0;
                syntax_highlighter_.setFileType(getFileType());
                setStatusMessage(
                    "Region: " + region_manager_.getRegionName() +
                    " | Tab: " + getCurrentDocument()->getFileName() +
                    (split_view_manager_.hasSplits() ? " | ↓: Return to split view" : ""));
            }
            return;
        } else if (event == Event::ArrowDown) {
            // 标签区域：下键切换到代码区
            if (region_manager_.navigateDown()) {
                setStatusMessage("Region: " + region_manager_.getRegionName() +
                                 (split_view_manager_.hasSplits() ? " | ↑: Return to tabs" : ""));
                return;
            }
        }
        // 其他方向键在标签区域不处理
        return;
    }

    if (event == Event::ArrowUp) {
        // 处理不同区域的向上导航
        if (split_view_manager_.hasSplits()) {
            // 分屏模式下：优先尝试区域切换（到标签区域），然后是分屏导航
            if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 在代码区顶部时，切换到标签区域
                if (cursor_row_ == 0) {
                    if (region_manager_.navigateUp()) {
                        region_manager_.setTabIndex(document_manager_.getCurrentIndex());
                        setStatusMessage("Region: " + region_manager_.getRegionName() +
                                         " | ←→: Switch tabs, ↓: Return to split view");
                        return;
                    }
                }
                // 不在顶部时，尝试分屏导航
                size_t old_active_index = 0;
                const auto* old_active = split_view_manager_.getActiveRegion();
                if (old_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == old_active) {
                            old_active_index = i;
                            break;
                        }
                    }
                }

                split_view_manager_.focusUpRegion();

                const auto* new_active = split_view_manager_.getActiveRegion();
                size_t new_active_index = 0;
                if (new_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == new_active) {
                            new_active_index = i;
                            break;
                        }
                    }
                }

                if (new_active_index != old_active_index) {
                    // 分屏导航成功，保存当前状态，更新文档，恢复新区域状态
                    saveCurrentRegionState();

                    const auto* active_region = split_view_manager_.getActiveRegion();
                    if (active_region) {
                        document_manager_.switchToDocument(active_region->document_index);
                        // 恢复新区域的状态
                        restoreRegionState(new_active_index);
                        syntax_highlighter_.setFileType(getFileType());
                    }
                    setStatusMessage("Split view: Region " + std::to_string(new_active_index + 1) +
                                     "/" + std::to_string(split_view_manager_.getRegionCount()) +
                                     " | Use ↑↓←→ to navigate between regions");
                    return;
                }
                // 分屏导航失败，在当前区域内移动光标
                moveCursorUp();
                return;
            }
        } else {
            // 非分屏模式下的传统导航
            if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 代码区：在顶部时切换到标签区，否则移动光标
                if (cursor_row_ == 0 && document_manager_.getDocumentCount() > 1) {
                    if (region_manager_.navigateUp()) {
                        region_manager_.setTabIndex(document_manager_.getCurrentIndex());
                        setStatusMessage("Region: " + region_manager_.getRegionName() +
                                         " | ←→: Switch tabs, ↓: Return");
                        return;
                    }
                }
                moveCursorUp();
            } else if (region_manager_.getCurrentRegion() == EditorRegion::TERMINAL) {
                // 终端：向上切换到代码区
                if (region_manager_.navigateUp()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            } else if (region_manager_.getCurrentRegion() == EditorRegion::FILE_BROWSER) {
                // 文件浏览器：向上切换到标签区
                if (region_manager_.navigateUp()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            }
        }
    } else if (event == Event::ArrowDown) {
        // 处理不同区域的向下导航
        if (split_view_manager_.hasSplits()) {
            // 分屏模式下：优先尝试分屏导航，然后是区域切换
            if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 首先尝试分屏导航
                size_t old_active_index = 0;
                const auto* old_active = split_view_manager_.getActiveRegion();
                if (old_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == old_active) {
                            old_active_index = i;
                            break;
                        }
                    }
                }

                split_view_manager_.focusDownRegion();

                const auto* new_active = split_view_manager_.getActiveRegion();
                size_t new_active_index = 0;
                if (new_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == new_active) {
                            new_active_index = i;
                            break;
                        }
                    }
                }

                if (new_active_index != old_active_index) {
                    // 分屏导航成功，保存当前状态，更新文档，恢复新区域状态
                    saveCurrentRegionState();

                    const auto* active_region = split_view_manager_.getActiveRegion();
                    if (active_region) {
                        document_manager_.switchToDocument(active_region->document_index);
                        // 恢复新区域的状态
                        restoreRegionState(new_active_index);
                        syntax_highlighter_.setFileType(getFileType());
                    }
                    setStatusMessage("Split view: Region " + std::to_string(new_active_index + 1) +
                                     "/" + std::to_string(split_view_manager_.getRegionCount()) +
                                     " | Use ↑↓←→ to navigate between regions");
                    return;
                }

                // 分屏导航失败，尝试区域切换（当光标在底部时）
                Document* doc = getCurrentDocument();
                if (doc) {
                    size_t total_lines = doc->lineCount();
                    int screen_height = screen_.dimy() - 6;
                    size_t last_visible_row = view_offset_row_ + screen_height - 1;

                    if (cursor_row_ >= total_lines - 1 || cursor_row_ >= last_visible_row) {
                        if (terminal_.isVisible()) {
                            if (region_manager_.navigateDown()) {
                                setStatusMessage("Region: " + region_manager_.getRegionName() +
                                                 " | ↑: Return to split view");
                                return;
                            }
                        } else if (file_browser_.isVisible()) {
                            // 如果终端不可见，尝试切换到文件浏览器
                            if (region_manager_.navigateDown()) {
                                setStatusMessage("Region: " + region_manager_.getRegionName() +
                                                 " | ↑: Return to split view");
                                return;
                            }
                        }
                    }
                }
                // 都不成功，在当前区域内移动光标
                moveCursorDown();
                return;
            }
        } else {
            // 非分屏模式下的传统导航
            if (region_manager_.getCurrentRegion() == EditorRegion::TAB_AREA) {
                // 标签区：向下切换到代码区
                if (region_manager_.navigateDown()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            } else if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 代码区：在底部时切换到终端，否则移动光标
                Document* doc = getCurrentDocument();
                if (doc && terminal_.isVisible()) {
                    size_t total_lines = doc->lineCount();
                    int screen_height = screen_.dimy() - 6;
                    size_t last_visible_row = view_offset_row_ + screen_height - 1;
                    if (cursor_row_ >= total_lines - 1 && cursor_row_ >= last_visible_row) {
                        if (region_manager_.navigateDown()) {
                            setStatusMessage("Region: " + region_manager_.getRegionName() +
                                             " | ↑: Return to editor");
                            return;
                        }
                    }
                }
                moveCursorDown();
            } else if (region_manager_.getCurrentRegion() == EditorRegion::TERMINAL) {
                // 终端已经在最下方，不移动
                return;
            } else if (region_manager_.getCurrentRegion() == EditorRegion::FILE_BROWSER) {
                // 文件浏览器：向下切换到代码区
                if (region_manager_.navigateDown()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            }
        }
    } else if (event == Event::ArrowLeft) {
        // 处理不同区域的向左导航
        if (split_view_manager_.hasSplits()) {
            // 分屏模式下：优先尝试分屏导航，然后是区域切换
            if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 首先尝试分屏导航
                size_t old_active_index = 0;
                const auto* old_active = split_view_manager_.getActiveRegion();
                if (old_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == old_active) {
                            old_active_index = i;
                            break;
                        }
                    }
                }

                split_view_manager_.focusLeftRegion();

                const auto* new_active = split_view_manager_.getActiveRegion();
                size_t new_active_index = 0;
                if (new_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == new_active) {
                            new_active_index = i;
                            break;
                        }
                    }
                }

                if (new_active_index != old_active_index) {
                    // 分屏导航成功，保存当前状态，更新文档，恢复新区域状态
                    saveCurrentRegionState();

                    const auto* active_region = split_view_manager_.getActiveRegion();
                    if (active_region) {
                        document_manager_.switchToDocument(active_region->document_index);
                        // 恢复新区域的状态
                        restoreRegionState(new_active_index);
                        syntax_highlighter_.setFileType(getFileType());
                    }
                    setStatusMessage("Split view: Region " + std::to_string(new_active_index + 1) +
                                     "/" + std::to_string(split_view_manager_.getRegionCount()) +
                                     " | Use ↑↓←→ to navigate between regions");
                    return;
                }

                // 分屏导航失败，尝试区域切换（当光标在行首时）
                if (cursor_col_ == 0 && file_browser_.isVisible()) {
                    if (region_manager_.navigateLeft()) {
                        setStatusMessage("Region: " + region_manager_.getRegionName() +
                                         " | →: Return to split view");
                        return;
                    }
                }
                // 都不成功，在当前区域内移动光标
                moveCursorLeft();
                return;
            }
        } else {
            // 非分屏模式下的传统导航
            if (region_manager_.getCurrentRegion() == EditorRegion::TAB_AREA) {
                // 标签区：左右键切换标签
                int old_index = region_manager_.getTabIndex();
                region_manager_.previousTab();
                int new_index = region_manager_.getTabIndex();
                if (new_index != old_index && new_index >= 0 &&
                    new_index < static_cast<int>(document_manager_.getDocumentCount())) {
                    document_manager_.switchToDocument(new_index);
                    cursor_row_ = 0;
                    cursor_col_ = 0;
                    view_offset_row_ = 0;
                    view_offset_col_ = 0;
                    syntax_highlighter_.setFileType(getFileType());
                    setStatusMessage("Region: " + region_manager_.getRegionName() +
                                     " | Tab: " + getCurrentDocument()->getFileName());
                }
                return;
            } else if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 代码区：在左边界时切换到文件浏览器，否则移动光标
                if (cursor_col_ == 0 && file_browser_.isVisible()) {
                    if (region_manager_.navigateLeft()) {
                        setStatusMessage("Region: " + region_manager_.getRegionName() +
                                         " | →: Return to editor");
                        return;
                    }
                }
                moveCursorLeft();
            } else if (region_manager_.getCurrentRegion() == EditorRegion::FILE_BROWSER) {
                // 文件浏览器已经在最左侧，不移动
                return;
            } else if (region_manager_.getCurrentRegion() == EditorRegion::TERMINAL) {
                // 终端：向左切换到文件浏览器或代码区
                if (region_manager_.navigateLeft()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            }
        }
    } else if (event == Event::ArrowRight) {
        // 处理不同区域的向右导航
        if (split_view_manager_.hasSplits()) {
            // 分屏模式下：优先尝试分屏导航
            if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 首先尝试分屏导航
                size_t old_active_index = 0;
                const auto* old_active = split_view_manager_.getActiveRegion();
                if (old_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == old_active) {
                            old_active_index = i;
                            break;
                        }
                    }
                }

                split_view_manager_.focusRightRegion();

                const auto* new_active = split_view_manager_.getActiveRegion();
                size_t new_active_index = 0;
                if (new_active) {
                    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
                        if (&split_view_manager_.getRegions()[i] == new_active) {
                            new_active_index = i;
                            break;
                        }
                    }
                }

                if (new_active_index != old_active_index) {
                    // 分屏导航成功，保存当前状态，更新文档，恢复新区域状态
                    saveCurrentRegionState();

                    const auto* active_region = split_view_manager_.getActiveRegion();
                    if (active_region) {
                        document_manager_.switchToDocument(active_region->document_index);
                        // 恢复新区域的状态
                        restoreRegionState(new_active_index);
                        syntax_highlighter_.setFileType(getFileType());
                    }
                    setStatusMessage("Split view: Region " + std::to_string(new_active_index + 1) +
                                     "/" + std::to_string(split_view_manager_.getRegionCount()) +
                                     " | Use ↑↓←→ to navigate between regions");
                    return;
                }
                // 分屏导航失败，在当前区域内移动光标
                moveCursorRight();
                return;
            } else if (region_manager_.getCurrentRegion() == EditorRegion::FILE_BROWSER) {
                // 文件浏览器：向右切换到代码区
                if (region_manager_.navigateRight()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            } else if (region_manager_.getCurrentRegion() == EditorRegion::TERMINAL) {
                // 终端：向右切换到代码区
                if (region_manager_.navigateRight()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            }
        } else {
            // 非分屏模式下的传统导航
            if (region_manager_.getCurrentRegion() == EditorRegion::TAB_AREA) {
                // 标签区：左右键切换标签
                int old_index = region_manager_.getTabIndex();
                region_manager_.nextTab();
                int new_index = region_manager_.getTabIndex();
                if (new_index != old_index && new_index >= 0 &&
                    new_index < static_cast<int>(document_manager_.getDocumentCount())) {
                    document_manager_.switchToDocument(new_index);
                    cursor_row_ = 0;
                    cursor_col_ = 0;
                    view_offset_row_ = 0;
                    view_offset_col_ = 0;
                    syntax_highlighter_.setFileType(getFileType());
                    setStatusMessage("Region: " + region_manager_.getRegionName() +
                                     " | Tab: " + getCurrentDocument()->getFileName());
                }
                return;
            } else if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
                // 代码区：向右没有其他区域，移动光标
                moveCursorRight();
            } else if (region_manager_.getCurrentRegion() == EditorRegion::FILE_BROWSER) {
                // 文件浏览器：向右切换到代码区
                if (region_manager_.navigateRight()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            } else if (region_manager_.getCurrentRegion() == EditorRegion::TERMINAL) {
                // 终端：向右切换到代码区
                if (region_manager_.navigateRight()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName());
                    return;
                }
            }
        }
    }
    // Shift+方向键进行选择（只在代码区生效，直接移动光标，不调用 moveCursor* 避免取消选中）
    else if (event == Event::ArrowUpCtrl) {
        // 只在代码区生效
        if (region_manager_.getCurrentRegion() != EditorRegion::CODE_AREA ||
            !getCurrentDocument()) {
            return;
        }
        if (!selection_active_) {
            startSelection();
        }
        // 直接移动光标，不调用 moveCursorUp（避免取消选中）
        if (cursor_row_ > 0) {
            cursor_row_--;
            adjustCursor();
            adjustViewOffset();
        }
    } else if (event == Event::ArrowDownCtrl) {
        // 只在代码区生效
        if (region_manager_.getCurrentRegion() != EditorRegion::CODE_AREA ||
            !getCurrentDocument()) {
            return;
        }
        if (!selection_active_) {
            startSelection();
        }
        // 直接移动光标，不调用 moveCursorDown（避免取消选中）
        if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
            cursor_row_++;
            adjustCursor();
            adjustViewOffset();
        }
    } else if (event == Event::ArrowLeftCtrl) {
        // 只在代码区生效
        if (region_manager_.getCurrentRegion() != EditorRegion::CODE_AREA ||
            !getCurrentDocument()) {
            return;
        }
        if (!selection_active_) {
            startSelection();
        }
        // 直接移动光标，不调用 moveCursorLeft（避免取消选中）
        if (cursor_col_ > 0) {
            cursor_col_--;
        } else if (cursor_row_ > 0) {
            cursor_row_--;
            cursor_col_ = getCurrentDocument()->getLine(cursor_row_).length();
            adjustCursor();
            adjustViewOffset();
        }
    } else if (event == Event::ArrowRightCtrl) {
        // 只在代码区生效
        if (region_manager_.getCurrentRegion() != EditorRegion::CODE_AREA ||
            !getCurrentDocument()) {
            return;
        }
        if (!selection_active_) {
            startSelection();
        }
        // 直接移动光标，不调用 moveCursorRight（避免取消选中）
        size_t line_len = getCurrentDocument()->getLine(cursor_row_).length();
        if (cursor_col_ < line_len) {
            cursor_col_++;
        } else if (cursor_row_ < getCurrentDocument()->lineCount() - 1) {
            cursor_row_++;
            cursor_col_ = 0;
            adjustCursor();
            adjustViewOffset();
        }
    }
    // 其他特殊键
    // 注意：Home/End/Tab 已通过新快捷键系统处理，这里不再重复处理
    // PageUp/PageDown 保留直接处理（基本导航，不是快捷键）
    if (event == Event::PageUp) {
        moveCursorPageUp();
    } else if (event == Event::PageDown) {
        moveCursorPageDown();
    }

    // 检查 Alt+0 和 Alt+9 组合键（也用于页面滚动）
    pnana::input::EventParser parser;
    std::string key_str = parser.eventToKey(event);
    if (key_str == "alt_0") {
        LOG("EditorInput: Alt+0 detected, calling moveCursorPageUp()");
        moveCursorPageUp();
    } else if (key_str == "alt_9") {
        LOG("EditorInput: Alt+9 detected, calling moveCursorPageDown()");
        moveCursorPageDown();
    } else if (event == Event::Backspace) {
        backspace();
    } else if (event == Event::Delete) {
        deleteChar();
    } else if (event == Event::Return) {
        insertNewline();
    }
    // 可打印字符 - 直接插入
    else if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            // 只接受可打印ASCII字符（32-126），排除控制字符
            // 注意：补全弹窗的导航键（上下键、Return、Tab、Escape）已在函数开头优先处理
            if (c >= 32 && c < 127) {
                insertChar(c);
            }
        }
    }
}

void Editor::handleSearchMode(Event event) {
    // 在搜索模式下，阻止所有字符输入到文档
    if (event == Event::Return) {
        executeSearch(true); // 按 Enter 时移动光标
        mode_ = EditorMode::NORMAL;
    } else if (event == Event::Escape) {
        mode_ = EditorMode::NORMAL;
        search_engine_.clearSearch();
        setStatusMessage("Search cancelled");
    } else if (event == Event::Backspace) {
        if (!input_buffer_.empty()) {
            input_buffer_.pop_back();
            // 实时执行搜索（如果还有输入）
            if (!input_buffer_.empty()) {
                executeSearch(false); // 实时搜索时不移动光标
            } else {
                search_engine_.clearSearch();
                setStatusMessage("Search: ");
            }
        } else {
            setStatusMessage("Search: ");
        }
    } else if (event.is_character()) {
        // 只接受可打印字符
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            if (c >= 32 && c < 127) {
                input_buffer_ += ch;
                // 实时执行搜索（不移动光标，只高亮）
                executeSearch(false);
            }
        }
    }
    // 其他事件（如方向键等）在搜索模式下被忽略，不会传递到文档编辑
}

void Editor::handleReplaceMode(Event event) {
    if (event == Event::Return) {
        executeReplace();
        mode_ = EditorMode::NORMAL;
    } else if (event == Event::Escape) {
        mode_ = EditorMode::NORMAL;
        setStatusMessage("Replace cancelled");
    } else if (event == Event::Backspace) {
        if (!input_buffer_.empty()) {
            input_buffer_.pop_back();
            setStatusMessage("Replace: " + input_buffer_);
        }
    } else if (event.is_character()) {
        input_buffer_ += event.character();
        setStatusMessage("Replace: " + input_buffer_);
    }
}

void Editor::handleFileBrowserInput(Event event) {
    LOG("Event type check - Return: " + std::string(event == Event::Return ? "yes" : "no"));
    LOG("Event type check - Escape: " + std::string(event == Event::Escape ? "yes" : "no"));
    LOG("Event input string: '" + event.input() + "'");
    LOG("Event is_character: " + std::string(event.is_character() ? "yes" : "no"));

    // 确保当前区域是文件浏览器
    if (region_manager_.getCurrentRegion() != EditorRegion::FILE_BROWSER) {
        LOG("Setting region to FILE_BROWSER");
        region_manager_.setRegion(EditorRegion::FILE_BROWSER);
    }
    LOG("Current region: " + region_manager_.getRegionName());

    // 首先检查是否是全局快捷键（Alt+A, Alt+F 等）
    // 这些快捷键应该在文件浏览器中也能工作
    using namespace pnana::input;

    // 调试信息：检查 Ctrl+P 事件
    if (event == ftxui::Event::CtrlP) {
        LOG("[DEBUG COPY] Ctrl+P event detected at start of handleInput!");
    }

    KeyAction action = key_binding_manager_.getAction(event);

    // 调试信息：检查 Ctrl+P 事件解析结果
    if (event == ftxui::Event::CtrlP) {
        LOG("[DEBUG COPY] After getAction, action: " + std::to_string(static_cast<int>(action)) +
            " (COPY=" + std::to_string(static_cast<int>(KeyAction::COPY)) +
            ", UNKNOWN=" + std::to_string(static_cast<int>(KeyAction::UNKNOWN)) + ")");
    }
    LOG("Action resolved: " + std::to_string(static_cast<int>(action)));
    if (action == KeyAction::SAVE_AS || action == KeyAction::CREATE_FOLDER) {
        LOG("Global shortcut detected, executing...");
        if (action_executor_.execute(action)) {
            LOG("Global shortcut executed, returning");
            return;
        }
    }

    // Ctrl+L: 在文件浏览器中选择文件后，触发分屏或关闭分屏
    // 直接检查事件，因为 Ctrl+L 不在全局快捷键中
    if (event == Event::CtrlL) {
        // 如果已经有分屏，直接显示关闭分屏对话框
        if (split_view_manager_.hasSplits()) {
            showSplitDialog();
            return;
        }

        // 没有分屏，检查是否有选中的文件
        if (file_browser_.hasSelection()) {
            std::string selected_file = file_browser_.getSelectedFile();
            if (!selected_file.empty()) {
                // 有选中的文件，打开分屏对话框
                // 先打开文件，然后分屏
                openFile(selected_file);
                showSplitDialog();
                return;
            } else {
                // 选中的是目录，提示用户选择文件
                setStatusMessage("Please select a file first, then press Ctrl+L to split");
                return;
            }
        } else {
            setStatusMessage("Please select a file first, then press Ctrl+L to split");
            return;
        }
    }

    // F5: SSH文件传输（仅当有SSH连接时）
    if (event == Event::F5) {
        if (!current_ssh_config_.host.empty()) {
            showSSHTransferDialog();
            return;
        }
    }

    // Ctrl+F: 搜索
    if (isCtrlKey(event, 'f')) {
        startSearch();
        return;
    }

    // Ctrl+R: 替换
    if (isCtrlKey(event, 'r')) {
        startReplace();
        return;
    }

    // 处理方向键区域切换
    if (event == Event::ArrowUp) {
        // 文件浏览器顶部时，向上切换到标签区
        if (file_browser_.getSelectedIndex() == 0 && document_manager_.getDocumentCount() > 1) {
            if (region_manager_.navigateUp()) {
                setStatusMessage("Region: " + region_manager_.getRegionName() +
                                 " | ↓: Return to file browser");
                return;
            }
        }
        file_browser_.selectPrevious();
        return;
    } else if (event == Event::ArrowDown) {
        // 文件浏览器底部时，向下切换到代码区
        if (file_browser_.getSelectedIndex() >= file_browser_.getItemCount() - 1) {
            if (region_manager_.navigateDown()) {
                setStatusMessage("Region: " + region_manager_.getRegionName() +
                                 " | ↑: Return to file browser");
                return;
            }
        }
        file_browser_.selectNext();
        return;
    } else if (event == Event::ArrowLeft) {
        // 文件浏览器已经在最左侧，不移动
        return;
    } else if (event == Event::ArrowRight) {
        // 文件浏览器：向右切换到代码区
        if (region_manager_.navigateRight()) {
            setStatusMessage("Region: " + region_manager_.getRegionName() +
                             " | ←: Return to file browser");
            return;
        }
    }

    // Check for Alt+D (increase width) and Alt+S (decrease width)
    // 使用 EventParser 来检测 Alt 组合键
    using namespace pnana::input;
    EventParser parser;
    std::string key_string = parser.eventToKey(event);

    // Alt+D : Increase width
    if (key_string == "alt_d") {
        if (file_browser_width_ < 80) { // Max 80 columns
            file_browser_width_ += 5;
            setStatusMessage(std::string(pnana::ui::icons::ARROW_RIGHT) +
                             " Browser width: " + std::to_string(file_browser_width_) +
                             " columns | Alt+D increase, Alt+S decrease");
            return;
        } else {
            setStatusMessage("Browser width already at maximum (80 columns)");
            return;
        }
    }
    // Alt+S : Decrease width
    else if (key_string == "alt_s") {
        if (file_browser_width_ > 20) { // Min 20 columns
            file_browser_width_ -= 5;
            setStatusMessage(std::string(pnana::ui::icons::ARROW_LEFT) +
                             " Browser width: " + std::to_string(file_browser_width_) +
                             " columns | Alt+D increase, Alt+S decrease");
            return;
        } else {
            setStatusMessage("Browser width already at minimum (20 columns)");
            return;
        }
    }

    if (event == Event::CtrlO) {
        // Ctrl+O closes file browser
        file_browser_.setVisible(false);
        region_manager_.setRegion(EditorRegion::CODE_AREA);
        setStatusMessage("File browser closed | Region: " + region_manager_.getRegionName());
    } else if (event == Event::Escape) {
        // Escape also closes
        file_browser_.setVisible(false);
        region_manager_.setRegion(EditorRegion::CODE_AREA);
        setStatusMessage("File browser closed | Region: " + region_manager_.getRegionName());
    } else if (event == Event::Return) {
        // Enter: toggle expand/collapse for directories, or open file
        LOG("=== File Browser: Return key pressed ===");
        LOG("Current directory: " + file_browser_.getCurrentDirectory());
        LOG("Calling file_browser_.toggleSelected()...");
        bool is_file = file_browser_.toggleSelected();
        LOG("toggleSelected() returned: " +
            std::string(is_file ? "true (file)" : "false (directory)"));

        if (is_file) {
            LOG("Getting selected file...");
            std::string selected = file_browser_.getSelectedFile();
            LOG("Selected file path: " + selected);
            LOG("Selected file length: " + std::to_string(selected.length()));
            LOG("Selected file empty check: " + std::string(selected.empty() ? "true" : "false"));

            if (!selected.empty()) {
                // It's a file, open it but keep browser open
                LOG("--- Starting file open process ---");
                LOG("Calling openFile() with path: " + selected);

                try {
                    bool open_result = openFile(selected);
                    LOG("openFile() returned: " + std::string(open_result ? "true" : "false"));

                    if (open_result) {
                        Document* doc = getCurrentDocument();
                        if (doc) {
                            LOG("File opened successfully, document pointer: " +
                                std::to_string(reinterpret_cast<uintptr_t>(doc)));
                            LOG("Document file name: " + doc->getFileName());
                            LOG("Document file path: " + doc->getFilePath());
                            LOG("Document line count: " + std::to_string(doc->lineCount()));
                            setStatusMessage(std::string(pnana::ui::icons::OPEN) +
                                             " Opened: " + doc->getFileName() +
                                             " | Press Ctrl+O to close browser | Region: " +
                                             region_manager_.getRegionName());
                        } else {
                            LOG_ERROR("openFile() returned true but getCurrentDocument() is null!");
                            setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                                             " Failed to open file: Document is null");
                        }
                    } else {
                        LOG_ERROR("openFile() returned false - file open failed");
                        setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                                         " Failed to open file");
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in openFile(): " + std::string(e.what()));
                    setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                                     " Exception: " + std::string(e.what()));
                } catch (...) {
                    LOG_ERROR("Unknown exception in openFile()");
                    setStatusMessage(std::string(pnana::ui::icons::ERROR) + " Unknown exception");
                }

                LOG("--- File open process completed ---");

                // 文件打开后，关闭文件浏览器并切换到代码区域
                file_browser_.setVisible(false);
                region_manager_.setRegion(EditorRegion::CODE_AREA);
                LOG("File browser closed and switched to CODE_AREA region after opening file");
            } else {
                LOG_WARNING("Selected file path is empty!");
            }
        } else {
            // It's a directory, toggled expand/collapse
            LOG("Directory toggled, current directory: " + file_browser_.getCurrentDirectory());
            setStatusMessage(std::string(pnana::ui::icons::FOLDER) + " " +
                             file_browser_.getCurrentDirectory() +
                             " | Region: " + region_manager_.getRegionName());
        }
        LOG("=== File Browser: Return key handling completed ===");
        LOG("File browser visible: " + std::string(file_browser_.isVisible() ? "true" : "false"));
        LOG("Current region: " + region_manager_.getRegionName());
        LOG("Document count: " + std::to_string(document_manager_.getDocumentCount()));
    } else if (event == Event::Backspace) {
        // Go to parent directory
        if (file_browser_.goUp()) {
            setStatusMessage(std::string(pnana::ui::icons::FOLDER_UP) + " " +
                             file_browser_.getCurrentDirectory() +
                             " | Region: " + region_manager_.getRegionName());
        }
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (ch == "h") {
            // Toggle show hidden files
            file_browser_.setShowHidden(!file_browser_.getShowHidden());
            setStatusMessage(
                file_browser_.getShowHidden()
                    ? "Showing hidden files | Region: " + region_manager_.getRegionName()
                    : "Hiding hidden files | Region: " + region_manager_.getRegionName());
        } else if (ch == "r") {
            // Refresh
            file_browser_.refresh();
            setStatusMessage(std::string(pnana::ui::icons::REFRESH) +
                             " File list refreshed | Region: " + region_manager_.getRegionName());
        }
    }

    // F2: 重命名
    if (event == Event::F2) {
        handleRenameFile();
        return;
    }

    // Delete: 删除文件/文件夹
    if (event == Event::Delete) {
        LOG("Delete key in file browser - deleting file");
        handleDeleteFile();
        return;
    }
}

// 搜索和替换
void Editor::startSearch() {
    if (!getCurrentDocument()) {
        setStatusMessage("No document to search in");
        return;
    }

    search_dialog_.show(
        [this](const std::string& pattern, const features::SearchOptions& options) {
            performSearch(pattern, options);
        },
        [this](const std::string& replacement) {
            performReplace(replacement);
        },
        [this](const std::string& replacement) {
            performReplaceAll(replacement);
        },
        [this]() {
            setStatusMessage("Search cancelled");
        });
}

void Editor::performSearch(const std::string& pattern, const features::SearchOptions& options) {
    if (!getCurrentDocument()) {
        setStatusMessage("No document to search in");
        return;
    }

    // 执行搜索
    const auto& lines = getCurrentDocument()->getLines();
    search_engine_.search(pattern, lines, options);

    current_search_options_ = options;

    if (search_engine_.hasMatches()) {
        search_highlight_active_ = true;
        search_dialog_.updateResults(search_engine_.getCurrentMatchIndex(),
                                     search_engine_.getTotalMatches());

        // 跳转到第一个匹配
        const auto* match = search_engine_.getCurrentMatch();
        if (match) {
            cursor_row_ = match->line;
            cursor_col_ = match->column;
            adjustViewOffset();
        }

        setStatusMessage("Found " + std::to_string(search_engine_.getTotalMatches()) +
                         " matches for: " + pattern);
    } else {
        search_highlight_active_ = false;
        setStatusMessage("No matches found for: " + pattern);
    }
}

void Editor::clearSearchHighlight() {
    if (search_highlight_active_) {
        search_highlight_active_ = false;
        search_engine_.clearSearch();
    }
}

void Editor::performReplace(const std::string& replacement) {
    if (!getCurrentDocument() || !search_highlight_active_ || !search_engine_.hasMatches()) {
        setStatusMessage("No active search to replace");
        return;
    }

    Document* doc = getCurrentDocument();
    const auto* match = search_engine_.getCurrentMatch();
    if (!match) {
        setStatusMessage("No current match to replace");
        return;
    }

    // 获取当前行的内容
    std::string line = doc->getLine(match->line);
    if (match->column + match->length > line.length()) {
        setStatusMessage("Invalid match position");
        return;
    }

    // 删除匹配的文本
    doc->deleteRange(match->line, match->column, match->line, match->column + match->length);

    // 插入替换文本
    if (!replacement.empty()) {
        doc->insertText(match->line, match->column, replacement);
    }

    // 重新搜索以更新匹配
    const auto& pattern = search_engine_.getPattern();
    const auto& lines = doc->getLines();
    search_engine_.search(pattern, lines, current_search_options_);

    // 更新搜索结果显示
    if (search_engine_.hasMatches()) {
        search_dialog_.updateResults(search_engine_.getCurrentMatchIndex(),
                                     search_engine_.getTotalMatches());
        setStatusMessage("Replaced 1 occurrence. " +
                         std::to_string(search_engine_.getTotalMatches()) + " matches remaining");
    } else {
        search_highlight_active_ = false;
        search_dialog_.updateResults(0, 0);
        setStatusMessage("Replaced 1 occurrence. No more matches");
    }
}

void Editor::performReplaceAll(const std::string& replacement) {
    if (!getCurrentDocument() || !search_highlight_active_ || !search_engine_.hasMatches()) {
        setStatusMessage("No active search to replace");
        return;
    }

    Document* doc = getCurrentDocument();
    const auto& matches = search_engine_.getAllMatches();
    size_t replaced_count = 0;

    // 从后往前替换，避免位置偏移
    for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
        const auto& match = *it;

        if (match.line >= doc->lineCount()) {
            continue;
        }

        std::string line = doc->getLine(match.line);
        if (match.column + match.length > line.length()) {
            continue;
        }

        // 删除匹配的文本
        doc->deleteRange(match.line, match.column, match.line, match.column + match.length);

        // 插入替换文本
        if (!replacement.empty()) {
            doc->insertText(match.line, match.column, replacement);
        }

        replaced_count++;
    }

    if (replaced_count > 0) {
        // 清除搜索状态
        search_highlight_active_ = false;
        search_engine_.clearSearch();
        search_dialog_.updateResults(0, 0);

        setStatusMessage("Replaced " + std::to_string(replaced_count) + " occurrences");
    } else {
        setStatusMessage("No replacements made");
    }
}

void Editor::startReplace() {
    startSearch(); // 替换实际上使用相同的对话框
}

void Editor::searchNext() {
    if (search_engine_.findNext()) {
        const auto* match = search_engine_.getCurrentMatch();
        if (match) {
            cursor_row_ = match->line;
            cursor_col_ = match->column;
            adjustViewOffset();

            std::ostringstream oss;
            oss << "Match " << (search_engine_.getCurrentMatchIndex() + 1) << " of "
                << search_engine_.getTotalMatches();
            setStatusMessage(oss.str());
        }
    }
}

void Editor::searchPrevious() {
    if (search_engine_.findPrevious()) {
        const auto* match = search_engine_.getCurrentMatch();
        if (match) {
            cursor_row_ = match->line;
            cursor_col_ = match->column;
            adjustViewOffset();

            std::ostringstream oss;
            oss << "Match " << (search_engine_.getCurrentMatchIndex() + 1) << " of "
                << search_engine_.getTotalMatches();
            setStatusMessage(oss.str());
        }
    }
}

void Editor::replaceCurrentMatch() {
    // TODO: 实现替换功能
    setStatusMessage("Replace not yet implemented");
}

void Editor::replaceAll() {
    // TODO: 实现全部替换
    setStatusMessage("Replace All not yet implemented");
}

void Editor::executeSearch(bool move_cursor) {
    if (input_buffer_.empty()) {
        setStatusMessage("Empty search pattern");
        return;
    }

    features::SearchOptions options;
    search_engine_.search(input_buffer_, getCurrentDocument()->getLines(), options);

    if (search_engine_.hasMatches()) {
        const auto* match = search_engine_.getCurrentMatch();
        if (match) {
            if (move_cursor) {
                // 只在按 Enter 时移动光标
                cursor_row_ = match->line;
                cursor_col_ = match->column;
                adjustViewOffset();
            }

            std::ostringstream oss;
            oss << "Found " << search_engine_.getTotalMatches() << " matches";
            setStatusMessage(oss.str());
        }
    } else {
        setStatusMessage("Pattern not found: " + input_buffer_);
    }
}

void Editor::executeReplace() {
    setStatusMessage("Replace feature coming soon!");
}

} // namespace core
} // namespace pnana