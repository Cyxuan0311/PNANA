#ifndef PNANA_CORE_EDITOR_H
#define PNANA_CORE_EDITOR_H

#include "core/config_manager.h"
#include "core/document.h"
#include "core/document_manager.h"
#include "core/region_manager.h"
#include "input/action_executor.h"
#include "input/key_binding_manager.h"
// 前向声明（避免循环依赖，但需要完整类型用于 unique_ptr）
#include "core/input/input_router.h"
#include "core/ui/ui_router.h"
#include "ui/binary_file_view.h"
#include "ui/create_folder_dialog.h"
#include "ui/cursor_config_dialog.h"
#include "ui/dialog.h"
#include "ui/encoding_dialog.h"
#include "ui/file_picker.h"
#include "ui/help.h"
#include "ui/helpbar.h"
#include "ui/new_file_prompt.h"
#include "ui/save_as_dialog.h"
#include "ui/split_dialog.h"
#include "ui/ssh_dialog.h"
#include "ui/statusbar.h"
#include "ui/tabbar.h"
#include "ui/theme.h"
#include "ui/theme_menu.h"
#include "ui/welcome_screen.h"
#ifdef BUILD_LUA_SUPPORT
#include "ui/plugin_manager_dialog.h"
#endif
#include "features/file_browser.h"
#include "features/search.h"
#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
#include "features/image_preview.h"
#endif
#include "features/SyntaxHighlighter/syntax_highlighter.h"
#include "features/command_palette.h"
#include "features/split_view.h"
#include "features/terminal.h"
#ifdef BUILD_LSP_SUPPORT
#include "features/lsp/document_change_tracker.h"
#include "features/lsp/lsp_async_manager.h"
#include "features/lsp/lsp_completion_cache.h"
#include "features/lsp/lsp_server_manager.h"
#include "ui/completion_popup.h"
#endif
#ifdef BUILD_LUA_SUPPORT
#include "plugins/plugin_manager.h"
#endif
#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 前向声明
namespace input {
class TerminalHandler;
class FileBrowserHandler;
} // namespace input

// 编辑器模式
enum class EditorMode {
    NORMAL, // 正常编辑模式
    SEARCH, // 搜索模式
    REPLACE // 替换模式
};

// 编辑器核心类
class Editor {
    // 友元类：允许ActionExecutor访问私有方法
    friend class pnana::input::ActionExecutor;
    // 友元类：允许UIRouter访问渲染方法
    friend class pnana::core::ui::UIRouter;
    // 友元类：允许TerminalHandler访问setStatusMessage
    friend class pnana::core::input::TerminalHandler;
    // 友元类：允许FileBrowserHandler访问setStatusMessage
    friend class pnana::core::input::FileBrowserHandler;
#ifdef BUILD_LUA_SUPPORT
    // 友元类：允许LuaAPI访问私有方法
    friend class plugins::LuaAPI;
#endif

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
    void startSaveAs(); // 启动另存为对话框
    bool closeFile();
    void newFile();
    void createFolder();   // 创建新文件夹
    void openFilePicker(); // 打开文件选择器

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

    // 文件浏览器导航
    void pageUp();   // 文件浏览器向上翻页
    void pageDown(); // 文件浏览器向下翻页

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
    void extendSelectionUp();
    void extendSelectionDown();
    void extendSelectionLeft();
    void extendSelectionRight();

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
    void closeSplitRegion(size_t region_index); // 关闭指定分屏区域
    void focusLeftRegion();
    void focusRightRegion();
    void focusUpRegion();
    void focusDownRegion();

    // 主题
    void setTheme(const std::string& theme_name);
    const pnana::ui::Theme& getTheme() const {
        return theme_;
    }
    pnana::ui::Theme& getTheme() {
        return theme_;
    }

    // 配置
    void loadConfig(const std::string& config_path = "");

