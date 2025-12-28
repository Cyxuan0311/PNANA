// 编辑器核心类实现
#include "core/editor.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

using namespace ftxui;

namespace pnana {
namespace core {

// 构造函数
Editor::Editor()
    : document_manager_(),
      key_binding_manager_(),
      action_executor_(this),
      theme_(),
      statusbar_(theme_),
      helpbar_(theme_),
      tabbar_(theme_),
      help_(theme_),
      dialog_(theme_),
      file_picker_(theme_),
      split_dialog_(theme_),
      search_engine_(),
      file_browser_(theme_),
      syntax_highlighter_(theme_),
      terminal_(theme_),
      split_view_manager_(),
      mode_(EditorMode::NORMAL),
      cursor_row_(0),
      cursor_col_(0),
      view_offset_row_(0),
      view_offset_col_(0),
      show_theme_menu_(false),
      selected_theme_index_(0),
      show_help_(false),
      show_create_folder_(false),
      folder_name_input_(""),
      show_save_as_(false),
      save_as_input_(""),
      selection_active_(false),
      selection_start_row_(0),
      selection_start_col_(0),
      show_line_numbers_(true),
      relative_line_numbers_(false),
      syntax_highlighting_(true),
      zoom_level_(0),
      file_browser_width_(35),  // 默认宽度35列
      input_buffer_(""),
      status_message_("pnana - Modern Terminal Editor | Ctrl+Q Quit | Ctrl+T Themes | Ctrl+O Files | F1 Help"),
      should_quit_(false),
      screen_(ScreenInteractive::Fullscreen()) {
    theme_.setTheme("monokai");
    
    // 初始化可用主题列表
    available_themes_ = {
        "monokai",
        "dracula",
        "solarized-dark",
        "solarized-light",
        "onedark",
        "nord",
        "gruvbox",
        "tokyo-night",
        "catppuccin",
        "material",
        "ayu",
        "github"
    };
    
    // 初始化文件浏览器到当前目录
    file_browser_.openDirectory(".");
    
    // 初始化命令面板
    initializeCommandPalette();
}

Document* Editor::getCurrentDocument() {
    return document_manager_.getCurrentDocument();
}

const Document* Editor::getCurrentDocument() const {
    return document_manager_.getCurrentDocument();
}

Editor::Editor(const std::string& filepath) : Editor() {
    openFile(filepath);
}

Editor::Editor(const std::vector<std::string>& filepaths) : Editor() {
    if (!filepaths.empty()) {
        openFile(filepaths[0]);
    }
}

void Editor::run() {
    main_component_ = CatchEvent(Renderer([this] {
        return renderUI();
    }), [this](Event event) {
        handleInput(event);
        return true;
    });
    
    screen_.Loop(main_component_);
}

// 视图操作
void Editor::toggleLineNumbers() {
    show_line_numbers_ = !show_line_numbers_;
    setStatusMessage(show_line_numbers_ ? "Line numbers shown" : "Line numbers hidden");
}

void Editor::toggleRelativeNumbers() {
    relative_line_numbers_ = !relative_line_numbers_;
    setStatusMessage(relative_line_numbers_ ? "Relative line numbers" : "Absolute line numbers");
}

void Editor::zoomIn() {
    zoom_level_++;
    setStatusMessage("Zoom: +" + std::to_string(zoom_level_));
}

void Editor::zoomOut() {
    zoom_level_--;
    setStatusMessage("Zoom: " + std::to_string(zoom_level_));
}

void Editor::zoomReset() {
    zoom_level_ = 0;
    setStatusMessage("Zoom reset");
}

void Editor::setTheme(const std::string& theme_name) {
    theme_.setTheme(theme_name);
    setStatusMessage("Theme: " + theme_name);
}

// 主题菜单
void Editor::toggleThemeMenu() {
    show_theme_menu_ = !show_theme_menu_;
    
    if (show_theme_menu_) {
        // 找到当前主题的索引
        std::string current = theme_.getCurrentThemeName();
        for (size_t i = 0; i < available_themes_.size(); ++i) {
            if (available_themes_[i] == current) {
                selected_theme_index_ = i;
                break;
            }
        }
        setStatusMessage("Select theme with ↑↓ and press Enter");
    }
}

void Editor::selectNextTheme() {
    if (available_themes_.empty()) return;
    selected_theme_index_ = (selected_theme_index_ + 1) % available_themes_.size();
}

void Editor::selectPreviousTheme() {
    if (available_themes_.empty()) return;
    if (selected_theme_index_ == 0) {
        selected_theme_index_ = available_themes_.size() - 1;
    } else {
        selected_theme_index_--;
    }
}

void Editor::applySelectedTheme() {
    if (selected_theme_index_ < available_themes_.size()) {
        std::string theme_name = available_themes_[selected_theme_index_];
        theme_.setTheme(theme_name);
        setStatusMessage("✓ Applied theme: " + theme_name);
    }
}

// 文件浏览器
void Editor::toggleFileBrowser() {
    file_browser_.toggle();
    if (file_browser_.isVisible()) {
        // 切换到文件浏览器区域
        region_manager_.setRegion(EditorRegion::FILE_BROWSER);
        setStatusMessage("File Browser opened | Region: " + region_manager_.getRegionName() + " | ↑↓: Navigate, →: Editor, Enter: Open");
    } else {
        // 文件浏览器关闭时，如果当前在文件浏览器区域，切换回代码区
        if (region_manager_.getCurrentRegion() == EditorRegion::FILE_BROWSER) {
            region_manager_.setRegion(EditorRegion::CODE_AREA);
        }
        setStatusMessage("File Browser closed | Region: " + region_manager_.getRegionName());
    }
}

// 帮助系统
void Editor::toggleHelp() {
    show_help_ = !show_help_;
    if (show_help_) {
        setStatusMessage(std::string(ui::icons::HELP) + " Press Esc or F1 to close help");
    } else {
        setStatusMessage("Help closed");
    }
}

// 终端
void Editor::toggleTerminal() {
    terminal_.toggle();
    if (terminal_.isVisible()) {
        // 切换到终端区域
        region_manager_.setRegion(EditorRegion::TERMINAL);
        setStatusMessage("Terminal opened | Region: " + region_manager_.getRegionName() + " | Type commands, 'exit' to close");
    } else {
        // 终端关闭时，如果当前在终端区域，切换回代码区
        if (region_manager_.getCurrentRegion() == EditorRegion::TERMINAL) {
            region_manager_.setRegion(EditorRegion::CODE_AREA);
        }
        setStatusMessage("Terminal closed | Region: " + region_manager_.getRegionName());
    }
}

void Editor::handleTerminalInput(Event event) {
    // 确保当前区域是终端
    if (region_manager_.getCurrentRegion() != EditorRegion::TERMINAL) {
        region_manager_.setRegion(EditorRegion::TERMINAL);
    }
    
    // 处理特殊键
    if (event == Event::Escape) {
        terminal_.setVisible(false);
        region_manager_.setRegion(EditorRegion::CODE_AREA);
        setStatusMessage("Terminal closed | Region: " + region_manager_.getRegionName());
        return;
    } else if (event == Event::Return) {
        std::string command = terminal_.getCurrentInput();
        if (command == "exit" || command == "quit") {
            terminal_.setVisible(false);
            region_manager_.setRegion(EditorRegion::CODE_AREA);
            setStatusMessage("Terminal closed | Region: " + region_manager_.getRegionName());
            return;
        }
        terminal_.executeCommand(command);
        terminal_.handleInput("");  // 清空输入
        return;
    } else if (event == Event::ArrowUp) {
        // 在终端顶部时，向上切换到代码区
        // 否则处理命令历史
        terminal_.handleKeyEvent("ArrowUp");
        return;
    } else if (event == Event::ArrowDown) {
        // 在终端底部时，向下没有其他区域
        // 否则处理命令历史
        terminal_.handleKeyEvent("ArrowDown");
        return;
    } else if (event == Event::ArrowLeft) {
        // 在终端左边界时，向左切换到文件浏览器或代码区
        // 否则移动光标
        if (terminal_.getCursorPosition() == 0 && file_browser_.isVisible()) {
            if (region_manager_.navigateLeft()) {
                setStatusMessage("Region: " + region_manager_.getRegionName() + " | →: Return to terminal");
                return;
            }
        }
        terminal_.handleKeyEvent("ArrowLeft");
        return;
    } else if (event == Event::ArrowRight) {
        // 在终端右边界时，向右切换到代码区
        // 否则移动光标
        std::string input = terminal_.getCurrentInput();
        if (terminal_.getCursorPosition() >= input.length()) {
            if (region_manager_.navigateRight()) {
                setStatusMessage("Region: " + region_manager_.getRegionName() + " | ←: Return to terminal");
                return;
            }
        }
        terminal_.handleKeyEvent("ArrowRight");
        return;
    } else if (event == Event::Home) {
        terminal_.handleKeyEvent("Home");
        return;
    } else if (event == Event::End) {
        terminal_.handleKeyEvent("End");
        return;
    } else if (event == Event::Backspace) {
        terminal_.handleKeyEvent("Backspace");
        return;
    } else if (event == Event::Delete) {
        terminal_.handleKeyEvent("Delete");
        return;
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            // 接受所有可打印字符
            if (c >= 32 && c < 127) {
                std::string current = terminal_.getCurrentInput();
                size_t pos = terminal_.getCursorPosition();
                std::string new_input = current.substr(0, pos) + c + current.substr(pos);
                terminal_.handleInput(new_input);
                // 移动光标到插入字符之后
                terminal_.setCursorPosition(pos + 1);
            }
        }
        return;
    }
}

// 命令面板
void Editor::openCommandPalette() {
    command_palette_.open();
    setStatusMessage("Command Palette - Type to search, ↑↓ to navigate, Enter to execute");
}

void Editor::handleCommandPaletteInput(Event event) {
    // 处理特殊键
    if (event == Event::Escape) {
        command_palette_.close();
        setStatusMessage("Command Palette closed");
        return;
    } else if (event == Event::Return) {
        command_palette_.executeSelected();
        return;
    } else if (event == Event::ArrowUp) {
        command_palette_.handleKeyEvent("ArrowUp");
        return;
    } else if (event == Event::ArrowDown) {
        command_palette_.handleKeyEvent("ArrowDown");
        return;
    } else if (event == Event::Backspace) {
        std::string current_input = command_palette_.getInput();
        if (!current_input.empty()) {
            command_palette_.handleInput(current_input.substr(0, current_input.length() - 1));
        }
        return;
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            // 接受所有可打印字符
            if (c >= 32 && c < 127) {
                std::string new_input = command_palette_.getInput() + c;
                command_palette_.handleInput(new_input);
            }
        }
        return;
    }
}

