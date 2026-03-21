#include "ui/todo_panel.h"
#include "features/todo/todo_manager.h"
#include "ui/icons.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
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
      new_todo_priority_(5), cursor_position_(0), is_editing_todo_(false), edit_todo_id_(""),
      edit_todo_content_(""), edit_todo_time_input_(""), edit_todo_priority_input_(""),
      edit_current_field_(0), edit_cursor_position_(0) {
    focus_area_ = FocusArea::LIST;
    selected_date_ = todayDate();
    todo_revision_ = 0;
    cached_todo_revision_ = -1;
    cached_calendar_year_ = -1;
    cached_calendar_month_ = -1;
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
    content.push_back(separator() | color(colors.dialog_border));

    if (is_creating_todo_) {
        content.push_back(renderCreateDialog());
    } else if (is_editing_todo_) {
        content.push_back(renderEditDialog());
    } else {
        content.push_back(renderBody());
    }

    content.push_back(separator() | color(colors.dialog_border));
    content.push_back(renderHelpBar());

    const int panel_min_width = 72;
    const int panel_min_height = 18;
    return window(text(" " + std::string(icons::CHECKLIST) + " Todo List ") |
                      color(colors.success) | bold,
                  vbox(std::move(content))) |
           size(WIDTH, GREATER_THAN, panel_min_width) |
           size(HEIGHT, GREATER_THAN, panel_min_height) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
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
    is_editing_todo_ = false;
    new_todo_content_.clear();
    new_todo_time_input_.clear();
    edit_todo_id_.clear();
    edit_todo_content_.clear();
    edit_todo_time_input_.clear();
    edit_todo_priority_input_.clear();
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
                        new_todo_priority_ = c - '0';
                        cursor_position_ = 1;
                    }
                }
            }
            return true;
        }
        return false;
    }

    // 处理编辑 todo 模式
    if (is_editing_todo_) {
        if (event == Event::Escape) {
            cancelEditingTodo();
            return true;
        }
        if (event == Event::Return) {
            if (edit_current_field_ < 2) {
                edit_current_field_++;
                edit_cursor_position_ = 0;
            } else {
                finishEditingTodo();
            }
            return true;
        }
        if (event == Event::Tab) {
            edit_current_field_ = (edit_current_field_ + 1) % 3;
            edit_cursor_position_ = 0;
            return true;
        }
        if (event == Event::TabReverse) {
            edit_current_field_ = (edit_current_field_ + 2) % 3;
            edit_cursor_position_ = 0;
            return true;
        }
        if (event == Event::ArrowUp) {
            edit_current_field_ = (edit_current_field_ + 2) % 3;
            edit_cursor_position_ = 0;
            return true;
        }
        if (event == Event::ArrowDown) {
            edit_current_field_ = (edit_current_field_ + 1) % 3;
            edit_cursor_position_ = 0;
            return true;
        }
        if (event == Event::ArrowLeft) {
            if (edit_cursor_position_ > 0) {
                edit_cursor_position_--;
            }
            return true;
        }
        if (event == Event::ArrowRight) {
            std::string* current_text = getCurrentEditField();
            if (current_text && edit_cursor_position_ < current_text->length()) {
                edit_cursor_position_++;
            }
            return true;
        }
        if (event == Event::Backspace) {
            std::string* current_text = getCurrentEditField();
            if (current_text && !current_text->empty() && edit_cursor_position_ > 0) {
                current_text->erase(edit_cursor_position_ - 1, 1);
                edit_cursor_position_--;
            }
            return true;
        }
        if (event == Event::Delete) {
            std::string* current_text = getCurrentEditField();
            if (current_text && edit_cursor_position_ < current_text->length()) {
                current_text->erase(edit_cursor_position_, 1);
            }
            return true;
        }
        if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1) {
                char c = ch[0];
                std::string* current_text = getCurrentEditField();
                if (!current_text) {
                    return true;
                }

                if (edit_current_field_ == 0) {
                    if (c >= 32 && c < 127) {
                        if (edit_cursor_position_ <= current_text->length()) {
                            current_text->insert(edit_cursor_position_, 1, c);
                            edit_cursor_position_++;
                        }
                    }
                } else if (edit_current_field_ == 1) {
                    if ((c >= '0' && c <= '9') || c == ':' || c == '-' || c == ' ') {
                        if (edit_cursor_position_ <= current_text->length()) {
                            current_text->insert(edit_cursor_position_, 1, c);
                            edit_cursor_position_++;
                        }
                    }
                } else if (edit_current_field_ == 2) {
                    if (c >= '1' && c <= '9') {
                        *current_text = c;
                        edit_cursor_position_ = 1;
                    }
                }
            }
            return true;
        }
        return false;
    }

    // 正常模式
    auto visible_indices = getVisibleTodoIndicesForSelectedDate();
    if (visible_indices.empty()) {
        if (event == Event::Character(' ')) {
            startCreatingTodo();
            return true;
        }
        if (event == Event::Escape) {
            hide();
            return true;
        }
        if (event == Event::Tab) {
            focus_area_ =
                (focus_area_ == FocusArea::CALENDAR) ? FocusArea::LIST : FocusArea::CALENDAR;
            return true;
        }
        // 允许在空列表时也能操作日历
        if (focus_area_ == FocusArea::CALENDAR) {
            if (event == Event::ArrowLeft) {
                selected_date_ = addDays(selected_date_, -1);
                clampSelectionToVisibleTodos();
                return true;
            }
            if (event == Event::ArrowRight) {
                selected_date_ = addDays(selected_date_, 1);
                clampSelectionToVisibleTodos();
                return true;
            }
            if (event == Event::ArrowUp) {
                selected_date_ = addDays(selected_date_, -7);
                clampSelectionToVisibleTodos();
                return true;
            }
            if (event == Event::ArrowDown) {
                selected_date_ = addDays(selected_date_, 7);
                clampSelectionToVisibleTodos();
                return true;
            }
        }
        return false;
    }

    // Tab: 切换焦点（日历/列表）
    if (event == Event::Tab) {
        focus_area_ = (focus_area_ == FocusArea::CALENDAR) ? FocusArea::LIST : FocusArea::CALENDAR;
        return true;
    }

    // 日历焦点：箭头移动日期（列表焦点时上下用于列表导航）
    if (focus_area_ == FocusArea::CALENDAR) {
        if (event == Event::ArrowLeft) {
            selected_date_ = addDays(selected_date_, -1);
            clampSelectionToVisibleTodos();
            return true;
        }
        if (event == Event::ArrowRight) {
            selected_date_ = addDays(selected_date_, 1);
            clampSelectionToVisibleTodos();
            return true;
        }
        if (event == Event::ArrowUp) {
            selected_date_ = addDays(selected_date_, -7);
            clampSelectionToVisibleTodos();
            return true;
        }
        if (event == Event::ArrowDown) {
            selected_date_ = addDays(selected_date_, 7);
            clampSelectionToVisibleTodos();
            return true;
        }
    } else {
        // 列表导航（基于过滤后的 indices）
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
            if (selected_index_ + 1 < visible_indices.size()) {
                selected_index_++;
                if (selected_index_ >= scroll_offset_ + 10) {
                    scroll_offset_ = selected_index_ - 9;
                }
            }
            return true;
        }
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

    // x: 切换完成状态
    if (event == Event::Character('x') || event == Event::Character('X')) {
        const auto& todos = todo_manager_.getTodos();
        if (selected_index_ < visible_indices.size()) {
            size_t idx = visible_indices[selected_index_];
            if (idx < todos.size()) {
                todo_manager_.markCompleted(todos[idx].id, !todos[idx].completed);
                todo_revision_++;
            }
        }
        return true;
    }

    // e: 编辑 todo
    if (event == Event::Character('e') || event == Event::Character('E')) {
        startEditingTodo();
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
    header.push_back(text("  "));
    header.push_back(text(std::to_string(todos.size())) | bold);
    header.push_back(text(" total  "));
    header.push_back(text(std::to_string(active_count)));
    header.push_back(text(" active"));
    if (due_count > 0) {
        header.push_back(text("  "));
        header.push_back(text(std::to_string(due_count)) | color(colors.error) | bold);
        header.push_back(text(" due") | color(colors.error));
    }

    return hbox(std::move(header)) | bgcolor(colors.dialog_title_bg) |
           color(colors.dialog_title_fg);
}

