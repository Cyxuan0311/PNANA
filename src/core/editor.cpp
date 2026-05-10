// 编辑器核心类实现
#include "core/editor.h"
#include "core/input/input_router.h"
#include "core/ui/ui_router.h"
#include "features/encoding_converter.h"
#include "features/package_manager/apt_manager.h"
#include "features/package_manager/brew_manager.h"
#include "features/package_manager/cargo_manager.h"
#include "features/package_manager/conda_manager.h"
#include "features/package_manager/go_mod_manager.h"
#include "features/package_manager/npm_manager.h"
#include "features/package_manager/package_manager_registry.h"
#include "features/package_manager/pacman_manager.h"
#include "features/package_manager/pip_manager.h"
#include "features/package_manager/yarn_manager.h"
#include "features/package_manager/yum_manager.h"
#include "ui/icons.h"
#include "ui/statusbar_theme.h"
#include "ui/toast.h"
#include "utils/file_type_detector.h"
#include "utils/logger.h"
#ifdef BUILD_LUA_SUPPORT
#include "plugins/plugin_manager.h"
#endif
#ifdef BUILD_AI_CLIENT_SUPPORT
#include "features/ai_client/ai_client.h"
#include "features/ai_client/assistant_system_prompt.h"
#include "features/ai_config/ai_config.h"
#endif
#include "features/logo_manager.h"
#include "features/md_render/markdown_renderer.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <sstream>
#include <streambuf>

using namespace ftxui;

