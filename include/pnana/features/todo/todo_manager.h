#ifndef PNANA_FEATURES_TODO_TODO_MANAGER_H
#define PNANA_FEATURES_TODO_TODO_MANAGER_H

#include <chrono>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace todo {

// Todo 项结构
struct TodoItem {
    std::string id;                                 // 唯一标识符
    std::string content;                            // 内容
    std::chrono::system_clock::time_point due_time; // 到期时间
    int priority;                                   // 优先级（数字越小优先级越高）
    bool completed;                                 // 是否完成

    TodoItem(const std::string& c, const std::chrono::system_clock::time_point& dt, int p = 5)
        : content(c), due_time(dt), priority(p), completed(false) {
        // 生成唯一ID（基于时间戳）
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        id =
            std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
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

    // 获取所有 todo
    const std::vector<TodoItem>& getTodos() const {
        return todos_;
    }

    // 获取未完成的 todo
    std::vector<TodoItem> getActiveTodos() const;

    // 获取需要提醒的 todo（当前时间已到达）
    std::vector<TodoItem> getDueTodos() const;

    // 排序 todo（按优先级）
    void sortByPriority();

    // 清空所有 todo
    void clear();

    // 获取 todo 数量
    size_t size() const {
        return todos_.size();
    }

  private:
    std::vector<TodoItem> todos_;

    // 查找 todo 索引
    size_t findTodoIndex(const std::string& id) const;
};

} // namespace todo
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TODO_TODO_MANAGER_H
