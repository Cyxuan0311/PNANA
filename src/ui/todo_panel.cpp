#include "ui/todo_panel.h"
#include "ui/icons.h"
#include <algorithm>
#include <chrono>
#include <ftxui/component/event.hpp>
#include <iomanip>
#include <sstream>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

TodoPanel::TodoPanel(Theme& theme)
    : theme_(theme), visible_(false), selected_index_(0), scroll_offset_(0),
      is_creating_todo_(false), current_field_(0), new_todo_content_(""), new_todo_time_input_(""),
      new_todo_priority_(5), cursor_position_(0), is_editing_priority_(false), priority_input_("") {
    main_component_ = Renderer([this] {
        return render();
    });
}

Element TodoPanel::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements content;
    content.push_back(renderHeader());
    content.push_back(separator());

    if (is_creating_todo_) {
        content.push_back(renderCreateDialog());
    } else if (is_editing_priority_) {
        content.push_back(renderPriorityEdit());
    } else {
        content.push_back(renderTodoList());
    }

    content.push_back(separator());
    content.push_back(renderHelpBar());

    return window(text(" Todo List ") | color(colors.success) | bold, vbox(std::move(content))) |
           size(WIDTH, GREATER_THAN, 80) | size(HEIGHT, GREATER_THAN, 20) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border);
}

Component TodoPanel::getComponent() {
    return main_component_;
}

