#include "core/overlay_manager.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace core {

OverlayManager::OverlayManager() {}

ftxui::Element OverlayManager::renderOverlays(ftxui::Element main_ui) {
    using namespace ftxui;

    // 如果帮助窗口打开，叠加显示
    if (is_help_visible_callback_ && is_help_visible_callback_() && render_help_callback_) {
        return dbox({main_ui, render_help_callback_() | center});
    }

    // 如果主题菜单打开，叠加显示
    if (is_theme_menu_visible_callback_ && is_theme_menu_visible_callback_() &&
        render_theme_menu_callback_) {
        return dbox({main_ui, render_theme_menu_callback_() | center});
    }

    // 如果创建文件夹对话框打开，叠加显示
    if (is_create_folder_visible_callback_ && is_create_folder_visible_callback_() &&
        render_create_folder_callback_) {
        return dbox({main_ui, render_create_folder_callback_() | center});
    }

    // 如果另存为对话框打开，叠加显示
    if (is_save_as_visible_callback_ && is_save_as_visible_callback_() &&
        render_save_as_callback_) {
        return dbox({main_ui, render_save_as_callback_() | center});
    }

    // 如果移动文件对话框打开，叠加显示
    if (is_move_file_visible_callback_ && is_move_file_visible_callback_() &&
        render_move_file_callback_) {
        return dbox({main_ui | dim, render_move_file_callback_() | center});
    }

    // 如果解压进度对话框打开，叠加显示（最高优先级）
    if (is_extract_progress_visible_callback_ && is_extract_progress_visible_callback_() &&
        render_extract_progress_callback_) {
        return dbox({main_ui | dim, render_extract_progress_callback_() | center});
    }

    // 如果解压路径对话框打开，叠加显示（优先级高于解压对话框）
    if (is_extract_path_visible_callback_ && is_extract_path_visible_callback_() &&
        render_extract_path_callback_) {
        return dbox({main_ui | dim, render_extract_path_callback_() | center});
    }

    // 如果解压对话框打开，叠加显示
    if (is_extract_visible_callback_ && is_extract_visible_callback_() &&
        render_extract_callback_) {
        return dbox({main_ui | dim, render_extract_callback_() | center});
    }

    // 光标配置对话框
    if (is_cursor_config_visible_callback_ && is_cursor_config_visible_callback_() &&
        render_cursor_config_callback_) {
        Elements dialog_elements = {main_ui, render_cursor_config_callback_() | center};
        return dbox(dialog_elements);
    }

    // AI配置对话框
    if (is_ai_config_visible_callback_ && is_ai_config_visible_callback_() &&
        render_ai_config_callback_) {
        Elements dialog_elements = {main_ui, render_ai_config_callback_() | center};
        return dbox(dialog_elements);
    }

    // AI助手面板
    if (is_ai_assistant_visible_callback_ && is_ai_assistant_visible_callback_() &&
        render_ai_assistant_callback_) {
        Elements dialog_elements = {main_ui, render_ai_assistant_callback_() | center};
        return dbox(dialog_elements);
    }

#ifdef BUILD_LUA_SUPPORT
    // 插件管理对话框
    if (is_plugin_manager_visible_callback_ && render_plugin_manager_callback_) {
        bool is_visible = is_plugin_manager_visible_callback_();
        if (is_visible) {
            Elements dialog_elements = {main_ui, render_plugin_manager_callback_() | center};
            return dbox(dialog_elements);
        }
    }
#endif

    // 如果命令面板打开，叠加显示
    if (is_command_palette_visible_callback_ && is_command_palette_visible_callback_() &&
        render_command_palette_callback_) {
        return dbox({main_ui, render_command_palette_callback_() | center});
    }

    // 如果格式化对话框打开，叠加显示
    if (is_format_visible_callback_ && is_format_visible_callback_() && render_format_callback_) {
        return dbox({main_ui, render_format_callback_() | center});
    }

    // 如果Git面板打开，叠加显示
    if (is_git_panel_visible_callback_ && is_git_panel_visible_callback_() &&
        render_git_panel_callback_) {
        Elements dialog_elements = {main_ui | dim, render_git_panel_callback_() | center};
        return dbox(dialog_elements);
    }

    // 如果Todo面板打开，叠加显示
    if (is_todo_panel_visible_callback_ && is_todo_panel_visible_callback_() &&
        render_todo_panel_callback_) {
        Elements dialog_elements = {main_ui | dim, render_todo_panel_callback_() | center};
        return dbox(dialog_elements);
    }

    // 如果包管理器面板打开，叠加显示
    if (is_package_manager_panel_visible_callback_ &&
        is_package_manager_panel_visible_callback_() && render_package_manager_panel_callback_) {
        Elements dialog_elements = {main_ui | dim,
                                    render_package_manager_panel_callback_() | center};
        return dbox(dialog_elements);
    }

#ifdef BUILD_LSP_SUPPORT
    // 如果补全弹窗打开，叠加显示
    if (is_completion_popup_visible_callback_ && is_completion_popup_visible_callback_() &&
        render_completion_popup_callback_) {
        // 这里需要从 Editor 类获取补全弹窗的位置信息
        // 暂时简化处理，后面可以扩展接口
        Elements completion_elements = {main_ui, render_completion_popup_callback_()};
        return dbox(completion_elements);
    }

    // 如果诊断弹窗打开，叠加显示
    if (is_diagnostics_popup_visible_callback_ && is_diagnostics_popup_visible_callback_() &&
        render_diagnostics_popup_callback_) {
        Elements diagnostics_elements = {main_ui | dim,
                                         render_diagnostics_popup_callback_() | center};
        return dbox(diagnostics_elements);
    }

    // 如果符号导航弹窗打开，叠加显示
    if (is_symbol_navigation_popup_visible_callback_ &&
        is_symbol_navigation_popup_visible_callback_() &&
        render_symbol_navigation_popup_callback_) {
        Elements symbol_nav_elements = {main_ui | dim,
                                        render_symbol_navigation_popup_callback_() | center};
        return dbox(symbol_nav_elements);
    }
#endif

    // 如果文件选择器打开，叠加显示
    if (is_file_picker_visible_callback_ && is_file_picker_visible_callback_() &&
        render_file_picker_callback_) {
        Elements picker_elements = {main_ui | dim, render_file_picker_callback_() | center};
        return dbox(picker_elements);
    }

    // 如果分屏对话框打开，叠加显示
    if (is_split_dialog_visible_callback_ && is_split_dialog_visible_callback_() &&
        render_split_dialog_callback_) {
        Elements split_elements = {main_ui | dim, render_split_dialog_callback_() | center};
        return dbox(split_elements);
    }

    // 如果 SSH 传输对话框打开，叠加显示
    if (is_ssh_transfer_visible_callback_ && is_ssh_transfer_visible_callback_() &&
        render_ssh_transfer_callback_) {
        Elements ssh_transfer_elements = {main_ui | dim, render_ssh_transfer_callback_() | center};
        return dbox(ssh_transfer_elements);
    }

    // 如果 SSH 对话框打开，叠加显示
    if (is_ssh_dialog_visible_callback_ && is_ssh_dialog_visible_callback_() &&
        render_ssh_dialog_callback_) {
        Elements ssh_elements = {main_ui | dim, render_ssh_dialog_callback_() | center};
        return dbox(ssh_elements);
    }

    // 如果编码对话框打开，叠加显示
    if (is_encoding_dialog_visible_callback_ && is_encoding_dialog_visible_callback_() &&
        render_encoding_dialog_callback_) {
        Elements encoding_elements = {main_ui | dim, render_encoding_dialog_callback_() | center};
        return dbox(encoding_elements);
    }

    // 如果最近文件弹窗打开，叠加显示
    if (is_recent_files_visible_callback_ && is_recent_files_visible_callback_() &&
        render_recent_files_callback_) {
        Elements recent_files_elements = {main_ui | dim, render_recent_files_callback_() | center};
        return dbox(recent_files_elements);
    }

    // 如果TUI配置弹窗打开，叠加显示
    if (is_tui_config_visible_callback_ && is_tui_config_visible_callback_() &&
        render_tui_config_callback_) {
        Elements tui_config_elements = {main_ui | dim, render_tui_config_callback_() | center};
        return dbox(tui_config_elements);
    }

    // 如果通用对话框打开，叠加显示
    if (is_dialog_visible_callback_ && is_dialog_visible_callback_() && render_dialog_callback_) {
        Elements dialog_elements = {main_ui | dim, render_dialog_callback_() | center};
        return dbox(dialog_elements);
    }

    // 没有对话框打开，返回主UI
    return main_ui;
}

} // namespace core
} // namespace pnana