void Editor::initializeCommandPalette() {
    using namespace pnana::features;
    
    // 注册文件操作命令
    command_palette_.registerCommand(Command(
        "file.new",
        "New File",
        "Create a new file",
        {"new", "file", "create"},
        [this]() { newFile(); }
    ));
    
    command_palette_.registerCommand(Command(
        "file.open",
        "Open File",
        "Open file browser",
        {"open", "file", "browser"},
        [this]() { toggleFileBrowser(); }
    ));
    
    command_palette_.registerCommand(Command(
        "file.save",
        "Save File",
        "Save current file",
        {"save", "file", "write"},
        [this]() { saveFile(); }
    ));
    
    command_palette_.registerCommand(Command(
        "file.save_as",
        "Save As",
        "Save file with new name",
        {"save", "as", "rename"},
        [this]() { startSaveAs(); }
    ));
    
    command_palette_.registerCommand(Command(
        "file.close",
        "Close Tab",
        "Close current tab",
        {"close", "tab", "file"},
        [this]() { closeCurrentTab(); }
    ));
    
    // 注册编辑操作命令
    command_palette_.registerCommand(Command(
        "edit.undo",
        "Undo",
        "Undo last action",
        {"undo", "edit"},
        [this]() { undo(); }
    ));
    
    command_palette_.registerCommand(Command(
        "edit.redo",
        "Redo",
        "Redo last undone action",
        {"redo", "edit"},
        [this]() { redo(); }
    ));
    
    command_palette_.registerCommand(Command(
        "edit.cut",
        "Cut",
        "Cut selection to clipboard",
        {"cut", "edit"},
        [this]() { cut(); }
    ));
    
    command_palette_.registerCommand(Command(
        "edit.copy",
        "Copy",
        "Copy selection to clipboard",
        {"copy", "edit"},
        [this]() { copy(); }
    ));
    
    command_palette_.registerCommand(Command(
        "edit.paste",
        "Paste",
        "Paste from clipboard",
        {"paste", "edit"},
        [this]() { paste(); }
    ));
    
    // 注册搜索和导航命令
    command_palette_.registerCommand(Command(
        "search.find",
        "Find",
        "Search in file",
        {"find", "search", "grep"},
        [this]() { startSearch(); }
    ));
    
    command_palette_.registerCommand(Command(
        "search.replace",
        "Replace",
        "Find and replace",
        {"replace", "search", "find"},
        [this]() { startReplace(); }
    ));
    
    // 注册分屏命令
    command_palette_.registerCommand(Command(
        "view.split",
        "Split View",
        "Split editor window",
        {"split", "side", "view", "window"},
        [this]() { showSplitDialog(); }
    ));
    
    command_palette_.registerCommand(Command(
        "navigation.goto_line",
        "Go to Line",
        "Jump to specific line number",
        {"goto", "line", "jump", "navigation"},
        [this]() { startGotoLineMode(); }
    ));
    
    // 注册视图操作命令
    command_palette_.registerCommand(Command(
        "view.theme",
        "Change Theme",
        "Open theme selection menu",
        {"theme", "color", "view", "appearance"},
        [this]() { toggleThemeMenu(); }
    ));
    
    command_palette_.registerCommand(Command(
        "view.help",
        "Help",
        "Show help window",
        {"help", "view", "documentation"},
        [this]() { toggleHelp(); }
    ));
    
    command_palette_.registerCommand(Command(
        "view.line_numbers",
        "Toggle Line Numbers",
        "Show/hide line numbers",
        {"line", "numbers", "view", "toggle"},
        [this]() { toggleLineNumbers(); }
    ));
    
    // 注册标签页操作命令
    command_palette_.registerCommand(Command(
        "tab.next",
        "Next Tab",
        "Switch to next tab",
        {"tab", "next", "switch"},
        [this]() { switchToNextTab(); }
    ));
    
    command_palette_.registerCommand(Command(
        "tab.previous",
        "Previous Tab",
        "Switch to previous tab",
        {"tab", "previous", "switch"},
        [this]() { switchToPreviousTab(); }
    ));
    
    // 注册终端命令
    command_palette_.registerCommand(Command(
        "terminal.open",
        "Open Terminal",
        "Open integrated terminal",
        {"terminal", "term", "shell", "cmd", "console"},
        [this]() { toggleTerminal(); }
    ));
    
    // 预留位置：未来可以添加更多命令
    // 例如：git提交、在线终端、设置等
    // command_palette_.registerCommand(Command(
    //     "git.commit",
    //     "Git Commit",
    //     "Commit changes to git",
    //     {"git", "commit", "version"},
    //     [this]() { /* TODO: 实现git提交 */ }
    // ));
}

