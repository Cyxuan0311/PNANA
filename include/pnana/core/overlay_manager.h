#ifndef PNANA_CORE_OVERLAY_MANAGER_H
#define PNANA_CORE_OVERLAY_MANAGER_H

#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory>

namespace pnana {
namespace core {

// 叠加窗口管理器 - 使用回调函数避免前向声明问题
class OverlayManager {
  public:
    OverlayManager();

    // 设置各种对话框的渲染回调
    void setRenderHelpCallback(std::function<ftxui::Element()> callback) {
        render_help_callback_ = callback;
    }
    void setRenderThemeMenuCallback(std::function<ftxui::Element()> callback) {
        render_theme_menu_callback_ = callback;
    }
    void setRenderCreateFolderCallback(std::function<ftxui::Element()> callback) {
        render_create_folder_callback_ = callback;
    }
    void setRenderSaveAsCallback(std::function<ftxui::Element()> callback) {
        render_save_as_callback_ = callback;
    }
    void setRenderMoveFileCallback(std::function<ftxui::Element()> callback) {
        render_move_file_callback_ = callback;
    }
    void setRenderExtractCallback(std::function<ftxui::Element()> callback) {
        render_extract_callback_ = callback;
    }
    void setRenderExtractPathCallback(std::function<ftxui::Element()> callback) {
        render_extract_path_callback_ = callback;
    }
    void setRenderExtractProgressCallback(std::function<ftxui::Element()> callback) {
        render_extract_progress_callback_ = callback;
    }
    void setRenderCursorConfigCallback(std::function<ftxui::Element()> callback) {
        render_cursor_config_callback_ = callback;
    }
    void setRenderAIConfigCallback(std::function<ftxui::Element()> callback) {
        render_ai_config_callback_ = callback;
    }
    void setRenderAIAssistantCallback(std::function<ftxui::Element()> callback) {
        render_ai_assistant_callback_ = callback;
    }
    void setRenderPluginManagerCallback(std::function<ftxui::Element()> callback) {
        render_plugin_manager_callback_ = callback;
    }
    void setRenderCommandPaletteCallback(std::function<ftxui::Element()> callback) {
        render_command_palette_callback_ = callback;
    }
    void setRenderFormatCallback(std::function<ftxui::Element()> callback) {
        render_format_callback_ = callback;
    }
    void setRenderGitPanelCallback(std::function<ftxui::Element()> callback) {
        render_git_panel_callback_ = callback;
    }
    void setRenderTodoPanelCallback(std::function<ftxui::Element()> callback) {
        render_todo_panel_callback_ = callback;
    }
    void setRenderCompletionPopupCallback(std::function<ftxui::Element()> callback) {
        render_completion_popup_callback_ = callback;
    }
    void setRenderDiagnosticsPopupCallback(std::function<ftxui::Element()> callback) {
        render_diagnostics_popup_callback_ = callback;
    }
    void setRenderFilePickerCallback(std::function<ftxui::Element()> callback) {
        render_file_picker_callback_ = callback;
    }
    void setRenderSplitDialogCallback(std::function<ftxui::Element()> callback) {
        render_split_dialog_callback_ = callback;
    }
    void setRenderSSHTansferCallback(std::function<ftxui::Element()> callback) {
        render_ssh_transfer_callback_ = callback;
    }
    void setRenderSSHDialogCallback(std::function<ftxui::Element()> callback) {
        render_ssh_dialog_callback_ = callback;
    }
    void setRenderEncodingDialogCallback(std::function<ftxui::Element()> callback) {
        render_encoding_dialog_callback_ = callback;
    }
    void setRenderRecentFilesCallback(std::function<ftxui::Element()> callback) {
        render_recent_files_callback_ = callback;
    }
    void setRenderTUIConfigCallback(std::function<ftxui::Element()> callback) {
        render_tui_config_callback_ = callback;
    }
    void setRenderDialogCallback(std::function<ftxui::Element()> callback) {
        render_dialog_callback_ = callback;
    }

