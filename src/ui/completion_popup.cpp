#include "ui/completion_popup.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace ui {

CompletionPopup::CompletionPopup()
    : visible_(false), selected_index_(0), max_items_(15), // 增加最大显示项数
      cursor_row_(0), cursor_col_(0), screen_width_(80), screen_height_(24), popup_x_(0),
      popup_y_(0), popup_width_(70), popup_height_(17), // 增加默认宽度和高度
      last_items_size_(0) {}

void CompletionPopup::show(const std::vector<features::CompletionItem>& items, int cursor_row,
                           int cursor_col, int screen_width, int screen_height,
                           const std::string& query) {
    auto show_start = std::chrono::steady_clock::now();
    LOG("[COMPLETION] [Popup] show() called with " + std::to_string(items.size()) +
        " items, query='" + query + "'");

    // 参考 VSCode：优化响应速度，减少不必要的更新
    bool was_visible = visible_;

    // 快速比较：检查内容是否真正变化
    bool items_changed = (items_.size() != items.size()) || (current_query_ != query);
    if (!items_changed && items_.size() > 0 && items.size() > 0) {
        // 比较前几个 item 的 label，如果相同则认为内容未变化
        bool content_same = true;
        size_t compare_count = std::min({items_.size(), items.size(), size_t(5)}); // 比较前 5 个
        for (size_t i = 0; i < compare_count; ++i) {
            if (items_[i].label != items[i].label) {
                content_same = false;
                break;
            }
        }
        items_changed = !content_same;
    }

    // 检查屏幕尺寸是否变化（需要重新计算位置）
    bool screen_changed = (screen_width_ != screen_width || screen_height_ != screen_height);

    // 检查光标位置是否变化（需要重新计算位置）
    bool cursor_changed = (cursor_row_ != cursor_row || cursor_col_ != cursor_col);

    LOG("[COMPLETION] [Popup] Changes: items=" + std::to_string(items_changed) + ", screen=" +
        std::to_string(screen_changed) + ", cursor=" + std::to_string(cursor_changed) +
        ", was_visible=" + std::to_string(was_visible));

    items_ = items;
    current_query_ = query;
    cursor_row_ = cursor_row;
    cursor_col_ = cursor_col;
    screen_width_ = screen_width;
    screen_height_ = screen_height;
    selected_index_ = 0;
    visible_ = !items_.empty();

    // 只在内容变化、屏幕尺寸变化、或光标位置变化时重新计算位置
    // 这样可以减少不必要的计算，提高响应速度
    if (visible_ && (items_changed || screen_changed || cursor_changed || !was_visible)) {
        auto calc_start = std::chrono::steady_clock::now();
        calculatePopupPosition();
        auto calc_end = std::chrono::steady_clock::now();
        auto calc_time =
            std::chrono::duration_cast<std::chrono::microseconds>(calc_end - calc_start);
        LOG("[COMPLETION] [Popup] Calculated position (took " +
            std::to_string(calc_time.count() / 1000.0) + " ms)");
    }

    auto show_end = std::chrono::steady_clock::now();
    auto show_time = std::chrono::duration_cast<std::chrono::microseconds>(show_end - show_start);
    LOG("[COMPLETION] [Popup] show() completed (took " +
        std::to_string(show_time.count() / 1000.0) + " ms, visible=" + std::to_string(visible_) +
        ")");
}

void CompletionPopup::updateCursorPosition(int row, int col, int screen_width, int screen_height) {
    // 增强的阈值机制，进一步减少不必要的弹窗位置更新
    int row_diff = std::abs(row - cursor_row_);
    int col_diff = std::abs(col - cursor_col_);
    bool screen_changed = (screen_width_ != screen_width || screen_height_ != screen_height);

    // 增大阈值以大幅减少抖动：
    // 行变化 >= 3，列变化 >= 8
    // 这样可以确保只有在光标发生明显移动时才更新弹窗位置
    bool needs_update = screen_changed || row_diff >= 3 || col_diff >= 8;

    cursor_row_ = row;
    cursor_col_ = col;
    screen_width_ = screen_width;
    screen_height_ = screen_height;

    if (visible_ && needs_update) {
        calculatePopupPosition();
    }
}

void CompletionPopup::hide() {
    visible_ = false;
    items_.clear();
    selected_index_ = 0;
}

