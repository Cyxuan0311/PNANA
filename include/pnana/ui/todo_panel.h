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
    struct Date {
        int year = 1970;
        int month = 1; // 1-12
        int day = 1;   // 1-31
    };

    enum class FocusArea { CALENDAR, LIST };

    Theme& theme_;
    features::todo::TodoManager todo_manager_;
    bool visible_;
    size_t selected_index_;
    size_t scroll_offset_;
    FocusArea focus_area_;

    // 日历选择状态
    Date selected_date_;
    // 日历懒加载缓存（仅当月份变化或 todo 变更时重算）
    int todo_revision_;
    mutable int cached_todo_revision_;
    mutable int cached_calendar_year_;
    mutable int cached_calendar_month_;
    mutable std::vector<int> cached_day_has_todo_; // index=day (1..days_in_month), value=count

    // 创建 todo 时的输入状态
    bool is_creating_todo_;
    int current_field_; // 0: 内容, 1: 时间, 2: 优先级
    std::string new_todo_content_;
    std::string new_todo_time_input_;
    int new_todo_priority_;
    size_t cursor_position_; // 当前字段的光标位置

    // 编辑 todo（内容/到期/优先级）状态
    bool is_editing_todo_;
    std::string edit_todo_id_;
    std::string edit_todo_content_;
    std::string edit_todo_time_input_;
    std::string edit_todo_priority_input_;
    int edit_current_field_; // 0: 内容, 1: 时间, 2: 优先级
    size_t edit_cursor_position_;

    // UI 组件
    ftxui::Component main_component_;

    // 渲染方法
    ftxui::Element renderHeader() const;
    ftxui::Element renderBody() const;
    ftxui::Element renderCalendar() const;
    ftxui::Element renderTodoList() const;
    ftxui::Element renderTodoItem(const features::todo::TodoItem& todo, size_t index,
                                  bool is_selected) const;
    ftxui::Element renderHelpBar() const;
    ftxui::Element renderCreateDialog() const;
    ftxui::Element renderEditDialog() const;

    // 辅助方法
    void startCreatingTodo();
    void finishCreatingTodo();
    void cancelCreatingTodo();
    void deleteSelectedTodo();
    std::string* getCurrentField(); // 获取当前输入字段的指针
    void startEditingTodo();
    void finishEditingTodo();
    void cancelEditingTodo();
    std::string* getCurrentEditField(); // 获取当前编辑字段的指针

    // 选中日期相关
    Date todayDate() const;
    Date addDays(Date d, int delta_days) const;
    bool sameDate(const Date& a, const Date& b) const;
    std::chrono::system_clock::time_point makeTimePointForDateAndTime(
        const Date& d, const std::chrono::system_clock::time_point& time_of_day) const;
    std::chrono::system_clock::time_point applySelectedDateToTimeInput(
        const std::string& time_input) const;
    std::vector<size_t> getVisibleTodoIndicesForSelectedDate() const;
    void clampSelectionToVisibleTodos();

    const std::vector<int>& getDayHasTodoForMonth(int year, int month, int days_in_month) const;

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