namespace pnana {
namespace core {

// 定义折叠缓存持续时间常量
const std::chrono::minutes Editor::FOLDING_CACHE_DURATION = std::chrono::minutes(30);

// 构造函数
Editor::Editor()
    : document_manager_(), key_binding_manager_(), action_executor_(this),
      overlay_manager_(std::make_unique<pnana::core::OverlayManager>()), theme_(),
      popup_manager_(std::make_unique<pnana::core::ui::PopupManager>(&theme_)), statusbar_(theme_),
      helpbar_(theme_), tabbar_(theme_, config_manager_), help_(theme_), dialog_(theme_),
      file_picker_(theme_), split_dialog_(theme_), ssh_dialog_(theme_),
      ssh_transfer_dialog_(theme_), terminal_session_dialog_(theme_),
      welcome_screen_(theme_, config_manager_), split_welcome_screen_(theme_),
      new_file_prompt_(theme_), theme_menu_(theme_), logo_menu_(theme_), animation_menu_(theme_),
      create_folder_dialog_(theme_), save_as_dialog_(theme_), move_file_dialog_(theme_),
      cursor_config_dialog_(theme_), binary_file_view_(theme_), encoding_dialog_(theme_),
      format_dialog_(theme_), recent_files_popup_(theme_), fzf_popup_(theme_),
      history_timeline_popup_(theme_), history_diff_popup_(theme_), tui_config_popup_(theme_),
      extract_dialog_(theme_), extract_path_dialog_(theme_), extract_progress_dialog_(theme_),
      ai_assistant_panel_(theme_), clipboard_panel_(theme_), ai_config_dialog_(theme_),
      todo_panel_(theme_), package_manager_panel_(theme_),
#ifdef BUILD_LUA_SUPPORT
      plugin_manager_dialog_(theme_, nullptr), // 将在 initializePluginManager 中设置
#endif
      git_panel_(theme_), statusbar_style_menu_(statusbar_, theme_), search_engine_(),
      file_browser_(theme_), search_highlight_active_(false), word_highlight_active_(false),
      current_word_(""), word_highlight_row_(0), word_highlight_col_(0),
#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
      image_preview_(),
#endif
      syntax_highlighter_(theme_),
#ifdef BUILD_TREE_SITTER_SUPPORT
      auto_indent_engine_(config_manager_),
#endif
      command_palette_(theme_), terminal_(theme_), split_view_manager_(),
      diagnostics_popup_(theme_),
#ifdef BUILD_LSP_SUPPORT
      symbol_navigation_popup_(theme_), lsp_status_popup_(theme_),
#endif
      mode_(EditorMode::NORMAL), cursor_row_(0), cursor_col_(0), view_offset_row_(0),
      view_offset_col_(0), show_theme_menu_(false), show_logo_menu_(false),
      show_animation_menu_(false), show_statusbar_style_menu_(false), show_help_(false),
      show_create_folder_(false), show_save_as_(false), show_move_file_(false),
      show_extract_dialog_(false), show_extract_path_dialog_(false),
      show_extract_progress_dialog_(false), selection_active_(false), selection_start_row_(0),
      selection_start_col_(0), show_line_numbers_(true), relative_line_numbers_(false),
      show_helpbar_(true), syntax_highlighting_(true), zoom_level_(0),
      file_browser_width_(35), // 默认宽度35列
      terminal_height_(0),     // 0 表示使用默认值（屏幕高度的1/3）
      input_buffer_(""), search_input_(""), replace_input_(""), search_cursor_pos_(0),
      replace_cursor_pos_(0), current_search_match_(0), total_search_matches_(0),
      current_option_index_(0), search_options_{false, false, false, false},
      status_message_(
          "pnana - Modern Terminal Editor | Ctrl+Q Quit | Ctrl+T Themes | Ctrl+O Files | F1 Help"),
      should_quit_(false), force_ui_update_(false), render_call_count_(0), undo_operation_count_(0),
      last_debug_stats_time_(std::chrono::steady_clock::now()), rendering_paused_(false),
      needs_render_(false), last_call_time_(std::chrono::steady_clock::now()),
      last_render_time_(std::chrono::steady_clock::now()), pending_cursor_update_(false),
      screen_(ScreenInteractive::Fullscreen()) {
    // 初始化 last_rendered_element_ 为有效的 ftxui 元素，避免空元素导致的崩溃
    last_rendered_element_ = ftxui::text("Initializing...");
    // 确保首次调用 renderUI() 时不会被增量渲染逻辑跳过，强制进行一次完整渲染
    force_ui_update_ = true;

    // 加载配置文件（使用默认路径）
    loadConfig();

    // 应用文件浏览器 UI 配置
    {
        const auto& display_cfg = config_manager_.getConfig().display;
        file_browser_.setFileBrowserViewConfig(display_cfg.file_browser_show_tree_style);
    }

    // 根据配置设置 AI 面板位置（左/右）
    if (overlay_manager_) {
        const auto& display_cfg = config_manager_.getConfig().display;
        bool ai_on_left = (display_cfg.ai_panel_side == "left");
        overlay_manager_->setAIPanelOnLeft(ai_on_left);
    }

    // 文件浏览器延迟加载：不在构造函数中加载目录，只在首次显示时才加载
    // 这样可以避免启动时扫描大目录导致的延迟

    // 包管理器注册表延迟加载：在首次打开包管理面板时才初始化
    // 避免启动时创建 11 个包管理器实例造成的延迟

    // 初始化命令面板
    initializeCommandPalette();

    // 状态栏样式确认时持久化到配置
    statusbar_style_menu_.setOnStyleConfirmed([this](const std::string& name) {
        config_manager_.getConfig().display.statusbar_style = name;
        config_manager_.saveConfig();
    });

    // 初始化最近文件管理器
    recent_files_manager_.setFileOpenCallback([this](const std::string& filepath) {
        this->openFile(filepath);
    });

    // 初始化最近文件弹窗
    recent_files_popup_.setFileOpenCallback([this](const std::string& filepath) {
        this->openFile(filepath);
    });
    recent_files_popup_.setFolderOpenCallback([this](const std::string& folderpath) {
        // 打开文件夹：更新文件浏览器的当前目录
        if (file_browser_.openDirectory(folderpath)) {
            // 添加到最近文件夹列表
            recent_files_manager_.addFolder(folderpath);
            setStatusMessage("Changed to directory: " + folderpath);
        } else {
            setStatusMessage("Failed to open directory: " + folderpath);
        }
    });
    // 设置 Toast 指针，用于显示操作提示
    recent_files_popup_.setToast(&toast_);

    // 初始化 FZF 模糊文件查找弹窗
    fzf_popup_.setFileOpenCallback([this](const std::string& filepath) {
        this->openFile(filepath);
    });
    fzf_popup_.setCursorColorGetter([this]() {
        return getCursorColor();
    });
    fzf_popup_.setOnLoadComplete([this](std::vector<std::string> files,
                                        std::vector<std::string> display_paths,
                                        std::string root_path) {
        screen_.Post([this, files = std::move(files), display_paths = std::move(display_paths),
                      root_path = std::move(root_path)]() mutable {
            fzf_popup_.receiveFiles(std::move(files), std::move(display_paths),
                                    std::move(root_path));
            force_ui_update_ = true;
            screen_.PostEvent(Event::Custom);
        });
    });
    fzf_popup_.setOnRemoteLoad([this](const std::string& ssh_uri) {
        onFzfRemoteLoad(ssh_uri);
    });

    history_timeline_popup_.setOnPreview([this](int version) {
        Document* doc = getCurrentDocument();
        if (!doc || doc->getFilePath().empty()) {
            setStatusMessage("No current file to preview");
            return;
        }

        auto versions = file_history_manager_.listVersions(doc->getFilePath());
        if (versions.empty()) {
            setStatusMessage("Preview failed: no history versions");
            return;
        }

        int latest_version = 0;
        for (const auto& v : versions) {
            if (v.version > latest_version) {
                latest_version = v.version;
            }
        }

        std::vector<pnana::features::diff::DiffRecord> records;
        if (!file_history_manager_.diffBetweenVersions(doc->getFilePath(), version, latest_version,
                                                       records)) {
            setStatusMessage("Preview failed: cannot generate diff");
            return;
        }

        history_diff_popup_.open(doc->getFilePath(), version, latest_version, records);
        setStatusMessage("History diff preview v" + std::to_string(version) + " -> v" +
                         std::to_string(latest_version) + " | PgUp/PgDn page, Esc back");
    });

    history_timeline_popup_.setOnRollback([this](int version) {
        Document* doc = getCurrentDocument();
        if (!doc || doc->getFilePath().empty()) {
            setStatusMessage("No current file to rollback");
            toast_.showError("No current file to rollback");
            return;
        }

        std::vector<std::string> lines;
        if (!file_history_manager_.restoreVersion(doc->getFilePath(), version, lines)) {
            setStatusMessage("Rollback failed: cannot restore selected version");
            toast_.showError("Rollback failed: cannot restore selected version");
            return;
        }

        doc->getLines() = lines;
        doc->setModified(true);
        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0;
        view_offset_col_ = 0;

        bool saved = saveFile();
        history_timeline_popup_.close();
        if (saved) {
            setStatusMessage("Rollback done: restored to version v" + std::to_string(version));
            toast_.showSuccess("Rollback done: restored to version v" + std::to_string(version));
        } else {
            setStatusMessage("Rollback applied in buffer (save failed), version v" +
                             std::to_string(version));
            toast_.showWarning("Rollback applied in buffer (save failed), version v" +
                               std::to_string(version));
        }
    });

    // 终端输出时触发 UI 刷新（PTY 后台线程写入输出后，FTXUI 需 PostEvent 才能重绘）
    // 必须设置 force_ui_update_=true，否则 renderUI 的渲染去抖可能跳过重绘，导致新输出不显示
    // PTY 线程直接调用 screen_.PostEvent（线程安全），触发主线程重绘。
    // 不能在 screen_.Post 回调里再调用 screen_.PostEvent，ftxui 会死锁（持锁重入）。
    terminal_.setOnOutputAdded([this]() {
        terminal_has_output_.store(true, std::memory_order_relaxed);
        screen_.PostEvent(Event::Custom);
    });

    // 用户输入 exit 导致 shell 退出时，关闭终端面板并切回代码区
    terminal_.setOnShellExit([this]() {
        screen_.Post([this]() {
            if (!terminal_.isVisible()) {
                return;
            }
            terminal_.setVisible(false);
            region_manager_.setTerminalEnabled(false);
            if (region_manager_.getCurrentRegion() == EditorRegion::TERMINAL) {
                region_manager_.setRegion(EditorRegion::CODE_AREA);
            }
            setStatusMessage("Terminal closed (shell exited)");
        });
    });

    // 设置文档切换回调，优化LSP诊断响应速度
    document_manager_.setDocumentSwitchedCallback([this](size_t /*old_index*/, size_t new_index) {
        try {
            if (new_index >= document_manager_.getDocumentCount()) {
                return;
            }

            Document* new_doc = document_manager_.getDocument(new_index);
            if (!new_doc) {
                return;
            }

            // 切换文档时更新语法高亮，避免沿用上一个文件的语法规则
            syntax_highlighter_.setFileType(getFileType());
#ifdef BUILD_TREE_SITTER_SUPPORT
            auto_indent_engine_.setFileType(getFileType());
#endif

            std::string filepath = new_doc->getFilePath();

            if (filepath.empty()) {
#ifdef BUILD_LSP_SUPPORT
                updateCurrentFileDiagnostics();
                updateCurrentFileFolding();
#endif
                needs_render_ = true;
                last_render_source_ = "document_switch";
                return;
            }

#ifdef BUILD_LSP_SUPPORT
            // 先同步 LSP 文档并可能替换 folding_manager_（切换不同语言时需对应 client）
            updateLspDocument(/* force_sync */ true);
#endif
            updateCurrentFileDiagnostics();
            updateCurrentFileFolding();
            preloadAdjacentDocuments(new_index);

            needs_render_ = true;
            last_render_source_ = "document_switch";
        } catch (...) {
        }
    });

    // 初始化TUI配置管理器
    tui_config_manager_.setConfigOpenCallback([this](const std::string& filepath) {
        this->openFile(filepath);
    });

    // 初始化TUI配置弹窗
    tui_config_popup_.setCursorColorGetter([this]() {
        return getCursorColor();
    });
    tui_config_popup_.setConfigOpenCallback([this](const features::TUIConfig& config) {
        this->tui_config_manager_.openConfig(config);
    });

    // 初始化AI助手
    initializeAIAssistant();

    // 初始化剪贴板面板回调
    clipboard_panel_.setOnInsertText([this](const std::string& text) {
        insertCodeAtCursor(text);
    });

    // 初始化输入和UI路由器（解耦优化）
    input_router_ = std::make_unique<pnana::core::input::InputRouter>();
    ui_router_ = std::make_unique<pnana::core::ui::UIRouter>();

    // 日志系统不会在这里自动初始化
    // 只有在 main.cpp 中指定 -l/--log 参数时才会初始化

#ifdef BUILD_LSP_SUPPORT
    // 初始化 LSP 客户端
    initializeLsp();
    // 注意：lsp_enabled_ 在 initializeLsp() 中已经设置，不要重置为 false
    completion_trigger_delay_ = 0;
    // 设置文档更新防抖时间为300ms，避免过于频繁的LSP更新
    document_update_debounce_interval_ = std::chrono::milliseconds(300);

    // 清理和迁移本地缓存文件到配置目录
    cleanupLocalCacheFiles();
#endif

#ifdef BUILD_LUA_SUPPORT
    // 启动即初始化插件系统：进入主界面时可立即使用插件能力
    // （包括命令面板中的插件命令）
    initializePlugins();
#endif
    // 启动后台动画/闪烁刷新调度：
    // - 光标闪烁开启时持续触发重绘
    // - 欢迎页显示时持续触发重绘（保证 logo 动画无需用户输入也能播放）
    ui_refresh_scheduler_.start(
        [this]() {
            bool blink_on = false;
            int rate = 0;
            bool should_animate_welcome = false;
            bool animation_enabled = true;
            try {
                blink_on = cursor_config_dialog_.getBlinkEnabled();
                rate = getCursorBlinkRate();
                should_animate_welcome = (getDocumentForActiveRegion() == nullptr);
                animation_enabled = config_manager_.getConfig().animation.enabled;
            } catch (...) {
                return false;
            }

            return !rendering_paused_ &&
                   ((blink_on && rate > 0) || (should_animate_welcome && animation_enabled));
        },
        [this]() {
            screen_.PostEvent(ftxui::Event::Custom);
        },
        std::chrono::milliseconds(config_manager_.getConfig().animation.refresh_interval_ms));

    // Start todo reminder detection thread (periodically check for due todos and trigger UI updates
    // to ensure blinking effect is visible)
    std::thread([this]() {
        using namespace std::chrono_literals;
        while (!should_quit_) {
            std::this_thread::sleep_for(
                250ms); // 250ms check interval, coordinated with blink frequency (300ms)

            try {
                // Check if there are any due todos
                auto due_todos = todo_panel_.getTodoManager().getDueTodos();
                if (!due_todos.empty() && !rendering_paused_) {
                    // There are due todos, trigger UI update to show blinking effect
                    screen_.PostEvent(ftxui::Event::Custom);
                }
            } catch (...) {
                // Avoid exceptions interrupting the thread
                continue;
            }
        }
    }).detach();
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

Editor::~Editor() {
    config_manager_.stopWatching();

    // 先取消解压操作，避免析构时线程仍在运行并访问已销毁的成员
    if (extract_manager_.isExtracting()) {
        extract_manager_.cancelExtraction();
    }
#ifdef BUILD_LSP_SUPPORT
    // 先关闭所有 LSP 连接，使 LspRequestManager worker 中可能阻塞的 documentSymbol() 等立即返回，
    // 避免析构时 join(worker) 一直等待导致进程无法退出
    if (lsp_manager_) {
        lsp_manager_->shutdownAll();
    }
#endif
}

void Editor::run() {
    // 强制隐藏宿主终端光标，避免与编辑器内部光标叠加闪烁。
    // 使用 RAII 确保无论正常退出或异常路径都恢复可见性。
    struct TerminalCursorGuard {
        TerminalCursorGuard() {
            std::cout << "\x1b[?25l" << std::flush;
        }
        ~TerminalCursorGuard() {
            std::cout << "\x1b[?25h" << std::flush;
        }
    };

    TerminalCursorGuard terminal_cursor_guard;
    main_component_ = CatchEvent(Renderer([this] {
                                     // 每帧渲染时再次隐藏宿主光标，压住 FTXUI 可能输出的
                                     // \x1b[?25h。 这里不记录 ANSI 日志，避免高频渲染导致日志爆炸。
                                     std::cout << "\x1b[?25l" << std::flush;
                                     return renderUI();
                                 }),
                                 [this](Event event) {
                                     handleInput(event);
                                     return true;
                                 });
    // 禁止 FTXUI 对 Ctrl+C / Ctrl+Z 的内置强制处理。
    // FTXUI 默认 force_handle_ctrl_c_ = true / force_handle_ctrl_z_ = true，
    // 这意味着即使 CatchEvent 返回 true（事件已被消费），FTXUI 仍会：
    //   Ctrl+C → 调用 ExitLoopClosure() 退出事件循环
    //   Ctrl+Z → 内部调用 raise(SIGTSTP) 挂起进程
    // 设为 false 后，事件完全交由编辑器自身的 handleInput() 处理：
    //   Ctrl+C → 编辑器的复制/撤销逻辑
    //   Ctrl+Z → 编辑器的撤销逻辑
    screen_.ForceHandleCtrlC(false);
    screen_.ForceHandleCtrlZ(false);

    // Post a Custom event so ftxui performs an initial render immediately
    // (ensures renderUI() runs once even if incremental-render logic would skip it)
    screen_.PostEvent(Event::Custom);
    screen_.Loop(main_component_);

#ifdef BUILD_LSP_SUPPORT
    // 清理 LSP 客户端
    shutdownLsp();
#endif
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

    // 更新配置并保存
    auto& config = config_manager_.getConfig();
    config.current_theme = theme_name;
    config.editor.theme = theme_name;

    // 保存配置到文件
    if (config_manager_.saveConfig()) {
        setStatusMessage("✓ Theme: " + theme_name + " (saved)");
    } else {
        setStatusMessage("Theme: " + theme_name + " (save failed)");
    }
}

void Editor::loadConfig(const std::string& config_path) {
    config_manager_.loadConfig(config_path);
    applyLoadedConfig();

    if (!config_watcher_registered_) {
        config_watcher_registered_ = true;
        config_manager_.registerChangeCallback([this]() {
            screen_.Post([this]() {
                applyLoadedConfig();
                setStatusMessage("Config hot-reloaded from disk");
            });
        });
        config_manager_.startWatching();
    }
}

void Editor::applyLoadedConfig() {
    // 注入配置中的自定义 Logo（供欢迎页与 Logo 菜单使用）
    features::LogoManager::setCustomLogos(config_manager_.getConfig().custom_logos);

    // 从配置获取主题名称并应用
    const auto& config = config_manager_.getConfig();
    std::string theme_name = config.current_theme;
    if (theme_name.empty()) {
        theme_name = config.editor.theme;
    }
    if (theme_name.empty()) {
        theme_name = "monokai"; // 默认主题
    }

    // 检查主题是否可用（预设主题、配置声明的主题或已加载插件提供的主题）
    std::vector<std::string> check_available_themes = ::pnana::ui::Theme::getAvailableThemes();
    std::vector<std::string> check_custom_themes = theme_.getCustomThemeNames();
    check_available_themes.insert(check_available_themes.end(), check_custom_themes.begin(),
                                  check_custom_themes.end());
    if (!config.available_themes.empty()) {
        check_available_themes.insert(check_available_themes.end(), config.available_themes.begin(),
                                      config.available_themes.end());
    }

    bool theme_available = false;
    for (const auto& available_theme : check_available_themes) {
        if (available_theme == theme_name) {
            theme_available = true;
            break;
        }
    }

    // 如果主题不可用（比如插件被禁用了），重置为默认主题
    if (!theme_available) {
        theme_name = "monokai";
        // 更新配置
        config_manager_.getConfig().current_theme = theme_name;
        config_manager_.getConfig().editor.theme = theme_name;
        config_manager_.saveConfig();
    }

    // 先清空自定义主题，再从配置加载（插件提供的主题会在其加载时注册）
    theme_.clearCustomThemes();
    for (const auto& [name, tcfg] : config.custom_themes) {
        theme_.loadThemeFromConfig(
            name, tcfg.background, tcfg.foreground, tcfg.current_line, tcfg.selection,
            tcfg.line_number, tcfg.line_number_current, tcfg.statusbar_bg, tcfg.statusbar_fg,
            tcfg.menubar_bg, tcfg.menubar_fg, tcfg.helpbar_bg, tcfg.helpbar_fg, tcfg.helpbar_key,
            tcfg.keyword, tcfg.string, tcfg.comment, tcfg.number, tcfg.function, tcfg.type,
            tcfg.operator_color, tcfg.error, tcfg.warning, tcfg.info, tcfg.success,
            /*dialog_bg*/ tcfg.background,
            /*dialog_fg*/ tcfg.foreground,
            /*dialog_title_bg*/ tcfg.statusbar_bg,
            /*dialog_title_fg*/ tcfg.statusbar_fg,
            /*dialog_border*/ tcfg.line_number);
    }

    // 设置当前主题
    theme_.setTheme(theme_name);

    // 更新可用主题列表：优先使用配置中的 available；否则用内置列表
    std::vector<std::string> available_themes;
    if (!config.available_themes.empty()) {
        available_themes = config.available_themes;
    } else {
        available_themes = ::pnana::ui::Theme::getAvailableThemes();
    }
    // 把通过配置加载的自定义主题名也追加进去，让主题面板可选
    {
        std::vector<std::string> custom_names = theme_.getCustomThemeNames();
        available_themes.insert(available_themes.end(), custom_names.begin(), custom_names.end());
    }

    theme_menu_.setAvailableThemes(available_themes);
    theme_menu_.setCursorColorGetter([this]() {
        return getCursorColor();
    });

    // 应用 display 配置
    show_line_numbers_ = config.display.show_line_numbers;
    relative_line_numbers_ = config.display.relative_line_numbers;
    show_helpbar_ = config.display.show_helpbar;

    // 应用状态栏样式（持久化）
    {
        std::string sb_style = config.display.statusbar_style;
        statusbar_.setBeautifyConfig(pnana::ui::getStatusbarConfigForStyle(sb_style));
    }

    // 应用 search 配置（默认搜索选项）
    search_options_[0] = config.search.case_sensitive;
    search_options_[1] = config.search.whole_word;
    search_options_[2] = config.search.regex;
    search_options_[3] = config.search.wrap_around;
    current_search_options_.case_sensitive = config.search.case_sensitive;
    current_search_options_.whole_word = config.search.whole_word;
    current_search_options_.regex = config.search.regex;
    current_search_options_.wrap_around = config.search.wrap_around;

    // 应用 UI 配置
    pnana::ui::Toast::setEnabled(config.ui.toast_enabled);

    {
        using pnana::ui::ToastStyle;
        ToastStyle style = ToastStyle::CLASSIC;
        const std::string& style_str = config.ui.toast_style;
        if (style_str == "minimal") {
            style = ToastStyle::MINIMAL;
        } else if (style_str == "solid") {
            style = ToastStyle::SOLID;
        } else if (style_str == "accent") {
            style = ToastStyle::ACCENT;
        } else if (style_str == "outline") {
            style = ToastStyle::OUTLINE;
        }
        pnana::ui::Toast::setDefaultStyle(style);
        pnana::ui::Toast::setDefaultDurationMs(config.ui.toast_duration_ms);
        pnana::ui::Toast::setDefaultMaxWidth(config.ui.toast_max_width);
        pnana::ui::Toast::setDefaultShowIcon(config.ui.toast_show_icon);
        pnana::ui::Toast::setDefaultBoldText(config.ui.toast_bold_text);
    }

    // 应用最近项目数量限制
    recent_files_manager_.max_recent_files =
        static_cast<size_t>(std::max(1, config.ui.max_recent_files));
    recent_files_manager_.max_recent_folders =
        static_cast<size_t>(std::max(1, config.ui.max_recent_folders));

    // 应用 history 保留配置
    {
        pnana::features::history::HistoryRetentionConfig hcfg;
        hcfg.enable = config.history.enable;
        hcfg.max_entries = config.history.max_entries;
        hcfg.max_age_days = config.history.max_age_days;
        hcfg.max_total_size = config.history.max_total_size;
        hcfg.keep_critical_versions = config.history.keep_critical_versions;
        hcfg.critical_change_threshold = config.history.critical_change_threshold;
        hcfg.critical_time_interval = config.history.critical_time_interval;
        file_history_manager_.setRetentionConfig(hcfg);
    }

    // 加载光标配置
    const auto& display_config = config.display;
    if (!display_config.cursor_style.empty()) {
        ::pnana::ui::CursorStyle style = ::pnana::ui::CursorStyle::BLOCK;
        if (display_config.cursor_style == "underline") {
            style = ::pnana::ui::CursorStyle::UNDERLINE;
        } else if (display_config.cursor_style == "bar") {
            style = ::pnana::ui::CursorStyle::BAR;
        } else if (display_config.cursor_style == "hollow") {
            style = ::pnana::ui::CursorStyle::HOLLOW;
        }
        cursor_config_dialog_.setCursorStyle(style);
    }
    if (!display_config.cursor_color.empty()) {
        cursor_config_dialog_.setCursorColor(display_config.cursor_color);
    }
    cursor_config_dialog_.setBlinkRate(display_config.cursor_blink_rate);
    cursor_config_dialog_.setSmoothCursor(display_config.cursor_smooth);

    // 设置应用回调
    cursor_config_dialog_.setOnApply([this]() {
        applyCursorConfig();
    });

#ifdef BUILD_LSP_SUPPORT
    // 重新加载 LSP 服务器配置
    if (lsp_manager_) {
        lsp_manager_->getConfigManager().loadFromConfig(config.lsp);
    }
#endif
}

void Editor::openCursorConfig() {
    cursor_config_dialog_.open();
    setStatusMessage(
        "Cursor Configuration | ↑↓: Navigate, ←→: Change Style, Enter: Apply, Esc: Cancel");
}

void Editor::applyCursorConfig() {
    // 获取配置
    auto style = cursor_config_dialog_.getCursorStyle();
    auto color = cursor_config_dialog_.getCursorColor();
    auto rate = cursor_config_dialog_.getBlinkRate();
    auto smooth = cursor_config_dialog_.getSmoothCursor();

    // 更新配置管理器
    auto& config = config_manager_.getConfig();
    std::string style_str = "block";
    switch (style) {
        case ::pnana::ui::CursorStyle::UNDERLINE:
            style_str = "underline";
            break;
        case ::pnana::ui::CursorStyle::BAR:
            style_str = "bar";
            break;
        case ::pnana::ui::CursorStyle::HOLLOW:
            style_str = "hollow";
            break;
        default:
            style_str = "block";
            break;
    }

    config.display.cursor_style = style_str;
    config.display.cursor_color = color;
    config.display.cursor_blink_rate = rate;
    config.display.cursor_smooth = smooth;

    // 保存配置
    if (config_manager_.saveConfig()) {
        setStatusMessage("✓ Cursor configuration saved");
    } else {
        setStatusMessage("Cursor configuration applied (save failed)");
    }

    // 配置已更新，下次渲染时会自动使用新配置
    // FTXUI 会在下一次事件循环时自动重新渲染
}

// 获取光标配置（用于渲染）- 直接使用当前对话框状态，保证配置变更立即生效
::pnana::ui::CursorStyle Editor::getCursorStyle() const {
    // 始终以对话框中的当前值为准（对话框在 loadConfig/applyCursorConfig 中与配置保持同步）
    return cursor_config_dialog_.getCursorStyle();
}

ftxui::Color Editor::getCursorColor() const {
    // 优先从光标配置对话框获取颜色字符串，确保用户调整后立即生效
    std::string color_str = cursor_config_dialog_.getCursorColor();

    // 解析颜色字符串 "R,G,B"
    if (color_str.empty()) {
        return theme_.getColors().foreground; // 默认前景色
    }

    // 移除空格
    color_str.erase(std::remove(color_str.begin(), color_str.end(), ' '), color_str.end());

    std::istringstream iss(color_str);
    std::string token;
    std::vector<int> values;

    while (std::getline(iss, token, ',')) {
        try {
            int value = std::stoi(token);
            if (value < 0)
                value = 0;
            if (value > 255)
                value = 255;
            values.push_back(value);
        } catch (...) {
            return theme_.getColors().foreground; // 解析失败，使用默认
        }
    }

    if (values.size() >= 3) {
        return ftxui::Color::RGB(values[0], values[1], values[2]);
    }

    return theme_.getColors().foreground; // 默认前景色
}

int Editor::getCursorBlinkRate() const {
    // 直接使用对话框中的最新值
    return cursor_config_dialog_.getBlinkRate();
}

bool Editor::getCursorSmooth() const {
    // 直接使用对话框中的最新值
    return cursor_config_dialog_.getSmoothCursor();
}

// 主题菜单
void Editor::toggleThemeMenu() {
    show_theme_menu_ = !show_theme_menu_;

    if (show_theme_menu_) {
        theme_menu_.onMenuOpened(); // 保存当前主题，Escape 时恢复；选中仅预览，Enter 才确认
        // 找到当前主题的索引
        std::string current = theme_.getCurrentThemeName();
        const auto& themes = theme_menu_.getAvailableThemes();
        for (size_t i = 0; i < themes.size(); ++i) {
            if (themes[i] == current) {
                theme_menu_.setSelectedIndex(i);
                break;
            }
        }
        setStatusMessage("↑↓: Preview theme  Enter: Apply  Esc: Cancel");
    }
}

void Editor::toggleStatusbarStyleMenu() {
    show_statusbar_style_menu_ = !show_statusbar_style_menu_;
    if (show_statusbar_style_menu_) {
        setStatusMessage("↑↓: Select style  Enter: Apply  Esc: Cancel");
    }
}

void Editor::selectNextTheme() {
    size_t current_index = theme_menu_.getSelectedIndex();
    const auto& themes = theme_menu_.getAvailableThemes();
    if (themes.empty())
        return;
    size_t next_index = (current_index + 1) % themes.size();
    theme_menu_.setSelectedIndex(next_index);
}

void Editor::selectPreviousTheme() {
    size_t current_index = theme_menu_.getSelectedIndex();
    const auto& themes = theme_menu_.getAvailableThemes();
    if (themes.empty())
        return;
    if (current_index == 0) {
        theme_menu_.setSelectedIndex(themes.size() - 1);
    } else {
        theme_menu_.setSelectedIndex(current_index - 1);
    }
}

void Editor::toggleLogoMenu() {
    show_logo_menu_ = !show_logo_menu_;
    if (show_logo_menu_) {
        logo_menu_.setCurrentStyle(config_manager_.getConfig().display.logo_style);
        setStatusMessage("↑↓: Select logo style  Enter: Apply  Esc: Cancel");
    }
}

void Editor::applySelectedLogoStyle() {
    std::string style_id = logo_menu_.getSelectedStyleId();
    if (features::LogoManager::isValidStyle(style_id)) {
        config_manager_.getConfig().display.logo_style = style_id;
        config_manager_.saveConfig();
        setStatusMessage("Logo style set to: " + style_id);
    }
}

void Editor::toggleAnimationMenu() {
    show_animation_menu_ = !show_animation_menu_;
    if (show_animation_menu_) {
        animation_menu_.setConfig(config_manager_.getConfig().animation);
        setStatusMessage(
            "Animation panel | Tab: switch panel, ↑↓: select, ←→: adjust, Enter: apply");
    }
}

void Editor::applySelectedAnimationConfig() {
    config_manager_.getConfig().animation = animation_menu_.getPendingConfig();
    if (config_manager_.saveConfig()) {
        setStatusMessage("✓ Animation config saved");
    } else {
        setStatusMessage("Animation config applied (save failed)");
    }

    ui_refresh_scheduler_.start(
        [this]() {
            bool blink_on = false;
            int rate = 0;
            bool should_animate_welcome = false;
            bool animation_enabled = true;
            try {
                blink_on = cursor_config_dialog_.getBlinkEnabled();
                rate = getCursorBlinkRate();
                should_animate_welcome = (getDocumentForActiveRegion() == nullptr);
                animation_enabled = config_manager_.getConfig().animation.enabled;
            } catch (...) {
                return false;
            }
            return !rendering_paused_ &&
                   ((blink_on && rate > 0) || (should_animate_welcome && animation_enabled));
        },
        [this]() {
            screen_.PostEvent(ftxui::Event::Custom);
        },
        std::chrono::milliseconds(config_manager_.getConfig().animation.refresh_interval_ms));
}

void Editor::applySelectedTheme() {
    // 使用 getSelectedThemeName() 获取当前选中的主题名称（支持过滤后的列表）
    std::string theme_name = theme_menu_.getSelectedThemeName();

    if (!theme_name.empty()) {
        // 检查主题是否真的可用（预设主题或当前加载插件提供的主题）
        std::vector<std::string> available_themes = ::pnana::ui::Theme::getAvailableThemes();
        std::vector<std::string> custom_themes = theme_.getCustomThemeNames();

        available_themes.insert(available_themes.end(), custom_themes.begin(), custom_themes.end());

        bool theme_available = false;
        for (const auto& available_theme : available_themes) {
            if (available_theme == theme_name) {
                theme_available = true;
                break;
            }
        }

        if (theme_available) {
            // 使用 setTheme 方法，它会自动保存配置
            setTheme(theme_name);
        } else {
            // 主题不可用，显示错误消息
            setStatusMessage("Theme '" + theme_name + "' is not available (plugin not loaded)");
        }
    }
}

// 文件浏览器
void Editor::toggleFileBrowser() {
    file_browser_.toggle();
    if (file_browser_.isVisible()) {
        // 切换到文件浏览器区域
        region_manager_.setRegion(EditorRegion::FILE_BROWSER);
        setStatusMessage("File Browser opened | Region: " + region_manager_.getRegionName() +
                         " | ↑↓: Navigate, →: Editor, Enter: Open");
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
        setStatusMessage(std::string(pnana::ui::icons::HELP) + " Press Esc or F1 to close help");
    } else {
        setStatusMessage("Help closed");
    }
}

void Editor::toggleMarkdownPreview() {
    // 检查当前文件是否为 Markdown 文件
    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No file open - Cannot preview Markdown");
        return;
    }

    std::string file_path = doc->getFilePath();
    bool is_markdown = false;

    // 检查文件扩展名
    if (!file_path.empty()) {
        std::string ext = "";
        size_t dot_pos = file_path.rfind('.');
        if (dot_pos != std::string::npos) {
            ext = file_path.substr(dot_pos);
            // 转换为小写
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            // 检查是否为 Markdown 扩展名
            is_markdown = (ext == ".md" || ext == ".markdown" || ext == ".mkd" || ext == ".mdown");
        }
    }

    // 如果不是 Markdown 文件，提示用户
    if (!is_markdown) {
        setStatusMessage("Not a Markdown file - Preview only supports .md/.markdown files");
        markdown_preview_enabled_ = false;
        force_ui_update_ = true;
        last_render_source_ = "toggleMarkdownPreview";
        return;
    }

    // Toggle lightweight preview flag and request UI update
    markdown_preview_enabled_ = !markdown_preview_enabled_;
    if (markdown_preview_enabled_) {
        setStatusMessage("Markdown preview enabled - Press Alt+W again to close");
    } else {
        setStatusMessage("Markdown preview closed");
    }
    force_ui_update_ = true;
    last_render_source_ = "toggleMarkdownPreview";
}

bool Editor::isMarkdownPreviewActive() const {
    return markdown_preview_enabled_;
}

ftxui::Element Editor::renderMarkdownPreview() {
    // Render preview using MarkdownRenderer directly (lightweight)
    pnana::features::MarkdownRenderConfig cfg;
    int half_width = std::max(10, getScreenWidth() / 2 - 4);
    cfg.max_width = half_width;
    cfg.use_color = true;
    cfg.theme = theme_.getCurrentThemeName();
    pnana::features::MarkdownRenderer renderer(cfg);
    std::string content = getCurrentDocumentContent();
    if (content.empty())
        return ftxui::text("");

    auto elem = renderer.render(content);

    // Diagnostic/fallback: render to an off-screen buffer and check if visible characters exist.
    try {
        int height = std::max(10, getScreenHeight() - 6);
        ftxui::Screen screen(half_width, height);
        ftxui::Render(screen, elem);
        std::string out = screen.ToString();
        // check for any non-space visible characters
        bool has_visible = false;
        for (char c : out) {
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                has_visible = true;
                break;
            }
        }
        if (!has_visible) {
            // Fallback: render plain text (no colors) to ensure content is visible
            Elements lines;
            std::istringstream iss(content);
            std::string line;
            while (std::getline(iss, line)) {
                lines.push_back(ftxui::text(line));
            }
            return vbox(std::move(lines));
        }
    } catch (...) {
        // ignore and return elem
    }