void TodoPanel::show() {
    visible_ = true;
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void TodoPanel::hide() {
    visible_ = false;
    is_creating_todo_ = false;
    is_editing_priority_ = false;
    new_todo_content_.clear();
    new_todo_time_input_.clear();
    priority_input_.clear();
}

bool TodoPanel::handleInput(Event event) {
    if (!visible_) {
        return false;
    }

    // 处理创建 todo 模式
    if (is_creating_todo_) {
        if (event == Event::Escape) {
            cancelCreatingTodo();
            return true;
        }
        if (event == Event::Return) {
            if (current_field_ < 2) {
                // 移动到下一个字段
                current_field_++;
                cursor_position_ = 0;
            } else {
                // 在最后一个字段，完成创建
                finishCreatingTodo();
            }
            return true;
        }
        if (event == Event::Tab) {
            // Tab 切换到下一个字段
            current_field_ = (current_field_ + 1) % 3;
            cursor_position_ = 0;
            return true;
        }
        if (event == Event::TabReverse) {
            // Shift+Tab 切换到上一个字段
            current_field_ = (current_field_ + 2) % 3;
            cursor_position_ = 0;
            return true;
        }
        if (event == Event::ArrowUp) {
            // 向上切换到上一个字段
            current_field_ = (current_field_ + 2) % 3;
            cursor_position_ = 0;
            return true;
        }
        if (event == Event::ArrowDown) {
            // 向下切换到下一个字段
            current_field_ = (current_field_ + 1) % 3;
            cursor_position_ = 0;
            return true;
        }
        if (event == Event::ArrowLeft) {
            // 左箭头移动光标
            if (cursor_position_ > 0) {
                cursor_position_--;
            }
            return true;
        }
        if (event == Event::ArrowRight) {
            // 右箭头移动光标
            std::string* current_text = getCurrentField();
            if (current_text && cursor_position_ < current_text->length()) {
                cursor_position_++;
            }
            return true;
        }
        if (event == Event::Backspace) {
            std::string* current_text = getCurrentField();
            if (current_text && !current_text->empty() && cursor_position_ > 0) {
                current_text->erase(cursor_position_ - 1, 1);
                cursor_position_--;
            }
            return true;
        }
        if (event == Event::Delete) {
            std::string* current_text = getCurrentField();
            if (current_text && cursor_position_ < current_text->length()) {
                current_text->erase(cursor_position_, 1);
            }
            return true;
        }
        if (event.is_character()) {
            // 处理字符输入
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                std::string* current_text = getCurrentField();

                if (current_field_ == 0) {
                    // 内容字段：接受所有可打印字符
                    if (c >= 32 && c < 127) {
                        if (cursor_position_ <= current_text->length()) {
                            current_text->insert(cursor_position_, 1, c);
                            cursor_position_++;
                        }
                    }
                } else if (current_field_ == 1) {
                    // 时间字段：只接受数字、冒号、连字符和空格
                    if ((c >= '0' && c <= '9') || c == ':' || c == '-' || c == ' ') {
                        if (cursor_position_ <= current_text->length()) {
                            current_text->insert(cursor_position_, 1, c);
                            cursor_position_++;
                        }
                    }
                } else if (current_field_ == 2) {
                    // 优先级字段：只接受数字 1-9
                    if (c >= '1' && c <= '9') {
                        *current_text = c;
                        new_todo_priority_ = c - '0';
                        cursor_position_ = 1;
                    }
                }
            }
            return true;
        }
        return false;
    }

    // 处理编辑优先级模式
    if (is_editing_priority_) {
        if (event == Event::Escape) {
            cancelEditingPriority();
            return true;
        }
        if (event == Event::Return) {
            finishEditingPriority();
            return true;
        }
        if (event.is_character()) {
            char c = event.character()[0];
            if (c >= '0' && c <= '9') {
                priority_input_ += c;
            }
            return true;
        }
        if (event == Event::Backspace && !priority_input_.empty()) {
            priority_input_.pop_back();
            return true;
        }
        return false;
    }

    // 正常模式
    const auto& todos = todo_manager_.getTodos();
    if (todos.empty()) {
        if (event == Event::Character(' ')) {
            startCreatingTodo();
            return true;
        }
        if (event == Event::Escape) {
            hide();
            return true;
        }
        return false;
    }

    // 导航
    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (selected_index_ < todos.size() - 1) {
            selected_index_++;
            if (selected_index_ >= scroll_offset_ + 15) {
                scroll_offset_ = selected_index_ - 14;
            }
        }
        return true;
    }

    // Space: 创建新 todo
    if (event == Event::Character(' ')) {
        startCreatingTodo();
        return true;
    }

    // Delete: 删除选中的 todo
    if (event == Event::Delete) {
        deleteSelectedTodo();
        return true;
    }

    // F1: 编辑优先级
    if (event == Event::F1) {
        startEditingPriority();
        return true;
    }

    // Escape: 关闭面板
    if (event == Event::Escape) {
        hide();
        return true;
    }

    return false;
}

Element TodoPanel::renderHeader() const {
    const auto& colors = theme_.getColors();
    const auto& todos = todo_manager_.getTodos();
    size_t active_count = todo_manager_.getActiveTodos().size();
    size_t due_count = todo_manager_.getDueTodos().size();

    Elements header;
    header.push_back(text(" Total: " + std::to_string(todos.size()) +
                          " | Active: " + std::to_string(active_count)));
    if (due_count > 0) {
        header.push_back(text(" | Due: " + std::to_string(due_count)) | color(colors.error) | bold);
    }

    return hbox(std::move(header)) | bgcolor(colors.dialog_title_bg) |
           color(colors.dialog_title_fg);
}

Element TodoPanel::renderTodoList() const {
    const auto& colors = theme_.getColors();
    const auto& todos = todo_manager_.getTodos();
    Elements list_elements;

    if (todos.empty()) {
        list_elements.push_back(hbox({text("  "), text("No todos. Press Space to create one.") |
                                                      color(colors.comment) | dim}));
    } else {
        size_t max_display = std::min(todos.size(), size_t(15));
        for (size_t i = 0; i < max_display && (scroll_offset_ + i) < todos.size(); ++i) {
            size_t index = scroll_offset_ + i;
            const auto& todo = todos[index];
            bool is_selected = (index == selected_index_);
            list_elements.push_back(renderTodoItem(todo, index, is_selected));
        }

        // 滚动提示
        if (scroll_offset_ > 0 || (scroll_offset_ + max_display) < todos.size()) {
            list_elements.push_back(text(""));
            std::string scroll_text = "... " + std::to_string(todos.size() - max_display) + " more";
            list_elements.push_back(
                hbox({text("  "), text(scroll_text) | color(colors.comment) | dim}));
        }
    }

    return vbox(list_elements);
}