    // 访问器（用于输入路由器和UI路由器等）
    RegionManager& getRegionManager() {
        return region_manager_;
    }
    const RegionManager& getRegionManager() const {
        return region_manager_;
    }
    pnana::input::KeyBindingManager& getKeyBindingManager() {
        return key_binding_manager_;
    }
    const pnana::input::KeyBindingManager& getKeyBindingManager() const {
        return key_binding_manager_;
    }
    pnana::input::ActionExecutor& getActionExecutor() {
        return action_executor_;
    }
    const pnana::input::ActionExecutor& getActionExecutor() const {
        return action_executor_;
    }
    bool isFileBrowserVisible() const;
    bool isTerminalVisible() const;
    features::Terminal& getTerminal() {
        return terminal_;
    }
    const features::Terminal& getTerminal() const {
        return terminal_;
    }
    EditorMode getMode() const {
        return mode_;
    }
    void setMode(EditorMode mode) {
        mode_ = mode;
    }
    int getFileBrowserWidth() const {
        return file_browser_width_;
    }
    void setFileBrowserWidth(int width) {
        file_browser_width_ = width;
    }
    int getTerminalHeight() const {
        return terminal_height_;
    }
    void setTerminalHeight(int height) {
        terminal_height_ = height;
    }
    int getScreenHeight() const;
    int getScreenWidth() const;

    // 退出
    void quit();
    bool shouldQuit() const {
        return should_quit_;
    }

  private:
    // 文档管理
    DocumentManager document_manager_;

    // 输入处理系统
    pnana::input::KeyBindingManager key_binding_manager_;
    pnana::input::ActionExecutor action_executor_;

    // 区域管理
    RegionManager region_manager_;

    // 输入和UI路由器（解耦优化）
    std::unique_ptr<pnana::core::input::InputRouter> input_router_;
    std::unique_ptr<pnana::core::ui::UIRouter> ui_router_;

    // UI组件
    pnana::ui::Theme theme_;
    core::ConfigManager config_manager_;
    pnana::ui::Statusbar statusbar_;
    pnana::ui::Helpbar helpbar_;
    pnana::ui::Tabbar tabbar_;
    pnana::ui::Help help_;
    pnana::ui::Dialog dialog_;
    pnana::ui::FilePicker file_picker_;
    pnana::ui::SplitDialog split_dialog_;
    pnana::ui::SSHDialog ssh_dialog_;
    pnana::ui::WelcomeScreen welcome_screen_;
    pnana::ui::NewFilePrompt new_file_prompt_;
    pnana::ui::ThemeMenu theme_menu_;
    pnana::ui::CreateFolderDialog create_folder_dialog_;
    pnana::ui::SaveAsDialog save_as_dialog_;
    pnana::ui::CursorConfigDialog cursor_config_dialog_;
    pnana::ui::BinaryFileView binary_file_view_;
    pnana::ui::EncodingDialog encoding_dialog_;
#ifdef BUILD_LUA_SUPPORT
    pnana::ui::PluginManagerDialog plugin_manager_dialog_;
#endif

    // 功能模块
    features::SearchEngine search_engine_;
    features::FileBrowser file_browser_;
#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
    features::ImagePreview image_preview_;
#endif
    features::SyntaxHighlighter syntax_highlighter_;
    features::CommandPalette command_palette_;
    features::Terminal terminal_;
    features::SplitViewManager split_view_manager_;

#ifdef BUILD_LSP_SUPPORT
    // LSP 服务器管理器（支持多语言）
    std::unique_ptr<features::LspServerManager> lsp_manager_;
    pnana::ui::CompletionPopup completion_popup_;
    bool lsp_enabled_;
    std::map<std::string, std::string> file_language_map_; // 文件路径 -> 语言ID
    std::string last_completion_trigger_;                  // 上次触发补全的字符
    int completion_trigger_delay_;                         // 补全触发延迟计数

    // URI 缓存（阶段1优化）
    std::map<std::string, std::string> uri_cache_; // filepath -> uri
    std::mutex uri_cache_mutex_;                   // 保护 URI 缓存的互斥锁

    // 文档更新防抖（阶段1优化）
    std::chrono::steady_clock::time_point last_document_update_time_;
    std::chrono::milliseconds document_update_debounce_interval_{200}; // 200ms 防抖间隔
    std::string pending_document_uri_;
    std::string pending_document_content_;
    int pending_document_version_;
    std::mutex document_update_mutex_;

    // 补全防抖（阶段1优化）
    std::chrono::steady_clock::time_point last_completion_trigger_time_;
    std::chrono::milliseconds completion_debounce_interval_{
        100}; // 100ms 防抖间隔（参考 VSCode，快速响应）
    std::mutex completion_debounce_mutex_;

    // 异步请求管理器（阶段2优化）
    std::unique_ptr<features::LspAsyncManager> lsp_async_manager_;

    // 文档变更跟踪器（阶段2优化）
    std::unique_ptr<features::DocumentChangeTracker> document_change_tracker_;