    return elem;
}

std::string Editor::getCurrentDocumentContent() const {
    const Document* doc = getCurrentDocument();
    if (!doc) {
        return "";
    }
    return doc->getContent();
}

// Git 面板
void Editor::toggleGitPanel() {
    git_panel_.toggle();
    if (git_panel_.isVisible()) {
        git_panel_.onShow();
        region_manager_.setGitPanelEnabled(true);
        region_manager_.setRegion(EditorRegion::GIT_PANEL);
        setStatusMessage(std::string(pnana::ui::icons::GIT_BRANCH) +
                         " Git Panel opened | Space: select | s: stage | u: unstage | c: commit | "
                         "b: branch | r: remote");
    } else {
        git_panel_.onHide();
        region_manager_.setGitPanelEnabled(false);
        if (region_manager_.getCurrentRegion() == EditorRegion::GIT_PANEL) {
            region_manager_.setRegion(EditorRegion::CODE_AREA);
        }
        setStatusMessage("Git Panel closed");
    }
}

// 终端
void Editor::toggleTerminal() {
    bool will_be_visible = !terminal_.isVisible();
    if (will_be_visible) {
        // Check if SSH is connected but sessions are empty (SSH session was closed by exit)
        // Must reconnect SSH BEFORE calling setVisible(true), because setVisible calls
        // startShellSession which would create a local shell if sessions are empty
        if (!terminal_.hasActiveSession() && !current_ssh_config_.host.empty()) {
            terminal_.startSSHSession(current_ssh_config_.host, current_ssh_config_.user,
                                      current_ssh_config_.port, current_ssh_config_.key_path,
                                      current_ssh_config_.password);
        }
    }

    // 使用 setVisible 而非 toggle，以便正确调用 startShellSession/stopShellSession
    terminal_.setVisible(will_be_visible);
    if (terminal_.isVisible()) {
        // 启用终端区域（必须先启用，才能切换）
        region_manager_.setTerminalEnabled(true);

        // 切换到终端区域并确保焦点正确
        region_manager_.setRegion(EditorRegion::TERMINAL);

        // 如果终端高度未设置，使用默认值
        if (terminal_height_ <= 0) {
            terminal_height_ = screen_.dimy() / 3;
        }
        setStatusMessage("Terminal opened | Region: " + region_manager_.getRegionName() +
                         " | Use +/- to adjust height, ←→ to switch panels");
    } else {
        // 禁用终端区域
        region_manager_.setTerminalEnabled(false);

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

    // Escape: 关闭终端
    if (event == Event::Escape) {
        terminal_.setVisible(false);
        region_manager_.setRegion(EditorRegion::CODE_AREA);
        setStatusMessage("Terminal closed | Region: " + region_manager_.getRegionName());
        return;
    }

    // 所有其他按键透传到 shell（含 Return/Enter/CtrlM/Character(\n)）
    if (event == Event::Return || event == Event::CtrlM ||
        (event.is_character() && (event.character() == "\n" || event.character() == "\r"))) {
        terminal_.handleKeyEvent("return");
        return;
    }
    if (event == Event::Tab) {
        terminal_.handleKeyEvent("tab");
        return;
    }
    if (event == Event::ArrowUp) {
        terminal_.handleKeyEvent("ArrowUp");
        return;
    }
    if (event == Event::ArrowDown) {
        terminal_.handleKeyEvent("ArrowDown");
        return;
    }
    if (event == Event::ArrowLeft) {
        terminal_.handleKeyEvent("ArrowLeft");
        return;
    }
    if (event == Event::ArrowRight) {
        terminal_.handleKeyEvent("ArrowRight");
        return;
    }
    if (event == Event::Home) {
        terminal_.handleKeyEvent("Home");
        return;
    }
    if (event == Event::End) {
        terminal_.handleKeyEvent("End");
        return;
    }
    if (event == Event::Backspace) {
        terminal_.handleKeyEvent("Backspace");
        return;
    }
    if (event == Event::Delete) {
        terminal_.handleKeyEvent("Delete");
        return;
    }
    if (event == Event::PageUp) {
        terminal_.scrollUp();
        return;
    }
    if (event == Event::PageDown) {
        terminal_.scrollDown();
        return;
    }
    if (event == Event::CtrlC) {
        terminal_.handleKeyEvent("ctrl_c");
        return;
    }
    if (event == Event::CtrlD) {
        terminal_.handleKeyEvent("ctrl_d");
        return;
    }
    if (event == Event::CtrlZ) {
        terminal_.handleKeyEvent("ctrl_z");
        return;
    }
    if (event == Event::CtrlL) {
        terminal_.handleKeyEvent("ctrl_l");
        return;
    }
    if (event == Event::CtrlU) {
        terminal_.handleKeyEvent("ctrl_u");
        return;
    }
    if (event == Event::CtrlK) {
        terminal_.handleKeyEvent("ctrl_k");
        return;
    }
    if (event == Event::CtrlA) {
        terminal_.handleKeyEvent("ctrl_a");
        return;
    }
    if (event == Event::CtrlE) {
        terminal_.handleKeyEvent("ctrl_e");
        return;
    }
    if (event == Event::CtrlW) {
        terminal_.handleKeyEvent("ctrl_w");
        return;
    }
    if (event == Event::CtrlH) {
        terminal_.handleKeyEvent("ctrl_h"); // Ctrl+H = Backspace（部分终端）
        return;
    }
    if (event.is_character()) {
        terminal_.handleKeyEvent(event.character());
        return;
    }
}

// 命令面板
void Editor::openCommandPalette() {
    command_palette_.open();
    setStatusMessage("Command Palette - Type to search, ↑↓ to navigate, Enter to execute");
}

void Editor::toggleAIAssistant() {
    if (ai_assistant_panel_.isVisible()) {
        ai_assistant_panel_.hide();
        region_manager_.setAiPanelEnabled(false);
        if (region_manager_.getCurrentRegion() == EditorRegion::AI_ASSISTANT_PANEL) {
            region_manager_.setRegion(EditorRegion::CODE_AREA);
        }
        setStatusMessage("AI Assistant closed");
    } else {
        ai_assistant_panel_.show();
        region_manager_.setAiPanelEnabled(true);
        setStatusMessage("AI Assistant opened - ←→↑↓: Switch panel | Type and Enter to send");
    }
}

bool Editor::isAIAssistantVisible() const {
    return ai_assistant_panel_.isVisible();
}

ftxui::Element Editor::renderAIAssistantPanel() {
    return ai_assistant_panel_.render();
}

void Editor::toggleClipboardPanel() {
    if (clipboard_panel_.isVisible()) {
        clipboard_panel_.hide();
        setStatusMessage("Clipboard History closed");
    } else {
        clipboard_panel_.show();
        setStatusMessage("Clipboard History opened - ↑↓: Navigate  Enter: Insert  Space: Select  "
                         "d: Delete  Esc: Close");
    }
}

bool Editor::isClipboardPanelVisible() const {
    return clipboard_panel_.isVisible();
}

ftxui::Element Editor::renderClipboardPanel() {
    return clipboard_panel_.render();
}

void Editor::toggleTodoPanel() {
    if (todo_panel_.isVisible()) {
        todo_panel_.hide();
        setStatusMessage("Todo Panel closed");
    } else {
        todo_panel_.show();
        region_manager_.setRegion(EditorRegion::GIT_PANEL); // 使用类似 git panel 的区域
        setStatusMessage("Todo Panel opened - Space: Create | Delete: Remove | F1: Edit Priority");
    }
}

bool Editor::handleTodoPanelInput(ftxui::Event event) {
    if (todo_panel_.isVisible()) {
        return todo_panel_.handleInput(event);
    }
    return false;
}

void Editor::togglePackageManagerPanel() {
    if (package_manager_panel_.isVisible()) {
        package_manager_panel_.hide();
        setStatusMessage("Package Manager Panel closed");
    } else {
        initializePackageManagerRegistry();
        package_manager_panel_.show();
        setStatusMessage(
            "Package Manager Panel opened - ←→: Switch Tab | R/F5: Refresh | Esc: Close");
    }
}

void Editor::initializePackageManagerRegistry() {
    if (package_manager_registry_initialized_) {
        return;
    }
    auto& registry = features::package_manager::PackageManagerRegistry::getInstance();
    registry.registerManager(std::make_shared<features::package_manager::PipManager>());
    registry.registerManager(std::make_shared<features::package_manager::AptManager>());
    registry.registerManager(std::make_shared<features::package_manager::CargoManager>());
    registry.registerManager(std::make_shared<features::package_manager::GoModManager>());
    registry.registerManager(std::make_shared<features::package_manager::NpmManager>());
    registry.registerManager(std::make_shared<features::package_manager::YarnManager>());
    registry.registerManager(std::make_shared<features::package_manager::CondaManager>());
    registry.registerManager(std::make_shared<features::package_manager::PacmanManager>());
    registry.registerManager(std::make_shared<features::package_manager::YumManager>());
    registry.registerManager(std::make_shared<features::package_manager::BrewManager>());
    package_manager_registry_initialized_ = true;
}

bool Editor::handlePackageManagerPanelInput(ftxui::Event event) {
    if (package_manager_panel_.isVisible()) {
        return package_manager_panel_.handleInput(event);
    }
    return false;
}

void Editor::initializeAIAssistant() {
    // 设置AI助手的回调函数
#ifdef BUILD_AI_CLIENT_SUPPORT
    ai_assistant_panel_.setOnSendMessage([this](const std::string& message) {
        handleAIMessage(message);
    });
    ai_config_dialog_.on_save_ = []() {
        pnana::features::ai_client::AIClientManager::getInstance().refreshFromConfig();
    };
#endif

    ai_assistant_panel_.setOnInsertCode([this](const std::string& code) {
        insertCodeAtCursor(code);
    });

    ai_assistant_panel_.setOnReplaceCode([this](const std::string& code) {
        replaceSelectedCode(code);
    });

    ai_assistant_panel_.setOnGetSelectedCode([this]() -> std::string {
        return getSelectedText();
    });

    ai_assistant_panel_.setOnGetCurrentFile([this]() -> std::string {
        Document* doc = getCurrentDocument();
        return doc ? doc->getContent() : "";
    });
}

void Editor::openRecentFilesDialog() {
    auto recent_projects = recent_files_manager_.getRecentProjects();
    if (!recent_projects.empty()) {
        recent_files_popup_.setData(true, recent_projects, 0);
        recent_files_popup_.open();
    }
}

void Editor::openFzfPopup() {
    // 设置根目录：优先使用文件浏览器当前目录（含 Alt+M 切换后的目录），否则当前文档所在目录，最后 .
    std::string root = ".";
    std::string browser_dir = file_browser_.getCurrentDirectory();
    if (!browser_dir.empty() && browser_dir != ".") {
        root = browser_dir; // 用户通过 Alt+M 选文件夹切换后，FZF
                            // 应显示该目录，与文件浏览器是否可见无关
    } else if (getCurrentDocument() && !getCurrentDocument()->getFileName().empty()) {
        try {
            std::filesystem::path p(getCurrentDocument()->getFileName());
            if (std::filesystem::exists(p)) {
                root = std::filesystem::canonical(p).parent_path().string();
            }
        } catch (...) {
        }
    }
    fzf_popup_.setRootDirectory(root);
    fzf_popup_.open();
    setStatusMessage("FZF - Type to filter, ↑↓ navigate, Enter to open");
}

void Editor::handleFzfPopupInput(Event event) {
    if (fzf_popup_.handleInput(event)) {
        if (!fzf_popup_.isOpen()) {
            setStatusMessage("pnana - Modern Terminal Editor | Ctrl+Q Quit | Ctrl+T Themes | "
                             "Ctrl+O Files | F1 Help");
        }
    }
}

void Editor::handleHistoryTimelineInput(Event event) {
    if (history_timeline_popup_.handleInput(event)) {
        if (!history_timeline_popup_.isOpen()) {
            setStatusMessage("pnana - Modern Terminal Editor | Ctrl+Q Quit | Ctrl+T Themes | "
                             "Ctrl+O Files | F1 Help");
        }
    }
}

#ifdef BUILD_LSP_SUPPORT
void Editor::openLspStatusPopup() {
    if (!lsp_manager_)
        return;
    lsp_status_popup_.setStatusProvider([this]() {
        return lsp_manager_->getStatusSnapshot();
    });
    lsp_status_popup_.setPidProvider([this](const std::string& lang_id) {
        return lsp_manager_->getServerPid(lang_id);
    });
    lsp_status_popup_.open();
    setStatusMessage("LSP Status — ↑↓ Select  Esc Close");
}

void Editor::handleLspStatusPopupInput(Event event) {
    if (lsp_status_popup_.handleInput(event)) {
        if (!lsp_status_popup_.isOpen()) {
            setStatusMessage("pnana - Modern Terminal Editor | Ctrl+Q Quit | Ctrl+T Themes | "
                             "Ctrl+O Files | F1 Help");
        }
    }
}
#endif

void Editor::openExtractDialog() {
    std::string current_dir = file_browser_.getCurrentDirectory();
    if (current_dir.empty()) {
        current_dir = ".";
    }

    // 扫描压缩文件
    auto archives = extract_manager_.scanArchiveFiles(current_dir);

    if (archives.empty()) {
        setStatusMessage("No archive files found in current directory");
        return;
    }

    extract_dialog_.setArchiveFiles(archives);
    extract_dialog_.show(
        current_dir,
        [this](const features::ArchiveFile& archive) {
            // 选择文件后，显示路径输入对话框
            std::string default_path = file_browser_.getCurrentDirectory();
            if (default_path.empty()) {
                default_path = ".";
            }
            // 默认解压到以压缩文件名命名的目录
            std::filesystem::path archive_path(archive.name);
            std::string default_extract_dir =
                (std::filesystem::path(default_path) / archive_path.stem()).string();

            show_extract_path_dialog_ = true;
            extract_path_dialog_.show(
                archive.name, default_extract_dir,
                [this, archive](const std::string& extract_path, const std::string& extract_name) {
                    // 组合路径和文件名
                    std::filesystem::path final_path(extract_path);
                    if (!extract_name.empty()) {
                        final_path = final_path / extract_name;
                    }
                    std::string full_extract_path = final_path.string();

                    // 显示进度对话框
                    show_extract_path_dialog_ = false;
                    show_extract_progress_dialog_ = true;
                    extract_progress_dialog_.show(archive.name, full_extract_path);

                    // 异步执行解压
                    extract_manager_.extractArchiveAsync(
                        archive.path, full_extract_path,
                        [this](float progress) {
                            // 更新进度
                            extract_progress_dialog_.setProgress(progress);
                            extract_progress_dialog_.setStatus(
                                "Extracting... " +
                                std::to_string(static_cast<int>(progress * 100)) + "%");
                            // 触发UI更新
                            screen_.PostEvent(ftxui::Event::Custom);
                        },
                        [this, archive, full_extract_path](bool success,
                                                           const std::string& error_msg) {
                            // 解压完成
                            show_extract_progress_dialog_ = false;
                            if (success) {
                                setStatusMessage("Extracted " + archive.name + " to " +
                                                 full_extract_path);
                                file_browser_.refresh(); // 刷新文件浏览器
                                // 显示成功 Toast 提示
                                toast_.showSuccess(
                                    "Extracted " + archive.name + " to " + full_extract_path, 2500);
                            } else {
                                setStatusMessage("Failed to extract " + archive.name +
                                                 (error_msg.empty() ? "" : ": " + error_msg));
                                // 显示失败 Toast 提示
                                toast_.showError("Failed to extract " + archive.name +
                                                     (error_msg.empty() ? "" : ": " + error_msg),
                                                 3000);
                            }
                            // 触发 UI 更新
                            screen_.PostEvent(ftxui::Event::Custom);
                        });
                },
                [this]() {
                    setStatusMessage("Extract cancelled");
                    show_extract_path_dialog_ = false;
                });
            show_extract_dialog_ = false;
        },
        [this]() {
            setStatusMessage("Extract cancelled");
            show_extract_dialog_ = false;
        });

    show_extract_dialog_ = true;
}

void Editor::handleExtractDialogInput(ftxui::Event event) {
    if (extract_dialog_.handleInput(event)) {
        if (!extract_dialog_.isVisible()) {
            show_extract_dialog_ = false;
        }
        return;
    }
}

void Editor::handleExtractPathDialogInput(ftxui::Event event) {
    if (extract_path_dialog_.handleInput(event)) {
        if (!extract_path_dialog_.isVisible()) {
            show_extract_path_dialog_ = false;
        }
        return;
    }
}

void Editor::openTUIConfigDialog() {
    auto available_configs = tui_config_manager_.getAvailableTUIConfigs();
    if (!available_configs.empty()) {
        tui_config_popup_.setData(true, available_configs, 0);
        tui_config_popup_.open();
    }
}

#ifdef BUILD_AI_CLIENT_SUPPORT
void Editor::handleAIMessage(const std::string& message) {
    using namespace pnana::features::ai_client;

    auto& manager = AIClientManager::getInstance();
    manager.refreshFromConfig();
    auto client = manager.getCurrentClient();
    if (!client) {
        ai_assistant_panel_.addErrorMessage(
            "AI provider not configured or model unavailable. Please configure AI provider and API "
            "Key in settings.");
        return;
    }

    auto current_config = pnana::features::ai_config::AIConfig::getInstance().getCurrentConfig();

    AIRequest request;
    request.prompt = message;
    request.system_message = getAssistantSystemPrompt();
    request.max_tokens = current_config.max_tokens;
    request.temperature = current_config.temperature;
    request.enable_tool_calling = true;
    request.tools = ai_assistant_panel_.getToolDefinitions();

    buildEnhancedContext(request);

    manager.setToolCallCallback([this](const ToolCall& tool_call) -> ToolCallResult {
        ai_assistant_panel_.addToolCall(tool_call);
        auto result = ai_assistant_panel_.executeToolCall(tool_call);
        if (result.success) {
            std::string summary = std::string(pnana::ui::icons::CHECK_CIRCLE) + " Tool '" +
                                  tool_call.function_name + "' completed";
            if (result.result.contains("output")) {
                std::string output = result.result["output"];
                summary += " (output: " + std::to_string(output.length()) + " chars)";
            }
            setStatusMessage(summary);
        } else {
            setStatusMessage(std::string(pnana::ui::icons::EXCLAMATION_CIRCLE) + " Tool '" +
                             tool_call.function_name + "' failed: " + result.error_message);
        }
        return result;
    });

    ai_assistant_panel_.startStreamingResponse(current_config.model);

    std::string accumulated_response;
    manager.sendStreamingRequest(request, [this, &accumulated_response, message](
                                              const std::string& chunk, bool is_finished) {
        if (!chunk.empty()) {
            ai_assistant_panel_.appendStreamingContent(chunk);
            accumulated_response += chunk;
        }
        if (is_finished) {
            ai_assistant_panel_.finishStreamingResponse();
            ai_assistant_panel_.addToConversationHistory(message, accumulated_response);
        }
    });
}
#endif // BUILD_AI_CLIENT_SUPPORT
void Editor::insertCodeAtCursor(const std::string& code) {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    // 在光标位置插入代码
    doc->insertText(cursor_row_, cursor_col_, code);
    // 更新光标位置
    size_t newlines = std::count(code.begin(), code.end(), '\n');
    if (newlines > 0) {
        cursor_row_ += newlines;
        size_t last_newline_pos = code.rfind('\n');
        cursor_col_ = code.length() - last_newline_pos - 1;
    } else {
        cursor_col_ += code.length();
    }
}

void Editor::replaceSelectedCode(const std::string& code) {
    if (!selection_active_)
        return;

    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    // 计算选区范围
    size_t start_row = std::min(selection_start_row_, cursor_row_);
    size_t end_row = std::max(selection_start_row_, cursor_row_);
    size_t start_col = (start_row == selection_start_row_) ? selection_start_col_ : cursor_col_;
    size_t end_col = (end_row == selection_start_row_) ? selection_start_col_ : cursor_col_;

    // 删除选中的文本
    doc->deleteRange(start_row, start_col, end_row, end_col);

    // 在开始位置插入新代码
    doc->insertText(start_row, start_col, code);

    // 清除选择
    selection_active_ = false;

    // 更新光标位置
    cursor_row_ = start_row;
    cursor_col_ = start_col + code.length();
}

std::string Editor::getSelectedText() const {
    if (!selection_active_)
        return "";

    const Document* doc = getCurrentDocument();
    if (!doc)
        return "";

    size_t start_row = std::min(selection_start_row_, cursor_row_);
    size_t end_row = std::max(selection_start_row_, cursor_row_);
    size_t start_col = (start_row == selection_start_row_) ? selection_start_col_ : cursor_col_;
    size_t end_col = (end_row == selection_start_row_) ? selection_start_col_ : cursor_col_;

    std::string result;
    const auto& lines = doc->getLines();

    for (size_t row = start_row; row <= end_row && row < lines.size(); ++row) {
        const std::string& line = lines[row];
        size_t col_start = (row == start_row) ? start_col : 0;
        size_t col_end = (row == end_row) ? std::min(end_col, line.length()) : line.length();

        if (col_start < col_end) {
            result += line.substr(col_start, col_end - col_start);
        }

        if (row < end_row) {
            result += "\n";
        }
    }

    return result;
}

// 构建增强的上下文信息（含上下文压缩与 token 优化）
#ifdef BUILD_AI_CLIENT_SUPPORT
namespace {
constexpr size_t kContextBudgetChars = 16000;  // ~4k tokens 启发式
constexpr size_t kCurrentFileMaxChars = 6000;  // 当前文件超长时启用 head+cursor+tail
constexpr size_t kConversationMaxChars = 2000; // 对话历史摘要最大长度
constexpr int kLinesNearCursor = 25;           // 光标附近保留行数
constexpr int kHeadLines = 60;                 // 文件开头保留行数
constexpr int kTailLines = 40;                 // 文件末尾保留行数
} // namespace

void Editor::buildEnhancedContext(pnana::features::ai_client::AIRequest& request) const {
    request.context.clear();
    size_t used = 0;
    auto add = [&](const std::string& s, size_t max_len = 0) {
        std::string out = s;
        if (max_len > 0 && out.size() > max_len) {
            out = out.substr(0, max_len) + "\n... [truncated for context limit]";
        }
        if (used + out.size() > kContextBudgetChars)
            return;
        request.context.push_back(out);
        used += out.size();
    };

    // 1. 光标/选区位置（必带，体积小）
    add("Cursor position: line " + std::to_string(cursor_row_ + 1) + ", column " +
        std::to_string(cursor_col_ + 1));
    std::string selected_code = getSelectedText();
    if (selection_active_) {
        add("Selection range: lines " +
            std::to_string(std::min(selection_start_row_, cursor_row_) + 1) + " to " +
            std::to_string(std::max(selection_start_row_, cursor_row_) + 1));
    }

    // 2. 当前文件元信息与内容（压缩：超长用 head + cursor 附近 + tail）
    const Document* doc = getCurrentDocument();
    if (doc && !doc->getFilePath().empty()) {
        add("Current file: " + doc->getFilePath());
        add("File extension: " + doc->getFileExtension());
        add("Detected file type: " +
            utils::FileTypeDetector::detectFileType(doc->getFileName(), doc->getFileExtension()));

        if (!doc->getContent().empty()) {
            std::string content = doc->getContent();
            if (content.size() > kCurrentFileMaxChars) {
                std::vector<std::string> lines;
                std::istringstream iss(content);
                for (std::string line; std::getline(iss, line);)
                    lines.push_back(line);
                const int total_lines = static_cast<int>(lines.size());
                int cur = static_cast<int>(cursor_row_);
                int start_near = std::max(0, cur - kLinesNearCursor / 2);
                int end_near = std::min(total_lines, start_near + kLinesNearCursor);
                std::string compressed;
                for (int i = 0; i < std::min(kHeadLines, total_lines); ++i) {
                    if (i)
                        compressed += "\n";
                    compressed += lines[i];
                }
                if (total_lines > kHeadLines) {
                    int omitted1 = std::max(0, start_near - kHeadLines);
                    compressed += "\n... [omitted " + std::to_string(omitted1) + " lines] ...\n";
                    for (int i = start_near; i < end_near && i < total_lines; ++i)
                        compressed += lines[i] + "\n";
                    int tail_start = std::max(0, total_lines - kTailLines);
                    if (end_near < tail_start) {
                        int omitted2 = std::max(0, tail_start - end_near);
                        compressed += "... [omitted " + std::to_string(omitted2) + " lines] ...\n";
                        for (int i = tail_start; i < total_lines; ++i)
                            compressed += lines[i] + "\n";
                    }
                }
                add("Current file content:\n" + compressed);
            } else {
                add("Current file content:\n" + content);
            }
        }
    }

    // 3. 选中代码
    if (!selected_code.empty())
        add("Selected code:\n" + selected_code);

    // 4. 对话历史摘要（仅保留最近，限制长度）
    std::string conversation_summary = ai_assistant_panel_.getConversationSummary();
    if (!conversation_summary.empty() && conversation_summary != "No previous conversation.")
        add("Conversation history:\n" + conversation_summary, kConversationMaxChars);

    // 5. 项目结构概览（紧凑）
    add("Project root directory: " + std::filesystem::current_path().string());
    addProjectStructureContext(request);

    // 6. 最近文件（仅路径列表）
    addRecentFilesContext(request);

    // 7. 会话状态
    addSessionStateContext(request);
}
#endif // BUILD_AI_CLIENT_SUPPORT

#ifdef BUILD_AI_CLIENT_SUPPORT
// 添加项目结构上下文
void Editor::addProjectStructureContext(pnana::features::ai_client::AIRequest& request) const {
    try {
        std::string project_root = std::filesystem::current_path().string();
        std::vector<std::string> important_files;

        // 查找重要的项目文件
        std::vector<std::string> patterns = {
            "CMakeLists.txt", "Makefile",  "package.json", "requirements.txt", "Cargo.toml",
            "go.mod",         "README.md", ".gitignore",   "pnana.json",       "config.json"};

        for (const auto& pattern : patterns) {
            if (std::filesystem::exists(pattern)) {
                important_files.push_back(pattern);
            }
        }

        // 查找源代码目录
        std::vector<std::string> src_dirs;
        for (const auto& entry : std::filesystem::directory_iterator(project_root)) {
            if (entry.is_directory()) {
                std::string dirname = entry.path().filename().string();
                if (dirname == "src" || dirname == "include" || dirname == "lib" ||
                    dirname == "app" || dirname == "core" || dirname == "ui") {
                    src_dirs.push_back(dirname + "/");
                }
            }
        }

        if (!important_files.empty()) {
            request.context.push_back("Important project files: " +
                                      joinStrings(important_files, ", "));
        }

        if (!src_dirs.empty()) {
            request.context.push_back("Source directories: " + joinStrings(src_dirs, ", "));
        }

    } catch (const std::exception&) {
        // 忽略文件系统错误
    }
}
#endif // BUILD_AI_CLIENT_SUPPORT

#ifdef BUILD_AI_CLIENT_SUPPORT
// 添加最近文件上下文
void Editor::addRecentFilesContext(pnana::features::ai_client::AIRequest& request) const {
    auto recent_files = recent_files_manager_.getRecentFiles();
    if (!recent_files.empty()) {
        std::vector<std::string> recent_names;
        for (size_t i = 0; i < std::min(size_t(5), recent_files.size()); ++i) {
            recent_names.push_back(std::filesystem::path(recent_files[i]).filename().string());
        }
        request.context.push_back("Recently opened files: " + joinStrings(recent_names, ", "));
    }
}
#endif // BUILD_AI_CLIENT_SUPPORT

#ifdef BUILD_AI_CLIENT_SUPPORT
// 添加会话状态上下文
void Editor::addSessionStateContext(pnana::features::ai_client::AIRequest& request) const {
    // 添加标签页信息
    auto tabs = document_manager_.getAllTabs();
    if (tabs.size() > 1) {
        std::vector<std::string> tab_names;
        for (const auto& tab : tabs) {
            std::string name = tab.filename.empty()
                                   ? "[Untitled]"
                                   : std::filesystem::path(tab.filename).filename().string();
            if (tab.is_modified)
                name += " *";
            tab_names.push_back(name);
        }
        request.context.push_back("Open tabs: " + joinStrings(tab_names, ", "));
    }

    // 添加当前模式信息
    std::string mode_str;
    switch (mode_) {
        case EditorMode::NORMAL:
            mode_str = "NORMAL";
            break;
        case EditorMode::SEARCH:
            mode_str = "SEARCH";
            break;
        case EditorMode::REPLACE:
            mode_str = "REPLACE";
            break;
        default:
            mode_str = "UNKNOWN";
            break;
    }
    request.context.push_back("Editor mode: " + mode_str);

    // 添加活动区域信息
    if (split_view_manager_.hasSplits()) {
        request.context.push_back("Editor layout: split view with " +
                                  std::to_string(split_view_manager_.getRegions().size()) +
                                  " regions");
    } else {
        request.context.push_back("Editor layout: single view");
    }
}
#endif // BUILD_AI_CLIENT_SUPPORT

// 辅助方法：连接字符串
std::string Editor::joinStrings(const std::vector<std::string>& strings,
                                const std::string& delimiter) const {
    if (strings.empty())
        return "";
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter + strings[i];
    }
    return result;
}

void Editor::openEncodingDialog() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No file open");
        return;
    }

    std::string current_encoding = doc->getEncoding();
    if (current_encoding.empty()) {
        current_encoding = "UTF-8";
    }

    encoding_dialog_.open(current_encoding);
    encoding_dialog_.setOnConfirm([this](const std::string& new_encoding) {
        convertFileEncoding(new_encoding);
    });
    encoding_dialog_.setOnCancel([this]() {
        setStatusMessage("Encoding conversion cancelled");
    });

    setStatusMessage("Encoding Dialog - ↑↓: Navigate, Enter: Confirm, Esc: Cancel");
}

