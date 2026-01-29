// 编辑器核心类实现
#include "core/editor.h"
#include "core/input/input_router.h"
#include "core/ui/ui_router.h"
#include "features/encoding_converter.h"
#include "ui/icons.h"
#include "utils/file_type_detector.h"
#include "utils/logger.h"
#ifdef BUILD_LUA_SUPPORT
#include "plugins/plugin_manager.h"
#endif
#ifdef BUILD_AI_CLIENT_SUPPORT
#include "features/ai_client/ai_client.h"
#endif
#include "features/md_render/markdown_renderer.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace core {

// 定义折叠缓存持续时间常量
const std::chrono::minutes Editor::FOLDING_CACHE_DURATION = std::chrono::minutes(30);

// 构造函数
Editor::Editor()
    : document_manager_(), key_binding_manager_(), action_executor_(this),
      overlay_manager_(std::make_unique<pnana::core::OverlayManager>()), theme_(),
      statusbar_(theme_), helpbar_(theme_), tabbar_(theme_), help_(theme_), dialog_(theme_),
      file_picker_(theme_), split_dialog_(theme_), ssh_dialog_(theme_),
      ssh_transfer_dialog_(theme_), welcome_screen_(theme_), split_welcome_screen_(theme_),
      new_file_prompt_(theme_), theme_menu_(theme_), create_folder_dialog_(theme_),
      save_as_dialog_(theme_), move_file_dialog_(theme_), cursor_config_dialog_(theme_),
      binary_file_view_(theme_), encoding_dialog_(theme_), format_dialog_(theme_),
      recent_files_popup_(theme_), tui_config_popup_(theme_), ai_assistant_panel_(theme_),
      ai_config_dialog_(theme_),
#ifdef BUILD_LUA_SUPPORT
      plugin_manager_dialog_(theme_, nullptr), // 将在 initializePluginManager 中设置
#endif
      git_panel_(theme_), search_engine_(), file_browser_(theme_), search_highlight_active_(false),
#ifdef BUILD_IMAGE_PREVIEW_SUPPORT
      image_preview_(),