// 辅助方法
void Editor::setStatusMessage(const std::string& message) {
    status_message_ = message;
}

std::string Editor::getFileType() const {
    const Document* doc = getCurrentDocument();
    if (!doc) {
        return "text";
    }
    std::string ext = doc->getFileExtension();
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" || ext == "hpp") return "cpp";
    if (ext == "c") return "c";
    if (ext == "py") return "python";
    if (ext == "js") return "javascript";
    if (ext == "ts") return "typescript";
    if (ext == "java") return "java";
    if (ext == "go") return "go";
    if (ext == "rs") return "rust";
    if (ext == "md") return "markdown";
    if (ext == "json") return "json";
    if (ext == "html") return "html";
    if (ext == "css") return "css";
    if (ext == "sh") return "shell";
    return "text";
}

bool Editor::isCtrlKey(const Event& event, char key) const {
    if (!event.is_character()) {
        return false;
    }
    
    std::string ch = event.character();
    if (ch.length() != 1) {
        return false;
    }
    
    // Ctrl+Key产生ASCII控制字符 (Ctrl+A = 1, Ctrl+B = 2, ...)
    char ctrl_char = key - 'a' + 1;
    return ch[0] == ctrl_char;
}

bool Editor::isShiftKey(const Event& event) const {
    // FTXUI通过特殊事件类型处理Shift组合键
    return event == Event::ArrowUpCtrl || 
           event == Event::ArrowDownCtrl ||
           event == Event::ArrowLeftCtrl ||
           event == Event::ArrowRightCtrl;
}

