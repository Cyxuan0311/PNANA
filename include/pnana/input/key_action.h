#ifndef PNANA_INPUT_KEY_ACTION_H
#define PNANA_INPUT_KEY_ACTION_H

#include <string>
#include <vector>

namespace pnana {
namespace input {

// 快捷键动作枚举
enum class KeyAction {
    // 文件操作
    SAVE_FILE,
    SAVE_AS,
    QUIT,
    NEW_FILE,
    OPEN_FILE,
    CLOSE_TAB,
    CREATE_FOLDER,
    FILE_PICKER, // 文件选择器

    // 编辑操作
    UNDO,
    REDO,
    CUT,
    COPY,
    PASTE,
    SELECT_ALL,
    SELECT_WORD,         // 选中词块
    SELECT_EXTEND_UP,    // 向上扩展选中
    SELECT_EXTEND_DOWN,  // 向下扩展选中
    SELECT_EXTEND_LEFT,  // 向左扩展选中
    SELECT_EXTEND_RIGHT, // 向右扩展选中
    DUPLICATE_LINE,
    DELETE_LINE,
    DELETE_WORD,
    MOVE_LINE_UP,
    MOVE_LINE_DOWN,
    INDENT_LINE,
    UNINDENT_LINE,
    TOGGLE_COMMENT,
    TRIGGER_COMPLETION, // 触发代码补全
    SHOW_DIAGNOSTICS,   // 显示诊断信息

    // 搜索和导航
    SEARCH,
    REPLACE,
    GOTO_LINE,
    SEARCH_NEXT,
    SEARCH_PREV,
    GOTO_FILE_START,
    GOTO_FILE_END,
    GOTO_LINE_START,
    GOTO_LINE_END,
    PAGE_UP,
    PAGE_DOWN,

    // 视图操作
    TOGGLE_THEME_MENU,
    SSH_CONNECT, // SSH 远程文件编辑
    TOGGLE_FILE_BROWSER,
    TOGGLE_HELP,
    TOGGLE_LINE_NUMBERS,
    SPLIT_VIEW,          // 分屏视图
    OPEN_PLUGIN_MANAGER, // 打开插件管理器

    // 标签页操作
    NEXT_TAB,
    PREV_TAB,

    // 命令面板
    COMMAND_PALETTE,

    // 分屏导航
    FOCUS_LEFT_REGION,  // 聚焦左侧区域
    FOCUS_RIGHT_REGION, // 聚焦右侧区域
    FOCUS_UP_REGION,    // 聚焦上方区域
    FOCUS_DOWN_REGION,  // 聚焦下方区域

    // 未知
    UNKNOWN
};

// 动作分组
enum class ActionGroup { FILE_OPS, EDIT_OPS, SEARCH_NAV, VIEW_OPS, TAB_OPS };

// 动作信息结构
struct ActionInfo {
    KeyAction action;
    ActionGroup group;
    std::string name;                      // 动作名称（用于显示）
    std::string description;               // 动作描述
    std::vector<std::string> default_keys; // 默认快捷键（可以有多个）

    ActionInfo(KeyAction a, ActionGroup g, const std::string& n, const std::string& d,
               const std::vector<std::string>& keys)
        : action(a), group(g), name(n), description(d), default_keys(keys) {}
};

// 获取所有动作信息
std::vector<ActionInfo> getAllActionInfos();

// 获取动作信息
ActionInfo getActionInfo(KeyAction action);

// 动作名称转枚举
KeyAction actionNameToEnum(const std::string& name);

// 枚举转动作名称
std::string actionEnumToName(KeyAction action);

} // namespace input
} // namespace pnana

#endif // PNANA_INPUT_KEY_ACTION_H