void CompletionPopup::selectNext() {
    if (items_.empty())
        return;
    selected_index_ = (selected_index_ + 1) % items_.size();
    // 确保选中的项在可见范围内（自动滚动）
    // getDisplayStart() 和 getDisplayEnd() 会在render时自动处理滚动
}

void CompletionPopup::selectPrevious() {
    if (items_.empty())
        return;
    if (selected_index_ == 0) {
        selected_index_ = items_.size() - 1;
    } else {
        selected_index_--;
    }
    // 确保选中的项在可见范围内（自动滚动）
    // getDisplayStart() 和 getDisplayEnd() 会在render时自动处理滚动
}

const features::CompletionItem* CompletionPopup::getSelectedItem() const {
    if (!visible_ || items_.empty() || selected_index_ >= items_.size()) {
        return nullptr;
    }
    return &items_[selected_index_];
}

void CompletionPopup::calculatePopupPosition() {
    // ========== 参考 Neovim 的实现策略 ==========
    // Neovim 使用固定尺寸的浮动窗口，避免频繁变化导致的抖动
    // 策略：
    // 1. 尺寸一旦确定就固定不变（除非内容发生重大变化）
    // 2. 位置使用"锚点"机制，只在光标移动超过阈值时更新
    // 3. 使用平滑的位置更新，避免突然跳跃

    // ========== 尺寸计算：固定策略 ==========
    // 只在首次显示或 items 数量发生重大变化时更新尺寸
    bool size_changed = false;

    if (popup_width_ == 0) {
        // 首次计算：使用固定宽度策略
        // Neovim 通常使用屏幕宽度的 40-60%，我们使用 50%
        popup_width_ = std::min(80, (screen_width_ * 50) / 100);
        if (popup_width_ < 50)
            popup_width_ = 50; // 最小宽度
        size_changed = true;
    } else if (items_.size() != last_items_size_) {
        // Items 数量变化：只在变化超过 50% 时才重新计算宽度
        size_t size_diff = (items_.size() > last_items_size_) ? (items_.size() - last_items_size_)
                                                              : (last_items_size_ - items_.size());
        if (last_items_size_ > 0 && size_diff * 100 / last_items_size_ > 50) {
            // 重新计算宽度（但保持稳定）
            int max_width = 0;
            for (const auto& item : items_) {
                size_t item_width = item.label.length();
                if (!item.detail.empty()) {
                    item_width += item.detail.length() + 3;
                }
                if (item_width > static_cast<size_t>(max_width)) {
                    max_width = static_cast<int>(item_width) + 15;
                }
            }
            int new_width = std::min(max_width, screen_width_ - 4);
            // 只在宽度变化超过 10 个字符时才更新
            if (std::abs(new_width - popup_width_) > 10) {
                popup_width_ = new_width;
                size_changed = true;
            }
        }
        last_items_size_ = items_.size();
    }

    // 高度：使用固定策略，不频繁变化
    size_t display_count = std::min(items_.size(), max_items_);
    int new_height = static_cast<int>(display_count);
    if (popup_height_ == 0) {
        popup_height_ = new_height;
        size_changed = true;
    } else if (std::abs(new_height - popup_height_) > 5) {
        // 只在高度变化超过 5 行时才更新
        popup_height_ = new_height;
        size_changed = true;
    }

    // ========== 位置计算：强制下方显示策略 ==========
    // 优先显示在光标下方，提高代码提示的可见性
    // 即使空间不足，也要尽量显示在下方

    // 计算目标位置（相对于光标）
    int target_x = cursor_col_;
    if (target_x + popup_width_ > screen_width_ - 2) {
        target_x = screen_width_ - popup_width_ - 2;
        if (target_x < 0) {
            target_x = 0;
        }
    }

    // 计算目标 Y 位置：强制优先显示在下方
    int cursor_screen_y = cursor_row_;
    int target_y;

    // 优先尝试显示在光标下方
    if (cursor_screen_y + popup_height_ + 2 < screen_height_ - 4) {
        // 下方有足够空间，显示在下方
        target_y = cursor_screen_y + 1;
    } else if (cursor_screen_y >= popup_height_ + 2) {
        // 下方空间不足，但上方有空间，显示在上方（作为最后手段）
        target_y = cursor_screen_y - popup_height_ - 1;
    } else {
        // 上下都没有足够空间，优先显示在下方，即使会超出屏幕边界
        // 这样可以保证提示内容的最大可见性
        target_y = cursor_screen_y + 1;
        // 如果完全超出屏幕，稍微向上调整以显示更多内容
        if (target_y + popup_height_ > screen_height_ - 2) {
            target_y = screen_height_ - popup_height_ - 2;
            if (target_y < 0)
                target_y = 0;
        }
    }

    // ========== 位置更新策略：平滑更新 ==========
    // 策略1：如果尺寸变化，立即更新位置
    if (size_changed) {
        popup_x_ = target_x;
        popup_y_ = target_y;
        return;
    }

    // 策略2：如果位置未初始化，立即更新
    if (popup_x_ == 0 && popup_y_ == 0) {
        popup_x_ = target_x;
        popup_y_ = target_y;
        return;
    }

    // 策略3：增强的阈值机制，大幅减少抖动
    // 使用更大的阈值来确保弹窗位置的稳定性
    int x_diff = std::abs(target_x - popup_x_);
    int y_diff = std::abs(target_y - popup_y_);

    // X 方向：只在变化超过 8 个字符时更新（进一步增大阈值）
    if (x_diff > 8) {
        popup_x_ = target_x;
    }

    // Y 方向：只在变化超过 5 行时更新（进一步增大阈值）
    if (y_diff > 5) {
        popup_y_ = target_y;
    }

    // 策略4：智能边界检查，避免弹窗超出屏幕但保持稳定性
    bool needs_boundary_adjustment = false;

    // 检查水平边界
    if (popup_x_ + popup_width_ > screen_width_ - 2) {
        // 只有当当前位置明显超出边界时才调整
        if (popup_x_ + popup_width_ > screen_width_ + 5) {
            popup_x_ = target_x;
            needs_boundary_adjustment = true;
        }
    } else if (popup_x_ < 0) {
        if (popup_x_ < -5) {
            popup_x_ = target_x;
            needs_boundary_adjustment = true;
        }
    }

    // 检查垂直边界（更宽松的检查，因为我们优先保证下方显示）
    if (popup_y_ + popup_height_ > screen_height_ - 2) {
        // 只有当严重超出边界时才调整，保持下方优先策略
        if (popup_y_ + popup_height_ > screen_height_ + 10) {
            popup_y_ = target_y;
            needs_boundary_adjustment = true;
        }
    } else if (popup_y_ < 0) {
        if (popup_y_ < -3) {
            popup_y_ = target_y;
            needs_boundary_adjustment = true;
        }
    }

    // 如果进行了边界调整，确保位置在合理范围内
    if (needs_boundary_adjustment) {
        if (popup_x_ + popup_width_ > screen_width_ - 2) {
            popup_x_ = screen_width_ - popup_width_ - 2;
            if (popup_x_ < 0)
                popup_x_ = 0;
        }
        if (popup_y_ + popup_height_ > screen_height_ - 2) {
            popup_y_ = screen_height_ - popup_height_ - 2;
            if (popup_y_ < 0)
                popup_y_ = 0;
        }
    }
}