void Editor::handleRenameFile() {
    if (!file_browser_.isVisible() || !file_browser_.hasSelection()) {
        return;
    }
    
    std::string current_name = file_browser_.getSelectedName();
    if (current_name == "..") {
        setStatusMessage("Cannot rename parent directory");
        return;
    }
    
    bool is_directory = file_browser_.getSelectedPath() != file_browser_.getSelectedFile();
    
    dialog_.showInput(
        "Rename " + std::string(is_directory ? "Folder" : "File"),
        "Enter new name:",
        current_name,
        [this](const std::string& new_name) {
            if (new_name.empty()) {
                setStatusMessage("Name cannot be empty");
                return;
            }
            
            if (file_browser_.renameSelected(new_name)) {
                setStatusMessage("Renamed to: " + new_name);
            } else {
                setStatusMessage("Failed to rename. Name may already exist or be invalid.");
            }
        },
        [this]() {
            setStatusMessage("Rename cancelled");
        }
    );
}

void Editor::handleDeleteFile() {
    if (!file_browser_.isVisible() || !file_browser_.hasSelection()) {
        return;
    }
    
    std::string selected_name = file_browser_.getSelectedName();
    if (selected_name == "..") {
        setStatusMessage("Cannot delete parent directory");
        return;
    }
    
    std::string selected_path = file_browser_.getSelectedPath();
    bool is_directory = file_browser_.getSelectedPath() != file_browser_.getSelectedFile();
    
    std::string message = "Are you sure you want to delete ";
    message += is_directory ? "folder" : "file";
    message += ":\n  " + selected_name + "?";
    if (is_directory) {
        message += "\n\nThis will delete all contents recursively!";
    }
    
    dialog_.showConfirm(
        "Delete " + std::string(is_directory ? "Folder" : "File"),
        message,
        [this, selected_path, selected_name, is_directory]() {
            if (file_browser_.deleteSelected()) {
                setStatusMessage("Deleted: " + selected_name);
            } else {
                setStatusMessage("Failed to delete: " + selected_name);
            }
        },
        [this]() {
            setStatusMessage("Delete cancelled");
        }
    );
}