Element TodoPanel::renderBody() const {
    const auto& colors = theme_.getColors();
    auto left = vbox({
                    renderCalendar(),
                }) |
                size(WIDTH, EQUAL, 26) | borderWithColor(colors.dialog_border);

    auto right = vbox({
                     renderTodoList(),
                 }) |
                 flex_grow | borderWithColor(colors.dialog_border);

    return hbox({
               left,
               separator() | color(colors.dialog_border),
               right,
           }) |
           flex_grow;
}

Element TodoPanel::renderCalendar() const {
    const auto& colors = theme_.getColors();
    auto today = todayDate();

    // 显示当前选中日期所在月份
    Date anchor = selected_date_;
    std::tm tm = {};
    tm.tm_year = anchor.year - 1900;
    tm.tm_mon = anchor.month - 1;
    tm.tm_mday = 1;
    std::mktime(&tm); // normalize

    // 计算当月第一天星期几（0=周日）
    int first_wday = tm.tm_wday;

    // 计算当月天数：下月1号-1天
    std::tm next_tm = tm;
    next_tm.tm_mon += 1;
    next_tm.tm_mday = 1;
    std::time_t next_t = std::mktime(&next_tm);
    next_t -= 24 * 60 * 60;
    std::tm* last_tm = std::localtime(&next_t);
    int days_in_month = last_tm ? last_tm->tm_mday : 30;

    // 月标题
    std::ostringstream title;
    title << std::setw(4) << std::setfill('0') << anchor.year << "-" << std::setw(2)
          << std::setfill('0') << anchor.month;

    Elements rows;
    rows.push_back(hbox({
        text(" " + std::string(icons::CALENDAR) + " ") | color(colors.keyword),
        text(title.str()) | bold | color(colors.dialog_fg),
    }));
    rows.push_back(separator() | color(colors.dialog_border));

    // 周标题
    rows.push_back(hbox({
        text("Su") | dim | color(colors.comment) | size(WIDTH, EQUAL, 3),
        text("Mo") | dim | color(colors.comment) | size(WIDTH, EQUAL, 3),
        text("Tu") | dim | color(colors.comment) | size(WIDTH, EQUAL, 3),
        text("We") | dim | color(colors.comment) | size(WIDTH, EQUAL, 3),
        text("Th") | dim | color(colors.comment) | size(WIDTH, EQUAL, 3),
        text("Fr") | dim | color(colors.comment) | size(WIDTH, EQUAL, 3),
        text("Sa") | dim | color(colors.comment) | size(WIDTH, EQUAL, 3),
    }));

    // 懒加载：哪些天有 todo（以 due_time 的日期判断）
    const auto& day_has_todo = getDayHasTodoForMonth(anchor.year, anchor.month, days_in_month);

    // 日期格
    int day = 1;
    for (int week = 0; week < 6; ++week) {
        Elements cells;
        for (int dow = 0; dow < 7; ++dow) {
            int cell_index = week * 7 + dow;
            if (cell_index < first_wday || day > days_in_month) {
                cells.push_back(text("   "));
                continue;
            }

            bool is_selected = (anchor.day == day);
            bool is_today =
                (today.year == anchor.year && today.month == anchor.month && today.day == day);
            bool has_todo = day_has_todo[day] > 0;

            std::ostringstream ds;
            ds << std::setw(2) << std::setfill(' ') << day;
            std::string s = ds.str();
            if (has_todo) {
                s += "•";
            } else {
                s += " ";
            }

            Element el = text(s) | size(WIDTH, EQUAL, 3);
            if (is_today) {
                el = el | color(colors.success) | bold;
            } else {
                el = el | color(colors.dialog_fg);
            }
            if (is_selected) {
                el = el | bgcolor(colors.selection) | bold;
            } else if (focus_area_ == FocusArea::CALENDAR) {
                // 轻微提示当前焦点区域
                el = el | dim;
            }

            cells.push_back(el);
            day++;
        }
        rows.push_back(hbox(std::move(cells)));
        if (day > days_in_month) {
            break;
        }
    }

    rows.push_back(separator() | color(colors.dialog_border));
    std::ostringstream sel;
    sel << std::setw(4) << std::setfill('0') << selected_date_.year << "-" << std::setw(2)
        << std::setfill('0') << selected_date_.month << "-" << std::setw(2) << std::setfill('0')
        << selected_date_.day;
    rows.push_back(text(" Selected: " + sel.str()) | color(colors.comment) | dim);
    rows.push_back(text(" Tab: focus  Arrows: move") | color(colors.comment) | dim);

    return vbox(std::move(rows)) | bgcolor(colors.background);
}