#endif
      syntax_highlighter_(theme_), command_palette_(theme_), terminal_(theme_),
      split_view_manager_(), diagnostics_popup_(theme_), mode_(EditorMode::NORMAL), cursor_row_(0),
      cursor_col_(0), view_offset_row_(0), view_offset_col_(0), show_theme_menu_(false),
      show_help_(false), show_create_folder_(false), show_save_as_(false), show_move_file_(false),
      selection_active_(false), selection_start_row_(0), selection_start_col_(0),
      show_line_numbers_(true), relative_line_numbers_(false), syntax_highlighting_(true),
      zoom_level_(0), file_browser_width_(35), // 默认宽度35列
      terminal_height_(0),                     // 0 表示使用默认值（屏幕高度的1/3）
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

    // 初始化文件浏览器到当前目录
    file_browser_.openDirectory(".");

    // 初始化命令面板
    initializeCommandPalette();

    // 初始化最近文件管理器
    recent_files_manager_.setFileOpenCallback([this](const std::string& filepath) {
        this->openFile(filepath);
    });

    // 初始化最近文件弹窗
    recent_files_popup_.setFileOpenCallback([this](size_t index) {
        this->recent_files_manager_.openFile(index);
    });

    // 设置文档切换回调，优化LSP诊断响应速度
    document_manager_.setDocumentSwitchedCallback([this](size_t old_index, size_t new_index) {
        try {
            LOG("[DOC_SWITCH] ===== DOCUMENT SWITCH START =====");
            LOG("[DOC_SWITCH] Document switched from " + std::to_string(old_index) + " to " +
                std::to_string(new_index));

            // 安全检查：验证新文档索引有效
            if (new_index >= document_manager_.getDocumentCount()) {
                LOG_ERROR(
                    "[DOC_SWITCH] Invalid new document index: " + std::to_string(new_index) +
                    ", total documents: " + std::to_string(document_manager_.getDocumentCount()));
                return;
            }

            // 获取新文档信息
            Document* new_doc = document_manager_.getDocument(new_index);
            if (!new_doc) {
                LOG_ERROR("[DOC_SWITCH] Failed to get document at index " +
                          std::to_string(new_index));
                return;
            }

            std::string filepath = new_doc->getFilePath();
            LOG("[DOC_SWITCH] New document filepath: " + filepath);

            // 安全检查：验证文档状态
            if (filepath.empty()) {
                LOG_WARNING("[DOC_SWITCH] Document has empty filepath, skipping LSP updates");
                // 对于无路径文档，只需要设置渲染标志
                needs_render_ = true;
                last_render_source_ = "document_switch";
                return;
            }

            // 立即更新当前文件的诊断信息，提升响应速度（不强制UI更新）
            LOG("[DOC_SWITCH] Updating diagnostics...");
            updateCurrentFileDiagnostics();

            // 检查诊断更新结果
            {
                std::lock_guard<std::mutex> lock(diagnostics_mutex_);
                LOG("[DOC_SWITCH] Current diagnostics count: " +
                    std::to_string(current_file_diagnostics_.size()));
            }

            // 立即更新折叠状态，提升响应速度（不强制UI更新）
            LOG("[DOC_SWITCH] Updating folding...");
            updateCurrentFileFolding();

            // 预加载相邻文档的诊断和折叠数据，提升后续切换响应速度
            LOG("[DOC_SWITCH] Starting preload for adjacent documents...");
            preloadAdjacentDocuments(new_index);

            // 设置渲染标志，避免强制UI更新导致的抖动
            needs_render_ = true;
            last_render_source_ = "document_switch";
            LOG("[DOC_SWITCH] Set needs_render_=true, last_render_source=document_switch");

            LOG("[DOC_SWITCH] ===== DOCUMENT SWITCH END =====");
        } catch (const std::exception& e) {
            LOG_ERROR("[DOC_SWITCH] Exception in document switch callback: " +
                      std::string(e.what()));
        } catch (...) {
            LOG_ERROR("[DOC_SWITCH] Unknown exception in document switch callback");
        }
    });

    // 初始化TUI配置管理器
    tui_config_manager_.setConfigOpenCallback([this](const std::string& filepath) {
        this->openFile(filepath);
    });

    // 初始化TUI配置弹窗
    tui_config_popup_.setConfigOpenCallback([this](const features::TUIConfig& config) {
        this->tui_config_manager_.openConfig(config);
    });

    // 初始化AI助手
    initializeAIAssistant();

    // 初始化输入和UI路由器（解耦优化）
    input_router_ = std::make_unique<pnana::core::input::InputRouter>();
    ui_router_ = std::make_unique<pnana::core::ui::UIRouter>();

    // 日志系统不会在这里自动初始化
    // 只有在 main.cpp 中指定 -l/--log 参数时才会初始化
    // LOG("Editor constructor started");  // 只有在启用日志时才记录

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
    // 初始化插件系统
    initializePlugins();