Element TodoPanel::renderTodoItem(const features::todo::TodoItem& todo, size_t /*index*/,
                                  bool is_selected) const {
    const auto& colors = theme_.getColors();
    Elements item_elements;

    // 选中标记
    if (is_selected) {
        item_elements.push_back(text("► ") | color(colors.success) | bold);
    } else {
        item_elements.push_back(text("  "));
    }

    // 优先级显示
    std::string priority_str = "[" + std::to_string(todo.priority) + "]";
    item_elements.push_back(text(priority_str) | color(colors.keyword) | dim);

    // 内容
    std::string content_display = todo.content;
    if (content_display.length() > 50) {
        content_display = content_display.substr(0, 47) + "...";
    }
    Color content_color = is_selected ? colors.dialog_fg : colors.foreground;
    if (todo.completed) {
        content_color = colors.comment;
        content_display = "✓ " + content_display;
    }
    Element content_text = text(" " + content_display) | color(content_color);
    if (is_selected) {
        content_text = content_text | bold;
    }
    item_elements.push_back(content_text);

    // 时间显示
    item_elements.push_back(filler());
    std::string time_str = formatTime(todo.due_time);
    bool is_due = isTimeDue(todo.due_time);
    Color time_color = is_due ? colors.error : colors.comment;
    item_elements.push_back(text(time_str) | color(time_color) | (is_due ? bold : dim));

    Element item_line = hbox(item_elements);
    if (is_selected) {
        item_line = item_line | bgcolor(colors.selection);
    }

    return item_line;
}

Element TodoPanel::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    Elements help;

    if (is_creating_todo_) {
        help.push_back(text("  ↑↓: Switch Field  Tab: Next  Enter: Next/Confirm  Esc: Cancel"));
    } else if (is_editing_priority_) {
        help.push_back(text("  Enter: Confirm  Esc: Cancel"));
    } else {
        help.push_back(
            text("  Space: Create  ↑↓: Navigate  Delete: Remove  F1: Edit Priority  Esc: Close"));
    }

    return hbox(help) | bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

Element TodoPanel::renderCreateDialog() const {
    const auto& colors = theme_.getColors();
    Elements dialog;

    dialog.push_back(text("  Create New Todo") | bold | color(colors.dialog_fg));
    dialog.push_back(text(""));

    // 内容输入
    std::string content_display = new_todo_content_;
    if (current_field_ == 0) {
        // 在当前字段显示光标
        if (cursor_position_ <= content_display.length()) {
            content_display.insert(cursor_position_, "_");
        } else {
            content_display += "_";
        }
    }
    Color content_bg = (current_field_ == 0) ? colors.selection : colors.background;
    dialog.push_back(hbox({text("  Content: "),
                           text(content_display) | color(colors.dialog_fg) | bgcolor(content_bg)}));
    dialog.push_back(text(""));

    // 时间输入
    std::string time_display = new_todo_time_input_;
    if (current_field_ == 1) {
        // 在当前字段显示光标
        if (cursor_position_ <= time_display.length()) {
            time_display.insert(cursor_position_, "_");
        } else {
            time_display += "_";
        }
    }
    Color time_bg = (current_field_ == 1) ? colors.selection : colors.background;
    dialog.push_back(hbox({text("  Time (YYYY-MM-DD HH:MM): "),
                           text(time_display) | color(colors.dialog_fg) | bgcolor(time_bg)}));
    dialog.push_back(text(""));

    // 优先级输入
    std::string priority_display = priority_input_.empty() ? "5" : priority_input_;
    if (current_field_ == 2) {
        // 在当前字段显示光标
        if (cursor_position_ <= priority_display.length()) {
            priority_display.insert(cursor_position_, "_");
        } else {
            priority_display += "_";
        }
    }
    Color priority_bg = (current_field_ == 2) ? colors.selection : colors.background;
    dialog.push_back(
        hbox({text("  Priority (1-9): "),
              text(priority_display) | color(colors.dialog_fg) | bgcolor(priority_bg)}));

    return vbox(dialog);
}

