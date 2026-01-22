#include "ui/search_dialog.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

SearchDialog::SearchDialog(Theme& theme)
    : theme_(theme), visible_(false), current_field_(0), cursor_position_(0), current_match_(0),
      total_matches_(0) {}

void SearchDialog::show(
    std::function<void(const std::string&, const features::SearchOptions&)> on_search,
    std::function<void(const std::string&)> on_replace,
    std::function<void(const std::string&)> on_replace_all, std::function<void()> on_cancel) {
    visible_ = true;
    current_field_ = 0;
    cursor_position_ = 0;
    on_search_ = on_search;
    on_replace_ = on_replace;
    on_replace_all_ = on_replace_all;
    on_cancel_ = on_cancel;

    // 重置输入但保持选项
    search_input_.clear();
    replace_input_.clear();
    current_match_ = 0;
    total_matches_ = 0;
}

void SearchDialog::hide() {
    visible_ = false;
}

bool SearchDialog::handleInput(Event event) {
    if (!visible_) {
        return false;
    }

    if (event == Event::Escape) {
        visible_ = false;
        if (on_cancel_) {
            on_cancel_();
        }
        return true;
    }

    if (event == Event::Return) {
        if (current_field_ == 0) {
            // 在搜索输入框中，按回车执行搜索
            performSearch();
        } else if (current_field_ == 1) {
            // 在替换输入框中，按回车执行搜索
            performSearch();
        } else if (current_field_ >= 2 && current_field_ <= 5) {
            // 在选项中，按回车切换选项
            toggleOption(current_field_ - 2);
        } else if (current_field_ == 6) {
            // 在Replace按钮区域，按回车执行替换
            performReplace();
        } else if (current_field_ == 7) {
            // 在Replace All按钮区域，按回车执行全部替换
            performReplaceAll();
        }
        return true;
    }

    if (event == Event::Tab) {
        current_field_ = (current_field_ + 1) %
                         8; // 0:搜索输入, 1:替换输入, 2-5:选项, 6:Replace按钮, 7:Replace All按钮
        if (current_field_ == 0) {
            cursor_position_ = search_input_.length();
        } else if (current_field_ == 1) {
            cursor_position_ = replace_input_.length();
        }
        return true;
    }

    if (event == Event::TabReverse) {
        current_field_ = (current_field_ + 7) %
                         8; // 0:搜索输入, 1:替换输入, 2-5:选项, 6:Replace按钮, 7:Replace All按钮
        if (current_field_ == 0) {
            cursor_position_ = search_input_.length();
        } else if (current_field_ == 1) {
            cursor_position_ = replace_input_.length();
        }
        return true;
    }

    if (event == Event::ArrowUp) {
        if (current_field_ > 0) {
            current_field_--;
            if (current_field_ == 0) {
                cursor_position_ = search_input_.length();
            } else if (current_field_ == 1) {
                cursor_position_ = replace_input_.length();
            }
        }
        return true;
    }

    if (event == Event::ArrowDown) {
        if (current_field_ < 7) {
            current_field_++;
            if (current_field_ == 0) {
                cursor_position_ = search_input_.length();
            } else if (current_field_ == 1) {
                cursor_position_ = replace_input_.length();
            }
        }
        return true;
    }

    if (event == Event::ArrowLeft) {
        if (current_field_ == 0) {
            moveCursorLeft();
        } else if (current_field_ == 1) {
            if (cursor_position_ > 0) {
                cursor_position_--;
            }
        }
        return true;
    }

    if (event == Event::ArrowRight) {
        if (current_field_ == 0) {
            moveCursorRight();
        } else if (current_field_ == 1) {
            if (cursor_position_ < replace_input_.length()) {
                cursor_position_++;
            }
        }
        return true;
    }

    if (event == Event::Backspace) {
        if (current_field_ == 0) {
            backspace();
        } else if (current_field_ == 1) {
            if (cursor_position_ > 0) {
                replace_input_.erase(cursor_position_ - 1, 1);
                cursor_position_--;
            }
        }
        return true;
    }

    if (event == Event::Delete) {
        if (current_field_ == 0) {
            deleteChar();
        } else if (current_field_ == 1) {
            if (cursor_position_ < replace_input_.length()) {
                replace_input_.erase(cursor_position_, 1);
            }
        }
        return true;
    }

    if (event == Event::Home) {
        if (current_field_ == 0 || current_field_ == 1) {
            cursor_position_ = 0;
        }
        return true;
    }

    if (event == Event::End) {
        if (current_field_ == 0) {
            cursor_position_ = search_input_.length();
        } else if (current_field_ == 1) {
            cursor_position_ = replace_input_.length();
        }
        return true;
    }

    if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            if (c >= 32 && c < 127) {
                if (current_field_ == 0) {
                    insertChar(c);
                } else if (current_field_ == 1) {
                    if (cursor_position_ <= replace_input_.length()) {
                        replace_input_.insert(cursor_position_, 1, c);
                        cursor_position_++;
                    }
                }
            }
        }
        return true;
    }

    // 空格键用于切换选项或执行按钮
    if (event == Event::Character(" ")) {
        if (current_field_ >= 2 && current_field_ <= 5) {
            toggleOption(current_field_ - 2);
            return true;
        } else if (current_field_ == 6) {
            // 在按钮区域，按空格执行替换
            performReplace();
            return true;
        } else if (current_field_ == 7) {
            // 在Replace All按钮区域，按空格执行全部替换
            performReplaceAll();
            return true;
        }
    }

    return false;
}

