// UI渲染相关实现
#include "core/editor.h"
#include "core/ui/border_manager.h"
#include "core/ui/ui_router.h"
#include "features/cursor/cursor_renderer.h"
#include "features/image_preview.h"
#include "features/ssh/ssh_client.h"
#include "ui/binary_file_view.h"
#include "ui/create_folder_dialog.h"
#include "ui/cursor_config_dialog.h"
#include "ui/extract_dialog.h"
#include "ui/extract_path_dialog.h"
#include "ui/extract_progress_dialog.h"
#include "ui/icons.h"
#include "ui/new_file_prompt.h"
#include "ui/save_as_dialog.h"
#include "ui/statusbar.h"
#include "ui/terminal_ui.h"
#include "ui/theme_menu.h"
#include "ui/welcome_screen.h"
#include "utils/bracket_matcher.h"
#include "utils/file_type_detector.h"
#include "utils/logger.h"
#include "utils/text_utils.h"

using namespace pnana::ui::icons;

// 获取git信息（异步缓存以提高性能）
static std::string cached_git_branch;
static int cached_git_uncommitted_count = -1;
static std::chrono::steady_clock::time_point last_git_check;
static const auto GIT_CACHE_DURATION = std::chrono::seconds(30); // 30秒缓存，减少频繁调用
static std::mutex git_cache_mutex;
static std::atomic<bool> git_update_in_progress(false);