std::string CompletionPopup::getKindIcon(const std::string& kind) const {
    // LSP CompletionItemKind 枚举值
    // 1=Text, 2=Method, 3=Function, 4=Constructor, 5=Field, 6=Variable,
    // 7=Class, 8=Interface, 9=Module, 10=Property, 11=Unit, 12=Value,
    // 13=Enum, 14=Keyword, 15=Snippet, 16=Color, 17=File, 18=Reference,
    // 19=Folder, 20=EnumMember, 21=Constant, 22=Struct, 23=Event,
    // 24=Operator, 25=TypeParameter

    if (kind.empty())
        return " ";

    int kind_num = 0;
    try {
        kind_num = std::stoi(kind);
    } catch (...) {
        return " ";
    }

    switch (kind_num) {
        case 1:
            return " "; // Text
        case 2:
            return icons::CODE; // Method
        case 3:
            return icons::CODE; // Function
        case 4:
            return icons::CODE; // Constructor
        case 5:
            return " "; // Field
        case 6:
            return " "; // Variable
        case 7:
            return icons::CODE; // Class
        case 8:
            return icons::CODE; // Interface
        case 9:
            return icons::FOLDER; // Module
        case 10:
            return " "; // Property
        case 11:
            return " "; // Unit
        case 12:
            return " "; // Value
        case 13:
            return icons::CODE; // Enum
        case 14:
            return icons::CODE; // Keyword
        case 15:
            return icons::CODE; // Snippet
        case 16:
            return " "; // Color
        case 17:
            return icons::FILE; // File
        case 18:
            return " "; // Reference
        case 19:
            return icons::FOLDER; // Folder
        case 20:
            return icons::CODE; // EnumMember
        case 21:
            return " "; // Constant
        case 22:
            return icons::CODE; // Struct
        case 23:
            return " "; // Event
        case 24:
            return " "; // Operator
        case 25:
            return icons::CODE; // TypeParameter
        default:
            return " ";
    }
}