    // 设置可见性检查回调
    void setIsHelpVisibleCallback(std::function<bool()> callback) {
        is_help_visible_callback_ = callback;
    }
    void setIsThemeMenuVisibleCallback(std::function<bool()> callback) {
        is_theme_menu_visible_callback_ = callback;
    }
    void setIsCreateFolderVisibleCallback(std::function<bool()> callback) {
        is_create_folder_visible_callback_ = callback;
    }
    void setIsSaveAsVisibleCallback(std::function<bool()> callback) {
        is_save_as_visible_callback_ = callback;
    }
    void setIsMoveFileVisibleCallback(std::function<bool()> callback) {
        is_move_file_visible_callback_ = callback;
    }
    void setIsExtractVisibleCallback(std::function<bool()> callback) {
        is_extract_visible_callback_ = callback;
    }
    void setIsExtractPathVisibleCallback(std::function<bool()> callback) {
        is_extract_path_visible_callback_ = callback;
    }
    void setIsExtractProgressVisibleCallback(std::function<bool()> callback) {
        is_extract_progress_visible_callback_ = callback;
    }
    void setIsCursorConfigVisibleCallback(std::function<bool()> callback) {
        is_cursor_config_visible_callback_ = callback;
    }
    void setIsAIConfigVisibleCallback(std::function<bool()> callback) {
        is_ai_config_visible_callback_ = callback;
    }
    void setIsAIAssistantVisibleCallback(std::function<bool()> callback) {
        is_ai_assistant_visible_callback_ = callback;
    }
    void setIsPluginManagerVisibleCallback(std::function<bool()> callback) {
        is_plugin_manager_visible_callback_ = callback;
    }
    void setIsCommandPaletteVisibleCallback(std::function<bool()> callback) {
        is_command_palette_visible_callback_ = callback;
    }
    void setIsFormatVisibleCallback(std::function<bool()> callback) {
        is_format_visible_callback_ = callback;
    }
    void setIsGitPanelVisibleCallback(std::function<bool()> callback) {
        is_git_panel_visible_callback_ = callback;
    }
    void setIsTodoPanelVisibleCallback(std::function<bool()> callback) {
        is_todo_panel_visible_callback_ = callback;
    }
    void setIsCompletionPopupVisibleCallback(std::function<bool()> callback) {
        is_completion_popup_visible_callback_ = callback;
    }
    void setIsDiagnosticsPopupVisibleCallback(std::function<bool()> callback) {
        is_diagnostics_popup_visible_callback_ = callback;
    }
    void setIsFilePickerVisibleCallback(std::function<bool()> callback) {
        is_file_picker_visible_callback_ = callback;
    }
    void setIsSplitDialogVisibleCallback(std::function<bool()> callback) {
        is_split_dialog_visible_callback_ = callback;
    }
    void setIsSSHTansferVisibleCallback(std::function<bool()> callback) {
        is_ssh_transfer_visible_callback_ = callback;
    }
    void setIsSSHDialogVisibleCallback(std::function<bool()> callback) {
        is_ssh_dialog_visible_callback_ = callback;
    }
    void setIsEncodingDialogVisibleCallback(std::function<bool()> callback) {
        is_encoding_dialog_visible_callback_ = callback;
    }
    void setIsRecentFilesVisibleCallback(std::function<bool()> callback) {
        is_recent_files_visible_callback_ = callback;
    }
    void setIsTUIConfigVisibleCallback(std::function<bool()> callback) {
        is_tui_config_visible_callback_ = callback;
    }
    void setIsDialogVisibleCallback(std::function<bool()> callback) {
        is_dialog_visible_callback_ = callback;
    }

    // 主渲染方法
    ftxui::Element renderOverlays(ftxui::Element main_ui);

  private:
    // 渲染回调函数
    std::function<ftxui::Element()> render_help_callback_;
    std::function<ftxui::Element()> render_theme_menu_callback_;
    std::function<ftxui::Element()> render_create_folder_callback_;
    std::function<ftxui::Element()> render_save_as_callback_;
    std::function<ftxui::Element()> render_move_file_callback_;
    std::function<ftxui::Element()> render_extract_callback_;
    std::function<ftxui::Element()> render_extract_path_callback_;
    std::function<ftxui::Element()> render_extract_progress_callback_;
    std::function<ftxui::Element()> render_cursor_config_callback_;
    std::function<ftxui::Element()> render_ai_config_callback_;
    std::function<ftxui::Element()> render_ai_assistant_callback_;
    std::function<ftxui::Element()> render_plugin_manager_callback_;
    std::function<ftxui::Element()> render_command_palette_callback_;
    std::function<ftxui::Element()> render_format_callback_;
    std::function<ftxui::Element()> render_git_panel_callback_;
    std::function<ftxui::Element()> render_todo_panel_callback_;
    std::function<ftxui::Element()> render_completion_popup_callback_;
    std::function<ftxui::Element()> render_diagnostics_popup_callback_;
    std::function<ftxui::Element()> render_file_picker_callback_;
    std::function<ftxui::Element()> render_split_dialog_callback_;
    std::function<ftxui::Element()> render_ssh_transfer_callback_;
    std::function<ftxui::Element()> render_ssh_dialog_callback_;
    std::function<ftxui::Element()> render_encoding_dialog_callback_;
    std::function<ftxui::Element()> render_recent_files_callback_;
    std::function<ftxui::Element()> render_tui_config_callback_;
    std::function<ftxui::Element()> render_dialog_callback_;

    // 可见性检查回调函数
    std::function<bool()> is_help_visible_callback_;
    std::function<bool()> is_theme_menu_visible_callback_;
    std::function<bool()> is_create_folder_visible_callback_;
    std::function<bool()> is_save_as_visible_callback_;
    std::function<bool()> is_move_file_visible_callback_;
    std::function<bool()> is_extract_visible_callback_;
    std::function<bool()> is_extract_path_visible_callback_;
    std::function<bool()> is_extract_progress_visible_callback_;
    std::function<bool()> is_cursor_config_visible_callback_;
    std::function<bool()> is_ai_config_visible_callback_;
    std::function<bool()> is_ai_assistant_visible_callback_;
    std::function<bool()> is_plugin_manager_visible_callback_;
    std::function<bool()> is_command_palette_visible_callback_;
    std::function<bool()> is_format_visible_callback_;
    std::function<bool()> is_git_panel_visible_callback_;
    std::function<bool()> is_todo_panel_visible_callback_;
    std::function<bool()> is_completion_popup_visible_callback_;
    std::function<bool()> is_diagnostics_popup_visible_callback_;
    std::function<bool()> is_file_picker_visible_callback_;
    std::function<bool()> is_split_dialog_visible_callback_;
    std::function<bool()> is_ssh_transfer_visible_callback_;
    std::function<bool()> is_ssh_dialog_visible_callback_;
    std::function<bool()> is_encoding_dialog_visible_callback_;
    std::function<bool()> is_recent_files_visible_callback_;
    std::function<bool()> is_tui_config_visible_callback_;
    std::function<bool()> is_dialog_visible_callback_;
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_OVERLAY_MANAGER_H