Element SearchDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements content;

    // 标题
    Elements title_elements;
    title_elements.push_back(text(icons::SEARCH) | color(Color::Blue));
    title_elements.push_back(text(" Search ") | color(colors.foreground) | bold);
    content.push_back(hbox(title_elements) | center);

    content.push_back(separator());

    // 搜索输入框
    content.push_back(renderSearchInput());

    // 替换输入框
    content.push_back(renderReplaceInput());

    // 搜索选项
    content.push_back(renderOptions());

    // 结果统计
    if (total_matches_ > 0) {
        content.push_back(renderResults());
    }

    // 按钮
    content.push_back(separator());
    content.push_back(renderButtons());

    Element dialog_content = vbox(content);

    return window(text("Search"), dialog_content) | size(WIDTH, GREATER_THAN, 60) |
           size(HEIGHT, GREATER_THAN, 15) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

void SearchDialog::updateResults(size_t current_match, size_t total_matches) {
    current_match_ = current_match;
    total_matches_ = total_matches;
}

void SearchDialog::setSearchOptions(const features::SearchOptions& options) {
    search_options_ = options;
}

void SearchDialog::insertChar(char ch) {
    if (cursor_position_ <= search_input_.length()) {
        search_input_.insert(cursor_position_, 1, ch);
        cursor_position_++;
    }
}

void SearchDialog::deleteChar() {
    if (cursor_position_ < search_input_.length()) {
        search_input_.erase(cursor_position_, 1);
    }
}

void SearchDialog::backspace() {
    if (cursor_position_ > 0) {
        search_input_.erase(cursor_position_ - 1, 1);
        cursor_position_--;
    }
}

void SearchDialog::moveCursorLeft() {
    if (cursor_position_ > 0) {
        cursor_position_--;
    }
}

void SearchDialog::moveCursorRight() {
    if (cursor_position_ < search_input_.length()) {
        cursor_position_++;
    }
}

std::string* SearchDialog::getCurrentField() {
    if (current_field_ == 0) {
        return &search_input_;
    } else if (current_field_ == 1) {
        return &replace_input_;
    }
    return nullptr;
}

void SearchDialog::toggleOption(int option_index) {
    switch (option_index) {
        case 0:
            search_options_.case_sensitive = !search_options_.case_sensitive;
            break;
        case 1:
            search_options_.whole_word = !search_options_.whole_word;
            break;
        case 2:
            search_options_.regex = !search_options_.regex;
            break;
        case 3:
            search_options_.wrap_around = !search_options_.wrap_around;
            break;
    }
}

void SearchDialog::performSearch() {
    if (on_search_) {
        visible_ = false;
        on_search_(search_input_, search_options_);
    }
}

void SearchDialog::performReplace() {
    if (total_matches_ > 0 && on_replace_) {
        on_replace_(replace_input_);
    }
}

void SearchDialog::performReplaceAll() {
    if (total_matches_ > 0 && on_replace_all_) {
        on_replace_all_(replace_input_);
    }
}

Element SearchDialog::renderSearchInput() {
    auto& colors = theme_.getColors();
    bool is_focused = (current_field_ == 0);

    std::string display_value = search_input_;
    if (display_value.empty()) {
        display_value = "(type to search...)";
    }

    Elements field_elements;
    field_elements.push_back(text("Search: ") | color(colors.comment) | size(WIDTH, EQUAL, 12));

    if (is_focused && !search_input_.empty()) {
        // 确保光标位置不超出字符串长度
        size_t cursor_pos = cursor_position_;
        if (cursor_pos > display_value.length()) {
            cursor_pos = display_value.length();
        }

        std::string before = display_value.substr(0, cursor_pos);
        std::string cursor_char =
            cursor_pos < display_value.length() ? display_value.substr(cursor_pos, 1) : " ";
        std::string after =
            cursor_pos < display_value.length() ? display_value.substr(cursor_pos + 1) : "";

        field_elements.push_back(
            hbox({text(before) | color(colors.foreground),
                  text(cursor_char) | bgcolor(colors.foreground) | color(colors.background) | bold,
                  text(after) | color(colors.foreground)}) |
            bgcolor(colors.current_line));
    } else {
        Color text_color =
            display_value == "(type to search...)" ? colors.comment : colors.foreground;
        field_elements.push_back(text(display_value) | color(text_color) | dim);
    }

    return hbox(field_elements);
}