void Editor::convertFileEncoding(const std::string& new_encoding) {
    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No file open");
        return;
    }

    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        setStatusMessage("Cannot convert encoding: file not saved");
        return;
    }

    std::string current_encoding = doc->getEncoding();
    if (current_encoding.empty()) {
        current_encoding = "UTF-8";
    }

    // 如果编码相同，不需要转换
    std::string upper_current = current_encoding;
    std::string upper_new = new_encoding;
    std::transform(upper_current.begin(), upper_current.end(), upper_current.begin(), ::toupper);
    std::transform(upper_new.begin(), upper_new.end(), upper_new.begin(), ::toupper);

    if (upper_current == upper_new) {
        setStatusMessage("Encoding already set to " + new_encoding);
        return;
    }

    try {
        // 读取文件的原始字节
        auto file_bytes = features::EncodingConverter::readFileAsBytes(filepath);
        if (file_bytes.empty() && doc->lineCount() > 0) {
            // 如果文件为空但文档有内容，从文档内容转换
            std::string content;
            for (size_t i = 0; i < doc->lineCount(); ++i) {
                if (i > 0)
                    content += "\n";
                content += doc->getLine(i);
            }

            // 将UTF-8内容转换为新编码
            std::vector<uint8_t> new_bytes =
                features::EncodingConverter::utf8ToEncoding(content, new_encoding);

            // 保存文件
            std::ofstream file(filepath, std::ios::binary);
            if (file.is_open()) {
                file.write(reinterpret_cast<const char*>(new_bytes.data()), new_bytes.size());
                file.close();
            }
        } else {
            // 从当前编码转换为UTF-8，再转换为新编码
            std::string utf8_content =
                features::EncodingConverter::encodingToUtf8(file_bytes, current_encoding);
            std::vector<uint8_t> new_bytes =
                features::EncodingConverter::utf8ToEncoding(utf8_content, new_encoding);

            // 保存文件
            std::ofstream file(filepath, std::ios::binary);
            if (file.is_open()) {
                file.write(reinterpret_cast<const char*>(new_bytes.data()), new_bytes.size());
                file.close();
            }
        }

        // 更新文档编码
        doc->setEncoding(new_encoding);

        // 重新加载文件（使用新编码读取并转换为UTF-8）
        // 先读取文件的原始字节
        auto new_file_bytes = features::EncodingConverter::readFileAsBytes(filepath);
        if (!new_file_bytes.empty()) {
            // 将新编码的内容转换为UTF-8
            std::string utf8_content =
                features::EncodingConverter::encodingToUtf8(new_file_bytes, new_encoding);

            // 更新文档内容
            std::vector<std::string> new_lines;
            std::istringstream iss(utf8_content);
            std::string line;
            while (std::getline(iss, line)) {
                // 移除行尾的\r（如果有）
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                new_lines.push_back(line);
            }

            // 如果内容以换行符结尾，添加空行
            if (!utf8_content.empty() &&
                (utf8_content.back() == '\n' || utf8_content.back() == '\r')) {
                if (new_lines.empty() || !new_lines.back().empty()) {
                    new_lines.push_back("");
                }
            }

            // 更新文档行
            doc->getLines() = new_lines;
            doc->setModified(false);
        } else {
            // 如果文件为空，重新加载
            doc->reload();
        }

        // 更新状态栏显示
        setStatusMessage("✓ File encoding converted to " + new_encoding);
    } catch (const std::exception& e) {
        setStatusMessage("Failed to convert encoding: " + std::string(e.what()));
    } catch (...) {
        setStatusMessage("Failed to convert encoding: Unknown error");
    }
}

