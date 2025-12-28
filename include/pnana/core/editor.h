#ifndef PNANA_CORE_EDITOR_H
#define PNANA_CORE_EDITOR_H

#include "core/document.h"
#include "core/document_manager.h"
#include "core/region_manager.h"
#include "input/key_binding_manager.h"
#include "input/action_executor.h"
#include "ui/theme.h"
#include "ui/statusbar.h"
#include "ui/helpbar.h"
#include "ui/tabbar.h"
#include "ui/help.h"
#include "ui/dialog.h"
#include "ui/file_picker.h"
#include "ui/split_dialog.h"
#include "features/search.h"
#include "features/file_browser.h"
#include "features/syntax_highlighter.h"
#include "features/command_palette.h"
#include "features/terminal.h"
#include "features/split_view.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <memory>
#include <vector>

namespace pnana {
namespace core {

// 编辑器模式
enum class EditorMode {
    NORMAL,        // 正常编辑模式
    SEARCH,        // 搜索模式
    REPLACE,       // 替换模式
    GOTO_LINE      // 跳转到行模式
};

// 编辑器核心类
class Editor {
    // 友元类：允许ActionExecutor访问私有方法
    friend class input::ActionExecutor;
    
public:
    Editor();
    explicit Editor(const std::string& filepath);
    Editor(const std::vector<std::string>& filepaths);
    
    // 运行编辑器
    void run();
    
    // 文件操作
    bool openFile(const std::string& filepath);
    bool saveFile();
    bool saveFileAs(const std::string& filepath);
    void startSaveAs();  // 启动另存为对话框
    bool closeFile();
    void newFile();
    void createFolder();  // 创建新文件夹
    void openFilePicker();  // 打开文件选择器
    
    // 光标移动
    void moveCursorUp();
    void moveCursorDown();
    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorPageUp();
    void moveCursorPageDown();
    void moveCursorLineStart();
    void moveCursorLineEnd();
    void moveCursorFileStart();
    void moveCursorFileEnd();
    void moveCursorWordForward();
    void moveCursorWordBackward();
    
    // 编辑操作
    void insertChar(char ch);
    void insertNewline();
    void deleteChar();
    void backspace();
    void deleteLine();
    void deleteWord();
    void duplicateLine();
    void moveLineUp();
    void moveLineDown();
    void indentLine();
    void unindentLine();
    void toggleComment();
    
    // 选择操作
    void startSelection();
    void endSelection();
    void selectAll();
    void selectLine();
    void selectWord();
    
    // 剪贴板操作
    void cut();
    void copy();
    void paste();
    
    // 撤销/重做
    void undo();
    void redo();
    
    // 搜索和替换
    void startSearch();
    void startReplace();
    void searchNext();
    void searchPrevious();
    void replaceCurrentMatch();
    void replaceAll();
    
    // 跳转
    void gotoLine(size_t line);
    void startGotoLineMode();
    
    // 视图操作
    void toggleLineNumbers();
    void toggleRelativeNumbers();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    
    // 分屏操作
    void showSplitDialog();
    void splitView(features::SplitDirection direction);
    void closeSplitRegion(size_t region_index);  // 关闭指定分屏区域
    void focusLeftRegion();
    void focusRightRegion();
    void focusUpRegion();
    void focusDownRegion();
    
    // 主题
    void setTheme(const std::string& theme_name);
    
    // 退出
    void quit();
    bool shouldQuit() const { return should_quit_; }
    
private:
    // 文档管理
    DocumentManager document_manager_;
    
    // 输入处理系统
    input::KeyBindingManager key_binding_manager_;
    input::ActionExecutor action_executor_;
    
    // 区域管理
    RegionManager region_manager_;
    
    // UI组件
    ui::Theme theme_;
    ui::Statusbar statusbar_;
    ui::Helpbar helpbar_;
    ui::Tabbar tabbar_;
    ui::Help help_;
    ui::Dialog dialog_;
    ui::FilePicker file_picker_;
    ui::SplitDialog split_dialog_;
    
