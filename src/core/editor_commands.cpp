// 命令面板注册相关实现
#include "core/editor.h"
#include "features/command_palette.h"

namespace pnana {
namespace core {

void Editor::initializeCommandPalette() {
    using namespace pnana::features;

    // 注册文件操作命令
    command_palette_.registerCommand(
        Command("file.new", "New File", "Create a new file", {"new", "file", "create"}, [this]() {
            newFile();
        }));

    command_palette_.registerCommand(Command("file.open", "Open File", "Open file browser",
                                             {"open", "file", "browser"}, [this]() {
                                                 toggleFileBrowser();
                                             }));

    command_palette_.registerCommand(
        Command("file.save", "Save File", "Save current file", {"save", "file", "write"}, [this]() {
            saveFile();
        }));

    command_palette_.registerCommand(Command("file.save_as", "Save As", "Save file with new name",
                                             {"save", "as", "rename"}, [this]() {
                                                 startSaveAs();
                                             }));

    command_palette_.registerCommand(
        Command("file.close", "Close Tab", "Close current tab", {"close", "tab", "file"}, [this]() {
            closeCurrentTab();
        }));

    command_palette_.registerCommand(Command("file.recent", "Recent Files",
                                             "Show recently opened files",
                                             {"recent", "files", "history", "open"}, [this]() {
                                                 openRecentFilesDialog();
                                             }));

    command_palette_.registerCommand(
        Command("tools.tui", "TUI Config Files", "Open configuration files for TUI tools",
                {"tui", "config", "terminal", "editor", "tools"}, [this]() {
                    openTUIConfigDialog();
                }));

    // 注册编辑操作命令
    command_palette_.registerCommand(
        Command("edit.undo", "Undo", "Undo last action", {"undo", "edit"}, [this]() {
            undo();
        }));

    command_palette_.registerCommand(
        Command("edit.redo", "Redo", "Redo last undone action", {"redo", "edit"}, [this]() {
            redo();
        }));

    command_palette_.registerCommand(
        Command("edit.cut", "Cut", "Cut selection to clipboard", {"cut", "edit"}, [this]() {
            cut();
        }));

    command_palette_.registerCommand(
        Command("edit.copy", "Copy", "Copy selection to clipboard", {"copy", "edit"}, [this]() {
            copy();
        }));

    command_palette_.registerCommand(
        Command("edit.paste", "Paste", "Paste from clipboard", {"paste", "edit"}, [this]() {
            paste();
        }));

    // 注册搜索和导航命令
    command_palette_.registerCommand(
        Command("search.find", "Find", "Search in file", {"find", "search", "grep"}, [this]() {
            startSearch();
        }));

    command_palette_.registerCommand(Command("search.replace", "Replace", "Find and replace",
                                             {"replace", "search", "find"}, [this]() {
                                                 startReplace();
                                             }));

    // 注册解压命令
    command_palette_.registerCommand(Command("file.extract", "Extract Archive",
                                             "Extract archive files",
                                             {"extract", "unzip", "untar", "archive"}, [this]() {
                                                 openExtractDialog();
                                             }));

    // 注册分屏命令
    command_palette_.registerCommand(Command("view.split", "Split View", "Split editor window",
                                             {"split", "side", "view", "window"}, [this]() {
                                                 showSplitDialog();
                                             }));

    command_palette_.registerCommand(Command("navigation.goto_line", "Go to Line",
                                             "Jump to specific line number",
                                             {"goto", "line", "jump", "navigation"}, [this]() {
                                                 startGotoLineMode();
                                             }));

    // 注册视图操作命令
    command_palette_.registerCommand(Command("view.theme", "Change Theme",
                                             "Open theme selection menu",
                                             {"theme", "color", "view", "appearance"}, [this]() {
                                                 toggleThemeMenu();
                                             }));

    command_palette_.registerCommand(Command("view.help", "Help", "Show help window",
                                             {"help", "view", "documentation"}, [this]() {
                                                 toggleHelp();
                                             }));

    command_palette_.registerCommand(Command("ai.assistant", "AI Assistant",
                                             "Open AI programming assistant",
                                             {"ai", "assistant", "chat", "code", "help"}, [this]() {
                                                 toggleAIAssistant();
                                             }));

    command_palette_.registerCommand(Command("editor.cursor", "Cursor Configuration",
                                             "Configure cursor style, color, and blink rate",
                                             {"cursor", "config", "settings", "style"}, [this]() {
                                                 openCursorConfig();
                                             }));

    command_palette_.registerCommand(Command("view.line_numbers", "Toggle Line Numbers",
                                             "Show/hide line numbers",
                                             {"line", "numbers", "view", "toggle"}, [this]() {
                                                 toggleLineNumbers();
                                             }));

    // 注册标签页操作命令
    command_palette_.registerCommand(
        Command("tab.next", "Next Tab", "Switch to next tab", {"tab", "next", "switch"}, [this]() {
            switchToNextTab();
        }));

    command_palette_.registerCommand(Command("tab.previous", "Previous Tab",
                                             "Switch to previous tab",
                                             {"tab", "previous", "switch"}, [this]() {
                                                 switchToPreviousTab();
                                             }));

    // 注册终端命令
    command_palette_.registerCommand(
        Command("terminal.open", "Open Terminal", "Open integrated terminal",
                {"terminal", "term", "shell", "cmd", "console"}, [this]() {
                    toggleTerminal();
                }));

    // 编码转换命令
    command_palette_.registerCommand(Command("file.reopen_with_encoding", "Reopen with Encoding",
                                             "Reopen current file with different encoding",
                                             {"coder", "encoding", "encode", "charset", "file"},
                                             [this]() {
                                                 openEncodingDialog();
                                             }));

    // 代码格式化命令
    command_palette_.registerCommand(
        Command("code.format", "Format Code", "Format selected files using LSP",
                {"format", "formatter", "code", "lsp", "beautify"}, [this]() {
                    openFormatDialog();
                }));

    // AI 配置命令
    command_palette_.registerCommand(
        Command("ai.config", "AI Configuration", "Configure AI model settings and API",
                {"ai", "config", "configuration", "openai", "model", "api"}, [this]() {
                    ai_config_dialog_.open();
                }));

    // Git 版本控制命令
    command_palette_.registerCommand(
        Command("git.panel", "Git Panel", "Open git version control panel",
                {"git", "vgit", "version", "control", "repository"}, [this]() {
                    toggleGitPanel();
                }));

    // 注册 Todo 命令
    command_palette_.registerCommand(Command("todo.panel", "Todo Panel", "Open todo list panel",
                                             {"todo", "task", "reminder", "schedule"}, [this]() {
                                                 toggleTodoPanel();
                                             }));

    // 注册包管理器命令
    command_palette_.registerCommand(Command("package.manager", "Package Manager",
                                             "Open package manager panel",
                                             {"paker", "package", "pip", "manager"}, [this]() {
                                                 togglePackageManagerPanel();
                                             }));
}

} // namespace core
} // namespace pnana