void Editor::handleEncodingDialogInput(Event event) {
    if (encoding_dialog_.handleInput(event)) {
        return;
    }
}

void Editor::openFormatDialog() {
#ifndef BUILD_LSP_SUPPORT
    setStatusMessage("LSP support not enabled");
    return;
#endif

    if (!lsp_enabled_ || !lsp_formatter_) {
        setStatusMessage("LSP not available. Format feature requires LSP support.");
        return;
    }

    // 获取项目根目录（总是递归扫描整个项目）
    std::string current_dir = std::filesystem::current_path().string();

    // 获取目录中支持格式化的文件
    auto supported_files = lsp_formatter_->getSupportedFilesInDirectory(current_dir);

    if (supported_files.empty()) {
        setStatusMessage(
            "No supported files found in project directory. Check LSP server installation.");
        return;
    }

    // 打开格式化对话框
    format_dialog_.open(supported_files, current_dir);
    format_dialog_.setOnConfirm([this](const std::vector<std::string>& files_to_format) {
        formatSelectedFiles(files_to_format);
    });
    format_dialog_.setOnCancel([this]() {
        setStatusMessage("Format cancelled");
    });

    setStatusMessage(
        "Format Dialog - ↑↓: Navigate, Space: Select, A: Select All, Enter: Format, Esc: Cancel");
}