Element TodoPanel::renderTodoList() const {
    const auto& colors = theme_.getColors();
    const auto& todos = todo_manager_.getTodos();
    Elements list_elements;

    auto visible_indices = getVisibleTodoIndicesForSelectedDate();

    // 右侧标题（日期 + 统计）
    {
        size_t total = visible_indices.size();
        size_t active = 0;
        for (auto idx : visible_indices) {
            if (idx < todos.size() && !todos[idx].completed) {
                active++;
            }
        }
        std::ostringstream date;
        date << std::setw(4) << std::setfill('0') << selected_date_.year << "-" << std::setw(2)
             << std::setfill('0') << selected_date_.month << "-" << std::setw(2)
             << std::setfill('0') << selected_date_.day;

        list_elements.push_back(
            hbox({
                text(" " + std::string(icons::CHECKLIST) + " ") | color(colors.success),
                text(date.str()) | bold | color(colors.dialog_fg),
                filler(),
                text(std::to_string(active) + "/" + std::to_string(total)) | color(colors.comment) |
                    dim,
                text(" active ") | color(colors.comment) | dim,
            }) |
            bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg));
        list_elements.push_back(separator() | color(colors.dialog_border));
    }

    if (visible_indices.empty()) {
        list_elements.push_back(text(""));
        list_elements.push_back(hbox({
            filler(),
            text("No todos for this date") | color(colors.comment) | dim,
            filler(),
        }));
        list_elements.push_back(hbox({
            filler(),
            text("Press Space to create one") | color(colors.comment) | dim,
            filler(),
        }));
        list_elements.push_back(text(""));
    } else {
        list_elements.push_back(text(""));
        size_t max_display = std::min(visible_indices.size(), size_t(10));
        for (size_t i = 0; i < max_display && (scroll_offset_ + i) < visible_indices.size(); ++i) {
            size_t list_i = scroll_offset_ + i;
            size_t todo_i = visible_indices[list_i];
            if (todo_i >= todos.size())
                continue;
            const auto& todo = todos[todo_i];
            bool is_selected = (list_i == selected_index_);
            list_elements.push_back(renderTodoItem(todo, todo_i, is_selected));
        }

        // 滚动提示
        if (scroll_offset_ > 0 || (scroll_offset_ + max_display) < visible_indices.size()) {
            list_elements.push_back(text(""));
            std::string scroll_text = "↑↓ " + std::to_string(visible_indices.size()) + " items";
            list_elements.push_back(
                hbox({text("  "), text(scroll_text) | color(colors.comment) | dim}));
        }
    }

    auto box = vbox(list_elements) | flex_grow;
    if (focus_area_ == FocusArea::LIST) {
        box = box | bgcolor(colors.background);
    } else {
        box = box | dim;
    }
    return box;
}