Element CompletionPopup::renderHighlightedLabel(const std::string& label, const std::string& query,
                                                bool is_selected,
                                                const ui::ThemeColors& colors) const {
    if (query.empty()) {
        // 没有查询，直接返回普通文本
        return text(label) |
               (is_selected ? color(colors.foreground) | bold : color(colors.foreground));
    }

    Elements parts;

    // 实现模糊匹配和高亮（支持驼峰匹配）
    auto fuzzyMatch = [&](const std::string& text,
                          const std::string& pattern) -> std::vector<std::pair<size_t, size_t>> {
        std::vector<std::pair<size_t, size_t>> matches;

        if (pattern.empty())
            return matches;

        // 转换为小写进行匹配
        std::string lower_text = text;
        std::string lower_pattern = pattern;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
        std::transform(lower_pattern.begin(), lower_pattern.end(), lower_pattern.begin(),
                       ::tolower);

        // 1. 首先尝试精确匹配
        size_t pos = lower_text.find(lower_pattern);
        if (pos != std::string::npos) {
            matches.emplace_back(pos, pattern.length());
            return matches;
        }

        // 2. 尝试驼峰匹配（CamelCase）
        if (pattern.length() >= 2) {
            size_t pattern_idx = 0;
            size_t text_idx = 0;
            size_t match_start = std::string::npos;

            while (text_idx < lower_text.length() && pattern_idx < lower_pattern.length()) {
                char text_char = lower_text[text_idx];
                char pattern_char = lower_pattern[pattern_idx];

                if (text_char == pattern_char) {
                    if (match_start == std::string::npos) {
                        match_start = text_idx;
                    }
                    pattern_idx++;
                } else if (match_start != std::string::npos) {
                    // 如果前面有匹配但当前不匹配，重置
                    match_start = std::string::npos;
                    pattern_idx = 0;
                    continue; // 不增加text_idx，重新检查当前字符
                }

                text_idx++;

                if (pattern_idx == lower_pattern.length()) {
                    matches.emplace_back(match_start, text_idx - match_start);
                    break;
                }
            }
        }

        // 3. 如果没有找到匹配，返回空
        return matches;
    };

    auto matches = fuzzyMatch(label, query);

    if (matches.empty()) {
        // 没有找到匹配，返回普通文本
        return text(label) |
               (is_selected ? color(colors.foreground) | bold : color(colors.foreground));
    }

    // 构建高亮文本
    size_t current_pos = 0;
    for (const auto& [match_start, match_length] : matches) {
        // 添加匹配前的文本
        if (match_start > current_pos) {
            std::string before = label.substr(current_pos, match_start - current_pos);
            parts.push_back(text(before) | (is_selected ? color(colors.foreground) | bold
                                                        : color(colors.foreground)));
        }

        // 添加高亮的匹配文本
        std::string match_text = label.substr(match_start, match_length);
        parts.push_back(text(match_text) | color(colors.function) | bold); // 使用函数颜色高亮匹配

        current_pos = match_start + match_length;
    }

    // 添加剩余的文本
    if (current_pos < label.length()) {
        std::string after = label.substr(current_pos);
        parts.push_back(text(after) |
                        (is_selected ? color(colors.foreground) | bold : color(colors.foreground)));
    }

    return hbox(std::move(parts));
}

Color CompletionPopup::getKindColor(const std::string& kind) const {
    if (kind.empty())
        return Color::Default;

    int kind_num = 0;
    try {
        kind_num = std::stoi(kind);
    } catch (...) {
        return Color::Default;
    }

    switch (kind_num) {
        case 2:
        case 3:
        case 4: // Method, Function, Constructor
            return Color::Cyan;
        case 7:
        case 8:
        case 22: // Class, Interface, Struct
            return Color::Blue;
        case 14: // Keyword
            return Color::Magenta;
        case 17: // File
            return Color::Yellow;
        case 19: // Folder
            return Color::Blue;
        default:
            return Color::Default;
    }
}

