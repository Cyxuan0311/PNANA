#ifndef PNANA_INPUT_ACTION_EXECUTOR_H
#define PNANA_INPUT_ACTION_EXECUTOR_H

#include "input/key_action.h"

namespace pnana {
namespace core {
// 前向声明
class Editor;
} // namespace core
} // namespace pnana

namespace pnana {
namespace input {

// 动作执行器
// 职责：执行快捷键动作，解耦 Editor 类的具体实现
class ActionExecutor {
  public:
    explicit ActionExecutor(core::Editor* editor);
    ~ActionExecutor() = default;

    // 执行动作
    bool execute(KeyAction action);

    // 检查动作是否可用
    bool canExecute(KeyAction action) const;

    // 获取动作描述
    std::string getActionDescription(KeyAction action) const;

  private:
    core::Editor* editor_;

    // 文件操作
    bool executeFileOperation(KeyAction action);

    // 编辑操作
    bool executeEditOperation(KeyAction action);

    // 搜索和导航操作
    bool executeSearchNavigation(KeyAction action);

    // 视图操作
    bool executeViewOperation(KeyAction action);

    // 标签页操作
    bool executeTabOperation(KeyAction action);

    // 分屏导航操作
    bool executeSplitNavigation(KeyAction action);
};

} // namespace input
} // namespace pnana

#endif // PNANA_INPUT_ACTION_EXECUTOR_H
