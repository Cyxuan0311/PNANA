#ifndef PNANA_UI_TODO_PANEL_H
#define PNANA_UI_TODO_PANEL_H

#include "features/todo/todo_manager.h"
#include "ui/theme.h"
#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace ui {

// Todo 面板
class TodoPanel {
  public:
    TodoPanel(Theme& theme);
    ~TodoPanel() = default;

    // UI 渲染
    ftxui::Element render();
    ftxui::Component getComponent();

    // 面板控制
    void show();
    void hide();
    bool isVisible() const {
        return visible_;
    }

    // 输入处理
    bool handleInput(ftxui::Event event);

    // 获取 TodoManager 引用
    features::todo::TodoManager& getTodoManager() {
        return todo_manager_;
    }

    const features::todo::TodoManager& getTodoManager() const {
        return todo_manager_;
    }

  private:
    Theme& theme_;
    features::todo::TodoManager todo_manager_;
    bool visible_;
    size_t selected_index_;
    size_t scroll_offset_;

    // 创建 todo 时的输入状态
    bool is_creating_todo_;
    int current_field_; // 0: 内容, 1: 时间, 2: 优先级
    std::string new_todo_content_;
    std::string new_todo_time_input_;
    int new_todo_priority_;
    size_t cursor_position_; // 当前字段的光标位置

    // 编辑优先级状态
    bool is_editing_priority_;
    std::string priority_input_;

    // UI 组件
    ftxui::Component main_component_;

    // 渲染方法
    ftxui::Element renderHeader() const;
    ftxui::Element renderTodoList() const;
    ftxui::Element renderTodoItem(const features::todo::TodoItem& todo, size_t index,
                                  bool is_selected) const;
    ftxui::Element renderHelpBar() const;
    ftxui::Element renderCreateDialog() const;
    ftxui::Element renderPriorityEdit() const;

    // 辅助方法
    void startCreatingTodo();
    void finishCreatingTodo();
    void cancelCreatingTodo();
    void deleteSelectedTodo();
    void startEditingPriority();
    void finishEditingPriority();
    void cancelEditingPriority();
    std::string* getCurrentField(); // 获取当前输入字段的指针

    // 解析时间输入（支持格式：HH:MM 或 HH:MM:SS）
    std::chrono::system_clock::time_point parseTimeInput(const std::string& time_str) const;

    // 格式化时间显示
    std::string formatTime(const std::chrono::system_clock::time_point& time) const;

    // 检查时间是否已到达
    bool isTimeDue(const std::chrono::system_clock::time_point& time) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_TODO_PANEL_H