Element TodoPanel::renderTodoItem(const features::todo::TodoItem& todo, size_t /*index*/,
                                  bool is_selected) const {
    const auto& colors = theme_.getColors();
    const int time_width = 17; // "YYYY-MM-DD HH:MM"

    // 选中标记 + 优先级
    std::string priority_str = "[" + std::to_string(todo.priority) + "] ";
    // 根据优先级设置颜色
    Color priority_color = colors.keyword;
    if (todo.priority <= 2) {
        priority_color = colors.error; // 高优先级（1-2）
    } else if (todo.priority <= 4) {
        priority_color = colors.warning; // 中高优先级（3-4）
    } else if (todo.priority <= 6) {
        priority_color = colors.keyword; // 中等优先级（5-6）
    } else {
        priority_color = colors.comment; // 低优先级（7-9）
    }
    Element left = is_selected
                       ? (hbox({text("► ") | color(colors.success) | bold,
                                text(priority_str) | color(priority_color) | dim}))
                       : (hbox({text("  "), text(priority_str) | color(priority_color) | dim}));

    // 内容（弹性，截断）
    std::string content_display = todo.content;
    const size_t max_content_len = 42;
    if (content_display.length() > max_content_len) {
        content_display = content_display.substr(0, max_content_len - 3) + "...";
    }
    Color content_color = is_selected ? colors.dialog_fg : colors.foreground;
    if (todo.completed) {
        content_color = colors.comment;
        content_display = "✓ " + content_display;
    }
    Element content_el = text(content_display) | color(content_color);
    if (is_selected) {
        content_el = content_el | bold;
    }
    content_el = content_el | flex_grow;

    // 时间（右对齐固定宽度）：到期后 1 分钟内闪烁，超过 1 分钟仅高亮不闪烁
    std::string time_str = formatTime(todo.due_time);
    bool is_due = isTimeDue(todo.due_time);
    bool should_blink = features::todo::TodoManager::isDueWithinBlinkWindow(todo.due_time);
    Color time_color = colors.comment;
    if (is_due) {
        if (should_blink) {
            auto now = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
                          .count();
            const int blink_period_ms = 500;
            double phase =
                (static_cast<double>(ms % blink_period_ms) / static_cast<double>(blink_period_ms)) *
                2.0 * M_PI;
            double intensity = (std::sin(phase) + 1.0) / 2.0;
            time_color = intensity > 0.5 ? colors.error : colors.warning;
        } else {
            time_color = colors.error;
        }
    }
    Element time_el =
        text(time_str) | color(time_color) | (is_due ? bold : dim) | size(WIDTH, EQUAL, time_width);

    Element item_line = hbox({
        left,
        content_el,
        time_el,
    });
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
    } else if (is_editing_todo_) {
        help.push_back(text("  ↑↓: Switch Field  Tab: Next  Enter: Next/Confirm  Esc: Cancel"));
    } else {
        help.push_back(text("  Tab: Focus  Space: Create  ↑↓: Navigate  e: Edit  x: Toggle Done  "
                            "Delete: Remove  Esc: Close"));
    }

    return hbox(help) | bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