void Editor::formatSelectedFiles(const std::vector<std::string>& file_paths) {
    if (file_paths.empty()) {
        setStatusMessage("No files selected for formatting");
        return;
    }

    if (!lsp_formatter_) {
        setStatusMessage("LSP formatter not available");
        return;
    }

    setStatusMessage("Formatting " + std::to_string(file_paths.size()) +
                     " file(s) in background...");

    // 在后台线程执行格式化，避免阻塞UI
    std::thread([this, file_paths]() {
        bool success = lsp_formatter_->formatFiles(file_paths);
        screen_.Post([this, success, count = file_paths.size()]() {
            if (success) {
                setStatusMessage("✓ Successfully formatted " + std::to_string(count) + " file(s)");
            } else {
                setStatusMessage("✗ Failed to format some files. Check LSP server status.");
            }
        });
    }).detach(); // 分离线程，让它在后台运行
}

void Editor::handleFormatDialogInput(Event event) {
    if (format_dialog_.handleInput(event)) {
        return;
    }
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
    } else if (event == Event::Tab) {
        command_palette_.handleKeyEvent("Tab");
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

// 辅助方法
void Editor::setStatusMessage(const std::string& message) {
    status_message_ = message;
}

std::string Editor::getFileType() const {
    const Document* doc = getCurrentDocument();
    if (!doc) {
        return "text";
    }

    return utils::FileTypeDetector::detectFileType(doc->getFileName(), doc->getFileExtension());
}

bool Editor::isCtrlKey(const Event& event, char key) const {
    // 首先检查 FTXUI 预定义的 Ctrl 事件
    switch (key) {
        case 'a':
            return event == Event::CtrlA;
        case 'b':
            return event == Event::CtrlB;
        case 'c':
            return event == Event::CtrlC;
        case 'd':
            return event == Event::CtrlD;
        case 'e':
            return event == Event::CtrlE;
        case 'f':
            return event == Event::CtrlF;
        case 'g':
            return event == Event::CtrlG;
        case 'h':
            return event == Event::CtrlH;
        case 'i':
            return event == Event::CtrlI;
        case 'j':
            return event == Event::CtrlJ;
        case 'k':
            return event == Event::CtrlK;
        case 'l':
            return event == Event::CtrlL;
        case 'm':
            return event == Event::CtrlM;
        case 'n':
            return event == Event::CtrlN;
        case 'o':
            return event == Event::CtrlO;
        case 'p':
            return event == Event::CtrlP;
        case 'q':
            return event == Event::CtrlQ;
        case 'r':
            return event == Event::CtrlR;
        case 's':
            return event == Event::CtrlS;
        case 't':
            return event == Event::CtrlT;
        case 'u':
            return event == Event::CtrlU;
        case 'v':
            return event == Event::CtrlV;
        case 'w':
            return event == Event::CtrlW;
        case 'x':
            return event == Event::CtrlX;
        case 'y':
            return event == Event::CtrlY;
        case 'z':
            return event == Event::CtrlZ;
        default:
            break;
    }

    // 如果没有匹配预定义事件，检查字符事件（Ctrl+Key产生ASCII控制字符）
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
    return event == Event::ArrowUpCtrl || event == Event::ArrowDownCtrl ||
           event == Event::ArrowLeftCtrl || event == Event::ArrowRightCtrl;
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
        "Rename " + std::string(is_directory ? "Folder" : "File"), "Enter new name:", current_name,
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
        });
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
        "Delete " + std::string(is_directory ? "Folder" : "File"), message,
        [this, selected_path, selected_name]() {
            if (file_browser_.deleteSelected()) {
                setStatusMessage("Deleted: " + selected_name);
            } else {
                setStatusMessage("Failed to delete: " + selected_name);
            }
        },
        [this]() {
            setStatusMessage("Delete cancelled");
        });
}