void Editor::openFilePicker() {
    // 获取当前文件所在目录，如果没有则使用当前工作目录
    std::string start_path = ".";
    if (getCurrentDocument() && !getCurrentDocument()->getFileName().empty()) {
        try {
            std::filesystem::path file_path = getCurrentDocument()->getFileName();
            if (std::filesystem::exists(file_path)) {
                start_path = std::filesystem::canonical(file_path).parent_path().string();
            }
        } catch (...) {
            start_path = ".";
        }
    }
    
    // 显示文件选择器（默认选择文件和文件夹）
    file_picker_.show(start_path, ui::FilePickerType::BOTH,
        [this](const std::string& path) {
            // 检查路径是否是目录
            try {
                if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                    // 如果是目录，更新文件浏览器的当前目录
                    if (file_browser_.openDirectory(path)) {
                        setStatusMessage("Changed to directory: " + path);
                    } else {
                        setStatusMessage("Failed to open directory: " + path);
                    }
                    return;
                }
            } catch (...) {
                // 如果检查失败，继续尝试打开（可能是新文件）
            }
            
            // 选择文件后打开
            if (openFile(path)) {
                setStatusMessage("Opened: " + path);
            } else {
                setStatusMessage("Failed to open: " + path);
            }
        },
        [this]() {
            setStatusMessage("File picker cancelled");
        }
    );
}

void Editor::handleFilePickerInput(ftxui::Event event) {
    if (file_picker_.handleInput(event)) {
        return;
    }
}

// 分屏操作
void Editor::showSplitDialog() {
    // 如果已经有分屏，显示关闭分屏对话框
    if (split_view_manager_.hasSplits()) {
        // 收集所有分屏信息
        std::vector<ui::SplitInfo> splits;
        const auto& regions = split_view_manager_.getRegions();
        auto tabs = document_manager_.getAllTabs();
        
        for (size_t i = 0; i < regions.size(); ++i) {
            const auto& region = regions[i];
            std::string doc_name = "[No Document]";
            bool is_modified = false;
            
            if (region.document_index < tabs.size()) {
                doc_name = tabs[region.document_index].filename;
                if (doc_name.empty()) {
                    doc_name = "[Untitled]";
                }
                is_modified = tabs[region.document_index].is_modified;
            }
            
            splits.emplace_back(i, region.document_index, doc_name, 
                              region.is_active, is_modified);
        }
        
        split_dialog_.showClose(
            splits,
            [this](size_t region_index) {
                closeSplitRegion(region_index);
            },
            [this]() {
                setStatusMessage("Close split cancelled");
            }
        );
    } else {
        // 没有分屏，显示创建分屏对话框
        split_dialog_.showCreate(
            [this](features::SplitDirection direction) {
                splitView(direction);
            },
            [this]() {
                setStatusMessage("Split cancelled");
            }
        );
    }
}