Element TodoPanel::renderCreateDialog() const {
    const auto& colors = theme_.getColors();
    const int label_width = 10;

    Elements dialog;
    dialog.push_back(text(""));
    dialog.push_back(text("  New todo") | bold | color(colors.dialog_fg));
    dialog.push_back(text(""));

    // 内容输入
    std::string content_display = new_todo_content_;
    if (current_field_ == 0) {
        if (cursor_position_ <= content_display.length()) {
            content_display.insert(cursor_position_, "|");
        } else {
            content_display += "|";
        }
    }
    Color content_bg = (current_field_ == 0) ? colors.selection : colors.background;
    dialog.push_back(hbox({
        text("  Content  ") | size(WIDTH, EQUAL, label_width + 2),
        text(content_display) | color(colors.dialog_fg) | bgcolor(content_bg) | flex_grow,
    }));
    dialog.push_back(text(""));

    // 时间输入
    std::string time_display = new_todo_time_input_;
    if (current_field_ == 1) {
        if (cursor_position_ <= time_display.length()) {
            time_display.insert(cursor_position_, "|");
        } else {
            time_display += "|";
        }
    }
    Color time_bg = (current_field_ == 1) ? colors.selection : colors.background;
    dialog.push_back(hbox({
        text("  Due       ") | size(WIDTH, EQUAL, label_width + 2),
        text(time_display) | color(colors.dialog_fg) | bgcolor(time_bg) | flex_grow,
    }));
    dialog.push_back(text("  (Format: YYYY-MM-DD HH:MM or HH:MM or 1h/2d)") |
                     color(colors.comment) | dim);
    dialog.push_back(text(""));

    // 优先级输入
    std::string priority_display = std::to_string(new_todo_priority_);
    if (current_field_ == 2) {
        if (cursor_position_ <= priority_display.length()) {
            priority_display.insert(cursor_position_, "|");
        } else {
            priority_display += "|";
        }
    }
    Color priority_bg = (current_field_ == 2) ? colors.selection : colors.background;
    dialog.push_back(hbox({
        text("  Priority  ") | size(WIDTH, EQUAL, label_width + 2),
        text(priority_display) | color(colors.dialog_fg) | bgcolor(priority_bg),
    }));
    dialog.push_back(text(""));

    return vbox(dialog);
}

void TodoPanel::startCreatingTodo() {
    is_creating_todo_ = true;
    current_field_ = 0;
    new_todo_content_.clear();

    // 默认时间：使用选中日期 + 当前时间（格式：YYYY-MM-DD HH:MM）
    auto now = std::chrono::system_clock::now();
    auto tnow = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_now = std::localtime(&tnow);
    int hh = tm_now ? tm_now->tm_hour : 9;
    int mm = tm_now ? tm_now->tm_min : 0;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << selected_date_.year << "-" << std::setw(2)
        << selected_date_.month << "-" << std::setw(2) << selected_date_.day << " " << std::setw(2)
        << hh << ":" << std::setw(2) << mm;
    new_todo_time_input_ = oss.str();

    new_todo_priority_ = 5;
    cursor_position_ = 0;
}