void Editor::openFilePicker() {
    // 与文件列表路径一致：若文件浏览器可见则使用其当前目录，否则使用当前文档所在目录或 cwd
    std::string start_path = ".";
    if (file_browser_.isVisible()) {
        std::string dir = file_browser_.getCurrentDirectory();
        if (!dir.empty()) {
            if (dir.size() > 6 && dir.compare(0, 6, "ssh://") == 0) {
                // 远程模式：取 ssh://host 后的路径部分给 file picker
                size_t path_start = dir.find('/', 6);
                start_path = (path_start != std::string::npos) ? dir.substr(path_start) : "/";
            } else {
                start_path = dir;
            }
        }
    } else if (getCurrentDocument() && !getCurrentDocument()->getFileName().empty()) {
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
    file_picker_.show(
        start_path, pnana::ui::FilePickerType::BOTH,
        [this](const std::string& path) {
            // 检查路径是否是目录
            try {
                if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                    // 如果是目录，更新文件浏览器的当前目录
                    if (file_browser_.openDirectory(path)) {
                        // 添加到最近文件夹列表
                        recent_files_manager_.addFolder(path);
                        setStatusMessage("Changed to directory: " + path);
                        // 显示 toast 提示用户已切换目录
                        toast_.showSuccess("Changed to directory: " + path);
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
        });
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
        std::vector<pnana::ui::SplitInfo> splits;
        const auto& regions = split_view_manager_.getRegions();
        auto tabs = document_manager_.getAllTabs();

        for (size_t i = 0; i < regions.size(); ++i) {
            const auto& region = regions[i];
            std::string doc_name = "[No Document]";
            bool is_modified = false;

            if (region.current_document_index < tabs.size()) {
                doc_name = tabs[region.current_document_index].filename;
                if (doc_name.empty()) {
                    doc_name = "[Untitled]";
                }
                is_modified = tabs[region.current_document_index].is_modified;
            }

            splits.emplace_back(i, region.current_document_index, doc_name, region.is_active,
                                is_modified);
        }

        split_dialog_.showClose(
            splits,
            [this](size_t region_index) {
                closeSplitRegion(region_index);
            },
            [this]() {
                setStatusMessage("Close split cancelled");
            });
    } else {
        // 没有分屏，显示创建分屏对话框
        split_dialog_.showCreate(
            [this](features::SplitDirection direction) {
                splitView(direction);
            },
            [this]() {
                setStatusMessage("Split cancelled");
            });
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
    if (region.current_document_index < tabs.size()) {
        if (tabs[region.current_document_index].is_modified) {
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
                if (regions[i].current_document_index < tabs.size()) {
                    document_manager_.switchToDocument(regions[i].current_document_index);
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

Document* Editor::getDocumentForActiveRegion() {
    if (!split_view_manager_.hasSplits()) {
        return getCurrentDocument();
    }

    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region) {
        return document_manager_.getDocument(active_region->current_document_index);
    }
    return nullptr;
}

const Document* Editor::getDocumentForActiveRegion() const {
    if (!split_view_manager_.hasSplits()) {
        return getCurrentDocument();
    }

    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region) {
        return document_manager_.getDocument(active_region->current_document_index);
    }
    return nullptr;
}

size_t Editor::getDocumentIndexForActiveRegion() const {
    if (!split_view_manager_.hasSplits()) {
        return document_manager_.getCurrentIndex();
    }

    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region) {
        return active_region->current_document_index;
    }
    return 0;
}

void Editor::setDocumentForActiveRegion(size_t document_index) {
    if (!split_view_manager_.hasSplits()) {
        document_manager_.switchToDocument(document_index);
        return;
    }

    split_view_manager_.setCurrentDocumentIndex(document_index);

    // 如果这是当前激活的区域，也切换全局文档管理器
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region && active_region->current_document_index == document_index) {
        document_manager_.switchToDocument(document_index);
    }
}

void Editor::openDocumentInActiveRegion(const std::string& file_path) {
    // 先尝试打开文档
    size_t new_doc_index = document_manager_.openDocument(file_path);
    if (new_doc_index == static_cast<size_t>(-1)) {
        return; // 打开失败
    }

    // 设置为激活区域的文档
    if (split_view_manager_.hasSplits()) {
        split_view_manager_.setCurrentDocumentIndex(new_doc_index);
    }
}

void Editor::saveCurrentRegionState() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }

    const auto* active_region = split_view_manager_.getActiveRegion();
    if (!active_region) {
        return;
    }

    // 找到激活区域的索引
    size_t region_index = 0;
    for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
        if (&split_view_manager_.getRegions()[i] == active_region) {
            region_index = i;
            break;
        }
    }

    // 确保 region_states_ 有足够的容量
    if (region_states_.size() <= region_index) {
        region_states_.resize(region_index + 1);
        // 初始化新区域的状态为默认值
        region_states_[region_index].cursor_row = 0;
        region_states_[region_index].cursor_col = 0;
        region_states_[region_index].view_offset_row = 0;
        region_states_[region_index].view_offset_col = 0;
        // 初始化单词高亮状态
        region_states_[region_index].word_highlight_active_ = false;
        region_states_[region_index].current_word_.clear();
        region_states_[region_index].word_matches_.clear();
        region_states_[region_index].word_highlight_row_ = 0;
        region_states_[region_index].word_highlight_col_ = 0;
    }

    // 保存当前状态
    region_states_[region_index].cursor_row = cursor_row_;
    region_states_[region_index].cursor_col = cursor_col_;
    region_states_[region_index].view_offset_row = view_offset_row_;
    region_states_[region_index].view_offset_col = view_offset_col_;

    // 注意：单词高亮状态不需要在这里保存，因为 updateWordHighlight() 会直接更新区域状态
    // 如果在这里保存全局状态，可能会覆盖区域状态中的正确值（因为全局状态在分屏模式下可能是空的）
}

void Editor::restoreRegionState(size_t region_index) {
    if (region_index >= region_states_.size()) {
        // 如果没有保存的状态，使用默认值
        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0;
        view_offset_col_ = 0;
        // 清除单词高亮
        if (split_view_manager_.hasSplits()) {
            // 分屏模式下，单词高亮由区域状态管理，不需要恢复全局状态
        } else {
            word_highlight_active_ = false;
            current_word_.clear();
            word_matches_.clear();
            word_highlight_row_ = 0;
            word_highlight_col_ = 0;
        }
        return;
    }

    // 恢复状态
    cursor_row_ = region_states_[region_index].cursor_row;
    cursor_col_ = region_states_[region_index].cursor_col;
    view_offset_row_ = region_states_[region_index].view_offset_row;
    view_offset_col_ = region_states_[region_index].view_offset_col;

    // 恢复单词高亮状态（仅在分屏模式下）
    if (split_view_manager_.hasSplits()) {
        // 分屏模式下，切换区域时清除单词高亮状态，避免显示之前区域的高亮
        // 用户移动光标时会自动触发 updateWordHighlight() 重新计算
        region_states_[region_index].word_highlight_active_ = false;
        region_states_[region_index].current_word_.clear();
        region_states_[region_index].word_matches_.clear();
        region_states_[region_index].word_highlight_row_ = 0;
        region_states_[region_index].word_highlight_col_ = 0;
        // 全局状态保持为空，避免影响其他区域
    } else {
        // 单视图模式下，恢复全局状态
        word_highlight_active_ = region_states_[region_index].word_highlight_active_;
        current_word_ = region_states_[region_index].current_word_;
        word_matches_ = region_states_[region_index].word_matches_;
        word_highlight_row_ = region_states_[region_index].word_highlight_row_;
        word_highlight_col_ = region_states_[region_index].word_highlight_col_;
    }

    // 调整光标位置以确保有效
    adjustCursor();
    adjustViewOffset();

    // 在分屏模式下，切换区域后重新计算单词高亮（基于新区域的光标位置）
    // 这样可以确保如果光标正好在单词上，会立即显示高亮
    if (split_view_manager_.hasSplits()) {
        updateWordHighlight();
    }
}

