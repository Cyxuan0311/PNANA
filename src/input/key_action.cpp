#include "input/key_action.h"
#include <algorithm>

namespace pnana {
namespace input {

static std::vector<ActionInfo> action_infos_;

// 初始化动作信息（延迟初始化）
static void initializeActionInfos() {
    if (!action_infos_.empty())
        return;

    // 文件操作
    action_infos_.emplace_back(KeyAction::SAVE_FILE, ActionGroup::FILE_OPS, "save_file",
                               "Save file", std::vector<std::string>{"ctrl_s"});
    action_infos_.emplace_back(KeyAction::SAVE_AS, ActionGroup::FILE_OPS, "save_as", "Save as",
                               std::vector<std::string>{"alt_a"});
    action_infos_.emplace_back(KeyAction::QUIT, ActionGroup::FILE_OPS, "quit", "Quit editor",
                               std::vector<std::string>{"ctrl_q"});
    action_infos_.emplace_back(KeyAction::NEW_FILE, ActionGroup::FILE_OPS, "new_file", "New file",
                               std::vector<std::string>{"ctrl_n"});
    action_infos_.emplace_back(KeyAction::OPEN_FILE, ActionGroup::FILE_OPS, "open_file",
                               "Open file browser", std::vector<std::string>{"ctrl_o"});
    action_infos_.emplace_back(KeyAction::CLOSE_TAB, ActionGroup::FILE_OPS, "close_tab",
                               "Close tab", std::vector<std::string>{"ctrl_w"});
    action_infos_.emplace_back(KeyAction::CREATE_FOLDER, ActionGroup::FILE_OPS, "create_folder",
                               "Create folder", std::vector<std::string>{"alt_f"});
    action_infos_.emplace_back(KeyAction::FILE_PICKER, ActionGroup::FILE_OPS, "file_picker",
                               "Open file picker", std::vector<std::string>{"alt_m"});

    // 编辑操作
    action_infos_.emplace_back(KeyAction::UNDO, ActionGroup::EDIT_OPS, "undo", "Undo",
                               std::vector<std::string>{"ctrl_z"});
    action_infos_.emplace_back(KeyAction::REDO, ActionGroup::EDIT_OPS, "redo", "Redo",
                               std::vector<std::string>{"ctrl_y", "ctrl_shift_z"});
    action_infos_.emplace_back(KeyAction::CUT, ActionGroup::EDIT_OPS, "cut", "Cut",
                               std::vector<std::string>{"ctrl_x"});
    action_infos_.emplace_back(KeyAction::COPY, ActionGroup::EDIT_OPS, "copy", "Copy",
                               std::vector<std::string>{"ctrl_p"});
    action_infos_.emplace_back(KeyAction::PASTE, ActionGroup::EDIT_OPS, "paste", "Paste",
                               std::vector<std::string>{"ctrl_v"});
    action_infos_.emplace_back(KeyAction::SELECT_ALL, ActionGroup::EDIT_OPS, "select_all",
                               "Select all", std::vector<std::string>{"ctrl_a"});
    action_infos_.emplace_back(KeyAction::SELECT_WORD, ActionGroup::EDIT_OPS, "select_word",
                               "Select word", std::vector<std::string>{"alt_d"});
    action_infos_.emplace_back(KeyAction::SELECT_EXTEND_UP, ActionGroup::EDIT_OPS,
                               "select_extend_up", "Extend selection up",
                               std::vector<std::string>{"alt_shift_arrow_up"});
    action_infos_.emplace_back(KeyAction::SELECT_EXTEND_DOWN, ActionGroup::EDIT_OPS,
                               "select_extend_down", "Extend selection down",
                               std::vector<std::string>{"alt_shift_arrow_down"});
    action_infos_.emplace_back(KeyAction::SELECT_EXTEND_LEFT, ActionGroup::EDIT_OPS,
                               "select_extend_left", "Extend selection left",
                               std::vector<std::string>{"alt_shift_arrow_left"});
    action_infos_.emplace_back(KeyAction::SELECT_EXTEND_RIGHT, ActionGroup::EDIT_OPS,
                               "select_extend_right", "Extend selection right",
                               std::vector<std::string>{"alt_shift_arrow_right"});
    action_infos_.emplace_back(KeyAction::DUPLICATE_LINE, ActionGroup::EDIT_OPS, "duplicate_line",
                               "Duplicate line", std::vector<std::string>{"ctrl_d"});
    action_infos_.emplace_back(KeyAction::DELETE_LINE, ActionGroup::EDIT_OPS, "delete_line",
                               "Delete line", std::vector<std::string>{"ctrl_l", "ctrl_shift_k"});
    action_infos_.emplace_back(KeyAction::DELETE_WORD, ActionGroup::EDIT_OPS, "delete_word",
                               "Delete word", std::vector<std::string>{"ctrl_backspace"});
    action_infos_.emplace_back(KeyAction::MOVE_LINE_UP, ActionGroup::EDIT_OPS, "move_line_up",
                               "Move line up", std::vector<std::string>{"alt_arrow_up"});
    action_infos_.emplace_back(KeyAction::MOVE_LINE_DOWN, ActionGroup::EDIT_OPS, "move_line_down",
                               "Move line down", std::vector<std::string>{"alt_arrow_down"});
    action_infos_.emplace_back(KeyAction::INDENT_LINE, ActionGroup::EDIT_OPS, "indent_line",
                               "Indent line", std::vector<std::string>{"tab"});
    action_infos_.emplace_back(KeyAction::UNINDENT_LINE, ActionGroup::EDIT_OPS, "unindent_line",
                               "Unindent line", std::vector<std::string>{"shift_tab"});
    action_infos_.emplace_back(KeyAction::TOGGLE_COMMENT, ActionGroup::EDIT_OPS, "toggle_comment",
                               "Toggle comment", std::vector<std::string>{"ctrl_slash"});
#ifdef BUILD_LSP_SUPPORT
    action_infos_.emplace_back(KeyAction::TRIGGER_COMPLETION, ActionGroup::EDIT_OPS,
                               "trigger_completion", "Trigger code completion",
                               std::vector<std::string>{"ctrl_space"});
    action_infos_.emplace_back(KeyAction::SHOW_DIAGNOSTICS, ActionGroup::EDIT_OPS,
                               "show_diagnostics", "Show diagnostics popup",
                               std::vector<std::string>{"alt_e"});
#endif

    // 搜索和导航
    action_infos_.emplace_back(KeyAction::SEARCH, ActionGroup::SEARCH_NAV, "search", "Search",
                               std::vector<std::string>{"ctrl_f"});
    action_infos_.emplace_back(KeyAction::REPLACE, ActionGroup::SEARCH_NAV, "replace", "Replace",
                               std::vector<std::string>{"ctrl_h"});
    action_infos_.emplace_back(KeyAction::GOTO_LINE, ActionGroup::SEARCH_NAV, "goto_line",
                               "Go to line", std::vector<std::string>{"ctrl_g"});
    action_infos_.emplace_back(KeyAction::SEARCH_NEXT, ActionGroup::SEARCH_NAV, "search_next",
                               "Next match", std::vector<std::string>{"ctrl_f3"});
    action_infos_.emplace_back(KeyAction::SEARCH_PREV, ActionGroup::SEARCH_NAV, "search_prev",
                               "Previous match", std::vector<std::string>{"ctrl_shift_f3"});
    action_infos_.emplace_back(KeyAction::COMMAND_PALETTE, ActionGroup::VIEW_OPS, "command_palette",
                               "Command Palette", std::vector<std::string>{"f3"});
    action_infos_.emplace_back(KeyAction::GOTO_FILE_START, ActionGroup::SEARCH_NAV,
                               "goto_file_start", "Go to file start",
                               std::vector<std::string>{"ctrl_home"});
    action_infos_.emplace_back(KeyAction::GOTO_FILE_END, ActionGroup::SEARCH_NAV, "goto_file_end",
                               "Go to file end", std::vector<std::string>{"ctrl_end"});
    action_infos_.emplace_back(KeyAction::GOTO_LINE_START, ActionGroup::SEARCH_NAV,
                               "goto_line_start", "Go to line start",
                               std::vector<std::string>{"home"});
    action_infos_.emplace_back(KeyAction::GOTO_LINE_END, ActionGroup::SEARCH_NAV, "goto_line_end",
                               "Go to line end", std::vector<std::string>{"end"});
    action_infos_.emplace_back(KeyAction::PAGE_UP, ActionGroup::SEARCH_NAV, "page_up", "Page up",
                               std::vector<std::string>{"pageup"});
    action_infos_.emplace_back(KeyAction::PAGE_DOWN, ActionGroup::SEARCH_NAV, "page_down",
                               "Page down", std::vector<std::string>{"pagedown"});

    // 视图操作
    action_infos_.emplace_back(KeyAction::TOGGLE_THEME_MENU, ActionGroup::VIEW_OPS,
                               "toggle_theme_menu", "Toggle theme menu",
                               std::vector<std::string>{"ctrl_t"});
    action_infos_.emplace_back(KeyAction::TOGGLE_FILE_BROWSER, ActionGroup::VIEW_OPS,
                               "toggle_file_browser", "Toggle file browser",
                               std::vector<std::string>{"ctrl_o"});
    action_infos_.emplace_back(KeyAction::TOGGLE_HELP, ActionGroup::VIEW_OPS, "toggle_help",
                               "Toggle help window", std::vector<std::string>{"f1"});
    action_infos_.emplace_back(KeyAction::SSH_CONNECT, ActionGroup::VIEW_OPS, "ssh_connect",
                               "SSH Remote File Editor", std::vector<std::string>{"f4"});
    action_infos_.emplace_back(KeyAction::TOGGLE_LINE_NUMBERS, ActionGroup::VIEW_OPS,
                               "toggle_line_numbers", "Toggle line numbers",
                               std::vector<std::string>{"ctrl_shift_l"});

    // 标签页操作
    action_infos_.emplace_back(KeyAction::NEXT_TAB, ActionGroup::TAB_OPS, "next_tab", "Next tab",
                               std::vector<std::string>{"alt_tab", "ctrl_pagedown"});
    action_infos_.emplace_back(KeyAction::PREV_TAB, ActionGroup::TAB_OPS, "prev_tab",
                               "Previous tab",
                               std::vector<std::string>{"alt_shift_tab", "ctrl_pageup"});
}

std::vector<ActionInfo> getAllActionInfos() {
    initializeActionInfos();
    return action_infos_;
}

ActionInfo getActionInfo(KeyAction action) {
    initializeActionInfos();
    for (const auto& info : action_infos_) {
        if (info.action == action) {
            return info;
        }
    }
    // 返回默认的未知动作信息
    return ActionInfo(KeyAction::UNKNOWN, ActionGroup::FILE_OPS, "unknown", "Unknown action",
                      std::vector<std::string>{});
}

KeyAction actionNameToEnum(const std::string& name) {
    initializeActionInfos();
    for (const auto& info : action_infos_) {
        if (info.name == name) {
            return info.action;
        }
    }
    return KeyAction::UNKNOWN;
}

std::string actionEnumToName(KeyAction action) {
    initializeActionInfos();
    for (const auto& info : action_infos_) {
        if (info.action == action) {
            return info.name;
        }
    }
    return "unknown";
}

} // namespace input
} // namespace pnana