std::string* TodoPanel::getCurrentField() {
    if (current_field_ == 0) {
        return &new_todo_content_;
    } else if (current_field_ == 1) {
        return &new_todo_time_input_;
    } else {
        // 兼容旧逻辑：优先级字段不再用字符串编辑，仅在创建时接收 1-9 单字符
        static std::string tmp;
        tmp = std::to_string(new_todo_priority_);
        return &tmp;
    }
}

void TodoPanel::finishCreatingTodo() {
    if (new_todo_content_.empty() || new_todo_time_input_.empty()) {
        // 输入不完整，取消创建
        cancelCreatingTodo();
        return;
    }

    try {
        auto due_time = applySelectedDateToTimeInput(new_todo_time_input_);
        todo_manager_.addTodo(new_todo_content_, due_time, new_todo_priority_);
        todo_manager_.sortByPriority();
        todo_revision_++;
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
    new_todo_priority_ = 5;
    cursor_position_ = 0;
}

void TodoPanel::deleteSelectedTodo() {
    auto visible_indices = getVisibleTodoIndicesForSelectedDate();
    const auto& todos = todo_manager_.getTodos();
    if (selected_index_ < visible_indices.size()) {
        size_t idx = visible_indices[selected_index_];
        if (idx < todos.size()) {
            todo_manager_.removeTodo(todos[idx].id);
            todo_revision_++;
        }
        clampSelectionToVisibleTodos();
    }
}

// 时间工具函数
namespace time_utils {
// 解析时间输入字符串
std::chrono::system_clock::time_point parseTimeInput(const std::string& time_str) {
    // 解析时间格式：
    // 1. 完整格式：YYYY-MM-DD HH:MM 或 YYYY-MM-DD HH:MM:SS
    // 2. 简化格式：HH:MM（使用当前日期）
    // 3. 相对时间：1h（1小时后）, 2d（2天后）, 30m（30分钟后）

    // 尝试解析相对时间
    if (time_str.length() > 1) {
        char unit = time_str.back();
        std::string num_str = time_str.substr(0, time_str.length() - 1);

        try {
            int value = std::stoi(num_str);
            auto now = std::chrono::system_clock::now();

            switch (unit) {
                case 'm': // 分钟
                    return now + std::chrono::minutes(value);
                case 'h': // 小时
                    return now + std::chrono::hours(value);
                case 'd': // 天
                    return now + std::chrono::hours(value * 24);
                case 'w': // 周
                    return now + std::chrono::hours(value * 24 * 7);
            }
        } catch (...) {
            // 解析失败，继续尝试其他格式
        }
    }

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

// 格式化时间点为字符串
std::string formatTime(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm* tm = std::localtime(&time_t);

    std::ostringstream oss;
    // 格式：YYYY-MM-DD HH:MM
    oss << std::setfill('0') << std::setw(4) << (tm->tm_year + 1900) << "-" << std::setw(2)
        << (tm->tm_mon + 1) << "-" << std::setw(2) << tm->tm_mday << " " << std::setw(2)
        << tm->tm_hour << ":" << std::setw(2) << tm->tm_min;

    return oss.str();
}
} // namespace time_utils

std::chrono::system_clock::time_point TodoPanel::parseTimeInput(const std::string& time_str) const {
    return time_utils::parseTimeInput(time_str);
}

std::string TodoPanel::formatTime(const std::chrono::system_clock::time_point& time) const {
    return time_utils::formatTime(time);
}

bool TodoPanel::isTimeDue(const std::chrono::system_clock::time_point& time) const {
    auto now = std::chrono::system_clock::now();
    return time <= now;
}

TodoPanel::Date TodoPanel::todayDate() const {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&t);
    Date d;
    if (tm) {
        d.year = tm->tm_year + 1900;
        d.month = tm->tm_mon + 1;
        d.day = tm->tm_mday;
    }
    return d;
}

TodoPanel::Date TodoPanel::addDays(Date d, int delta_days) const {
    std::tm tm = {};
    tm.tm_year = d.year - 1900;
    tm.tm_mon = d.month - 1;
    tm.tm_mday = d.day + delta_days;
    std::mktime(&tm);
    Date out;
    out.year = tm.tm_year + 1900;
    out.month = tm.tm_mon + 1;
    out.day = tm.tm_mday;
    return out;
}

bool TodoPanel::sameDate(const Date& a, const Date& b) const {
    return a.year == b.year && a.month == b.month && a.day == b.day;
}

std::chrono::system_clock::time_point TodoPanel::makeTimePointForDateAndTime(
    const Date& d, const std::chrono::system_clock::time_point& time_of_day) const {
    auto tt = std::chrono::system_clock::to_time_t(time_of_day);
    std::tm* tm_time = std::localtime(&tt);
    int hh = tm_time ? tm_time->tm_hour : 0;
    int mm = tm_time ? tm_time->tm_min : 0;
    int ss = tm_time ? tm_time->tm_sec : 0;

    std::tm tm = {};
    tm.tm_year = d.year - 1900;
    tm.tm_mon = d.month - 1;
    tm.tm_mday = d.day;
    tm.tm_hour = hh;
    tm.tm_min = mm;
    tm.tm_sec = ss;
    auto t = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(t);
}

std::chrono::system_clock::time_point TodoPanel::applySelectedDateToTimeInput(
    const std::string& time_input) const {
    // 如果用户输入的是完整日期（YYYY-MM-DD ...），parseTimeInput 会直接解析并返回；
    // 如果是 HH:MM 或相对时间等，则我们把 date 强制为 selected_date_。
    auto tp = parseTimeInput(time_input);

    // 简单判断：是否包含完整日期前缀 "YYYY-"
    bool has_date = time_input.size() >= 5 && time_input[4] == '-' &&
                    std::count(time_input.begin(), time_input.end(), '-') >= 2;
    if (has_date) {
        return tp;
    }
    return makeTimePointForDateAndTime(selected_date_, tp);
}

std::vector<size_t> TodoPanel::getVisibleTodoIndicesForSelectedDate() const {
    const auto& todos = todo_manager_.getTodos();
    std::vector<size_t> indices;
    indices.reserve(todos.size());
    for (size_t i = 0; i < todos.size(); ++i) {
        auto tt = std::chrono::system_clock::to_time_t(todos[i].due_time);
        std::tm* tm = std::localtime(&tt);
        if (!tm)
            continue;
        Date d{tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday};
        if (sameDate(d, selected_date_)) {
            indices.push_back(i);
        }
    }
    return indices;
}

void TodoPanel::clampSelectionToVisibleTodos() {
    auto visible = getVisibleTodoIndicesForSelectedDate();
    if (visible.empty()) {
        selected_index_ = 0;
        scroll_offset_ = 0;
        return;
    }
    if (selected_index_ >= visible.size()) {
        selected_index_ = visible.size() - 1;
    }
    if (scroll_offset_ > selected_index_) {
        scroll_offset_ = selected_index_;
    }
    if (selected_index_ >= scroll_offset_ + 10) {
        scroll_offset_ = selected_index_ - 9;
    }
}

void TodoPanel::startEditingTodo() {
    auto visible_indices = getVisibleTodoIndicesForSelectedDate();
    const auto& todos = todo_manager_.getTodos();
    if (selected_index_ >= visible_indices.size()) {
        return;
    }
    size_t idx = visible_indices[selected_index_];
    if (idx >= todos.size()) {
        return;
    }
    const auto& todo = todos[idx];
    is_editing_todo_ = true;
    edit_todo_id_ = todo.id;
    edit_todo_content_ = todo.content;
    edit_todo_time_input_ = formatTime(todo.due_time);
    edit_todo_priority_input_ = std::to_string(todo.priority);
    edit_current_field_ = 0;
    edit_cursor_position_ = 0;
}

void TodoPanel::finishEditingTodo() {
    if (edit_todo_id_.empty() || edit_todo_content_.empty() || edit_todo_time_input_.empty() ||
        edit_todo_priority_input_.empty()) {
        cancelEditingTodo();
        return;
    }
    try {
        int p = std::stoi(edit_todo_priority_input_);
        if (p < 1)
            p = 1;
        if (p > 9)
            p = 9;
        auto due = applySelectedDateToTimeInput(edit_todo_time_input_);
        todo_manager_.updateTodo(edit_todo_id_, edit_todo_content_, due, p);
        todo_manager_.sortByPriority();
        todo_revision_++;
    } catch (...) {
        // ignore
    }
    cancelEditingTodo();
    clampSelectionToVisibleTodos();
}

void TodoPanel::cancelEditingTodo() {
    is_editing_todo_ = false;
    edit_todo_id_.clear();
    edit_todo_content_.clear();
    edit_todo_time_input_.clear();
    edit_todo_priority_input_.clear();
    edit_current_field_ = 0;
    edit_cursor_position_ = 0;
}

std::string* TodoPanel::getCurrentEditField() {
    if (edit_current_field_ == 0) {
        return &edit_todo_content_;
    } else if (edit_current_field_ == 1) {
        return &edit_todo_time_input_;
    } else {
        return &edit_todo_priority_input_;
    }
}

Element TodoPanel::renderEditDialog() const {
    const auto& colors = theme_.getColors();
    const int label_width = 10;

    Elements dialog;
    dialog.push_back(text(""));
    dialog.push_back(text("  Edit todo") | bold | color(colors.dialog_fg));
    dialog.push_back(text(""));

    // 内容
    std::string content_display = edit_todo_content_;
    if (edit_current_field_ == 0) {
        if (edit_cursor_position_ <= content_display.length()) {
            content_display.insert(edit_cursor_position_, "|");
        } else {
            content_display += "|";
        }
    }
    Color content_bg = (edit_current_field_ == 0) ? colors.selection : colors.background;
    dialog.push_back(hbox({
        text("  Content  ") | size(WIDTH, EQUAL, label_width + 2),
        text(content_display) | color(colors.dialog_fg) | bgcolor(content_bg) | flex_grow,
    }));
    dialog.push_back(text(""));

    // 时间
    std::string time_display = edit_todo_time_input_;
    if (edit_current_field_ == 1) {
        if (edit_cursor_position_ <= time_display.length()) {
            time_display.insert(edit_cursor_position_, "|");
        } else {
            time_display += "|";
        }
    }
    Color time_bg = (edit_current_field_ == 1) ? colors.selection : colors.background;
    dialog.push_back(hbox({
        text("  Due       ") | size(WIDTH, EQUAL, label_width + 2),
        text(time_display) | color(colors.dialog_fg) | bgcolor(time_bg) | flex_grow,
    }));
    dialog.push_back(text("  (YYYY-MM-DD HH:MM or HH:MM or 1h/2d)") | color(colors.comment) | dim);
    dialog.push_back(text(""));

    // 优先级
    std::string pr_display = edit_todo_priority_input_.empty() ? "5" : edit_todo_priority_input_;
    if (edit_current_field_ == 2) {
        if (edit_cursor_position_ <= pr_display.length()) {
            pr_display.insert(edit_cursor_position_, "|");
        } else {
            pr_display += "|";
        }
    }
    Color pr_bg = (edit_current_field_ == 2) ? colors.selection : colors.background;
    dialog.push_back(hbox({
        text("  Priority  ") | size(WIDTH, EQUAL, label_width + 2),
        text(pr_display) | color(colors.dialog_fg) | bgcolor(pr_bg),
    }));
    dialog.push_back(text(""));
    return vbox(dialog);
}

const std::vector<int>& TodoPanel::getDayHasTodoForMonth(int year, int month,
                                                         int days_in_month) const {
    if (cached_todo_revision_ == todo_revision_ && cached_calendar_year_ == year &&
        cached_calendar_month_ == month &&
        static_cast<int>(cached_day_has_todo_.size()) == (days_in_month + 1)) {
        return cached_day_has_todo_;
    }

    cached_todo_revision_ = todo_revision_;
    cached_calendar_year_ = year;
    cached_calendar_month_ = month;
    cached_day_has_todo_.assign(days_in_month + 1, 0);

    for (const auto& todo : todo_manager_.getTodos()) {
        auto t = std::chrono::system_clock::to_time_t(todo.due_time);
        std::tm* lt = std::localtime(&t);
        if (!lt)
            continue;
        int y = lt->tm_year + 1900;
        int m = lt->tm_mon + 1;
        int d = lt->tm_mday;
        if (y == year && m == month && d >= 1 && d <= days_in_month) {
            cached_day_has_todo_[d] += 1;
        }
    }

    return cached_day_has_todo_;
}

} // namespace ui
} // namespace pnana