Element SearchDialog::renderOptions() {
    auto& colors = theme_.getColors();

    Elements options;

    // 选项列表
    std::vector<std::pair<std::string, bool>> option_list = {
        {"Case sensitive", search_options_.case_sensitive},
        {"Whole word", search_options_.whole_word},
        {"Regex", search_options_.regex},
        {"Wrap around", search_options_.wrap_around}};

    for (size_t i = 0; i < option_list.size(); ++i) {
        const auto& [label, enabled] = option_list[i];
        bool is_selected = (current_field_ == static_cast<int>(i + 1));

        Elements option_elements;
        option_elements.push_back(text("  "));

        // 选中指示器
        if (is_selected) {
            option_elements.push_back(text("► ") | color(colors.function));
        } else {
            option_elements.push_back(text("  "));
        }

        // 复选框
        std::string checkbox = enabled ? "[✓]" : "[ ]";
        option_elements.push_back(text(checkbox) |
                                  color(enabled ? colors.success : colors.comment));

        // 标签
        option_elements.push_back(text(" " + label) |
                                  color(is_selected ? colors.foreground : colors.comment));

        options.push_back(hbox(option_elements));
    }

    return vbox(options);
}

Element SearchDialog::renderReplaceInput() {
    auto& colors = theme_.getColors();
    bool is_focused = (current_field_ == 1);

    std::string display_value = replace_input_;
    if (display_value.empty()) {
        display_value = "(replace with...)";
    }

    Elements field_elements;
    field_elements.push_back(text("Replace: ") | color(colors.comment) | size(WIDTH, EQUAL, 12));

    if (is_focused && !replace_input_.empty()) {
        size_t cursor_pos = cursor_position_;
        if (cursor_pos > display_value.length()) {
            cursor_pos = display_value.length();
        }

        std::string before = display_value.substr(0, cursor_pos);
        std::string cursor_char =
            cursor_pos < display_value.length() ? display_value.substr(cursor_pos, 1) : " ";
        std::string after =
            cursor_pos < display_value.length() ? display_value.substr(cursor_pos + 1) : "";

        field_elements.push_back(
            hbox({text(before) | color(colors.foreground),
                  text(cursor_char) | bgcolor(colors.foreground) | color(colors.background) | bold,
                  text(after) | color(colors.foreground)}) |
            bgcolor(colors.current_line));
    } else {
        Color text_color =
            display_value == "(replace with...)" ? colors.comment : colors.foreground;
        field_elements.push_back(text(display_value) | color(text_color) | dim);
    }

    return hbox(field_elements);
}

Element SearchDialog::renderResults() {
    auto& colors = theme_.getColors();

    std::string result_text;
    if (total_matches_ == 0) {
        result_text = "No matches found";
    } else if (total_matches_ == 1) {
        result_text = "1 match";
    } else {
        result_text = std::to_string(current_match_ + 1) + " of " + std::to_string(total_matches_) +
                      " matches";
    }

    return hbox({text("  "), text(result_text) | color(colors.info) | bold});
}

Element SearchDialog::renderButtons() {
    auto& colors = theme_.getColors();

    Elements buttons;

    // 操作按钮
    Elements actions;
    actions.push_back(text("  "));

    // 替换按钮
    std::string replace_text = "[Replace]";
    Color replace_color = colors.warning;
    if (current_field_ == 6 && total_matches_ > 0) {
        replace_color = colors.function;
        replace_text = "[" + replace_text + "]";
    } else if (total_matches_ == 0) {
        replace_color = colors.comment;
    }
    actions.push_back(text(replace_text) | color(replace_color) | bold);
    actions.push_back(text("  "));

    // 替换全部按钮
    std::string replace_all_text = "[Replace All]";
    Color replace_all_color = colors.error;
    if (current_field_ == 7 && total_matches_ > 0) {
        replace_all_color = colors.function;
        replace_all_text = "[" + replace_all_text + "]";
    } else if (total_matches_ == 0) {
        replace_all_color = colors.comment;
    }
    actions.push_back(text(replace_all_text) | color(replace_all_color) | bold);

    buttons.push_back(hbox(actions) | center);

    // 提示信息
    Elements hints;
    hints.push_back(text("↑↓: Navigate  "));
    hints.push_back(text("Tab: Next field  "));
    hints.push_back(text("Enter: Search  "));
    hints.push_back(text("Space: Toggle option  "));
    hints.push_back(text("Esc: Cancel"));
    buttons.push_back(hbox(hints) | color(colors.comment) | center);

    return vbox(buttons);
}

} // namespace ui
} // namespace pnana
