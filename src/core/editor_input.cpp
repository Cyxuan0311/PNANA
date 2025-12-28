// 输入处理相关实现
#include "core/editor.h"
#include "ui/icons.h"
#include "input/key_action.h"
#include <ftxui/component/event.hpp>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

using namespace ftxui;

namespace pnana {
namespace core {

// 事件处理
void Editor::handleInput(Event event) {
    // 更新区域可用性
    region_manager_.setTabAreaEnabled(document_manager_.getDocumentCount() > 1);
    region_manager_.setFileBrowserEnabled(file_browser_.isVisible());
    region_manager_.setTerminalEnabled(terminal_.isVisible());
    region_manager_.setHelpWindowEnabled(show_help_);
    
    // 首先检查全局快捷键（在任何模式下都有效，包括对话框打开时）
    // 使用新的输入处理系统
    using namespace pnana::input;
    KeyAction action = key_binding_manager_.getAction(event);
    
    // Alt+A (另存为)、Alt+F (创建文件夹) 和 Alt+M (文件选择器) 应该能够在任何情况下工作
    // 包括在对话框中或文件浏览器打开时
    if (action == KeyAction::SAVE_AS || action == KeyAction::CREATE_FOLDER || action == KeyAction::FILE_PICKER) {
        if (action_executor_.execute(action)) {
            return;
        }
    }
    
    // 如果命令面板打开，优先处理
    if (command_palette_.isOpen()) {
        handleCommandPaletteInput(event);
        return;
    }
    
    // 如果终端打开，优先处理
    if (terminal_.isVisible()) {
        handleTerminalInput(event);
        return;
    }
    
    // 如果当前在对话框中，其他快捷键不处理（让对话框处理输入）
    // 但文件选择器可以在任何情况下打开
    bool in_dialog = show_save_as_ || show_create_folder_ || show_theme_menu_ || show_help_ || split_dialog_.isVisible();
    
    // 如果当前在搜索模式，优先处理搜索输入（除了 Escape 和 Return）
    bool in_search_mode = (mode_ == EditorMode::SEARCH);
    bool should_skip_shortcuts = in_search_mode && (event != Event::Escape && event != Event::Return);
    
    if (in_dialog) {
        // 对话框内的输入处理在下面
        // 但文件选择器仍然可以打开
        if (action == KeyAction::FILE_PICKER) {
            if (action_executor_.execute(action)) {
                return;
            }
        }
    } else if (action != KeyAction::UNKNOWN && action != KeyAction::SPLIT_VIEW && !should_skip_shortcuts) {
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
            save_as_input_ = "";
            setStatusMessage("Save as cancelled");
        } else if (event == Event::Return) {
            if (!save_as_input_.empty()) {
                // 保存文件
                if (saveFileAs(save_as_input_)) {
                    show_save_as_ = false;
                    save_as_input_ = "";
                }
            }
        } else if (event == Event::Backspace) {
            if (!save_as_input_.empty()) {
                save_as_input_.pop_back();
            }
        } else if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                // 接受所有可打印字符（文件路径可以包含各种字符）
                if (c >= 32 && c < 127) {
                    save_as_input_ += c;
                }
            }
        }
        return;
    }
    
    // 如果创建文件夹对话框打开，优先处理
    if (show_create_folder_) {
        if (event == Event::Escape) {
            show_create_folder_ = false;
            folder_name_input_ = "";
            setStatusMessage("Folder creation cancelled");
        } else if (event == Event::Return) {
            if (!folder_name_input_.empty()) {
                // 创建文件夹 - 使用C++ filesystem
                try {
                    fs::path folder_path = fs::path(file_browser_.getCurrentDirectory()) / folder_name_input_;
                    if (fs::create_directory(folder_path)) {
                        show_create_folder_ = false;
                        std::string created_name = folder_name_input_;
                        folder_name_input_ = "";
                        file_browser_.refresh();
                        setStatusMessage(std::string(ui::icons::FOLDER) + " Folder created: " + created_name);
                    } else {
                        setStatusMessage(std::string(ui::icons::ERROR) + " Failed to create folder (may already exist): " + folder_name_input_);
                    }
                } catch (const std::exception& e) {
                    setStatusMessage(std::string(ui::icons::ERROR) + " Error: " + std::string(e.what()));
                }
            }
        } else if (event == Event::Backspace) {
            if (!folder_name_input_.empty()) {
                folder_name_input_.pop_back();
            }
        } else if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                // 只接受可打印ASCII字符，排除文件名禁用字符
                if ((c >= 32 && c < 127) && c != '/' && c != '\\' && c != ':' && c != '*' && 
                    c != '?' && c != '"' && c != '<' && c != '>' && c != '|') {
                    folder_name_input_ += c;
                }
            }
        }
        return;
    }
    
    // 如果帮助窗口打开，优先处理
    if (show_help_) {
        region_manager_.setRegion(EditorRegion::HELP_WINDOW);
        if (event == Event::Escape || event == Event::F1) {
            show_help_ = false;
            region_manager_.setRegion(EditorRegion::CODE_AREA);
            setStatusMessage("Help closed | 区域: " + region_manager_.getRegionName());
        }
        return;
    }
    
    // 如果主题菜单打开，优先处理
    if (show_theme_menu_) {
        if (event == Event::Escape) {
            show_theme_menu_ = false;
            setStatusMessage("Theme selection cancelled | 区域: " + region_manager_.getRegionName());
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
    
    // 优先处理对话框输入
    if (dialog_.isVisible()) {
        if (dialog_.handleInput(event)) {
            return;
        }
    }
    
    // 优先处理文件选择器输入
    if (file_picker_.isVisible()) {
        if (file_picker_.handleInput(event)) {
            return;
        }
    }
    
    // 优先处理分屏对话框输入
    if (split_dialog_.isVisible()) {
        if (split_dialog_.handleInput(event)) {
            return;
        }
    }
    
    // 处理鼠标事件（用于拖动分屏线）
    if (event.is_mouse() && split_view_manager_.hasSplits()) {
        int screen_width = screen_.dimx();
        int screen_height = screen_.dimy();
        
        // 计算编辑器区域的偏移（考虑文件浏览器、标签栏等）
        int editor_x_offset = 0;
        int editor_y_offset = 1;  // 标签栏
        
        if (file_browser_.isVisible()) {
            editor_x_offset = file_browser_width_ + 1;  // 文件浏览器宽度 + 分隔符
        }
        
        // 计算编辑器区域尺寸（偏移在 handleMouseEvent 中处理）
        int editor_width = screen_width - editor_x_offset;
        int editor_height = screen_height - 6;  // 减去标签栏、状态栏等
        
        if (split_view_manager_.handleMouseEvent(event, editor_width, editor_height, editor_x_offset, editor_y_offset)) {
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
        if (action == KeyAction::SAVE_AS || action == KeyAction::CREATE_FOLDER || action == KeyAction::FILE_PICKER) {
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
    if (action != KeyAction::UNKNOWN && !should_skip_shortcuts) {
        if (action_executor_.execute(action)) {
            return;
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
        case EditorMode::GOTO_LINE:
            handleGotoLineMode(event);
            break;
    }
    
    adjustViewOffset();
    
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
            setStatusMessage(std::string(ui::icons::NEW) + " New document created | Region: " + region_manager_.getRegionName());
            return;
        }
    }
    
    // If no document, ignore other inputs
    if (getCurrentDocument() == nullptr) {
        return;
    }
    
    // Normal mode = editing mode, can input directly
    // Arrow keys - 智能区域导航系统
    if (event == Event::ArrowUp) {
        // 处理不同区域的向上导航
        if (region_manager_.getCurrentRegion() == EditorRegion::TAB_AREA) {
            // 标签区已经在最上方，不移动
            return;
        } else if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
            // 代码区：在顶部时切换到标签区，否则移动光标
            if (cursor_row_ == 0 && document_manager_.getDocumentCount() > 1) {
                if (region_manager_.navigateUp()) {
                    region_manager_.setTabIndex(document_manager_.getCurrentIndex());
                    setStatusMessage("Region: " + region_manager_.getRegionName() + " | ←→: Switch tabs, ↓: Return");
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
    } else if (event == Event::ArrowDown) {
        // 处理不同区域的向下导航
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
                        setStatusMessage("Region: " + region_manager_.getRegionName() + " | ↑: Return to editor");
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
    } else if (event == Event::ArrowLeft) {
        // 处理不同区域的向左导航
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
                setStatusMessage("Region: " + region_manager_.getRegionName() + " | Tab: " + getCurrentDocument()->getFileName());
            }
            return;
        } else if (region_manager_.getCurrentRegion() == EditorRegion::CODE_AREA) {
            // 代码区：在左边界时切换到文件浏览器，否则移动光标
            if (cursor_col_ == 0 && file_browser_.isVisible()) {
                if (region_manager_.navigateLeft()) {
                    setStatusMessage("Region: " + region_manager_.getRegionName() + " | →: Return to editor");
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
    } else if (event == Event::ArrowRight) {
        // 处理不同区域的向右导航
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
                setStatusMessage("Region: " + region_manager_.getRegionName() + " | Tab: " + getCurrentDocument()->getFileName());
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
    // Shift+方向键进行选择
    else if (event == Event::ArrowUpCtrl) {
        if (!selection_active_) {
            startSelection();
        }
        moveCursorUp();
    } else if (event == Event::ArrowDownCtrl) {
        if (!selection_active_) {
            startSelection();
        }
        moveCursorDown();
    } else if (event == Event::ArrowLeftCtrl) {
        if (!selection_active_) {
            startSelection();
        }
        moveCursorLeft();
    } else if (event == Event::ArrowRightCtrl) {
        if (!selection_active_) {
            startSelection();
        }
        moveCursorRight();
    }
    // 其他特殊键
    // 注意：Home/End/Tab 已通过新快捷键系统处理，这里不再重复处理
    // PageUp/PageDown 保留直接处理（基本导航，不是快捷键）
    if (event == Event::PageUp) {
        moveCursorPageUp();
    } else if (event == Event::PageDown) {
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
            if (c >= 32 && c < 127) {
                insertChar(c);
            }
        }
    }
}

void Editor::handleSearchMode(Event event) {
    // 在搜索模式下，阻止所有字符输入到文档
    if (event == Event::Return) {
        executeSearch(true);  // 按 Enter 时移动光标
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
                executeSearch(false);  // 实时搜索时不移动光标
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

void Editor::handleGotoLineMode(Event event) {
    if (event == Event::Return) {
        executeGotoLine();
        mode_ = EditorMode::NORMAL;
    } else if (event == Event::Escape) {
        mode_ = EditorMode::NORMAL;
        setStatusMessage("Goto cancelled");
    } else if (event == Event::Backspace) {
        if (!input_buffer_.empty()) {
            input_buffer_.pop_back();
            setStatusMessage("Go to line: " + input_buffer_);
        }
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1 && std::isdigit(ch[0])) {
            input_buffer_ += ch;
            setStatusMessage("Go to line: " + input_buffer_);
        }
    }
}

void Editor::handleFileBrowserInput(Event event) {
    // 确保当前区域是文件浏览器
    if (region_manager_.getCurrentRegion() != EditorRegion::FILE_BROWSER) {
        region_manager_.setRegion(EditorRegion::FILE_BROWSER);
    }
    
    // 首先检查是否是全局快捷键（Alt+A, Alt+F 等）
    // 这些快捷键应该在文件浏览器中也能工作
    using namespace pnana::input;
    KeyAction action = key_binding_manager_.getAction(event);
    if (action == KeyAction::SAVE_AS || action == KeyAction::CREATE_FOLDER) {
        if (action_executor_.execute(action)) {
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
    
    // 处理方向键区域切换
    if (event == Event::ArrowUp) {
        // 文件浏览器顶部时，向上切换到标签区
        if (file_browser_.getSelectedIndex() == 0 && document_manager_.getDocumentCount() > 1) {
            if (region_manager_.navigateUp()) {
                setStatusMessage("Region: " + region_manager_.getRegionName() + " | ↓: Return to file browser");
                return;
            }
        }
        file_browser_.selectPrevious();
        return;
    } else if (event == Event::ArrowDown) {
        // 文件浏览器底部时，向下切换到代码区
        if (file_browser_.getSelectedIndex() >= file_browser_.getItemCount() - 1) {
            if (region_manager_.navigateDown()) {
                setStatusMessage("Region: " + region_manager_.getRegionName() + " | ↑: Return to file browser");
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
            setStatusMessage("Region: " + region_manager_.getRegionName() + " | ←: Return to file browser");
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
            if (file_browser_width_ < 80) {  // Max 80 columns
                file_browser_width_ += 5;
                setStatusMessage(std::string(ui::icons::ARROW_RIGHT) + " Browser width: " + 
                               std::to_string(file_browser_width_) + " columns | Alt+D increase, Alt+S decrease");
                return;
        } else {
            setStatusMessage("Browser width already at maximum (80 columns)");
            return;
        }
    }
    // Alt+S : Decrease width
    else if (key_string == "alt_s") {
            if (file_browser_width_ > 20) {  // Min 20 columns
                file_browser_width_ -= 5;
                setStatusMessage(std::string(ui::icons::ARROW_LEFT) + " Browser width: " + 
                               std::to_string(file_browser_width_) + " columns | Alt+D increase, Alt+S decrease");
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
        if (file_browser_.toggleSelected()) {
            std::string selected = file_browser_.getSelectedFile();
            if (!selected.empty()) {
                // It's a file, open it but keep browser open
                openFile(selected);
                setStatusMessage(std::string(ui::icons::OPEN) + " Opened: " + 
                               getCurrentDocument()->getFileName() + " | Press Ctrl+O to close browser | Region: " + 
                               region_manager_.getRegionName());
            }
            } else {
            // It's a directory, toggled expand/collapse
                setStatusMessage(std::string(ui::icons::FOLDER) + " " + 
                               file_browser_.getCurrentDirectory() + " | Region: " + 
                               region_manager_.getRegionName());
        }
    } else if (event == Event::Backspace) {
        // Go to parent directory
        if (file_browser_.goUp()) {
            setStatusMessage(std::string(ui::icons::FOLDER_UP) + " " + 
                           file_browser_.getCurrentDirectory() + " | Region: " + 
                           region_manager_.getRegionName());
        }
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (ch == "h") {
            // Toggle show hidden files
            file_browser_.setShowHidden(!file_browser_.getShowHidden());
            setStatusMessage(file_browser_.getShowHidden() ? 
                           "Showing hidden files | Region: " + region_manager_.getRegionName() : 
                           "Hiding hidden files | Region: " + region_manager_.getRegionName());
        } else if (ch == "r") {
            // Refresh
            file_browser_.refresh();
            setStatusMessage(std::string(ui::icons::REFRESH) + " File list refreshed | Region: " + 
                           region_manager_.getRegionName());
        }
    }
    
    // F2: 重命名
    if (event == Event::F2) {
        handleRenameFile();
        return;
    }
    
    // Delete: 删除文件/文件夹
    if (event == Event::Delete) {
        handleDeleteFile();
        return;
    }
}

// 搜索和替换
void Editor::startSearch() {
    mode_ = EditorMode::SEARCH;
    input_buffer_ = "";
    setStatusMessage("Search: ");
}

void Editor::startReplace() {
    mode_ = EditorMode::REPLACE;
    input_buffer_ = "";
    setStatusMessage("Replace: ");
}

void Editor::searchNext() {
    if (search_engine_.findNext()) {
        const auto* match = search_engine_.getCurrentMatch();
        if (match) {
            cursor_row_ = match->line;
            cursor_col_ = match->column;
            adjustViewOffset();
            
            std::ostringstream oss;
            oss << "Match " << (search_engine_.getCurrentMatchIndex() + 1)
                << " of " << search_engine_.getTotalMatches();
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
            oss << "Match " << (search_engine_.getCurrentMatchIndex() + 1)
                << " of " << search_engine_.getTotalMatches();
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

void Editor::executeGotoLine() {
    if (input_buffer_.empty()) {
        return;
    }
    
    try {
        size_t line = std::stoull(input_buffer_);
        gotoLine(line);
    } catch (const std::exception&) {
        setStatusMessage("Invalid line number");
    }
}

} // namespace core
} // namespace pnana