Element CompletionPopup::renderItem(const features::CompletionItem& item, bool is_selected,
                                    const ui::Theme& theme, const std::string& query) const {
    const auto& colors = theme.getColors();

    Elements item_elements;

    // 图标（参考neovim，使用更简洁的样式）
    std::string icon = getKindIcon(item.kind);
    Color icon_color = getKindColor(item.kind);

    // 代码片段使用特殊的图标
    if (item.isSnippet) {
        icon = ""; // 代码片段图标
        icon_color = Color::Cyan;
    }

    item_elements.push_back(text(icon.empty() ? " " : icon) | color(icon_color) |
                            size(WIDTH, EQUAL, 2));
    item_elements.push_back(text(" ")); // 图标和文本之间的间距

    // 标签（主要文本）- 增强版本：支持匹配高亮
    std::string label = item.label;
    size_t max_label_width = popup_width_ - 25; // 预留空间给图标、detail等
    if (label.length() > max_label_width) {
        label = label.substr(0, max_label_width - 3) + "...";
    }

    // 高亮匹配的字符（参考VSCode）
    Element label_elem = renderHighlightedLabel(label, query, is_selected, colors);
    item_elements.push_back(label_elem);

    // 详细信息（detail）- 只在选中时显示，参考neovim
    if (!item.detail.empty() && is_selected) {
        std::string detail = item.detail;
        size_t max_detail_width = popup_width_ - label.length() - 30;
        if (detail.length() > max_detail_width) {
            detail = detail.substr(0, max_detail_width - 3) + "...";
        }
        item_elements.push_back(text(" "));
        item_elements.push_back(text(detail) | color(colors.comment) | dim);
    }

    // 填充剩余空间
    item_elements.push_back(filler());

    // 构建行元素
    Element line = hbox(item_elements);

    // 选中状态样式（参考neovim：使用当前行背景色）
    if (is_selected) {
        line = line | bgcolor(colors.current_line) | color(colors.foreground);
    } else {
        line = line | bgcolor(colors.background);
    }

    return line;
}

Element CompletionPopup::render(const ui::Theme& theme) const {
    if (!visible_ || items_.empty()) {
        return text("");
    }

    const auto& colors = theme.getColors();

    // 计算显示范围
    size_t display_start = getDisplayStart();
    size_t display_end = getDisplayEnd();

    Elements lines;

    // 参考neovim：不显示标题栏，直接显示补全项，更简洁
    // 只在底部显示选中项信息（可选）

    // 显示补全项（带匹配高亮）
    for (size_t i = display_start; i < display_end && i < items_.size(); ++i) {
        const auto& item = items_[i];
        bool is_selected = (i == selected_index_);
        lines.push_back(renderItem(item, is_selected, theme, current_query_));
    }

    // 限制最大高度
    size_t max_height = max_items_;
    if (lines.size() > max_height) {
        lines.resize(max_height);
    }

    // 构建弹窗（参考neovim：使用更简洁的边框样式）
    Element popup = vbox(lines) | borderRounded | bgcolor(colors.background);

    // 设置固定尺寸以避免抖动
    popup = popup | size(WIDTH, EQUAL, popup_width_) | size(HEIGHT, EQUAL, popup_height_);

    return popup;
}

std::string CompletionPopup::applySelected() const {
    const auto* item = getSelectedItem();
    if (!item) {
        return "";
    }

    // 优先使用 insertText，否则使用 label
    return item->insertText.empty() ? item->label : item->insertText;
}

size_t CompletionPopup::getDisplayStart() const {
    if (items_.size() <= max_items_) {
        return 0;
    }

    // 确保选中的项在可见范围内
    if (selected_index_ < max_items_ / 2) {
        return 0;
    } else if (selected_index_ + max_items_ / 2 >= items_.size()) {
        return items_.size() - max_items_;
    } else {
        return selected_index_ - max_items_ / 2;
    }
}

size_t CompletionPopup::getDisplayEnd() const {
    size_t start = getDisplayStart();
    return std::min(start + max_items_, items_.size());
}

} // namespace ui
} // namespace pnana