Element TodoPanel::renderPriorityEdit() const {
    const auto& colors = theme_.getColors();
    const auto& todos = todo_manager_.getTodos();
    Elements dialog;

    if (selected_index_ < todos.size()) {
        const auto& todo = todos[selected_index_];
        dialog.push_back(text("  Edit Priority for: " + todo.content) | bold |
                         color(colors.dialog_fg));
        dialog.push_back(text(""));
        std::string priority_display =
            priority_input_.empty() ? std::to_string(todo.priority) + "_" : priority_input_ + "_";
        dialog.push_back(
            hbox({text("  Priority (1-9): "),
                  text(priority_display) | color(colors.dialog_fg) | bgcolor(colors.selection)}));
    }

    return vbox(dialog);
}

void TodoPanel::startCreatingTodo() {
    is_creating_todo_ = true;
    current_field_ = 0;
    new_todo_content_.clear();

    // 设置默认时间为当前时间（格式：YYYY-MM-DD HH:MM）
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << (tm->tm_year + 1900) << "-" << std::setw(2)
        << (tm->tm_mon + 1) << "-" << std::setw(2) << tm->tm_mday << " " << std::setw(2)
        << tm->tm_hour << ":" << std::setw(2) << tm->tm_min;
    new_todo_time_input_ = oss.str();

    priority_input_.clear();
    new_todo_priority_ = 5;
    cursor_position_ = 0;
}

std::string* TodoPanel::getCurrentField() {
    if (current_field_ == 0) {
        return &new_todo_content_;
    } else if (current_field_ == 1) {
        return &new_todo_time_input_;
    } else {
        return &priority_input_;
    }
}

void TodoPanel::finishCreatingTodo() {
    if (new_todo_content_.empty() || new_todo_time_input_.empty()) {
        // 输入不完整，取消创建
        cancelCreatingTodo();
        return;
    }

    try {
        auto due_time = parseTimeInput(new_todo_time_input_);
        todo_manager_.addTodo(new_todo_content_, due_time, new_todo_priority_);
        todo_manager_.sortByPriority();
        cancelCreatingTodo();
    } catch (...) {
        // 时间解析失败，取消创建
        cancelCreatingTodo();
    }
}

void TodoPanel::cancelCreatingTodo() {
    is_creating_todo_ = false;
    current_field_ = 0;
    new_todo_content_.clear();
    new_todo_time_input_.clear();
    priority_input_.clear();
    new_todo_priority_ = 5;
    cursor_position_ = 0;
}

void TodoPanel::deleteSelectedTodo() {
    const auto& todos = todo_manager_.getTodos();
    if (selected_index_ < todos.size()) {
        todo_manager_.removeTodo(selected_index_);
        if (selected_index_ >= todo_manager_.size() && selected_index_ > 0) {
            selected_index_--;
        }
        if (selected_index_ < scroll_offset_) {
            scroll_offset_ = selected_index_;
        }
    }
}

void TodoPanel::startEditingPriority() {
    const auto& todos = todo_manager_.getTodos();
    if (selected_index_ < todos.size()) {
        is_editing_priority_ = true;
        priority_input_ = std::to_string(todos[selected_index_].priority);
    }
}