    // 功能模块
    features::SearchEngine search_engine_;
    features::FileBrowser file_browser_;
    features::SyntaxHighlighter syntax_highlighter_;
    features::CommandPalette command_palette_;
    features::Terminal terminal_;
    features::SplitViewManager split_view_manager_;
    
    // 编辑器状态
    EditorMode mode_;
    size_t cursor_row_;
    size_t cursor_col_;
    size_t view_offset_row_;
    size_t view_offset_col_;
    
    // 主题选择
    bool show_theme_menu_;
    size_t selected_theme_index_;
    std::vector<std::string> available_themes_;
    
    // 帮助窗口
    bool show_help_;
    
    // 创建文件夹弹窗
    bool show_create_folder_;
    std::string folder_name_input_;
    
    // 另存为弹窗
    bool show_save_as_;
    std::string save_as_input_;
    
    // 选择
    bool selection_active_;
    size_t selection_start_row_;
    size_t selection_start_col_;
    
    // 显示选项
    bool show_line_numbers_;
    bool relative_line_numbers_;
    bool syntax_highlighting_;
    int zoom_level_;
    int file_browser_width_;  // 文件浏览器宽度
    
    // 输入缓冲区（用于搜索、跳转等）
    std::string input_buffer_;
    
    // 状态消息
    std::string status_message_;
    bool should_quit_;
    
    // FTXUI
    ftxui::ScreenInteractive screen_;
    ftxui::Component main_component_;
    
    // 事件处理
    void handleInput(ftxui::Event event);
    void handleNormalMode(ftxui::Event event);
    void handleSearchMode(ftxui::Event event);
    void handleReplaceMode(ftxui::Event event);
    void handleGotoLineMode(ftxui::Event event);
    
    // UI渲染
    ftxui::Element renderUI();
    ftxui::Element renderTabbar();
    ftxui::Element renderEditor();
    ftxui::Element renderSplitEditor();  // 分屏编辑器渲染
    ftxui::Element renderEditorRegion(const features::ViewRegion& region, Document* doc);  // 渲染单个区域
    ftxui::Element renderWelcomeScreen();
    ftxui::Element renderLine(size_t line_num, bool is_current);
    ftxui::Element renderLineNumber(size_t line_num, bool is_current);
    ftxui::Element renderStatusbar();
    ftxui::Element renderHelpbar();
    ftxui::Element renderInputBox();
    ftxui::Element renderThemeMenu();
    ftxui::Element renderFileBrowser();
    ftxui::Element renderCreateFolderDialog();  // 创建文件夹对话框
    ftxui::Element renderSaveAsDialog();  // 另存为对话框
    ftxui::Element renderHelp();
    ftxui::Element renderCommandPalette();  // 命令面板
    ftxui::Element renderTerminal();  // 终端
    ftxui::Element renderFilePicker();  // 文件选择器
    
    // 辅助方法
    void adjustCursor();
    void adjustViewOffset();
    void setStatusMessage(const std::string& message);
    std::string getFileType() const;
    void executeSearch(bool move_cursor = true);
    void executeReplace();
    void executeGotoLine();
    
    // 快捷键检查
    bool isCtrlKey(const ftxui::Event& event, char key) const;
    bool isShiftKey(const ftxui::Event& event) const;
    
    // 主题菜单
    void toggleThemeMenu();
    void selectNextTheme();
    void selectPreviousTheme();
    void applySelectedTheme();
    
    // 文件浏览器
    void toggleFileBrowser();
    void handleFileBrowserInput(ftxui::Event event);
    void handleRenameFile();
    void handleDeleteFile();
    
    // 终端
    void toggleTerminal();
    void handleTerminalInput(ftxui::Event event);
    
    // 标签页管理
    void closeCurrentTab();
    void switchToNextTab();
    void switchToPreviousTab();
    void switchToTab(size_t index);
    
    // 帮助系统
    void toggleHelp();
    
    // 命令面板
    void openCommandPalette();
    void handleCommandPaletteInput(ftxui::Event event);
    void initializeCommandPalette();
    
    // 文件选择器
    void handleFilePickerInput(ftxui::Event event);
    
    // 获取当前文档（便捷方法）
    Document* getCurrentDocument();
    const Document* getCurrentDocument() const;
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_EDITOR_H