    // 补全缓存（阶段2优化）
    std::unique_ptr<features::LspCompletionCache> completion_cache_;
#endif
#ifdef BUILD_LUA_SUPPORT
    // 插件管理器
    std::unique_ptr<plugins::PluginManager> plugin_manager_;
#endif

    // 编辑器状态
    EditorMode mode_;
    size_t cursor_row_;
    size_t cursor_col_;
    size_t view_offset_row_;
    size_t view_offset_col_;

    // 主题选择
    bool show_theme_menu_;

    // 帮助窗口
    bool show_help_;

    // 创建文件夹弹窗
    bool show_create_folder_;

    // 另存为弹窗
    bool show_save_as_;

    // 选择
    bool selection_active_;
    size_t selection_start_row_;
    size_t selection_start_col_;

    // 显示选项
    bool show_line_numbers_;
    bool relative_line_numbers_;
    bool syntax_highlighting_;
    int zoom_level_;
    int file_browser_width_; // 文件浏览器宽度
    int terminal_height_;    // 终端高度（行数）

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

    // UI渲染
    ftxui::Element renderUI();
    ftxui::Element renderUILegacy();                       // 原有的UI渲染逻辑（后备）
    ftxui::Element overlayDialogs(ftxui::Element main_ui); // 叠加对话框
    ftxui::Element renderTabbar();
    ftxui::Element renderEditor();
    ftxui::Element renderSplitEditor(); // 分屏编辑器渲染
    ftxui::Element renderEditorRegion(const features::ViewRegion& region,
                                      Document* doc); // 渲染单个区域
    ftxui::Element renderLine(size_t line_num, bool is_current);
    ftxui::Element renderLineNumber(size_t line_num, bool is_current);
    ftxui::Element renderStatusbar();
    ftxui::Element renderHelpbar();
    ftxui::Element renderInputBox();
    ftxui::Element renderFileBrowser();
    ftxui::Element renderHelp();
    ftxui::Element renderCommandPalette(); // 命令面板
    ftxui::Element renderTerminal();       // 终端
    ftxui::Element renderFilePicker();     // 文件选择器

    // 辅助方法
    void adjustCursor();
    void adjustViewOffset();
    void adjustViewOffsetForUndo(size_t target_row, size_t target_col);
    void setStatusMessage(const std::string& message);
    std::string getFileType() const;
    void executeSearch(bool move_cursor = true);
    void executeReplace();

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

    // SSH 远程文件编辑
    void showSSHDialog();
    void handleSSHConnect(const pnana::ui::SSHConfig& config);

    // 标签页管理
    void closeCurrentTab();
    void switchToNextTab();
    void switchToPreviousTab();
    void switchToTab(size_t index);

    // 帮助系统
    void toggleHelp();

    // 光标配置
    void openCursorConfig();
    void openEncodingDialog();
    void applyCursorConfig();

#ifdef BUILD_LUA_SUPPORT
    // 插件管理
    void openPluginManager();
#endif

    // 获取光标配置（用于渲染）
    ::pnana::ui::CursorStyle getCursorStyle() const;
    ftxui::Color getCursorColor() const;
    int getCursorBlinkRate() const;
    bool getCursorSmooth() const;

    // 渲染光标元素
    ftxui::Element renderCursorElement(const std::string& cursor_char, size_t cursor_pos,
                                       size_t line_length) const;

    // 命令面板
    void openCommandPalette();
    void handleCommandPaletteInput(ftxui::Event event);
    void initializeCommandPalette();

    // 文件选择器
    void handleFilePickerInput(ftxui::Event event);

    // 编码对话框
    void handleEncodingDialogInput(ftxui::Event event);
    void convertFileEncoding(const std::string& new_encoding);

#ifdef BUILD_LSP_SUPPORT
    // LSP 相关方法
    void initializeLsp();
    void shutdownLsp();
    void cleanupLocalCacheFiles();
    std::string detectLanguageId(const std::string& filepath);
    std::string filepathToUri(const std::string& filepath);
    void triggerCompletion();
    void handleCompletionInput(ftxui::Event event);
    void applyCompletion();
    void updateLspDocument();
    ftxui::Element renderCompletionPopup();
#endif

    // 获取当前文档（便捷方法）
    Document* getCurrentDocument();
    const Document* getCurrentDocument() const;

#ifdef BUILD_LUA_SUPPORT
    // 插件系统
    void initializePlugins();
    plugins::PluginManager* getPluginManager() {
        return plugin_manager_.get();
    }
#endif
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_EDITOR_H