void TodoPanel::finishEditingPriority() {
    const auto& todos = todo_manager_.getTodos();
    if (selected_index_ < todos.size() && !priority_input_.empty()) {
        try {
            int new_priority = std::stoi(priority_input_);
            if (new_priority >= 1 && new_priority <= 9) {
                todo_manager_.updateTodoPriority(selected_index_, new_priority);
                todo_manager_.sortByPriority();
                // 重新找到当前 todo 的位置（排序后可能改变）
                const auto& updated_todos = todo_manager_.getTodos();
                for (size_t i = 0; i < updated_todos.size(); ++i) {
                    if (updated_todos[i].id == todos[selected_index_].id) {
                        selected_index_ = i;
                        break;
                    }
                }
            }
        } catch (...) {
            // 解析失败，忽略
        }
    }
    cancelEditingPriority();
}

void TodoPanel::cancelEditingPriority() {
    is_editing_priority_ = false;
    priority_input_.clear();
}

std::chrono::system_clock::time_point TodoPanel::parseTimeInput(const std::string& time_str) const {
    // 解析时间格式：YYYY-MM-DD HH:MM 或 YYYY-MM-DD HH:MM:SS
    // 也支持简化的格式：HH:MM（使用当前日期）

    std::istringstream iss(time_str);
    std::tm tm = {};

    // 尝试解析完整格式：YYYY-MM-DD HH:MM
    int year, month, day, hour, minute, second = 0;
    char dash1, dash2, space, colon1, colon2;

    if (iss >> year >> dash1 >> month >> dash2 >> day >> space >> hour >> colon1 >> minute) {
        if (dash1 == '-' && dash2 == '-' && space == ' ' && colon1 == ':') {
            // 尝试读取秒数（可选）
            if (iss >> colon2 >> second) {
                // 有秒数
            } else {
                second = 0;
            }

            tm.tm_year = year - 1900;
            tm.tm_mon = month - 1;
            tm.tm_mday = day;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = second;

            auto target_time = std::mktime(&tm);
            if (target_time != -1) {
                return std::chrono::system_clock::from_time_t(target_time);
            }
        }
    }

    // 如果完整格式解析失败，尝试简化格式：HH:MM
    iss.clear();
    iss.str(time_str);
    hour = minute = 0;
    second = 0;
    char colon1_simple, colon2_simple;

    if (iss >> hour >> colon1_simple >> minute) {
        if (colon1_simple == ':') {
            // 尝试读取秒数（可选）
            if (iss >> colon2_simple >> second) {
                // 有秒数
            } else {
                second = 0;
            }

            // 获取当前日期
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm* now_tm = std::localtime(&time_t);

            // 使用当前日期，设置时间
            tm = *now_tm;
            tm.tm_hour = hour;
            tm.tm_min = minute;
            tm.tm_sec = second;

            // 如果时间已过，设置为明天
            auto target_time = std::mktime(&tm);
            auto target_tp = std::chrono::system_clock::from_time_t(target_time);
            if (target_tp <= now) {
                // 加一天
                target_tp += std::chrono::hours(24);
            }

            return target_tp;
        }
    }

    // 解析失败，返回当前时间
    return std::chrono::system_clock::now();
}

std::string TodoPanel::formatTime(const std::chrono::system_clock::time_point& time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&time_t);

    std::ostringstream oss;
    // 格式：YYYY-MM-DD HH:MM
    oss << std::setfill('0') << std::setw(4) << (tm->tm_year + 1900) << "-" << std::setw(2)
        << (tm->tm_mon + 1) << "-" << std::setw(2) << tm->tm_mday << " " << std::setw(2)
        << tm->tm_hour << ":" << std::setw(2) << tm->tm_min;

    return oss.str();
}

bool TodoPanel::isTimeDue(const std::chrono::system_clock::time_point& time) const {
    auto now = std::chrono::system_clock::now();
    return time <= now;
}

} // namespace ui
} // namespace pnana
