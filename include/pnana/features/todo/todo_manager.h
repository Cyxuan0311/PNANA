#ifndef PNANA_FEATURES_TODO_TODO_MANAGER_H
#define PNANA_FEATURES_TODO_TODO_MANAGER_H

#include <chrono>
#include <filesystem>
#include <stack>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace todo {

// Todo 项结构
struct TodoItem {
    std::string id;                                     // 唯一标识符
    std::string content;                                // 内容
    std::chrono::system_clock::time_point due_time;     // 到期时间
    std::chrono::system_clock::time_point created_time; // 创建时间
    int priority;   // 优先级（数字越小优先级越高）
    bool completed; // 是否完成

    TodoItem(const std::string& c, const std::chrono::system_clock::time_point& dt, int p = 5)
        : content(c), due_time(dt), created_time(std::chrono::system_clock::now()), priority(p),
          completed(false) {
        // 生成唯一ID（基于时间戳）
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        id =
            std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
};

// 操作类型枚举
enum class OperationType { ADD, REMOVE, UPDATE, MARK_COMPLETED, CLEAR };

// 操作记录结构体
struct Operation {
    OperationType type;
    std::vector<TodoItem> state_before; // 操作前的状态
    std::vector<TodoItem> state_after;  // 操作后的状态
};

// Todo 管理器
class TodoManager {
  public:
    TodoManager();
    ~TodoManager() = default;

    // 添加 todo
    void addTodo(const std::string& content, const std::chrono::system_clock::time_point& due_time,
                 int priority = 5);

    // 删除 todo
    void removeTodo(const std::string& id);
    void removeTodo(size_t index);

    // 更新 todo
    void updateTodo(const std::string& id, const std::string& content,
                    const std::chrono::system_clock::time_point& due_time, int priority);
    void updateTodoPriority(const std::string& id, int priority);
    void updateTodoPriority(size_t index, int priority);

    // 标记完成
    void markCompleted(const std::string& id, bool completed = true);
    void markCompleted(size_t index, bool completed = true);

    // 撤销/重做
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

    // 获取所有 todo
    const std::vector<TodoItem>& getTodos() const {
        return todos_;
    }

    // 获取未完成的 todo
    std::vector<TodoItem> getActiveTodos() const;

    // 获取已完成的 todo
    std::vector<TodoItem> getCompletedTodos() const;

    // 获取需要提醒的 todo（当前时间已到达）
    std::vector<TodoItem> getDueTodos() const;

    // 获取高优先级的 todo（优先级 <= 3）
    std::vector<TodoItem> getHighPriorityTodos() const;

    // 过滤选项枚举
    enum class FilterType { ALL, ACTIVE, COMPLETED, DUE, HIGH_PRIORITY };

    // 根据过滤类型获取 todo
    std::vector<TodoItem> getFilteredTodos(FilterType filter) const;

    // 是否处于“到期后 1 分钟内”的窗口（用于 UI 闪烁，过期超过 1 分钟不再闪烁）
    static bool isDueWithinBlinkWindow(const std::chrono::system_clock::time_point& due_time);

    // 格式化剩余时间或过期时间（辅助方法）
    static std::string formatTimeRemaining(const std::chrono::system_clock::time_point& due_time);

    // 排序选项枚举
    enum class SortBy { PRIORITY, DUE_TIME, CREATED_TIME };

    // 排序顺序枚举
    enum class SortOrder { ASCENDING, DESCENDING };

    // 排序 todo
    void sortByPriority(SortOrder order = SortOrder::ASCENDING);
    void sortByDueTime(SortOrder order = SortOrder::ASCENDING);
    void sortByCreatedTime(SortOrder order = SortOrder::ASCENDING);

    // 通用排序方法
    void sort(SortBy sort_by, SortOrder order = SortOrder::ASCENDING);

    // 清空所有 todo
    void clear();

    // 获取 todo 数量
    size_t size() const {
        return todos_.size();
    }

    // 数据持久化
    bool saveToFile(const std::string& file_path = "");
    bool loadFromFile(const std::string& file_path = "");

  private:
    std::vector<TodoItem> todos_;
    std::stack<Operation> undo_stack_; // 撤销栈
    std::stack<Operation> redo_stack_; // 重做栈

    // 查找 todo 索引
    size_t findTodoIndex(const std::string& id) const;

    // 获取默认数据文件路径
    std::string getDefaultDataPath() const;

    // 记录操作
    void recordOperation(OperationType type);
};

} // namespace todo
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TODO_TODO_MANAGER_H