void Editor::closeSplitRegion(size_t region_index) {
    // 检查该区域的文档是否已保存
    const auto& regions = split_view_manager_.getRegions();
    if (region_index >= regions.size()) {
        setStatusMessage("Invalid region index");
        return;
    }
    
    const auto& region = regions[region_index];
    auto tabs = document_manager_.getAllTabs();
    
    // 检查该区域关联的文档是否已修改
    if (region.document_index < tabs.size()) {
        if (tabs[region.document_index].is_modified) {
            setStatusMessage("Cannot close: document has unsaved changes. Save first (Ctrl+S)");
            return;
        }
    }
    
    // 如果关闭的是当前激活的区域，需要切换到其他区域
    if (region.is_active && regions.size() > 1) {
        // 找到另一个区域并激活
        for (size_t i = 0; i < regions.size(); ++i) {
            if (i != region_index) {
                // 切换到该区域的文档
                if (regions[i].document_index < tabs.size()) {
                    document_manager_.switchToDocument(regions[i].document_index);
                }
                break;
            }
        }
    }
    
    // 关闭区域
    split_view_manager_.closeRegion(region_index);
    
    // 如果只剩下一个区域，重置分屏管理器
    if (!split_view_manager_.hasSplits()) {
        split_view_manager_.reset();
        setStatusMessage("Split closed, back to single view");
    } else {
        setStatusMessage("Split region closed");
    }
}

void Editor::splitView(features::SplitDirection direction) {
    // 确保当前有文档
    Document* current_doc = getCurrentDocument();
    if (!current_doc) {
        setStatusMessage("No document to split");
        return;
    }
    
    // 找到当前文档索引
    auto tabs = document_manager_.getAllTabs();
    size_t current_doc_index = 0;
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (tabs[i].filepath == current_doc->getFilePath()) {
            current_doc_index = i;
            break;
        }
    }
    
    // 如果没有分屏，先初始化第一个区域
    if (!split_view_manager_.hasSplits()) {
        split_view_manager_.setCurrentDocumentIndex(current_doc_index);
    }
    
    int screen_width = screen_.dimx();
    int screen_height = screen_.dimy();
    
    // 如果文件浏览器打开，需要减去文件浏览器的宽度
    if (file_browser_.isVisible()) {
        screen_width -= file_browser_width_ + 1;  // +1 for separator
    }
    
    // 减去状态栏和标签栏的高度
    screen_height -= 6;  // 标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1)
    
    // 执行分屏
    if (direction == features::SplitDirection::VERTICAL) {
        split_view_manager_.splitVertical(screen_width, screen_height);
        setStatusMessage("Split vertically");
    } else {
        split_view_manager_.splitHorizontal(screen_width, screen_height);
        setStatusMessage("Split horizontally");
    }
    
    // 更新新创建区域的文档索引（新区域显示相同的文档）
    auto& regions = const_cast<std::vector<features::ViewRegion>&>(split_view_manager_.getRegions());
    for (auto& region : regions) {
        // 新创建的区域（document_index 可能无效）设置为当前文档
        if (region.document_index >= tabs.size() || !split_view_manager_.hasSplits()) {
            region.document_index = current_doc_index;
        }
    }
    
    // 更新区域尺寸
    split_view_manager_.updateRegionSizes(screen_width, screen_height);
}

void Editor::focusLeftRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }
    split_view_manager_.focusLeftRegion();
    
    // 切换到激活区域的文档
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region) {
        auto tabs = document_manager_.getAllTabs();
        if (active_region->document_index < tabs.size()) {
            document_manager_.switchToDocument(active_region->document_index);
        }
    }
    setStatusMessage("Focus left region");
}

void Editor::focusRightRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }
    split_view_manager_.focusRightRegion();
    
    // 切换到激活区域的文档
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region) {
        auto tabs = document_manager_.getAllTabs();
        if (active_region->document_index < tabs.size()) {
            document_manager_.switchToDocument(active_region->document_index);
        }
    }
    setStatusMessage("Focus right region");
}

void Editor::focusUpRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }
    split_view_manager_.focusUpRegion();
    
    // 切换到激活区域的文档
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region) {
        auto tabs = document_manager_.getAllTabs();
        if (active_region->document_index < tabs.size()) {
            document_manager_.switchToDocument(active_region->document_index);
        }
    }
    setStatusMessage("Focus up region");
}

void Editor::focusDownRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }
    split_view_manager_.focusDownRegion();
    
    // 切换到激活区域的文档
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region) {
        auto tabs = document_manager_.getAllTabs();
        if (active_region->document_index < tabs.size()) {
            document_manager_.switchToDocument(active_region->document_index);
        }
    }
    setStatusMessage("Focus down region");
}

} // namespace core
} // namespace pnana
