#include "features/todo/todo_manager.h"
#include <algorithm>

namespace pnana {
namespace features {
namespace todo {

TodoManager::TodoManager() {}

void TodoManager::addTodo(const std::string& content,
                          const std::chrono::system_clock::time_point& due_time, int priority) {
    todos_.emplace_back(content, due_time, priority);
}

void TodoManager::removeTodo(const std::string& id) {
    auto it = std::remove_if(todos_.begin(), todos_.end(), [&id](const TodoItem& item) {
        return item.id == id;
    });
    todos_.erase(it, todos_.end());
}

void TodoManager::removeTodo(size_t index) {
    if (index < todos_.size()) {
        todos_.erase(todos_.begin() + index);
    }
}

void TodoManager::updateTodo(const std::string& id, const std::string& content,
                             const std::chrono::system_clock::time_point& due_time, int priority) {
    size_t index = findTodoIndex(id);
    if (index < todos_.size()) {
        todos_[index].content = content;
        todos_[index].due_time = due_time;
        todos_[index].priority = priority;
    }
}

void TodoManager::updateTodoPriority(const std::string& id, int priority) {
    size_t index = findTodoIndex(id);
    if (index < todos_.size()) {
        todos_[index].priority = priority;
    }
}

void TodoManager::updateTodoPriority(size_t index, int priority) {
    if (index < todos_.size()) {
        todos_[index].priority = priority;
    }
}

void TodoManager::markCompleted(const std::string& id, bool completed) {
    size_t index = findTodoIndex(id);
    if (index < todos_.size()) {
        todos_[index].completed = completed;
    }
}

void TodoManager::markCompleted(size_t index, bool completed) {
    if (index < todos_.size()) {
        todos_[index].completed = completed;
    }
}

std::vector<TodoItem> TodoManager::getActiveTodos() const {
    std::vector<TodoItem> active;
    for (const auto& todo : todos_) {
        if (!todo.completed) {
            active.push_back(todo);
        }
    }
    return active;
}

std::vector<TodoItem> TodoManager::getDueTodos() const {
    auto now = std::chrono::system_clock::now();
    std::vector<TodoItem> due;
    for (const auto& todo : todos_) {
        if (!todo.completed && todo.due_time <= now) {
            due.push_back(todo);
        }
    }
    return due;
}

void TodoManager::sortByPriority() {
    std::sort(todos_.begin(), todos_.end(), [](const TodoItem& a, const TodoItem& b) {
        return a.priority < b.priority;
    });
}

void TodoManager::clear() {
    todos_.clear();
}

size_t TodoManager::findTodoIndex(const std::string& id) const {
    for (size_t i = 0; i < todos_.size(); ++i) {
        if (todos_[i].id == id) {
            return i;
        }
    }
    return todos_.size(); // 未找到
}

} // namespace todo
} // namespace features
} // namespace pnana
