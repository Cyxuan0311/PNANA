#include "features/todo/todo_manager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace pnana {
namespace features {
namespace todo {

TodoManager::TodoManager() {
    // 尝试从默认路径加载数据
    loadFromFile();
}

void TodoManager::recordOperation(OperationType type) {
    Operation op;
    op.type = type;
    op.state_before = todos_; // 操作前的状态
    // 操作后的状态会在调用此方法后由具体操作设置
    undo_stack_.push(op);
    // 清空重做栈
    while (!redo_stack_.empty()) {
        redo_stack_.pop();
    }
}

std::string TodoManager::getDefaultDataPath() const {
    // 获取用户家目录
    const char* home = getenv("HOME");
    if (!home) {
        return "todos.json";
    }

    // 构建默认路径: ~/.config/pnana/todos.json
    std::string path = std::string(home) + "/.config/pnana";

    // 创建目录（如果不存在）
    std::filesystem::create_directories(path);

    return path + "/todos.json";
}

void TodoManager::addTodo(const std::string& content,
                          const std::chrono::system_clock::time_point& due_time, int priority) {
    // 记录操作
    recordOperation(OperationType::ADD);

    todos_.emplace_back(content, due_time, priority);

    // 更新操作记录的state_after
    if (!undo_stack_.empty()) {
        undo_stack_.top().state_after = todos_;
    }

    // 自动保存到文件
    saveToFile();
}

void TodoManager::removeTodo(const std::string& id) {
    // 记录操作
    recordOperation(OperationType::REMOVE);

    auto it = std::remove_if(todos_.begin(), todos_.end(), [&id](const TodoItem& item) {
        return item.id == id;
    });
    todos_.erase(it, todos_.end());

    // 更新操作记录的state_after
    if (!undo_stack_.empty()) {
        undo_stack_.top().state_after = todos_;
    }

    // 自动保存到文件
    saveToFile();
}

void TodoManager::removeTodo(size_t index) {
    if (index < todos_.size()) {
        // 记录操作
        recordOperation(OperationType::REMOVE);

        todos_.erase(todos_.begin() + index);

        // 更新操作记录的state_after
        if (!undo_stack_.empty()) {
            undo_stack_.top().state_after = todos_;
        }

        // 自动保存到文件
        saveToFile();
    }
}

void TodoManager::updateTodo(const std::string& id, const std::string& content,
                             const std::chrono::system_clock::time_point& due_time, int priority) {
    size_t index = findTodoIndex(id);
    if (index < todos_.size()) {
        // 记录操作
        recordOperation(OperationType::UPDATE);

        todos_[index].content = content;
        todos_[index].due_time = due_time;
        todos_[index].priority = priority;

        // 更新操作记录的state_after
        if (!undo_stack_.empty()) {
            undo_stack_.top().state_after = todos_;
        }

        // 自动保存到文件
        saveToFile();
    }
}

void TodoManager::updateTodoPriority(const std::string& id, int priority) {
    size_t index = findTodoIndex(id);
    if (index < todos_.size()) {
        // 记录操作
        recordOperation(OperationType::UPDATE);

        todos_[index].priority = priority;

        // 更新操作记录的state_after
        if (!undo_stack_.empty()) {
            undo_stack_.top().state_after = todos_;
        }

        // 自动保存到文件
        saveToFile();
    }
}

void TodoManager::updateTodoPriority(size_t index, int priority) {
    if (index < todos_.size()) {
        // 记录操作
        recordOperation(OperationType::UPDATE);

        todos_[index].priority = priority;

        // 更新操作记录的state_after
        if (!undo_stack_.empty()) {
            undo_stack_.top().state_after = todos_;
        }

        // 自动保存到文件
        saveToFile();
    }
}

void TodoManager::markCompleted(const std::string& id, bool completed) {
    size_t index = findTodoIndex(id);
    if (index < todos_.size()) {
        // 记录操作
        recordOperation(OperationType::MARK_COMPLETED);

        todos_[index].completed = completed;

        // 更新操作记录的state_after
        if (!undo_stack_.empty()) {
            undo_stack_.top().state_after = todos_;
        }

        // 自动保存到文件
        saveToFile();
    }
}

void TodoManager::markCompleted(size_t index, bool completed) {
    if (index < todos_.size()) {
        // 记录操作
        recordOperation(OperationType::MARK_COMPLETED);

        todos_[index].completed = completed;

        // 更新操作记录的state_after
        if (!undo_stack_.empty()) {
            undo_stack_.top().state_after = todos_;
        }

        // 自动保存到文件
        saveToFile();
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

std::vector<TodoItem> TodoManager::getCompletedTodos() const {
    std::vector<TodoItem> completed;
    for (const auto& todo : todos_) {
        if (todo.completed) {
            completed.push_back(todo);
        }
    }
    return completed;
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

std::vector<TodoItem> TodoManager::getHighPriorityTodos() const {
    std::vector<TodoItem> high_priority;
    for (const auto& todo : todos_) {
        if (todo.priority <= 3) {
            high_priority.push_back(todo);
        }
    }
    return high_priority;
}

std::vector<TodoItem> TodoManager::getFilteredTodos(FilterType filter) const {
    switch (filter) {
        case FilterType::ALL:
            return todos_;
        case FilterType::ACTIVE:
            return getActiveTodos();
        case FilterType::COMPLETED:
            return getCompletedTodos();
        case FilterType::DUE:
            return getDueTodos();
        case FilterType::HIGH_PRIORITY:
            return getHighPriorityTodos();
        default:
            return todos_;
    }
}

bool TodoManager::isDueWithinBlinkWindow(const std::chrono::system_clock::time_point& due_time) {
    auto now = std::chrono::system_clock::now();
    if (due_time > now) {
        return false; // 未到期不闪烁
    }
    auto overdue = now - due_time;
    return overdue <= std::chrono::minutes(1); // 仅到期后 1 分钟内闪烁
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

void TodoManager::sortByPriority(SortOrder order) {
    std::sort(todos_.begin(), todos_.end(), [order](const TodoItem& a, const TodoItem& b) {
        if (order == SortOrder::ASCENDING) {
            return a.priority < b.priority;
        } else {
            return a.priority > b.priority;
        }
    });
}

void TodoManager::sortByDueTime(SortOrder order) {
    std::sort(todos_.begin(), todos_.end(), [order](const TodoItem& a, const TodoItem& b) {
        if (order == SortOrder::ASCENDING) {
            return a.due_time < b.due_time;
        } else {
            return a.due_time > b.due_time;
        }
    });
}

void TodoManager::sortByCreatedTime(SortOrder order) {
    std::sort(todos_.begin(), todos_.end(), [order](const TodoItem& a, const TodoItem& b) {
        if (order == SortOrder::ASCENDING) {
            return a.created_time < b.created_time;
        } else {
            return a.created_time > b.created_time;
        }
    });
}

void TodoManager::sort(SortBy sort_by, SortOrder order) {
    switch (sort_by) {
        case SortBy::PRIORITY:
            sortByPriority(order);
            break;
        case SortBy::DUE_TIME:
            sortByDueTime(order);
            break;
        case SortBy::CREATED_TIME:
            sortByCreatedTime(order);
            break;
    }
    // 自动保存到文件
    saveToFile();
}

void TodoManager::clear() {
    // 记录操作
    recordOperation(OperationType::CLEAR);

    todos_.clear();

    // 更新操作记录的state_after
    if (!undo_stack_.empty()) {
        undo_stack_.top().state_after = todos_;
    }

    // 自动保存到文件
    saveToFile();
}

size_t TodoManager::findTodoIndex(const std::string& id) const {
    for (size_t i = 0; i < todos_.size(); ++i) {
        if (todos_[i].id == id) {
            return i;
        }
    }
    return todos_.size(); // Not found
}

bool TodoManager::saveToFile(const std::string& file_path) {
    try {
        std::string path = file_path.empty() ? getDefaultDataPath() : file_path;

        // 构建JSON对象
        json j;
        for (const auto& todo : todos_) {
            json todo_obj;
            todo_obj["id"] = todo.id;
            todo_obj["content"] = todo.content;
            todo_obj["due_time"] = std::chrono::system_clock::to_time_t(todo.due_time);
            todo_obj["created_time"] = std::chrono::system_clock::to_time_t(todo.created_time);
            todo_obj["priority"] = todo.priority;
            todo_obj["completed"] = todo.completed;
            j.push_back(todo_obj);
        }

        // 写入文件
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        file << j.dump(4); // 缩进4个空格，便于阅读
        file.close();

        return true;
    } catch (...) {
        return false;
    }
}

bool TodoManager::loadFromFile(const std::string& file_path) {
    try {
        std::string path = file_path.empty() ? getDefaultDataPath() : file_path;

        // 检查文件是否存在
        if (!std::filesystem::exists(path)) {
            return true; // 文件不存在，视为成功（空列表）
        }

        // 读取文件
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }

        // 解析JSON
        json j;
        file >> j;
        file.close();

        // 清空现有todos
        todos_.clear();

        // 解析每个todo项
        for (const auto& todo_obj : j) {
            TodoItem todo(todo_obj["content"],
                          std::chrono::system_clock::from_time_t(todo_obj["due_time"]),
                          todo_obj["priority"]);
            todo.id = todo_obj["id"];
            // 处理created_time字段（兼容旧版本）
            if (todo_obj.contains("created_time")) {
                todo.created_time =
                    std::chrono::system_clock::from_time_t(todo_obj["created_time"]);
            }
            todo.completed = todo_obj["completed"];
            todos_.push_back(todo);
        }

        // 清空操作栈
        while (!undo_stack_.empty()) {
            undo_stack_.pop();
        }
        while (!redo_stack_.empty()) {
            redo_stack_.pop();
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool TodoManager::undo() {
    if (undo_stack_.empty()) {
        return false;
    }

    Operation op = undo_stack_.top();
    undo_stack_.pop();

    // 保存当前状态到重做栈
    Operation redo_op;
    redo_op.type = op.type;
    redo_op.state_before = todos_;
    redo_op.state_after = op.state_before;
    redo_stack_.push(redo_op);

    // 恢复到操作前的状态
    todos_ = op.state_before;

    // 自动保存到文件
    saveToFile();

    return true;
}

bool TodoManager::redo() {
    if (redo_stack_.empty()) {
        return false;
    }

    Operation op = redo_stack_.top();
    redo_stack_.pop();

    // 保存当前状态到撤销栈
    Operation undo_op;
    undo_op.type = op.type;
    undo_op.state_before = todos_;
    undo_op.state_after = op.state_after;
    undo_stack_.push(undo_op);

    // 恢复到操作后的状态
    todos_ = op.state_after;

    // 自动保存到文件
    saveToFile();

    return true;
}

bool TodoManager::canUndo() const {
    return !undo_stack_.empty();
}

bool TodoManager::canRedo() const {
    return !redo_stack_.empty();
}

} // namespace todo
} // namespace features
} // namespace pnana