#endif
    // 启动光标闪烁刷新线程（轻量级，仅在启用闪烁时触发 UI 刷新）
    std::thread([this]() {
        using namespace std::chrono_literals;
        while (!should_quit_) {
            std::this_thread::sleep_for(50ms); // 50ms 刷新检查间隔

            // 仅在启用了闪烁且设置了有效频率时触发重绘
            bool blink_on = false;
            int rate = 0;
            try {
                blink_on = cursor_config_dialog_.getBlinkEnabled();
                rate = getCursorBlinkRate();
            } catch (...) {
                // 避免异常中断线程
                continue;
            }

            if (blink_on && rate > 0 && !rendering_paused_) {
                // 触发一次自定义事件，让增量渲染逻辑根据时间重新绘制光标
                screen_.PostEvent(ftxui::Event::Custom);
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

void Editor::run() {
    main_component_ = CatchEvent(Renderer([this] {
                                     return renderUI();
                                 }),
                                 [this](Event event) {
                                     handleInput(event);
                                     return true;
                                 });
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
    // 加载配置文件
    config_manager_.loadConfig(config_path);

    // 从配置获取主题名称并应用
    const auto& config = config_manager_.getConfig();
    std::string theme_name = config.current_theme;
    if (theme_name.empty()) {
        theme_name = config.editor.theme;
    }
    if (theme_name.empty()) {
        theme_name = "monokai"; // 默认主题
    }

    // 检查主题是否可用（预设主题或已加载插件提供的主题）
    std::vector<std::string> check_available_themes = ::pnana::ui::Theme::getAvailableThemes();
    std::vector<std::string> check_custom_themes = theme_.getCustomThemeNames();
    check_available_themes.insert(check_available_themes.end(), check_custom_themes.begin(),
                                  check_custom_themes.end());

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

    theme_.setTheme(theme_name);

    // 更新可用主题列表
    std::vector<std::string> available_themes;
    if (!config.available_themes.empty()) {
        available_themes = config.available_themes;
    } else {
        // 使用 Theme 类提供的所有可用主题
        available_themes = ::pnana::ui::Theme::getAvailableThemes();
    }

    // 注意：自定义主题由插件系统管理，不在这里自动添加
    // 插件加载时会通过 Lua API 更新主题菜单

    // 清除所有自定义主题，确保只有当前加载的插件的主题会被显示
    theme_.clearCustomThemes();

    theme_menu_.setAvailableThemes(available_themes);

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
        // 找到当前主题的索引
        std::string current = theme_.getCurrentThemeName();
        const auto& themes = theme_menu_.getAvailableThemes();
        for (size_t i = 0; i < themes.size(); ++i) {
            if (themes[i] == current) {
                theme_menu_.setSelectedIndex(i);
                break;
            }
        }
        setStatusMessage("Select theme with ↑↓ and press Enter");
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

void Editor::applySelectedTheme() {
    const auto& themes = theme_menu_.getAvailableThemes();
    size_t selected_index = theme_menu_.getSelectedIndex();

    if (selected_index < themes.size()) {
        std::string theme_name = themes[selected_index];

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
    // Toggle lightweight preview flag and request UI update
    markdown_preview_enabled_ = !markdown_preview_enabled_;
    if (markdown_preview_enabled_) {
        LOG("[DEBUG] Markdown preview enabled (lightweight)");
        setStatusMessage("Markdown preview enabled - Press Alt+W again to close");
    } else {
        LOG("[DEBUG] Markdown preview disabled");
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
    terminal_.toggle();
    if (terminal_.isVisible()) {
        // 启用终端区域（必须先启用，才能切换）
        region_manager_.setTerminalEnabled(true);

        // 切换到终端区域并确保焦点正确
        region_manager_.setRegion(EditorRegion::TERMINAL);

        // 如果终端高度未设置，使用默认值
        if (terminal_height_ <= 0) {
            terminal_height_ = screen_.dimy() / 3;
        }
        // 清空终端输入，准备接收新输入
        terminal_.handleInput("");
        terminal_.setCursorPosition(0);
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
        terminal_.handleInput(""); // 清空输入
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
                setStatusMessage("Region: " + region_manager_.getRegionName() +
                                 " | →: Return to terminal");
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
                setStatusMessage("Region: " + region_manager_.getRegionName() +
                                 " | ←: Return to terminal");
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
    } else if (event == Event::Tab) {
        // Tab 补全
        if (terminal_.handleTabCompletion()) {
            setStatusMessage("Tab completion applied");
        } else {
            setStatusMessage("No completion found");
        }
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

void Editor::toggleAIAssistant() {
    if (ai_assistant_panel_.isVisible()) {
        ai_assistant_panel_.hide();
        setStatusMessage("AI Assistant closed");
    } else {
        ai_assistant_panel_.show();
        setStatusMessage("AI Assistant opened - Type your message and press Enter");
    }
}

void Editor::initializeAIAssistant() {
    // 设置AI助手的回调函数
#ifdef BUILD_AI_CLIENT_SUPPORT
    ai_assistant_panel_.setOnSendMessage([this](const std::string& message) {
        handleAIMessage(message);
    });
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

    // 创建AI请求
    AIRequest request;
    request.prompt = message;
    request.system_message = R"(
You are an AI programming assistant with access to various tools. You can:

1. Read files using the read_file tool
2. Search for patterns in code using grep_search
3. Run terminal commands using run_terminal_command
4. List directory contents using list_directory
5. Analyze code for issues using analyze_code

When the user asks you to perform actions on their codebase, use the appropriate tools to gather information before providing your response. Be helpful, accurate, and provide actionable suggestions.

Available tools:
- read_file: Read file contents
- grep_search: Search for text patterns
- run_terminal_command: Execute terminal commands
- list_directory: List directory contents
- analyze_code: Analyze code for issues
)";
    request.max_tokens = 4096; // 增加token限制以支持工具调用
    request.temperature = 0.7f;
    request.enable_tool_calling = true;                       // 启用工具调用
    request.tools = ai_assistant_panel_.getToolDefinitions(); // 添加工具定义

    // 构建更丰富的上下文信息
    buildEnhancedContext(request);

    // 设置工具调用回调
    AIClientManager& manager = AIClientManager::getInstance();
    manager.setToolCallCallback([this](const ToolCall& tool_call) -> ToolCallResult {
        return ai_assistant_panel_.executeToolCall(tool_call);
    });

    // 发送请求
    std::string accumulated_response;
    manager.sendStreamingRequest(request, [this, &accumulated_response, message](
                                              const std::string& chunk, bool is_finished) {
        if (!chunk.empty()) {
            ai_assistant_panel_.appendStreamingContent(chunk);
            accumulated_response += chunk;
        }
        if (is_finished) {
            ai_assistant_panel_.finishStreamingResponse();
            // 添加到对话历史
            ai_assistant_panel_.addToConversationHistory(message, accumulated_response);
        }
    });

    // 设置工具调用回调以显示工具使用状态
    manager.setToolCallCallback([this](const pnana::features::ai_client::ToolCall& tool_call)
                                    -> pnana::features::ai_client::ToolCallResult {
        // 显示工具调用状态
        ai_assistant_panel_.addToolCall(tool_call);

        // 执行工具调用
        auto result = ai_assistant_panel_.executeToolCall(tool_call);

        // 在UI中显示工具调用结果摘要
        if (result.success) {
            std::string summary = "✅ Tool '" + tool_call.function_name + "' completed";
            if (result.result.contains("output")) {
                std::string output = result.result["output"];
                summary += " (output: " + std::to_string(output.length()) + " chars)";
            }
            setStatusMessage(summary);
        } else {
            setStatusMessage("❌ Tool '" + tool_call.function_name +
                             "' failed: " + result.error_message);
        }

        return result;
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

// 构建增强的上下文信息
#ifdef BUILD_AI_CLIENT_SUPPORT
void Editor::buildEnhancedContext(pnana::features::ai_client::AIRequest& request) const {
    // 添加项目基本信息
    request.context.push_back("Project root directory: " +
                              std::filesystem::current_path().string());

    // 添加对话历史上下文
    std::string conversation_summary = ai_assistant_panel_.getConversationSummary();
    if (!conversation_summary.empty() && conversation_summary != "No previous conversation.") {
        request.context.push_back("Conversation history:\n" + conversation_summary);
    }

    // 添加当前文件信息
    const Document* doc = getCurrentDocument();
    if (doc && !doc->getFilePath().empty()) {
        request.context.push_back("Current file: " + doc->getFilePath());
        request.context.push_back("File extension: " + doc->getFileExtension());
        request.context.push_back("File size: " + std::to_string(doc->getContent().size()) +
                                  " characters");

        // 添加文件类型检测
        std::string file_type =
            utils::FileTypeDetector::detectFileType(doc->getFileName(), doc->getFileExtension());
        request.context.push_back("Detected file type: " + file_type);

        // 添加当前文件内容（限制大小）
        if (!doc->getContent().empty()) {
            std::string content = doc->getContent();
            if (content.size() > 8000) { // 限制上下文大小
                content = content.substr(0, 8000) + "\n... [content truncated]";
            }
            request.context.push_back("Current file content:\n" + content);
        }
    }

    // 添加选中的代码作为上下文
    std::string selected_code = getSelectedText();
    if (!selected_code.empty()) {
        request.context.push_back("Selected code:\n" + selected_code);

        // 添加选择位置信息
        if (selection_active_) {
            request.context.push_back(
                "Selection range: lines " +
                std::to_string(std::min(selection_start_row_, cursor_row_) + 1) + " to " +
                std::to_string(std::max(selection_start_row_, cursor_row_) + 1));
        }
    }

    // 添加光标位置信息
    request.context.push_back("Cursor position: line " + std::to_string(cursor_row_ + 1) +
                              ", column " + std::to_string(cursor_col_ + 1));

    // 添加项目结构概览
    addProjectStructureContext(request);

    // 添加最近文件信息
    addRecentFilesContext(request);

    // 添加当前会话状态
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
        LOG("Async format: Starting background formatting thread");
        bool success = lsp_formatter_->formatFiles(file_paths);
        LOG("Async format: Formatting completed, success: " +
            std::string(success ? "true" : "false"));

        // 在主线程中更新状态消息
        LOG("Async format: Posting UI update to main thread");
        screen_.Post([this, success, count = file_paths.size()]() {
            LOG("Async format: UI update callback executed");
            if (success) {
                setStatusMessage("✓ Successfully formatted " + std::to_string(count) + " file(s)");
            } else {
                setStatusMessage("✗ Failed to format some files. Check LSP server status.");
            }
            LOG("Async format: Status message updated");
        });
        LOG("Async format: Background thread completed");
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
    }

    // 保存当前状态
    region_states_[region_index].cursor_row = cursor_row_;
    region_states_[region_index].cursor_col = cursor_col_;
    region_states_[region_index].view_offset_row = view_offset_row_;
    region_states_[region_index].view_offset_col = view_offset_col_;
}

void Editor::restoreRegionState(size_t region_index) {
    if (region_index >= region_states_.size()) {
        // 如果没有保存的状态，使用默认值
        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0;
        view_offset_col_ = 0;
        return;
    }

    // 恢复状态
    cursor_row_ = region_states_[region_index].cursor_row;
    cursor_col_ = region_states_[region_index].cursor_col;
    view_offset_row_ = region_states_[region_index].view_offset_row;
    view_offset_col_ = region_states_[region_index].view_offset_col;

    // 调整光标位置以确保有效
    adjustCursor();
    adjustViewOffset();
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
    }
    setStatusMessage("Focus left region");

    // 强制UI更新，确保标签栏立即刷新显示当前分屏的文档
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
    }
    setStatusMessage("Focus down region");

    // 强制UI更新，确保标签栏立即刷新显示当前分屏的文档
    force_ui_update_ = true;
}

#ifdef BUILD_LUA_SUPPORT
void Editor::initializePlugins() {
    plugin_manager_ = std::make_unique<plugins::PluginManager>(this);
    if (!plugin_manager_ || !plugin_manager_->initialize()) {
        LOG_ERROR("Failed to initialize plugin system");
        plugin_manager_.reset();
    } else {
        // 设置插件管理对话框的插件管理器指针
        plugin_manager_dialog_.setPluginManager(plugin_manager_.get());
    }
}

void Editor::openPluginManager() {
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
        LOG("[DEBUG INCREMENTAL] Triggering pending cursor update");
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

} // namespace core
} // namespace pnana