static void updateGitInfoAsync() {
    // 如果正在更新中，直接返回
    if (git_update_in_progress.load()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    if (cached_git_uncommitted_count != -1 && (now - last_git_check) <= GIT_CACHE_DURATION) {
        return; // 缓存仍然有效
    }

    // 标记开始更新
    git_update_in_progress.store(true);

    // 在后台线程中执行git命令
    std::thread([]() {
        try {
            auto [branch, count] = pnana::ui::Statusbar::getGitInfo();

            // 使用互斥锁保护共享数据
            std::lock_guard<std::mutex> lock(git_cache_mutex);
            cached_git_branch = branch;
            cached_git_uncommitted_count = count;
            last_git_check = std::chrono::steady_clock::now();
        } catch (...) {
            // 静默处理错误
        }

        // 标记更新完成
        git_update_in_progress.store(false);
    }).detach(); // 分离线程，让它在后台运行
}

static void updateGitInfo() {
    // 异步更新git信息（非阻塞）
    updateGitInfoAsync();
}

static std::string expandTabsForDisplay(const std::string& s, int tab_size) {
    if (tab_size <= 0)
        tab_size = 4;
    std::string out;
    out.reserve(s.size() * 2);
    size_t col = 0;
    for (char c : s) {
        if (c == '\t') {
            size_t spaces = static_cast<size_t>(tab_size) - (col % static_cast<size_t>(tab_size));
            out.append(spaces, ' ');
            col += spaces;
        } else {
            out.push_back(c);
            col += 1;
        }
    }
    return out;
}

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <climits>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <map>
#include <mutex>
#include <regex>
#include <sstream>
#include <thread>

using namespace ftxui;

namespace pnana {
namespace core {

// 与 editor_input 中一致：超过此行数视为大文件，搜索高亮用按行即时计算
constexpr size_t LARGE_FILE_SEARCH_HIGHLIGHT_THRESHOLD = 50000;

// 在单行内找出所有匹配（用于大文件搜索时的按行高亮），与 SearchEngine 的选项一致
static void findMatchesInLine(const std::string& line, size_t line_num, const std::string& pattern,
                              const features::SearchOptions& options,
                              std::vector<features::SearchMatch>& out) {
    if (pattern.empty())
        return;
    if (options.regex) {
        try {
            std::regex::flag_type flags = std::regex::ECMAScript;
            if (!options.case_sensitive)
                flags |= std::regex::icase;
            std::regex re(pattern, flags);
            std::sregex_iterator it(line.begin(), line.end(), re), end;
            for (; it != end; ++it) {
                out.emplace_back(line_num, it->position(), it->length());
            }
        } catch (const std::regex_error&) {
        }
        return;
    }
    std::string search_line = line;
    std::string search_pattern = pattern;
    if (!options.case_sensitive) {
        std::transform(search_line.begin(), search_line.end(), search_line.begin(), ::tolower);
        std::transform(search_pattern.begin(), search_pattern.end(), search_pattern.begin(),
                       ::tolower);
    }
    size_t pos = 0;
    while (pos < search_line.size()) {
        size_t col = search_line.find(search_pattern, pos);
        if (col == std::string::npos)
            break;
        if (options.whole_word) {
            bool word_start =
                (col == 0) || !std::isalnum(static_cast<unsigned char>(search_line[col - 1]));
            bool word_end =
                (col + pattern.size() >= search_line.size()) ||
                !std::isalnum(static_cast<unsigned char>(search_line[col + pattern.size()]));
            if (!word_start || !word_end) {
                pos = col + 1;
                continue;
            }
        }
        out.emplace_back(line_num, col, pattern.size());
        pos = col + pattern.size();
    }
}

// 根据文档最大行号计算行号区域所需字符宽度（至少 2 格便于对齐）
static size_t getLineNumberWidthForLineCount(size_t line_count) {
    if (line_count == 0)
        return 2;
    size_t digits = 0;
    for (size_t n = line_count; n > 0; n /= 10)
        digits++;
    return digits < 2 ? 2 : digits;
}

int Editor::getContentOriginX() const {
    return 0;
}

int Editor::getContentOriginY() const {
    int origin = 0;
    origin += 1; // tabbar
    origin += 1; // separator
    return origin;
}

int Editor::getContentBottomY() const {
    int screen_height = screen_.dimy();
    int reserved_height = 0;
    reserved_height += 1; // tabbar
    reserved_height += 1; // separator
    reserved_height += 1; // statusbar
    if (mode_ == EditorMode::SEARCH || mode_ == EditorMode::REPLACE) {
        reserved_height += 1; // input box
    }
    if (show_helpbar_) {
        reserved_height += 1; // helpbar
    }
    return std::max(0, screen_height - reserved_height);
}

int Editor::getCompletionAnchorX() const {
    // 补全弹窗锚点直接使用编辑区左侧坐标系内的光标列
    return cursor_col_;
}

int Editor::getCompletionAnchorY() const {
    // 当前编辑区起始于 tabbar+separator 之后
    return getContentOriginY() + static_cast<int>(cursor_row_);
}

// UI渲染
Element Editor::renderUI() {
    // PTY 有新输出时强制刷新（由 on_output_added_ 原子标记，主线程在此消费）
    if (terminal_has_output_.exchange(false, std::memory_order_relaxed)) {
        force_ui_update_ = true;
        needs_render_ = true;
    }

    // 检查是否暂停渲染
    if (rendering_paused_) {
        needs_render_ = true;
        return last_rendered_element_; // 返回上次渲染结果
    }

    // Markdown预览延迟更新检查
    auto current_time = std::chrono::steady_clock::now();
    if (markdown_preview_needs_update_ &&
        current_time - last_markdown_preview_update_time_ >= markdown_preview_update_delay_) {
        // 延迟时间已到，触发预览更新
        force_ui_update_ = true;
        markdown_preview_needs_update_ = false;
        last_markdown_preview_update_time_ = current_time;
    }

    // 增量渲染优化：抑制快速的光标移动渲染
    auto time_since_last_render = current_time - last_render_time_;

    // 检查是否是高优先级更新（诊断、折叠状态变化等）
    bool is_high_priority_update = last_render_source_.find("diagnostic") != std::string::npos ||
                                   last_render_source_.find("folding") != std::string::npos ||
                                   last_render_source_.find("lsp") != std::string::npos;

    // 优化渲染逻辑：对LSP状态变化更敏感
    bool is_lsp_state_change = last_render_source_.find("diagnostic") != std::string::npos ||
                               last_render_source_.find("folding") != std::string::npos ||
                               last_render_source_.find("lsp") != std::string::npos;

    // 渲染去抖机制：合并短时间内的多个渲染请求（轻量实现，无额外调试日志）
    static auto last_needs_render_time = std::chrono::steady_clock::now();
    const auto RENDER_DEBOUNCE_INTERVAL = std::chrono::milliseconds(50); // 50ms去抖间隔

    // 检查是否需要渲染
    bool should_render = force_ui_update_ || is_high_priority_update || is_lsp_state_change ||
                         time_since_last_render >= MIN_RENDER_INTERVAL ||
                         (last_render_source_.find("resumeRendering") != std::string::npos ||
                          last_render_source_.find("Event::Custom") != std::string::npos);

    // 如果有needs_render_请求，应用去抖（无日志）
    if (!should_render && needs_render_) {
        auto now = std::chrono::steady_clock::now();
        auto time_since_last_needs_render = now - last_needs_render_time;

        if (time_since_last_needs_render >= RENDER_DEBOUNCE_INTERVAL || is_high_priority_update) {
            should_render = true;
            needs_render_ = false; // 重置标志
            last_needs_render_time = now;
        }
    }

    if (should_render) {
        // 允许渲染，更新时间戳
        last_render_time_ = current_time;
        last_render_source_ = "";
        pending_cursor_update_ = false;
        force_ui_update_ = false; // 重置强制更新标志
    } else {
        // 标记有待处理的更新，稍后会通过定时器或事件触发
        pending_cursor_update_ = true;
        // 返回上次渲染结果，避免闪烁
        return last_rendered_element_;
    }

    // 使用 UIRouter 进行渲染（如果已初始化）
    // UIRouter::render() 内部已调用 overlayDialogs，无需再叠加一次，否则会导致 AI 面板等被渲染两次
    if (ui_router_) {
        last_rendered_element_ = ui_router_->render(this);
        return last_rendered_element_;
    }

    // 如果 UIRouter 未初始化，使用原有逻辑
    last_rendered_element_ = renderUILegacy();
    return last_rendered_element_;
}

// 原有的 UI 渲染逻辑（保留作为后备）
Element Editor::renderUILegacy() {
    Element editor_content;

    // 如果文件浏览器打开，使用左右分栏布局，位置可通过配置切换
    if (file_browser_.isVisible()) {
        auto fb = renderFileBrowser() | size(WIDTH, EQUAL, file_browser_width_);
        auto code = renderEditor() | flex;
        // 默认文件列表在左侧；当 display.file_browser_side == "right" 时放到右侧
        const auto& display_cfg = config_manager_.getConfig().display;
        bool file_browser_on_left = display_cfg.file_browser_side != "right";
        if (file_browser_on_left) {
            editor_content = hbox({fb, separator(), code});
        } else {
            editor_content = hbox({code, separator(), fb});
        }
    } else {
        editor_content = renderEditor() | flex;
    }

    // 如果终端打开，使用上下分栏布局，位置可通过配置切换
    Element main_content;
    int screen_height = screen_.dimy();
    int reserved_height = 0;
    reserved_height += 1; // tabbar
    reserved_height += 1; // separator
    reserved_height += 1; // statusbar
    if (mode_ == EditorMode::SEARCH || mode_ == EditorMode::REPLACE) {
        reserved_height += 1; // input box
    }
    if (show_helpbar_) {
        reserved_height += 1; // helpbar
    }
    LOG_DEBUG("[EditorUI] layout screen_h=" + std::to_string(screen_height) + " reserved=" +
              std::to_string(reserved_height) + " mode=" + std::to_string(static_cast<int>(mode_)) +
              " helpbar=" + std::string(show_helpbar_ ? "1" : "0") +
              " terminal=" + std::string(terminal_.isVisible() ? "1" : "0"));
    int main_available = std::max(3, screen_height - reserved_height);
    if (terminal_.isVisible()) {
        int terminal_height = terminal_height_;
        if (terminal_height <= 0) {
            // 使用默认高度（主区域高度的1/3）
            terminal_height = main_available / 3;
        }
        int min_editor_height = 3;
        int max_terminal = std::max(1, main_available - min_editor_height - 1); // 分隔线占1行
        if (terminal_height > max_terminal) {
            terminal_height = max_terminal;
        }
        if (terminal_height < 1) {
            terminal_height = 1;
        }
        bool terminal_on_top = config_manager_.getConfig().display.terminal_side == "top";
        LOG_DEBUG("[TerminalLayout] screen=" + std::to_string(screen_height) + " reserved=" +
                  std::to_string(reserved_height) + " main=" + std::to_string(main_available) +
                  " term=" + std::to_string(terminal_height) +
                  " editor_min=" + std::to_string(min_editor_height) +
                  " on_top=" + std::string(terminal_on_top ? "1" : "0") + " visible=1");
        if (terminal_on_top) {
            main_content = vbox({renderTerminal() | size(HEIGHT, EQUAL, terminal_height),
                                 separator(), editor_content | flex});
        } else {
            main_content = vbox({editor_content | flex, separator(),
                                 renderTerminal() | size(HEIGHT, EQUAL, terminal_height)});
        }
    } else {
        LOG_DEBUG("[TerminalLayout] screen=" + std::to_string(screen_height) +
                  " reserved=" + std::to_string(reserved_height) +
                  " main=" + std::to_string(main_available) + " visible=0");
        main_content = editor_content;
    }

    // 主内容区使用 flex，避免终端高度变化时状态栏位置上下移动
    auto main_ui = vbox({renderTabbar(), separator(), main_content | flex, renderStatusbar(),
                         renderInputBox(), renderHelpbar()}) |
                   bgcolor(theme_.getColors().background);

    return overlayDialogs(main_ui);
}

// 叠加对话框
Element Editor::overlayDialogs(Element main_ui) {
    if (!overlay_manager_) {
        return main_ui;
    }

    // 设置渲染回调函数
    overlay_manager_->setRenderHelpCallback([this]() {
        return renderHelp();
    });
    overlay_manager_->setRenderThemeMenuCallback([this]() {
        return theme_menu_.render();
    });
    overlay_manager_->setRenderLogoMenuCallback([this]() {
        return logo_menu_.render();
    });
    overlay_manager_->setRenderAnimationMenuCallback([this]() {
        return animation_menu_.render();
    });
    overlay_manager_->setRenderStatusbarStyleMenuCallback([this]() {
        return statusbar_style_menu_.render();
    });
    overlay_manager_->setRenderCreateFolderCallback([this]() {
        return create_folder_dialog_.render();
    });
    overlay_manager_->setRenderSaveAsCallback([this]() {
        return save_as_dialog_.render();
    });
    overlay_manager_->setRenderMoveFileCallback([this]() {
        return move_file_dialog_.render();
    });
    overlay_manager_->setRenderExtractCallback([this]() {
        return extract_dialog_.render();
    });
    overlay_manager_->setRenderExtractPathCallback([this]() {
        return extract_path_dialog_.render();
    });
    overlay_manager_->setRenderExtractProgressCallback([this]() {
        return extract_progress_dialog_.render();
    });
    overlay_manager_->setRenderCursorConfigCallback([this]() {
        return cursor_config_dialog_.render();
    });
    overlay_manager_->setRenderAIConfigCallback([this]() {
        return ai_config_dialog_.render();
    });
    overlay_manager_->setRenderAIAssistantCallback([this]() {
        ftxui::Element el = ai_assistant_panel_.render();
        bool ai_active = (region_manager_.getCurrentRegion() == EditorRegion::AI_ASSISTANT_PANEL);
        core::ui::BorderManager bm;
        return bm.applyBorder(el, EditorRegion::AI_ASSISTANT_PANEL, ai_active, theme_);
    });
    overlay_manager_->setRenderCommandPaletteCallback([this]() {
        return renderCommandPalette();
    });
    overlay_manager_->setRenderRecentFilesCallback([this]() {
        auto recent_projects = recent_files_manager_.getRecentProjects();
        recent_files_popup_.setData(recent_files_popup_.isOpen(), recent_projects,
                                    recent_files_popup_.getSelectedIndex());
        return recent_files_popup_.render();
    });
    overlay_manager_->setRenderFormatCallback([this]() {
        return format_dialog_.render();
    });
    overlay_manager_->setRenderGitPanelCallback([this]() {
        return renderGitPanel();
    });
    overlay_manager_->setRenderTodoPanelCallback([this]() {
        return todo_panel_.render();
    });
    overlay_manager_->setRenderPackageManagerPanelCallback([this]() {
        return package_manager_panel_.render();
    });
    overlay_manager_->setRenderFilePickerCallback([this]() {
        return file_picker_.render();
    });
    overlay_manager_->setRenderSplitDialogCallback([this]() {
        return split_dialog_.render();
    });
    overlay_manager_->setRenderSSHTansferCallback([this]() {
        return ssh_transfer_dialog_.render();
    });
    overlay_manager_->setRenderSSHDialogCallback([this]() {
        return ssh_dialog_.render();
    });
    overlay_manager_->setRenderTerminalSessionDialogCallback([this]() {
        return terminal_session_dialog_.render();
    });
    overlay_manager_->setRenderEncodingDialogCallback([this]() {
        return encoding_dialog_.render();
    });
    overlay_manager_->setRenderRecentFilesCallback([this]() {
        auto recent_projects = recent_files_manager_.getRecentProjects();
        recent_files_popup_.setData(recent_files_popup_.isOpen(), recent_projects,
                                    recent_files_popup_.getSelectedIndex());
        return recent_files_popup_.render();
    });
    overlay_manager_->setIsRecentFilesVisibleCallback([this]() {
        return recent_files_popup_.isOpen();
    });
    overlay_manager_->setRenderFzfPopupCallback([this]() {
        return fzf_popup_.render();
    });
    overlay_manager_->setRenderHistoryTimelinePopupCallback([this]() {
        return history_timeline_popup_.render();
    });
    overlay_manager_->setRenderHistoryDiffPopupCallback([this]() {
        return history_diff_popup_.render();
    });
    overlay_manager_->setIsFzfPopupVisibleCallback([this]() {
        return fzf_popup_.isOpen();
    });
    overlay_manager_->setIsHistoryTimelinePopupVisibleCallback([this]() {
        return history_timeline_popup_.isOpen();
    });
    overlay_manager_->setIsHistoryDiffPopupVisibleCallback([this]() {
        return history_diff_popup_.isOpen();
    });
#ifdef BUILD_LSP_SUPPORT
    overlay_manager_->setRenderLspStatusPopupCallback([this]() {
        return lsp_status_popup_.render();
    });
    overlay_manager_->setIsLspStatusPopupVisibleCallback([this]() {
        return lsp_status_popup_.isOpen();
    });
#endif
    overlay_manager_->setRenderTUIConfigCallback([this]() {
        // 仅打开时用已有列表，避免每次渲染都拉取（SSH 下会数百次远程调用导致卡顿）
        const auto& configs = tui_config_popup_.getCurrentConfigs();
        tui_config_popup_.setData(tui_config_popup_.isOpen(), configs,
                                  tui_config_popup_.getSelectedIndex());
        return tui_config_popup_.render();
    });
    overlay_manager_->setIsTUIConfigVisibleCallback([this]() {
        return tui_config_popup_.isOpen();
    });
    overlay_manager_->setRenderDialogCallback([this]() {
        return dialog_.render();
    });
    overlay_manager_->setIsDialogVisibleCallback([this]() {
        return dialog_.isVisible();
    });

#ifdef BUILD_LUA_SUPPORT
    overlay_manager_->setRenderPluginManagerCallback([this]() {
        return plugin_manager_dialog_.render();
    });
#endif

#ifdef BUILD_LSP_SUPPORT
    overlay_manager_->setRenderCompletionPopupCallback([this]() {
        return renderCompletionPopup();
    });
    overlay_manager_->setRenderDiagnosticsPopupCallback([this]() {
        return renderDiagnosticsPopup();
    });
    overlay_manager_->setRenderSymbolNavigationPopupCallback([this]() {
        return renderSymbolNavigationPopup();
    });
#endif

    // 设置可见性检查回调函数
    overlay_manager_->setIsHelpVisibleCallback([this]() {
        return show_help_;
    });
    overlay_manager_->setIsThemeMenuVisibleCallback([this]() {
        return show_theme_menu_;
    });
    overlay_manager_->setIsLogoMenuVisibleCallback([this]() {
        return show_logo_menu_;
    });
    overlay_manager_->setIsAnimationMenuVisibleCallback([this]() {
        return show_animation_menu_;
    });
    overlay_manager_->setIsStatusbarStyleMenuVisibleCallback([this]() {
        return show_statusbar_style_menu_;
    });
    overlay_manager_->setIsCreateFolderVisibleCallback([this]() {
        return show_create_folder_;
    });
    overlay_manager_->setIsSaveAsVisibleCallback([this]() {
        return show_save_as_;
    });
    overlay_manager_->setIsMoveFileVisibleCallback([this]() {
        return show_move_file_;
    });
    overlay_manager_->setIsExtractVisibleCallback([this]() {
        return show_extract_dialog_;
    });
    overlay_manager_->setIsExtractPathVisibleCallback([this]() {
        return show_extract_path_dialog_;
    });
    overlay_manager_->setIsExtractProgressVisibleCallback([this]() {
        return show_extract_progress_dialog_;
    });
    overlay_manager_->setIsCursorConfigVisibleCallback([this]() {
        return cursor_config_dialog_.isVisible();
    });
    overlay_manager_->setIsAIConfigVisibleCallback([this]() {
        return ai_config_dialog_.isVisible();
    });
    overlay_manager_->setIsAIAssistantVisibleCallback([this]() {
        return ai_assistant_panel_.isVisible();
    });
    overlay_manager_->setIsCommandPaletteVisibleCallback([this]() {
        return command_palette_.isOpen();
    });
    overlay_manager_->setIsFormatVisibleCallback([this]() {
        return format_dialog_.isOpen();
    });
    overlay_manager_->setIsGitPanelVisibleCallback([this]() {
        return isGitPanelVisible();
    });
    overlay_manager_->setIsTodoPanelVisibleCallback([this]() {
        return todo_panel_.isVisible();
    });
    overlay_manager_->setIsPackageManagerPanelVisibleCallback([this]() {
        return package_manager_panel_.isVisible();
    });
    overlay_manager_->setIsFilePickerVisibleCallback([this]() {
        return file_picker_.isVisible();
    });
    overlay_manager_->setIsSplitDialogVisibleCallback([this]() {
        return split_dialog_.isVisible();
    });
    overlay_manager_->setIsSSHTansferVisibleCallback([this]() {
        return ssh_transfer_dialog_.isVisible();
    });
    overlay_manager_->setIsSSHDialogVisibleCallback([this]() {
        return ssh_dialog_.isVisible();
    });
    overlay_manager_->setIsTerminalSessionDialogVisibleCallback([this]() {
        return terminal_session_dialog_.isVisible();
    });
    overlay_manager_->setIsEncodingDialogVisibleCallback([this]() {
        return encoding_dialog_.isVisible();
    });

#ifdef BUILD_LUA_SUPPORT
    overlay_manager_->setIsPluginManagerVisibleCallback([this]() {
        return plugin_manager_dialog_.isVisible();
    });
#endif

#ifdef BUILD_LSP_SUPPORT
    overlay_manager_->setIsCompletionPopupVisibleCallback([this]() {
        return config_manager_.getConfig().lsp.completion_popup_enabled &&
               completion_popup_.isVisible();
    });
    overlay_manager_->setIsDiagnosticsPopupVisibleCallback([this]() {
        return show_diagnostics_popup_;
    });
    overlay_manager_->setIsSymbolNavigationPopupVisibleCallback([this]() {
        return show_symbol_navigation_popup_ && symbol_navigation_popup_.isVisible();
    });
#endif

    // 使用 OverlayManager 渲染叠加窗口
    auto overlayed = overlay_manager_->renderOverlays(main_ui);

    // 叠加插件 PopupManager（内核弹窗）
    if (popup_manager_) {
        overlayed = popup_manager_->render(overlayed, screen_.dimx(), screen_.dimy());
    }

    // 更新并渲染 Toast 通知（右下角，叠加效果）
    toast_.update();
    if (toast_.isVisible()) {
        // 渲染 Toast（使用主题色）
        auto toast_element = toast_.render(theme_);

        // 使用 dbox 实现叠加效果，Toast 浮动在右下角
        overlayed = dbox({overlayed, hbox({filler(), vbox({filler(), toast_element})})});
    }

    return overlayed;
}

Element Editor::renderTabbar() {
    auto tabs = document_manager_.getAllTabs();

    // 如果没有文档，显示"Welcome"标签
    if (tabs.empty()) {
        return hbox({text(" "), text(pnana::ui::icons::ROCKET) | color(theme_.getColors().keyword),
                     text(" Welcome ") | color(theme_.getColors().foreground) | bold, text(" ")}) |
               bgcolor(theme_.getColors().menubar_bg);
    }

    // 如果有分屏，修改标签显示以反映当前激活区域的文档，实现完全隔离
    if (split_view_manager_.hasSplits()) {
        const auto* active_region = split_view_manager_.getActiveRegion();
        if (active_region) {
            // 如果激活区域显示欢迎页面，显示特殊的欢迎标签
            if (active_region->current_document_index == SIZE_MAX) {
                return hbox({text(" "),
                             text(pnana::ui::icons::SPLIT) | color(theme_.getColors().keyword),
                             text(" Split View - Open a file ") |
                                 color(theme_.getColors().foreground) | bold,
                             text(" ")}) |
                       bgcolor(theme_.getColors().menubar_bg);
            }

            // 创建过滤后的标签列表，只显示当前激活分屏区域管理的文档
            std::vector<DocumentManager::TabInfo> filtered_tabs;

            // 获取当前激活区域的文档索引列表
            const auto& region_doc_indices = active_region->document_indices;

            // 根据文档索引列表创建标签
            for (size_t doc_index : region_doc_indices) {
                if (doc_index < tabs.size()) {
                    DocumentManager::TabInfo tab_info = tabs[doc_index];
                    // 如果这是当前激活区域正在编辑的文档，标记为当前
                    tab_info.is_current = (doc_index == active_region->current_document_index);
                    filtered_tabs.push_back(tab_info);
                }
            }

            // 如果有过滤后的标签，使用它们；否则显示所有标签
            if (!filtered_tabs.empty()) {
                return tabbar_.render(filtered_tabs);
            }
        }
    }

    return tabbar_.render(tabs);
}

Element Editor::renderEditor() {
    // 如果启用了分屏（区域数量 > 1），使用分屏渲染
    if (split_view_manager_.hasSplits()) {
        return renderSplitEditor();
    }

    // 单视图渲染（没有分屏）
    Document* doc = getCurrentDocument();

    // 图片预览逻辑：始终尝试加载图片预览（双后端模式）
    if (doc && !doc->getFilePath().empty() &&
        features::ImagePreview::isImageFile(doc->getFilePath())) {
        std::string image_path = doc->getFilePath();

        // SSH 远程图片：先下载到本地临时文件，再走现有图片预览链路
        if (image_path.rfind("ssh://", 0) == 0) {
            auto it = remote_image_temp_files_.find(image_path);
            if (it != remote_image_temp_files_.end() && std::filesystem::exists(it->second)) {
                image_path = it->second;
            } else if (!current_ssh_config_.host.empty()) {
                std::regex ssh_regex(R"(^ssh://([^@]+)@([^:/]+)(?::(\d+))?(/.*)$)");
                std::smatch m;
                const std::string remote_uri = doc->getFilePath();
                if (std::regex_match(remote_uri, m, ssh_regex)) {
                    pnana::ui::SSHConfig cfg = current_ssh_config_;
                    cfg.user = m[1].str();
                    cfg.host = m[2].str();
                    cfg.port = (m[3].matched && !m[3].str().empty()) ? std::stoi(m[3].str()) : 22;
                    cfg.remote_path = m[4].str();

                    std::string ext = std::filesystem::path(cfg.remote_path).extension().string();
                    if (ext.empty())
                        ext = ".img";
                    std::string temp_name =
                        "pnana_ssh_preview_" +
                        std::to_string(std::hash<std::string>{}(doc->getFilePath())) + ext;
                    std::string local_temp =
                        (std::filesystem::temp_directory_path() / temp_name).string();

                    features::ssh::Client ssh_client;
                    auto dl = ssh_client.downloadFile(cfg, cfg.remote_path, local_temp);
                    if (dl.success && std::filesystem::exists(local_temp)) {
                        remote_image_temp_files_[doc->getFilePath()] = local_temp;
                        image_path = local_temp;
                    }
                }
            }
        }

        // 计算代码区的实际可用尺寸
        int code_area_width = screen_.dimx();
        int code_area_height = screen_.dimy() - 7; // 减去标签栏、状态栏等6行 + 边框1行

        // 如果文件浏览器打开，减去文件浏览器的宽度
        if (file_browser_.isVisible()) {
            code_area_width -= (file_browser_width_ + 1); // +1 是分隔符
        }

        // 预留一些边距和图片信息空间（标题、尺寸、分隔符 = 3 行）
        code_area_width -= 4;
        int available_height = code_area_height - 3 - 4; // 减去图片信息行和边距

        // 确保最小尺寸
        if (code_area_width < 40)
            code_area_width = 40;
        if (available_height < 10)
            available_height = 10;

        int preview_width = code_area_width;
        int preview_height = available_height;

        if (!image_preview_.isLoaded() || image_preview_.getImagePath() != image_path ||
            image_preview_.getRenderWidth() != preview_width ||
            image_preview_.getRenderHeight() != preview_height) {
            image_preview_.loadImage(image_path, preview_width, preview_height);
        }

        if (image_preview_.isLoaded()) {
            return image_preview_.render();
        }
    }

    // 如果之前加载了图片但现在不需要，清空预览
    if (image_preview_.isLoaded()) {
        image_preview_.clear();
    }

    // 如果没有文档，显示欢迎界面
    if (!doc) {
        return welcome_screen_.render();
    }

    // 如果是图片文件，先尝试显示图片预览（双后端模式）
    bool is_image_file = features::ImagePreview::isImageFile(doc->getFilePath());

    // 如果是二进制文件（非图片），显示二进制文件视图
    if (doc->isBinary() && !is_image_file) {
        binary_file_view_.setFilePath(doc->getFilePath());
        return binary_file_view_.render();
    }

    // 如果是新文件且内容为空，显示新文件输入提示界面
    if (doc->getFilePath().empty() && doc->lineCount() == 1 && doc->getLine(0).empty()) {
        return new_file_prompt_.render();
    }

    // 括号匹配高亮：渲染前按需更新（有缓存，不会每帧重复扫描）
    updateBracketHighlight();

    Elements lines;

    // 统一计算屏幕高度：减去标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1) =
    // 6行，再减去边框(2) = 8行
    int screen_height = screen_.dimy() - 7;

    // 获取可见行（考虑折叠状态）
    std::vector<size_t> visible_lines = doc->getVisibleLines();
    size_t total_visible_lines = visible_lines.size();

    // 只在可见行数少于屏幕高度时，确保从0开始显示（这样最后一行也能显示）
    // 如果可见行数大于屏幕高度，保持当前的视图偏移，让用户自己滚动
    if (total_visible_lines > 0 && total_visible_lines <= static_cast<size_t>(screen_height)) {
        // 可见行数少于屏幕高度，从0开始显示所有行（包括最后一行）
        view_offset_row_ = 0;
    }
    // 如果可见行数大于屏幕高度，不强制调整视图偏移，保持用户当前的滚动位置

    // 计算实际显示的行数范围
    size_t max_lines = std::min(view_offset_row_ + screen_height, total_visible_lines);

    // 行号区域宽度（根据文档总行数动态计算）
    const size_t line_num_width = getLineNumberWidthForLineCount(doc->lineCount());
    const std::string empty_line_placeholder =
        show_line_numbers_ ? (std::string(line_num_width + 1, ' ') + "~") : "~";

    // 渲染可见行
    // 限制渲染的行数，避免大文件卡住
    const size_t MAX_RENDER_LINES = 200; // 最多渲染200行
    size_t render_count = std::min(max_lines - view_offset_row_, MAX_RENDER_LINES);

    try {
        for (size_t i = view_offset_row_; i < view_offset_row_ + render_count; ++i) {
            size_t actual_line_index = visible_lines[i];
            try {
                // 性能优化：对于超长行，跳过语法高亮
                std::string line_content = doc->getLine(actual_line_index);
                if (line_content.length() > 5000) {
                    // 超长行，使用简单渲染
                    Elements simple_line;
                    if (show_line_numbers_) {
                        simple_line.push_back(renderLineNumber(doc, actual_line_index,
                                                               actual_line_index == cursor_row_));
                    }
                    simple_line.push_back(text(line_content.substr(0, 5000) + "...") |
                                          color(theme_.getColors().foreground));
                    lines.push_back(hbox(simple_line));
                } else {
                    lines.push_back(renderLine(doc, actual_line_index,
                                               actual_line_index == cursor_row_, false, false,
                                               nullptr));
                }
            } catch (const std::exception& e) {
                // 如果渲染某一行失败，使用空行替代
                Elements error_line;
                if (show_line_numbers_) {
                    error_line.push_back(text(empty_line_placeholder) |
                                         color(theme_.getColors().comment));
                } else {
                    error_line.push_back(text("~") | color(theme_.getColors().comment));
                }
                lines.push_back(hbox(error_line));
            } catch (...) {
                // 如果渲染某一行失败，使用空行替代
                Elements error_line;
                if (show_line_numbers_) {
                    error_line.push_back(text(empty_line_placeholder) |
                                         color(theme_.getColors().comment));
                } else {
                    error_line.push_back(text("~") | color(theme_.getColors().comment));
                }
                lines.push_back(hbox(error_line));
            }
        }
    } catch (const std::exception& e) {
        // 如果整个渲染循环失败，返回错误信息
        return vbox({text("Error rendering file: " + std::string(e.what())) | color(Color::Red)});
    } catch (...) {
        return vbox({text("Unknown error rendering file") | color(Color::Red)});
    }

    // 填充空行
    for (int i = lines.size(); i < screen_height; ++i) {
        Elements empty_line;
        if (show_line_numbers_) {
            empty_line.push_back(text(empty_line_placeholder) | color(theme_.getColors().comment));
        } else {
            empty_line.push_back(text("~") | color(theme_.getColors().comment));
        }
        lines.push_back(hbox(empty_line));
    }
    return vbox(lines);
}

Element Editor::renderSplitEditor() {
    int screen_width = screen_.dimx();
    int screen_height = screen_.dimy() - 7; // 减去标签栏、状态栏等6行 + 边框2行

    // 检查是否有分屏，如果没有则回退到单视图
    if (!split_view_manager_.hasSplits()) {
        return renderEditor();
    }

    // 使用分屏管理器的渲染方法
    auto get_document = [this](size_t index) -> void* {
        return static_cast<void*>(document_manager_.getDocument(index));
    };

    auto switch_document = [this](size_t index) {
        document_manager_.switchToDocument(index);
    };

    auto get_document_count = [this]() -> size_t {
        return document_manager_.getDocumentCount();
    };

    auto render_region = [this](const features::ViewRegion& region, void* doc_ptr,
                                size_t region_index) -> ftxui::Element {
        Document* doc = static_cast<Document*>(doc_ptr);
        return renderEditorRegion(region, doc, region_index);
    };

    const auto& tc = theme_.getColors();
    ftxui::Element result = split_view_manager_.renderSplitEditor(
        get_document, switch_document, get_document_count, render_region, screen_width,
        screen_height, tc.dialog_border, tc.line_number_current);

    // 如果渲染结果为空（表示单区域尺寸无效），重置分屏管理器并回退
    if (result == ftxui::text("")) {
        split_view_manager_.reset();
        Document* doc = getCurrentDocument();
        if (!doc) {
            return welcome_screen_.render();
        }
        return renderEditor();
    }

    return result;
}

Element Editor::renderEditorRegion(const features::ViewRegion& region, Document* doc,
                                   size_t region_index) {
    // 如果没有文档，在分屏模式下显示分屏欢迎界面
    if (!doc) {
        if (split_view_manager_.hasSplits()) {
            // 在分屏模式下，显示分屏欢迎界面
            return split_welcome_screen_.render() | size(HEIGHT, EQUAL, region.height);
        } else {
            // 在单视图模式下，显示传统的空行
            Elements empty_lines;
            for (int i = 0; i < region.height; ++i) {
                empty_lines.push_back(text("~") | color(theme_.getColors().comment));
            }
            return vbox(empty_lines);
        }
    }

    // 如果是图片文件，先尝试显示图片预览（双后端模式）
    bool is_image_file = features::ImagePreview::isImageFile(doc->getFilePath());

    // 如果是二进制文件（非图片），显示二进制文件视图
    if (doc->isBinary() && !is_image_file) {
        binary_file_view_.setFilePath(doc->getFilePath());
        return binary_file_view_.render();
    }

    // 图片预览逻辑：在分屏模式下为每个区域独立加载图片预览
    if (is_image_file && !doc->getFilePath().empty()) {
        std::string image_path = doc->getFilePath();

        // 计算区域的实际可用尺寸
        int preview_width = region.width;
        int preview_height = region.height - 3; // 减去标题、尺寸、分隔符

        // 确保最小尺寸
        if (preview_width < 20)
            preview_width = 20;
        if (preview_height < 5)
            preview_height = 5;

        // 检查是否需要重新加载图片
        bool need_reload = false;
        if (region_index < region_states_.size()) {
            auto& region_state = region_states_[region_index];
            if (!region_state.image_loaded_ || region_state.image_path_ != image_path ||
                region_state.image_render_width_ != preview_width ||
                region_state.image_render_height_ != preview_height) {
                need_reload = true;
            }
        } else {
            need_reload = true;
        }

        // 重新加载图片预览
        if (need_reload) {
            // 使用临时对象加载图片，避免污染全局的 image_preview_
            features::ImagePreview temp_preview;
            temp_preview.loadImage(image_path, preview_width, preview_height);

            // 保存到区域状态
            if (region_index < region_states_.size()) {
                auto& region_state = region_states_[region_index];
                region_state.image_path_ = image_path;
                region_state.image_width_ = temp_preview.getImageWidth();
                region_state.image_height_ = temp_preview.getImageHeight();
                region_state.image_preview_lines_ = temp_preview.getPreviewLines();
                region_state.image_preview_pixels_ = temp_preview.getPreviewPixels();
                region_state.image_loaded_ = temp_preview.isLoaded();
                region_state.image_render_width_ = preview_width;
                region_state.image_render_height_ = preview_height;
            }
        }

        // 使用区域保存的预览数据渲染
        if (region_index < region_states_.size()) {
            auto& region_state = region_states_[region_index];
            if (region_state.image_loaded_) {
                // 创建临时的 ImagePreview 对象用于渲染，避免影响全局的 image_preview_
                features::ImagePreview temp_preview;
                temp_preview.setPreviewData(
                    region_state.image_preview_lines_, region_state.image_preview_pixels_,
                    region_state.image_path_, region_state.image_width_, region_state.image_height_,
                    region_state.image_render_width_, region_state.image_render_height_);
                auto result = temp_preview.render() | size(HEIGHT, EQUAL, region.height) |
                              size(WIDTH, EQUAL, region.width);
                return result;
            }
        }
    }

    // 括号匹配高亮：渲染前按需更新（有缓存，不会每帧重复扫描）
    updateBracketHighlight();

    Elements lines;

    // 获取可见行（考虑折叠状态）
    std::vector<size_t> visible_lines = doc->getVisibleLines();
    size_t total_visible_lines = visible_lines.size();
    int region_height = region.height;

    // 获取该区域的状态：激活区域用全局光标/视口（实时），非激活区域用各自保存的状态
    size_t region_cursor_row;
    size_t region_view_offset_row;
    size_t region_view_offset_col;
    if (region.is_active) {
        region_cursor_row = cursor_row_;
        region_view_offset_row = view_offset_row_;
        region_view_offset_col = view_offset_col_;
    } else if (region_index < region_states_.size()) {
        region_cursor_row = region_states_[region_index].cursor_row;
        region_view_offset_row = region_states_[region_index].view_offset_row;
        region_view_offset_col = region_states_[region_index].view_offset_col;
    } else {
        region_cursor_row = 0;
        region_view_offset_row = 0;
        region_view_offset_col = 0;
    }

    // 使用区域特定的视图偏移
    size_t start_line = region_view_offset_row;
    size_t max_lines = std::min(start_line + region_height, total_visible_lines);

    // 获取该区域的单词高亮状态
    bool region_word_highlight_active = false;
    const std::vector<features::SearchMatch>* region_word_matches = nullptr;

    if (region_index < region_states_.size()) {
        region_word_highlight_active = region_states_[region_index].word_highlight_active_;
        region_word_matches = &region_states_[region_index].word_matches_;
    }

    // 渲染可见行
    for (size_t i = start_line; i < max_lines && i < start_line + region_height; ++i) {
        size_t actual_line_index = visible_lines[i];
        bool is_current = (region.is_active && actual_line_index == region_cursor_row);
        lines.push_back(renderLine(doc, actual_line_index, is_current, true,
                                   region_word_highlight_active, region_word_matches, region.width,
                                   region_view_offset_col));
    }

    // 填充空行（行号宽度与文档总行数一致）
    const size_t region_line_num_width = getLineNumberWidthForLineCount(doc->lineCount());
    const std::string region_empty_placeholder =
        show_line_numbers_ ? (std::string(region_line_num_width + 1, ' ') + "~") : "~";
    for (int i = lines.size(); i < region_height; ++i) {
        Elements empty_line;
        if (show_line_numbers_) {
            empty_line.push_back(text(region_empty_placeholder) |
                                 color(theme_.getColors().comment));
        } else {
            empty_line.push_back(text("~") | color(theme_.getColors().comment));
        }
        lines.push_back(hbox(empty_line));
    }

    return vbox(lines);
}

Element Editor::renderLine(Document* doc, size_t line_num, bool is_current,
                           bool use_region_word_highlight, bool region_word_highlight_active,
                           const std::vector<features::SearchMatch>* region_word_matches,
                           int max_width, size_t view_offset_col) {
    Elements line_elements;

    // 创建光标渲染器并配置
    pnana::ui::CursorRenderer cursor_renderer;
    pnana::ui::CursorConfig cursor_config;
    cursor_config.style = static_cast<pnana::ui::CursorStyle>(getCursorStyle());
    cursor_config.color = getCursorColor();
    cursor_config.smooth = getCursorSmooth();
    // 闪烁开关：由光标配置面板控制
    cursor_config.blink_enabled = cursor_config_dialog_.getBlinkEnabled();
    cursor_renderer.setConfig(cursor_config);
    // 闪烁频率：沿用现有的光标频率设置
    cursor_renderer.setBlinkRate(getCursorBlinkRate());

    // 更新光标动画状态（轻量级，不会影响性能）
    cursor_renderer.updateCursorState();

    // 行内容
    // Document* doc is passed in

    // 折叠指示器
#ifdef BUILD_LSP_SUPPORT
    if (doc) {
        std::string fold_indicator = " ";
        bool can_fold = false;

        // 首先检查文档是否有折叠状态（从缓存恢复的）
        bool is_folded_in_doc = doc->isFolded(static_cast<int>(line_num));

        if (lsp_enabled_ && folding_manager_ && folding_manager_->isInitialized()) {
            try {
                // 如果折叠管理器已初始化，使用管理器的状态（来自 LSP）
                const auto& foldable_lines = folding_manager_->getFoldableLines();
                bool is_foldable = std::find(foldable_lines.begin(), foldable_lines.end(),
                                             static_cast<int>(line_num)) != foldable_lines.end();

                if (is_foldable) {
                    can_fold = true;
                    bool is_folded = folding_manager_->isFolded(static_cast<int>(line_num));

                    if (is_folded) {
                        fold_indicator = "▶";
                    } else {
                        fold_indicator = "▼";
                    }
                }
            } catch (const std::exception& e) {
                // 如果出现异常，回退到文档状态
                if (is_folded_in_doc) {
                    can_fold = true;
                    fold_indicator = "▶";
                }
            }
        } else if (is_folded_in_doc) {
            // 如果折叠管理器未初始化，但文档中有折叠状态，显示折叠指示器
            can_fold = true;
            fold_indicator = "▶"; // 显示为折叠状态
        }

        if (can_fold) {
            line_elements.push_back(text(fold_indicator) | color(theme_.getColors().keyword));
        } else {
            line_elements.push_back(text(" "));
        }
    } else {
        line_elements.push_back(text(" "));
    }
#endif

    // 行号
    if (show_line_numbers_) {
        line_elements.push_back(renderLineNumber(doc, line_num, is_current));
        line_elements.push_back(text(" "));
    }
    if (!doc) {
        return hbox({text("~") | color(theme_.getColors().comment)});
    }

    if (line_num >= doc->lineCount()) {
        return hbox({text("~") | color(theme_.getColors().comment)});
    }

    std::string content;
    try {
        content = doc->getLine(line_num);
    } catch (const std::exception& e) {
        content = "";
    } catch (...) {
        content = "";
    }

    std::string original_content = content;
    size_t visible_cursor_col = cursor_col_;
    int tab_size = std::max(1, std::min(8, config_manager_.getConfig().editor.tab_size));

    bool is_split_mode = (max_width > 0);
    size_t effective_view_offset_col = is_split_mode ? view_offset_col : view_offset_col_;

    if (effective_view_offset_col > 0) {
        std::string display_content = expandTabsForDisplay(content, tab_size);
        if (effective_view_offset_col < display_content.length()) {
            content = display_content.substr(effective_view_offset_col);
        } else {
            content = "";
        }
        if (is_current) {
            size_t display_cursor = rawColToDisplayCol(original_content, cursor_col_, tab_size);
            if (display_cursor >= effective_view_offset_col) {
                visible_cursor_col = display_cursor - effective_view_offset_col;
            } else {
                visible_cursor_col = 0;
            }
        }
    }

    size_t line_count = doc->lineCount();
    size_t ln_width = 2;
    if (line_count > 0) {
        size_t digits = 0;
        for (size_t n = line_count; n > 0; n /= 10)
            digits++;
        ln_width = digits < 2 ? 2 : digits;
    }
    int effective_screen_width = is_split_mode ? max_width : screen_.dimx();
    int max_content_width = effective_screen_width - static_cast<int>(ln_width) - 4;
    if (max_content_width < 20)
        max_content_width = 20;
    if (content.length() > static_cast<size_t>(max_content_width)) {
        content = content.substr(0, static_cast<size_t>(max_content_width));
        if (is_current && visible_cursor_col >= static_cast<size_t>(max_content_width)) {
            visible_cursor_col = static_cast<size_t>(max_content_width) - 1;
        }
    }

    // 获取当前行的搜索匹配
    std::vector<features::SearchMatch> line_matches;
    if (search_highlight_active_ && search_engine_.hasMatches()) {
        const auto& all_matches = search_engine_.getAllMatches();
        for (const auto& match : all_matches) {
            if (match.line == line_num) {
                line_matches.push_back(match);
            }
        }
    } else if ((mode_ == EditorMode::SEARCH || mode_ == EditorMode::REPLACE) &&
               !search_input_.empty() && doc &&
               doc->lineCount() > LARGE_FILE_SEARCH_HIGHLIGHT_THRESHOLD) {
        // 大文件搜索模式：未做全量搜索，对当前可见行即时计算匹配并高亮
        findMatchesInLine(content, line_num, search_input_, current_search_options_, line_matches);
    }

    // 获取当前行的单词高亮匹配（优先级低于搜索高亮）
    std::vector<features::SearchMatch> word_line_matches;
    if (!search_highlight_active_) {
        const std::vector<features::SearchMatch>* word_src = nullptr;
        if (use_region_word_highlight && region_word_highlight_active && region_word_matches) {
            word_src = region_word_matches;
        } else if (word_highlight_active_ && !word_matches_.empty()) {
            word_src = &word_matches_;
        }
        if (word_src) {
            for (const auto& match : *word_src) {
                if (match.line == line_num) {
                    word_line_matches.push_back(match);
                }
            }
        }
    }

    // 获取当前行的括号匹配高亮（仅两个位置，O(1)判定）
    std::vector<features::SearchMatch> bracket_line_matches;
    if (bracket_highlight_active_) {
        if (bracket_current_line_ == line_num) {
            bracket_line_matches.emplace_back(line_num, bracket_current_col_, 1);
        }
        if (bracket_match_line_ == line_num && !(bracket_match_line_ == bracket_current_line_ &&
                                                 bracket_match_col_ == bracket_current_col_)) {
            bracket_line_matches.emplace_back(line_num, bracket_match_col_, 1);
        }
    }

    Element content_elem;

    // 检查当前行是否在选中范围内
    bool line_in_selection = false;
    size_t selection_start_col = 0;
    size_t selection_end_col = 0;

    if (selection_active_) {
        size_t start_row = selection_start_row_;
        size_t start_col = selection_start_col_;
        size_t end_row = cursor_row_;
        size_t end_col = cursor_col_;

        // 确保开始位置在结束位置之前
        if (start_row > end_row || (start_row == end_row && start_col > end_col)) {
            std::swap(start_row, end_row);
            std::swap(start_col, end_col);
        }

        // 检查当前行是否在选中范围内
        if (line_num >= start_row && line_num <= end_row) {
            line_in_selection = true;
            if (line_num == start_row && line_num == end_row) {
                // 同一行内的选择
                selection_start_col = start_col;
                selection_end_col = end_col;
            } else if (line_num == start_row) {
                // 选中开始行
                selection_start_col = start_col;
                selection_end_col = content.length();
            } else if (line_num == end_row) {
                // 选中结束行
                selection_start_col = 0;
                selection_end_col = end_col;
            } else {
                // 中间行，整行都被选中
                selection_start_col = 0;
                selection_end_col = content.length();
            }
        }
    }

    // 渲染带搜索高亮和选中高亮的行内容
    auto renderLineWithHighlights = [&, tab_size](const std::string& line_content,
                                                  size_t cursor_pos, bool has_cursor) -> Element {
        Elements parts;
        auto& colors = theme_.getColors();

        // 光标在 \t 上时只画单列块，后补 (tab_size-1) 空格，既保持行宽又不让光标变“大块”
        auto pushCursorPart = [&](const std::string& cursor_char, Element cursor_elem) {
            parts.push_back(std::move(cursor_elem));
            if (cursor_char == "\t" && tab_size > 1)
                parts.push_back(ftxui::text(std::string(static_cast<size_t>(tab_size - 1), ' ')));
        };
        auto cursorCharForBlock = [](const std::string& ch) -> const std::string& {
            static const std::string space = " ";
            return (ch == "\t") ? space : ch;
        };

        // 性能优化：如果行太长，限制语法高亮处理
        const size_t MAX_HIGHLIGHT_LENGTH = 5000; // 最多处理5000字符
        bool line_too_long = line_content.length() > MAX_HIGHLIGHT_LENGTH;

        // 辅助函数：渲染文本段，应用选中高亮（段内 \t 按 tab_size 展开以对齐缩进）
        auto renderSegment = [&, tab_size](const std::string& segment_text, size_t /* start_pos */,
                                           bool is_selected) -> Element {
            if (segment_text.empty()) {
                return ftxui::text("");
            }
            std::string display_text = expandTabsForDisplay(segment_text, tab_size);

            Element elem;
            if (syntax_highlighting_ && !line_too_long) {
                try {
                    elem = syntax_highlighter_.highlightLine(display_text);
                } catch (...) {
                    elem = ftxui::text(display_text) | color(colors.foreground);
                }
            } else {
                elem = ftxui::text(display_text) | color(colors.foreground);
            }

            // 如果这段文本在选中范围内，添加选中背景色
            if (is_selected) {
                elem = elem | bgcolor(colors.selection);
            }

            return elem;
        };

        // 处理选中高亮：将行内容按照选中范围分割
        if (line_in_selection) {
            // 有选中内容，需要分段渲染
            size_t pos = 0;

            while (pos < line_content.length()) {
                if (pos < selection_start_col) {
                    // 选中前的部分
                    std::string before_selection =
                        line_content.substr(pos, selection_start_col - pos);
                    if (has_cursor && cursor_pos < selection_start_col && cursor_pos >= pos) {
                        // 光标在选中前的部分
                        size_t before_cursor = cursor_pos - pos;
                        std::string before = before_selection.substr(0, before_cursor);
                        std::string cursor_char =
                            before_cursor < before_selection.length()
                                ? pnana::utils::getUtf8CharAt(before_selection, before_cursor)
                                : " ";
                        std::string after = before_cursor < before_selection.length()
                                                ? before_selection.substr(before_cursor + 1)
                                                : "";

                        if (!before.empty()) {
                            parts.push_back(renderSegment(before, pos, false));
                        }
                        pushCursorPart(cursor_char, cursor_renderer.renderCursorElement(
                                                        cursorCharForBlock(cursor_char), cursor_pos,
                                                        line_content.length(), colors.foreground,
                                                        colors.background));
                        if (!after.empty()) {
                            parts.push_back(renderSegment(after, cursor_pos + 1, false));
                        }
                        pos = selection_start_col;
                    } else {
                        parts.push_back(renderSegment(before_selection, pos, false));
                        pos = selection_start_col;
                    }
                } else if (pos < selection_end_col) {
                    // 选中部分
                    std::string selected = line_content.substr(pos, selection_end_col - pos);
                    if (has_cursor && cursor_pos >= pos && cursor_pos < selection_end_col) {
                        // 光标在选中部分内
                        size_t before_cursor = cursor_pos - pos;
                        std::string before = selected.substr(0, before_cursor);
                        std::string cursor_char =
                            before_cursor < selected.length()
                                ? pnana::utils::getUtf8CharAt(selected, before_cursor)
                                : " ";
                        std::string after = before_cursor < selected.length()
                                                ? selected.substr(before_cursor + 1)
                                                : "";

                        if (!before.empty()) {
                            Element before_elem = renderSegment(before, pos, true);
                            parts.push_back(before_elem);
                        }
                        // 光标在选中部分，也需要选中背景色
                        Element cursor_elem = cursor_renderer.renderCursorElement(
                            cursorCharForBlock(cursor_char), cursor_pos, line_content.length(),
                            colors.foreground, colors.background);
                        cursor_elem = cursor_elem | bgcolor(colors.selection);
                        pushCursorPart(cursor_char, std::move(cursor_elem));
                        if (!after.empty()) {
                            Element after_elem = renderSegment(after, cursor_pos + 1, true);
                            parts.push_back(after_elem);
                        }
                        pos = selection_end_col;
                    } else {
                        parts.push_back(renderSegment(selected, pos, true));
                        pos = selection_end_col;
                    }
                } else {
                    // 选中后的部分
                    std::string after_selection = line_content.substr(pos);
                    if (has_cursor && cursor_pos >= pos) {
                        // 光标在选中后的部分
                        size_t before_cursor = cursor_pos - pos;
                        std::string before = after_selection.substr(0, before_cursor);
                        std::string cursor_char =
                            before_cursor < after_selection.length()
                                ? pnana::utils::getUtf8CharAt(after_selection, before_cursor)
                                : " ";
                        std::string after = before_cursor < after_selection.length()
                                                ? after_selection.substr(before_cursor + 1)
                                                : "";

                        if (!before.empty()) {
                            parts.push_back(renderSegment(before, pos, false));
                        }
                        pushCursorPart(cursor_char, cursor_renderer.renderCursorElement(
                                                        cursorCharForBlock(cursor_char), cursor_pos,
                                                        line_content.length(), colors.foreground,
                                                        colors.background));
                        if (!after.empty()) {
                            parts.push_back(renderSegment(after, cursor_pos + 1, false));
                        }
                    } else {
                        parts.push_back(renderSegment(after_selection, pos, false));
                    }
                    break;
                }
            }
        } else if (!word_line_matches.empty() || !bracket_line_matches.empty()) {
            std::vector<features::SearchMatch> combined_matches = word_line_matches;
            combined_matches.insert(combined_matches.end(), bracket_line_matches.begin(),
                                    bracket_line_matches.end());
            std::sort(combined_matches.begin(), combined_matches.end(),
                      [](const features::SearchMatch& a, const features::SearchMatch& b) {
                          if (a.column != b.column)
                              return a.column < b.column;
                          return a.length < b.length;
                      });

            size_t pos = 0;
            size_t match_idx = 0;

            while (pos < line_content.length()) {
                bool found_match = false;
                for (size_t i = match_idx; i < combined_matches.size(); ++i) {
                    if (combined_matches[i].column == pos) {
                        size_t match_len = combined_matches[i].length;
                        size_t match_end = pos + match_len;

                        bool cursor_in_match =
                            has_cursor && cursor_pos >= pos && cursor_pos < match_end;
                        bool match_in_selection = line_in_selection && pos < selection_end_col &&
                                                  match_end > selection_start_col;

                        if (cursor_in_match) {
                            size_t before_cursor = cursor_pos - pos;
                            size_t after_cursor = match_end - cursor_pos;

                            if (before_cursor > 0) {
                                std::string before = line_content.substr(pos, before_cursor);
                                bool is_selected = match_in_selection && pos >= selection_start_col;
                                Element before_elem = renderSegment(before, pos, is_selected);
                                if (!is_selected) {
                                    before_elem = before_elem | bgcolor(Color::GrayDark);
                                }
                                parts.push_back(before_elem);
                            }

                            std::string cursor_char =
                                pnana::utils::getUtf8CharAt(line_content, cursor_pos);
                            Element cursor_elem = cursor_renderer.renderCursorElement(
                                cursorCharForBlock(cursor_char), cursor_pos, line_content.length(),
                                colors.foreground, colors.background);
                            if (match_in_selection && cursor_pos >= selection_start_col &&
                                cursor_pos < selection_end_col) {
                                cursor_elem = cursor_elem | bgcolor(colors.selection);
                            } else {
                                cursor_elem = cursor_elem | bgcolor(Color::GrayDark);
                            }
                            pushCursorPart(cursor_char, std::move(cursor_elem));

                            if (after_cursor > 1) {
                                std::string after =
                                    line_content.substr(cursor_pos + 1, after_cursor - 1);
                                bool is_selected =
                                    match_in_selection && cursor_pos + 1 >= selection_start_col;
                                Element after_elem =
                                    renderSegment(after, cursor_pos + 1, is_selected);
                                if (!is_selected) {
                                    after_elem = after_elem | bgcolor(Color::GrayDark);
                                }
                                parts.push_back(after_elem);
                            }
                        } else {
                            std::string match_text = line_content.substr(pos, match_len);
                            Element match_elem = renderSegment(match_text, pos, match_in_selection);
                            if (!match_in_selection) {
                                match_elem = match_elem | bgcolor(Color::GrayDark);
                            }
                            parts.push_back(match_elem);
                        }

                        pos = match_end;
                        match_idx = i + 1;
                        found_match = true;
                        break;
                    }
                }

                if (!found_match) {
                    size_t next_match_pos = line_content.length();
                    for (size_t i = match_idx; i < combined_matches.size(); ++i) {
                        if (combined_matches[i].column > pos &&
                            combined_matches[i].column < next_match_pos) {
                            next_match_pos = combined_matches[i].column;
                        }
                    }

                    std::string segment = line_content.substr(pos, next_match_pos - pos);
                    bool segment_in_selection = line_in_selection && pos < selection_end_col &&
                                                next_match_pos > selection_start_col;

                    if (has_cursor && cursor_pos >= pos && cursor_pos < next_match_pos) {
                        size_t before_cursor = cursor_pos - pos;
                        std::string before = segment.substr(0, before_cursor);
                        std::string cursor_char = before_cursor < segment.length()
                                                      ? segment.substr(before_cursor, 1)
                                                      : " ";
                        std::string after = before_cursor < segment.length()
                                                ? segment.substr(before_cursor + 1)
                                                : "";

                        if (!before.empty()) {
                            parts.push_back(renderSegment(
                                before, pos, segment_in_selection && pos >= selection_start_col));
                        }
                        Element cursor_elem = cursor_renderer.renderCursorElement(
                            cursorCharForBlock(cursor_char), cursor_pos, line_content.length(),
                            colors.foreground, colors.background);
                        if (segment_in_selection && cursor_pos >= selection_start_col &&
                            cursor_pos < selection_end_col) {
                            cursor_elem = cursor_elem | bgcolor(colors.selection);
                        }
                        pushCursorPart(cursor_char, std::move(cursor_elem));
                        if (!after.empty()) {
                            parts.push_back(renderSegment(
                                after, cursor_pos + 1,
                                segment_in_selection && cursor_pos + 1 >= selection_start_col));
                        }
                    } else {
                        parts.push_back(renderSegment(segment, pos, segment_in_selection));
                    }

                    pos = next_match_pos;
                }
            }
        } else if (line_matches.empty()) {
            // 没有搜索匹配和选中，正常渲染
            if (has_cursor && cursor_pos <= line_content.length()) {
                std::string before = line_content.substr(0, cursor_pos);
                std::string cursor_char =
                    cursor_pos < line_content.length()
                        ? pnana::utils::getUtf8CharAt(line_content, cursor_pos)
                        : " ";
                std::string after =
                    cursor_pos < line_content.length() ? line_content.substr(cursor_pos + 1) : "";

                if (!before.empty()) {
                    parts.push_back(renderSegment(before, 0, false));
                }
                // 使用配置的光标样式渲染
                pushCursorPart(cursor_char,
                               cursor_renderer.renderCursorElement(
                                   cursorCharForBlock(cursor_char), cursor_pos,
                                   line_content.length(), colors.foreground, colors.background));
                if (!after.empty()) {
                    parts.push_back(renderSegment(after, cursor_pos + 1, false));
                }
            } else {
                // 没有光标，渲染整行
                parts.push_back(renderSegment(line_content, 0, false));
            }
        } else {
            // 有搜索匹配，需要同时处理搜索高亮和选中高亮
            size_t pos = 0;
            size_t match_idx = 0;

            while (pos < line_content.length()) {
                // 检查是否有匹配从当前位置开始
                bool found_match = false;
                for (size_t i = match_idx; i < line_matches.size(); ++i) {
                    if (line_matches[i].column == pos) {
                        // 找到匹配，高亮显示
                        size_t match_len = line_matches[i].length;
                        size_t match_end = pos + match_len;

                        // 检查光标是否在匹配范围内
                        bool cursor_in_match =
                            has_cursor && cursor_pos >= pos && cursor_pos < match_end;

                        // 检查匹配是否在选中范围内
                        bool match_in_selection = line_in_selection && pos < selection_end_col &&
                                                  match_end > selection_start_col;

                        if (cursor_in_match) {
                            // 光标在匹配内，需要分割匹配文本
                            size_t before_cursor = cursor_pos - pos;
                            size_t after_cursor = match_end - cursor_pos;

                            if (before_cursor > 0) {
                                std::string before = line_content.substr(pos, before_cursor);
                                bool is_selected = match_in_selection && pos >= selection_start_col;
                                Element before_elem = renderSegment(before, pos, is_selected);
                                // 如果不在选中范围内，应用搜索高亮
                                if (!is_selected) {
                                    before_elem = before_elem | bgcolor(Color::GrayDark);
                                }
                                parts.push_back(before_elem);
                            }

                            // 光标位置的字符
                            std::string cursor_char =
                                pnana::utils::getUtf8CharAt(line_content, cursor_pos);
                            Element cursor_elem = cursor_renderer.renderCursorElement(
                                cursorCharForBlock(cursor_char), cursor_pos, line_content.length(),
                                colors.foreground, colors.background);
                            // 选中高亮优先于搜索高亮
                            if (match_in_selection && cursor_pos >= selection_start_col &&
                                cursor_pos < selection_end_col) {
                                cursor_elem = cursor_elem | bgcolor(colors.selection);
                            } else {
                                cursor_elem = cursor_elem | bgcolor(Color::GrayDark);
                            }
                            pushCursorPart(cursor_char, std::move(cursor_elem));

                            if (after_cursor > 1) {
                                std::string after =
                                    line_content.substr(cursor_pos + 1, after_cursor - 1);
                                bool is_selected =
                                    match_in_selection && cursor_pos + 1 >= selection_start_col;
                                Element after_elem =
                                    renderSegment(after, cursor_pos + 1, is_selected);
                                // 如果不在选中范围内，应用搜索高亮
                                if (!is_selected) {
                                    after_elem = after_elem | bgcolor(Color::GrayDark);
                                }
                                parts.push_back(after_elem);
                            }
                        } else {
                            // 光标不在匹配内，正常高亮匹配
                            std::string match_text = line_content.substr(pos, match_len);
                            Element match_elem = renderSegment(match_text, pos, match_in_selection);
                            // 如果不在选中范围内，应用搜索高亮
                            if (!match_in_selection) {
                                match_elem = match_elem | bgcolor(Color::GrayDark);
                            }
                            parts.push_back(match_elem);
                        }

                        pos = match_end;
                        match_idx = i + 1;
                        found_match = true;
                        break;
                    }
                }

                if (!found_match) {
                    // 没有匹配，找到下一个匹配的位置
                    size_t next_match_pos = line_content.length();
                    for (size_t i = match_idx; i < line_matches.size(); ++i) {
                        if (line_matches[i].column > pos &&
                            line_matches[i].column < next_match_pos) {
                            next_match_pos = line_matches[i].column;
                        }
                    }

                    std::string segment = line_content.substr(pos, next_match_pos - pos);

                    // 检查这段是否在选中范围内
                    bool segment_in_selection = line_in_selection && pos < selection_end_col &&
                                                next_match_pos > selection_start_col;

                    // 检查光标是否在这个段内
                    if (has_cursor && cursor_pos >= pos && cursor_pos < next_match_pos) {
                        size_t before_cursor = cursor_pos - pos;
                        std::string before = segment.substr(0, before_cursor);
                        std::string cursor_char = before_cursor < segment.length()
                                                      ? segment.substr(before_cursor, 1)
                                                      : " ";
                        std::string after = before_cursor < segment.length()
                                                ? segment.substr(before_cursor + 1)
                                                : "";

                        if (!before.empty()) {
                            parts.push_back(renderSegment(
                                before, pos, segment_in_selection && pos >= selection_start_col));
                        }
                        Element cursor_elem = cursor_renderer.renderCursorElement(
                            cursorCharForBlock(cursor_char), cursor_pos, line_content.length(),
                            colors.foreground, colors.background);
                        if (segment_in_selection && cursor_pos >= selection_start_col &&
                            cursor_pos < selection_end_col) {
                            cursor_elem = cursor_elem | bgcolor(colors.selection);
                        }
                        pushCursorPart(cursor_char, std::move(cursor_elem));
                        if (!after.empty()) {
                            parts.push_back(renderSegment(
                                after, cursor_pos + 1,
                                segment_in_selection && cursor_pos + 1 >= selection_start_col));
                        }
                    } else {
                        // 没有光标，正常渲染
                        parts.push_back(renderSegment(segment, pos, segment_in_selection));
                    }

                    pos = next_match_pos;
                }
            }
        }

        return hbox(parts);
    };

    try {
        content_elem = renderLineWithHighlights(content, visible_cursor_col, is_current);
    } catch (const std::exception& e) {
        // 如果高亮失败，使用简单文本
        content_elem = text(content) | color(theme_.getColors().foreground);
    } catch (...) {
        // 如果高亮失败，使用简单文本
        content_elem = text(content) | color(theme_.getColors().foreground);
    }

    line_elements.push_back(content_elem);

    Element line_elem = hbox(line_elements);

    // 高亮当前行背景（可配置关闭）
    if (is_current && config_manager_.getConfig().display.highlight_current_line) {
        line_elem = line_elem | bgcolor(theme_.getColors().current_line);
    }

    return line_elem;
}

int Editor::getLineNumberWidth(Document* doc) const {
    size_t total = doc ? doc->lineCount() : 1;
    return static_cast<int>(getLineNumberWidthForLineCount(total));
}

Element Editor::renderLineNumber(Document* doc, size_t line_num, bool is_current) {
    const size_t total_lines = doc ? doc->lineCount() : 1;
    const size_t LINE_NUM_WIDTH = getLineNumberWidthForLineCount(total_lines);

    std::string line_str;

    if (relative_line_numbers_ && !is_current) {
        // 在相对行号模式下，需要计算相对于当前光标的可见行差
        if (doc) {
            size_t current_visible_line = doc->actualLineToDisplayLine(cursor_row_);
            size_t this_visible_line = doc->actualLineToDisplayLine(line_num);
            size_t diff = (this_visible_line > current_visible_line)
                              ? (this_visible_line - current_visible_line)
                              : (current_visible_line - this_visible_line);
            line_str = std::to_string(diff);
        } else {
            size_t diff =
                (line_num > cursor_row_) ? (line_num - cursor_row_) : (cursor_row_ - line_num);
            line_str = std::to_string(diff);
        }
    } else {
        // 在正常行号模式下，通常显示可见行号（从1开始）
        // 但如果这行是折叠起始或处于折叠范围内，显示文件的实际行号（更接近 Neovim 行为）
        if (doc) {
            bool show_actual_for_fold = false;
#ifdef BUILD_LSP_SUPPORT
            if (lsp_enabled_ && folding_manager_) {
                // 折叠起始行、折叠内被隐藏行：显示实际行号
                if (folding_manager_->isFolded(static_cast<int>(line_num)) ||
                    folding_manager_->isLineInFoldedRange(static_cast<int>(line_num))) {
                    show_actual_for_fold = true;
                }
                // 折叠的下一行（紧接在折叠块后的可见行）仍显示实际行号，不因折叠而改变
                if (!show_actual_for_fold) {
                    auto folded_ranges = folding_manager_->getFoldedRanges();
                    for (const auto& fr : folded_ranges) {
                        if (static_cast<size_t>(fr.endLine + 1) == line_num) {
                            show_actual_for_fold = true;
                            break;
                        }
                    }
                }
            }
#endif
            if (show_actual_for_fold) {
                // 折叠行只显示该行行号，不显示范围（与图示一致）
                line_str = std::to_string(line_num + 1);
            } else {
                size_t visible_line_num = doc->actualLineToDisplayLine(line_num) + 1;
                line_str = std::to_string(visible_line_num);
            }
        } else {
            line_str = std::to_string(line_num + 1);
        }
    }

    // 右对齐：使用文档最大行号所需列宽
    while (line_str.length() < LINE_NUM_WIDTH) {
        line_str = " " + line_str;
    }

    // 检查是否有诊断信息
    bool has_diagnostic = false;
    ftxui::Color line_number_bg = ftxui::Color::Default;
    ftxui::Color line_number_fg = theme_.getColors().line_number;

#ifdef BUILD_LSP_SUPPORT
    if (lsp_enabled_) {
        std::lock_guard<std::mutex> lock(diagnostics_mutex_);
        for (const auto& diagnostic : current_file_diagnostics_) {
            if (static_cast<size_t>(diagnostic.range.start.line) == line_num) {
                has_diagnostic = true;
                if (diagnostic.severity == 1) { // Error - 红色背景
                    line_number_bg = ftxui::Color::Red;
                    line_number_fg = ftxui::Color::White; // 白色文字以提高对比度
                } else if (diagnostic.severity == 2) { // Warning - 黄色背景（更适合警告）
                    line_number_bg = ftxui::Color::Yellow;
                    line_number_fg = ftxui::Color::Black; // 黑色文字以提高对比度
                }
                break;
            }
        }
    }
#endif

    // 渲染行号文本
    Element line_number_element;
    if (is_current) {
        // 当前行：使用当前行颜色，但如果有诊断则覆盖
        if (has_diagnostic) {
            line_number_element =
                text(line_str) | color(line_number_fg) | bgcolor(line_number_bg) | bold;
        } else {
            line_number_element =
                text(line_str) | color(theme_.getColors().line_number_current) | bold;
        }
    } else {
        // 普通行：如果有诊断则使用诊断颜色，否则使用普通颜色
        if (has_diagnostic) {
            line_number_element = text(line_str) | color(line_number_fg) | bgcolor(line_number_bg);
        } else {
            line_number_element = text(line_str) | color(theme_.getColors().line_number);
        }
    }

    return line_number_element;
}

Element Editor::renderStatusbar() {
    // 异步更新git信息（非阻塞）
    updateGitInfo();

    // 获取git信息（线程安全）
    std::string git_branch;
    int git_uncommitted_count;
    {
        std::lock_guard<std::mutex> lock(git_cache_mutex);
        git_branch = cached_git_branch;
        git_uncommitted_count = cached_git_uncommitted_count;
    }

    // Check for due todos and add blinking reminder
    auto due_todos = todo_panel_.getTodoManager().getDueTodos();
    std::string todo_reminder = "";
    bool has_todo_reminder = false;
    bool todo_reminder_blink = false; // 仅到期后 1 分钟内为 true，状态栏闪烁
    if (!due_todos.empty()) {
        // Sort by priority, show highest priority todo first
        std::vector<features::todo::TodoItem> sorted_todos = due_todos;
        std::sort(sorted_todos.begin(), sorted_todos.end(),
                  [](const features::todo::TodoItem& a, const features::todo::TodoItem& b) {
                      return a.priority < b.priority; // Lower number = higher priority
                  });

        const auto& first_todo = sorted_todos[0];
        std::string time_str =
            features::todo::TodoManager::formatTimeRemaining(first_todo.due_time);

        // Build reminder text: ⚠ P1 content (Overdue Xm)
        todo_reminder = "⚠ P" + std::to_string(first_todo.priority) + " " + first_todo.content +
                        " (" + time_str + ")";

        if (sorted_todos.size() > 1) {
            todo_reminder += " (+" + std::to_string(sorted_todos.size() - 1) + " more)";
        }
        has_todo_reminder = true;
        todo_reminder_blink =
            features::todo::TodoManager::isDueWithinBlinkWindow(first_todo.due_time);
    }

    // 构建状态消息，包含SSH连接信息和Todo提醒
    // 到期后 1 分钟内用 TODO_REMINDER（状态栏闪烁），超过 1 分钟用
    // TODO_REMINDER_STATIC（仅红色不闪烁）
    std::string display_message = status_message_;
    if (has_todo_reminder) {
        const char* tag = todo_reminder_blink ? "TODO_REMINDER" : "TODO_REMINDER_STATIC";
        std::string wrap =
            "[[" + std::string(tag) + "]]" + todo_reminder + "[[/" + std::string(tag) + "]]";
        if (!display_message.empty()) {
            display_message = wrap + " | " + display_message;
        } else {
            display_message = wrap;
        }
    }
    if (!current_ssh_config_.host.empty()) {
        std::string ssh_info = "SSH: " + current_ssh_config_.user + "@" + current_ssh_config_.host;
        if (!display_message.empty()) {
            display_message += " | " + ssh_info;
        } else {
            display_message = ssh_info;
        }
    }

    // 使用 getDocumentForActiveRegion：分屏时若当前激活区域无文档（welcome），应显示 Welcome
    // 而非 getCurrentDocument() 可能返回的其他区域的文档
    const Document* doc = getDocumentForActiveRegion();

    // If no document in active region, show welcome status
    if (doc == nullptr) {
        std::string welcome_msg =
            display_message.empty() ? "Press i to start editing" : display_message;
        return statusbar_.render(
            "Welcome",
            false, // not modified
            false, // not readonly
            0,     // line
            0,     // col
            0,     // total lines
            "UTF-8", "LF", "text", welcome_msg, region_manager_.getRegionName(),
            false, // syntax highlighting
            false, // has selection
            0,     // selection length
            git_branch, git_uncommitted_count, current_ssh_config_.host, current_ssh_config_.user);
    }

    // 获取行尾类型
    std::string line_ending;
    switch (doc->getLineEnding()) {
        case LineEnding::LF:
            line_ending = "LF";
            break;
        case LineEnding::CRLF:
            line_ending = "CRLF";
            break;
        case LineEnding::CR:
            line_ending = "CR";
            break;
    }

    // 在文件浏览器区域时，使用文件浏览器的选中信息
    bool has_selection = selection_active_;
    size_t selection_length = 0;

    if (region_manager_.getCurrentRegion() == EditorRegion::FILE_BROWSER &&
        file_browser_.isVisible()) {
        // 文件浏览器区域：显示文件选中数量
        size_t file_selection_count = file_browser_.getSelectedCount();
        has_selection = file_selection_count > 0;
        selection_length = file_selection_count;
    } else {
        // 代码编辑区域：显示文本选择长度
        selection_length =
            selection_active_
                ? (cursor_row_ != selection_start_row_ || cursor_col_ != selection_start_col_ ? 1
                                                                                              : 0)
                : 0;
    }

    std::string file_type =
        utils::FileTypeDetector::detectFileType(doc->getFileName(), doc->getFileExtension());

    return statusbar_.render(
        doc->getFileName(), doc->isModified(), doc->isReadOnly(), cursor_row_, cursor_col_,
        doc->lineCount(), doc->getEncoding(), line_ending, file_type, display_message,
        region_manager_.getRegionName(), syntax_highlighting_, has_selection, selection_length,
        git_branch, git_uncommitted_count, current_ssh_config_.host, current_ssh_config_.user);
}

Element Editor::renderHelpbar() {
    if (!show_helpbar_)
        return ftxui::text("");
    return helpbar_.render(pnana::ui::Helpbar::getDefaultHelp());
}

Element Editor::renderInputBox() {
    if (mode_ == EditorMode::SEARCH) {
        return renderSearchInputBox();
    } else if (mode_ == EditorMode::REPLACE) {
        return renderReplaceInputBox();
    }
    return text("");
}

Element Editor::renderSearchInputBox() {
    auto& colors = theme_.getColors();
    Elements elements;

    // 搜索提示
    elements.push_back(text("Search: ") | color(colors.comment));

    // 搜索输入区域
    if (search_input_.empty()) {
        elements.push_back(text("(type to search...)") | color(colors.comment) | dim);
    } else {
        // 渲染带光标的搜索输入
        if (search_cursor_pos_ <= search_input_.length()) {
            std::string before = search_input_.substr(0, search_cursor_pos_);
            std::string cursor_char = search_cursor_pos_ < search_input_.length()
                                          ? search_input_.substr(search_cursor_pos_, 1)
                                          : " ";
            std::string after = search_cursor_pos_ < search_input_.length()
                                    ? search_input_.substr(search_cursor_pos_ + 1)
                                    : "";

            if (!before.empty()) {
                elements.push_back(text(before) | color(colors.foreground));
            }
            elements.push_back(text(cursor_char) | bgcolor(colors.foreground) |
                               color(colors.background) | bold);
            if (!after.empty()) {
                elements.push_back(text(after) | color(colors.foreground));
            }
        } else {
            elements.push_back(text(search_input_) | color(colors.foreground));
        }
    }

    // 显示搜索选项
    Elements options;
    const char* option_names[] = {"Case", "Word", "Regex", "Wrap"};

    for (int i = 0; i < 4; ++i) {
        Color option_color = (i == current_option_index_) ? colors.function : colors.comment;
        std::string indicator = search_options_[i] ? "●" : "○";
        options.push_back(text(std::string(" ") + indicator + option_names[i]) |
                          color(option_color));
    }

    elements.push_back(hbox(std::move(options)));

    // 匹配计数
    if (total_search_matches_ > 0) {
        std::string count_str = " [" + std::to_string(current_search_match_ + 1) + "/" +
                                std::to_string(total_search_matches_) + "]";
        elements.push_back(text(count_str) | color(colors.info));
    }

    // 快捷键提示（Ctrl+G 下一项，Ctrl+I 上一项）
    elements.push_back(
        text("  [Ctrl+G: next, Ctrl+Shift+F3: prev, ↑↓: options, Tab: replace, Enter: "
             "accept, Esc: cancel]") |
        color(colors.comment) | dim);

    return hbox(std::move(elements)) | bgcolor(colors.menubar_bg);
}

Element Editor::renderReplaceInputBox() {
    auto& colors = theme_.getColors();
    Elements elements;

    // 替换提示
    elements.push_back(text("Replace: ") | color(colors.comment));

    // 搜索模式显示
    if (!search_input_.empty()) {
        elements.push_back(text(search_input_) | color(colors.foreground));
        elements.push_back(text(" → ") | color(colors.comment));
    }

    // 替换输入区域
    if (replace_input_.empty()) {
        elements.push_back(text("(type replacement...)") | color(colors.comment) | dim);
    } else {
        // 渲染带光标的替换输入
        if (replace_cursor_pos_ <= replace_input_.length()) {
            std::string before = replace_input_.substr(0, replace_cursor_pos_);
            std::string cursor_char = replace_cursor_pos_ < replace_input_.length()
                                          ? replace_input_.substr(replace_cursor_pos_, 1)
                                          : " ";
            std::string after = replace_cursor_pos_ < replace_input_.length()
                                    ? replace_input_.substr(replace_cursor_pos_ + 1)
                                    : "";

            if (!before.empty()) {
                elements.push_back(text(before) | color(colors.foreground));
            }
            elements.push_back(text(cursor_char) | bgcolor(colors.foreground) |
                               color(colors.background) | bold);
            if (!after.empty()) {
                elements.push_back(text(after) | color(colors.foreground));
            }
        } else {
            elements.push_back(text(replace_input_) | color(colors.foreground));
        }
    }

    // 显示搜索选项
    Elements options;
    const char* option_names[] = {"Case", "Word", "Regex", "Wrap"};

    for (int i = 0; i < 4; ++i) {
        Color option_color = (i == current_option_index_) ? colors.function : colors.comment;
        std::string indicator = search_options_[i] ? "●" : "○";
        options.push_back(text(std::string(" ") + indicator + option_names[i]) |
                          color(option_color));
    }

    elements.push_back(hbox(std::move(options)));

    // 匹配计数
    if (total_search_matches_ > 0) {
        std::string count_str = " [" + std::to_string(current_search_match_ + 1) + "/" +
                                std::to_string(total_search_matches_) + "]";
        elements.push_back(text(count_str) | color(colors.info));
    }

    // 快捷键提示（Ctrl+G/Ctrl+I 在匹配项间导航）
    elements.push_back(
        text("  [Ctrl+G: next, Ctrl+Shift+F3: prev, ↑↓: options, Enter: replace, Esc: cancel]") |
        color(colors.comment) | dim);

    return hbox(std::move(elements)) | bgcolor(colors.menubar_bg);
}

Element Editor::renderFileBrowser() {
    int height = screen_.dimy() - 4; // 减去状态栏等高度
    return file_browser_.render(height);
}

Element Editor::renderHelp() {
    int width = screen_.dimx();
    int height = screen_.dimy();
    return help_.render(width, height);
}

Element Editor::renderCommandPalette() {
    return command_palette_.render();
}

Element Editor::renderTerminal() {
    int height = terminal_height_;
    if (height <= 0) {
        height = screen_.dimy() / 3;
    }
    pnana::ui::TerminalCursorOptions cursor_opts;
    cursor_opts.config.style = static_cast<pnana::ui::CursorStyle>(getCursorStyle());
    cursor_opts.config.color = getCursorColor();
    cursor_opts.config.smooth = getCursorSmooth();
    cursor_opts.config.blink_enabled = cursor_config_dialog_.getBlinkEnabled();
    cursor_opts.blink_rate_ms = getCursorBlinkRate();

#ifdef BUILD_LIBVTERM_SUPPORT
    if (terminal_.sessionCount() > 1) {
        // 标签栏 + 终端内容，使用 vbox 但去除元素间隔
        return ftxui::vbox({
            pnana::ui::renderTerminalTabs(terminal_),
            pnana::ui::renderTerminal(terminal_, height - 1, &cursor_opts), // 减去标签栏高度
        });
    }
#endif
    return pnana::ui::renderTerminal(terminal_, height, &cursor_opts);
}

Element Editor::renderGitPanel() {
    return git_panel_.getComponent()->Render();
}

Element Editor::renderFilePicker() {
    return file_picker_.render();
}

} // namespace core
} // namespace pnana