bool Editor::resizeActiveSplitRegion(int delta) {
    if (!split_view_manager_.hasSplits()) {
        return false;
    }

    const auto* active_region = split_view_manager_.getActiveRegion();
    if (!active_region) {
        return false;
    }

    // 临时修复：如果发现区域坐标异常，尝试重新计算分屏线位置
    if (active_region->x < 0 || active_region->y < 0 || active_region->width <= 0 ||
        active_region->height <= 0) {
        // 重新计算分屏线位置
        split_view_manager_.updateRegionSizes(screen_.dimx(), screen_.dimy());
        // 重新获取区域信息
        const auto* fixed_region = split_view_manager_.getActiveRegion();
        if (fixed_region && fixed_region->x >= 0 && fixed_region->y >= 0) {
            // 递归调用自己
            return resizeActiveSplitRegion(delta);
        }
    }

    // 找到与激活区域相邻的分屏线
    const auto& split_lines = split_view_manager_.getSplitLines();

    for (size_t i = 0; i < split_lines.size(); ++i) {
        const auto& line = split_lines[i];
        bool should_adjust = false;

        // 验证分屏线位置的一致性 - 这个逻辑有问题，移除它
        // split line可能在任何region边界上，不一定在active region的边界上

        // 检查分屏线是否与激活区域相邻
        if (line.is_vertical) {
            // 竖直分屏线：检查是否正好在激活区域的边界上
            int right_boundary = active_region->x + active_region->width;
            int left_boundary = active_region->x;

            // 分屏线应该正好在激活区域的边界上（精确匹配）
            if (line.position == left_boundary || line.position == right_boundary) {
                should_adjust = true;
            }
        } else {
            // 横向分屏线：检查是否在激活区域的上下边界附近
            int bottom_boundary = active_region->y + active_region->height;
            int top_boundary = active_region->y;

            // 检查分屏线是否在激活区域的任何一个边界附近（2像素容差）
            if (std::abs(line.position - top_boundary) <= 2 ||
                std::abs(line.position - bottom_boundary) <= 2) {
                should_adjust = true;
            }
        }

        if (should_adjust) {
            // 调整分屏线位置
            split_view_manager_.adjustSplitLinePosition(i, delta, screen_.dimx(), screen_.dimy());
            return true;
        }
    }

    return false; // 没有找到可调整的分屏线
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
        screen_width -= file_browser_width_ + 1; // +1 for separator
    }

    // 减去状态栏和标签栏的高度
    screen_height -= 6; // 标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1)

    // 执行分屏
    if (direction == features::SplitDirection::VERTICAL) {
        split_view_manager_.splitVertical(screen_width, screen_height);
        setStatusMessage("Split vertically");
    } else {
        split_view_manager_.splitHorizontal(screen_width, screen_height);
        setStatusMessage("Split horizontally");
    }

    // 新分屏已经通过SplitViewManager正确设置了文档索引和文档列表
    // 这里不需要额外处理，新区域会继承当前区域的文档列表

    // 更新区域尺寸
    split_view_manager_.updateRegionSizes(screen_width, screen_height);

    // 为新区域初始化状态
    const auto& regions = split_view_manager_.getRegions();
    size_t old_size = region_states_.size();
    if (old_size < regions.size()) {
        region_states_.resize(regions.size());
        // 只初始化新区域的状态，保持现有区域的状态不变
        for (size_t i = old_size; i < regions.size(); ++i) {
            // 对于新创建的区域，如果它显示欢迎页面（SIZE_MAX），使用默认状态
            // 如果它显示现有文档，从当前全局状态复制
            if (regions[i].current_document_index == SIZE_MAX) {
                // 欢迎页面：从文件开头开始
                region_states_[i].cursor_row = 0;
                region_states_[i].cursor_col = 0;
                region_states_[i].view_offset_row = 0;
                region_states_[i].view_offset_col = 0;
            } else {
                // 显示现有文档：从当前全局状态复制
                region_states_[i].cursor_row = cursor_row_;
                region_states_[i].cursor_col = cursor_col_;
                region_states_[i].view_offset_row = view_offset_row_;
                region_states_[i].view_offset_col = view_offset_col_;
            }
        }
    }
}

void Editor::focusLeftRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }

    // 保存当前区域状态
    saveCurrentRegionState();

    split_view_manager_.focusLeftRegion();

    // 切换到激活区域的文档 - 更新全局文档管理器
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region && active_region->current_document_index != SIZE_MAX) {
        document_manager_.switchToDocument(active_region->current_document_index);

        // 找到新激活区域的索引并恢复状态
        size_t region_index = 0;
        for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
            if (&split_view_manager_.getRegions()[i] == active_region) {
                region_index = i;
                break;
            }
        }
        restoreRegionState(region_index);
        syntax_highlighter_.setFileType(getFileType());
#ifdef BUILD_TREE_SITTER_SUPPORT
        auto_indent_engine_.setFileType(getFileType());
#endif
    }
    setStatusMessage("Focus left region");

    // 强制UI更新，确保标签栏立即刷新显示当前分屏的文档
    force_ui_update_ = true;
}

void Editor::focusTabBar() {
    if (!region_manager_.isTabAreaEnabled()) {
        setStatusMessage("Tab bar: open more than one file to focus");
        return;
    }
    region_manager_.setTabIndex(static_cast<int>(document_manager_.getCurrentIndex()));
    region_manager_.setRegion(EditorRegion::TAB_AREA);
    setStatusMessage("Region: " + region_manager_.getRegionName());
    force_ui_update_ = true;
}

void Editor::focusRightRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }

    // 保存当前区域状态
    saveCurrentRegionState();

    split_view_manager_.focusRightRegion();

    // 切换到激活区域的文档 - 更新全局文档管理器
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region && active_region->current_document_index != SIZE_MAX) {
        document_manager_.switchToDocument(active_region->current_document_index);

        // 找到新激活区域的索引并恢复状态
        size_t region_index = 0;
        for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
            if (&split_view_manager_.getRegions()[i] == active_region) {
                region_index = i;
                break;
            }
        }
        restoreRegionState(region_index);
        syntax_highlighter_.setFileType(getFileType());
#ifdef BUILD_TREE_SITTER_SUPPORT
        auto_indent_engine_.setFileType(getFileType());
#endif
    }
    setStatusMessage("Focus right region");

    // 强制UI更新，确保标签栏立即刷新显示当前分屏的文档
    force_ui_update_ = true;
}

void Editor::focusUpRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }

    // 保存当前区域状态
    saveCurrentRegionState();

    split_view_manager_.focusUpRegion();

    // 切换到激活区域的文档 - 更新全局文档管理器
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region && active_region->current_document_index != SIZE_MAX) {
        document_manager_.switchToDocument(active_region->current_document_index);

        // 找到新激活区域的索引并恢复状态
        size_t region_index = 0;
        for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
            if (&split_view_manager_.getRegions()[i] == active_region) {
                region_index = i;
                break;
            }
        }
        restoreRegionState(region_index);
        syntax_highlighter_.setFileType(getFileType());
#ifdef BUILD_TREE_SITTER_SUPPORT
        auto_indent_engine_.setFileType(getFileType());
#endif
    }
    setStatusMessage("Focus up region");

    // 强制UI更新，确保标签栏立即刷新显示当前分屏的文档
    force_ui_update_ = true;
}

void Editor::focusDownRegion() {
    if (!split_view_manager_.hasSplits()) {
        return;
    }

    // 保存当前区域状态
    saveCurrentRegionState();

    split_view_manager_.focusDownRegion();

    // 切换到激活区域的文档 - 更新全局文档管理器
    const auto* active_region = split_view_manager_.getActiveRegion();
    if (active_region && active_region->current_document_index != SIZE_MAX) {
        document_manager_.switchToDocument(active_region->current_document_index);

        // 找到新激活区域的索引并恢复状态
        size_t region_index = 0;
        for (size_t i = 0; i < split_view_manager_.getRegions().size(); ++i) {
            if (&split_view_manager_.getRegions()[i] == active_region) {
                region_index = i;
                break;
            }
        }
        restoreRegionState(region_index);
        syntax_highlighter_.setFileType(getFileType());
#ifdef BUILD_TREE_SITTER_SUPPORT
        auto_indent_engine_.setFileType(getFileType());
#endif
    }
    setStatusMessage("Focus down region");

    // 强制UI更新，确保标签栏立即刷新显示当前分屏的文档
    force_ui_update_ = true;
}

#ifdef BUILD_LUA_SUPPORT
void Editor::initializePlugins() {
    plugin_manager_ = std::make_unique<plugins::PluginManager>(this);
    if (!plugin_manager_ || !plugin_manager_->initialize()) {
        plugin_manager_.reset();
    } else {
        // 设置插件管理对话框的插件管理器指针
        plugin_manager_dialog_.setPluginManager(plugin_manager_.get());
        // 设置 Toast 指针，用于显示操作提示
        plugin_manager_dialog_.setToast(&toast_);
    }
    plugin_manager_initialized_ = true;
}

void Editor::ensurePluginManagerInitialized() {
    if (!plugin_manager_initialized_) {
        initializePlugins();
    }
}

void Editor::openPluginManager() {
    // 延迟初始化插件系统
    ensurePluginManagerInitialized();

    if (plugin_manager_) {
        plugin_manager_dialog_.open();
        setStatusMessage("Plugin Manager | ↑↓: Navigate, Space/Enter: Toggle, Esc: Close");
    } else {
        setStatusMessage("Plugin system not available");
    }
}
#endif // BUILD_LUA_SUPPORT

bool Editor::isFileBrowserVisible() const {
    return file_browser_.isVisible();
}

bool Editor::isDialogVisible() const {
    return dialog_.isVisible() || (popup_manager_ && popup_manager_->isVisible());
}

bool Editor::handleDialogInput(ftxui::Event event) {
    if (popup_manager_ && popup_manager_->isVisible()) {
        return popup_manager_->handleInput(event);
    }
    return dialog_.handleInput(event);
}

bool Editor::isTerminalVisible() const {
    return terminal_.isVisible();
}

bool Editor::isGitPanelVisible() const {
    return git_panel_.isVisible();
}

int Editor::getScreenHeight() const {
    return screen_.dimy();
}

int Editor::getScreenWidth() const {
    return screen_.dimx();
}

// 渲染批处理控制实现（方案1）
void Editor::pauseRendering() {
    rendering_paused_ = true;
}

void Editor::resumeRendering() {
    rendering_paused_ = false;

    if (needs_render_ || pending_cursor_update_) {
        needs_render_ = false;
        pending_cursor_update_ = false;
        screen_.PostEvent(Event::Custom); // 手动触发渲染更新
    }
}

// 强制触发待处理的光标更新
void Editor::triggerPendingCursorUpdate() {
    if (pending_cursor_update_ && !rendering_paused_) {
        pending_cursor_update_ = false;
        screen_.PostEvent(Event::Custom);
    }
}

// 获取调用栈信息（简化版，用于调试）
std::string Editor::getCallStackInfo() {
    // 由于C++没有简单的栈追踪，这里用时间戳和状态来标识调用来源
    static std::chrono::steady_clock::time_point last_call_time;
    auto now = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_call_time);
    last_call_time = now;

    std::string info = "time_diff=" + std::to_string(time_diff.count()) + "ms";

    // 添加当前状态信息
    if (rendering_paused_) {
        info += ", paused=true";
    }
    if (needs_render_) {
        info += ", needs_render=true";
    }

    return info;
}

#ifdef BUILD_LUA_SUPPORT
void Editor::triggerPluginEvent(const std::string& event, const std::vector<std::string>& args) {
    // 延迟初始化插件系统
    ensurePluginManagerInitialized();

    if (plugin_manager_) {
        plugin_manager_->triggerEvent(event, args);
    }
}
#endif

} // namespace core
} // namespace pnana
