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

std::string TodoManager::formatTimeRemaining(
    const std::chrono::system_clock::time_point& due_time) {
    auto now = std::chrono::system_clock::now();
    auto diff = due_time - now;
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();

    if (total_seconds < 0) {
        // Overdue
        auto overdue_seconds = -total_seconds;

        if (overdue_seconds < 60) {
            // Less than 1 minute: show seconds
            return "Overdue " + std::to_string(overdue_seconds) + "s";
        } else if (overdue_seconds < 3600) {
            // Less than 1 hour: show minutes and seconds
            auto minutes = overdue_seconds / 60;
            auto seconds = overdue_seconds % 60;
            if (seconds == 0) {
                return "Overdue " + std::to_string(minutes) + "m";
            } else {
                return "Overdue " + std::to_string(minutes) + "m" + std::to_string(seconds) + "s";
            }
        } else {
            // 1 hour or more: show hours, minutes, and seconds
            auto hours = overdue_seconds / 3600;
            auto remaining_seconds = overdue_seconds % 3600;
            auto minutes = remaining_seconds / 60;
            auto seconds = remaining_seconds % 60;

            std::string result = "Overdue " + std::to_string(hours) + "h";
            if (minutes > 0 || seconds > 0) {
                result += std::to_string(minutes) + "m";
                if (seconds > 0) {
                    result += std::to_string(seconds) + "s";
                }
            }
            return result;
        }
    } else {
        // Not yet due
        if (total_seconds < 60) {
            // Less than 1 minute: show seconds
            return "In " + std::to_string(total_seconds) + "s";
        } else if (total_seconds < 3600) {
            // Less than 1 hour: show minutes and seconds
            auto minutes = total_seconds / 60;
            auto seconds = total_seconds % 60;
            if (seconds == 0) {
                return "In " + std::to_string(minutes) + "m";
            } else {
                return "In " + std::to_string(minutes) + "m" + std::to_string(seconds) + "s";
            }
        } else {
            // 1 hour or more: show hours, minutes, and seconds
            auto hours = total_seconds / 3600;
            auto remaining_seconds = total_seconds % 3600;
            auto minutes = remaining_seconds / 60;
            auto seconds = remaining_seconds % 60;

            std::string result = "In " + std::to_string(hours) + "h";
            if (minutes > 0 || seconds > 0) {
                result += std::to_string(minutes) + "m";
                if (seconds > 0) {
                    result += std::to_string(seconds) + "s";
                }
            }
            return result;
        }
    }
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
    return todos_.size(); // Not found
}

} // namespace todo
} // namespace features
} // namespace pnana
