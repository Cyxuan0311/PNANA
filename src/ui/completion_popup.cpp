#include "ui/completion_popup.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

// Neovim 风格：单线边框（无圆角），简洁
static inline Decorator borderNeovim(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

CompletionPopup::CompletionPopup()
    : visible_(false), selected_index_(0), max_items_(12), anchor_screen_y_(0), anchor_screen_x_(0),
      screen_width_(80), screen_height_(24), content_top_(0), content_bottom_(-1), popup_x_(0),
      popup_y_(0), popup_width_(55), popup_height_(14), last_items_size_(0) {}

void CompletionPopup::show(const std::vector<features::CompletionItem>& items, int anchor_screen_y,
                           int anchor_screen_x, int screen_width, int screen_height,
                           int content_top, int content_bottom, const std::string& query) {
    if (items.empty()) {
        hide();
        return;
    }

    // 过滤掉 label 为空的项，避免出现“空条”的弹窗
    std::vector<features::CompletionItem> filtered;
    filtered.reserve(items.size());
    for (const auto& item : items) {
        std::string display = item.label.empty() ? item.filterText : item.label;
        if (!display.empty()) {
            filtered.push_back(item);
        }
    }
    if (filtered.empty()) {
        hide();
        return;
    }

    const std::vector<features::CompletionItem>& use_items = filtered;

    // 参考 VSCode：优化响应速度，减少不必要的更新
    bool was_visible = visible_;

    // 快速比较：检查内容是否真正变化
    bool items_changed = (items_.size() != use_items.size()) || (current_query_ != query);
    if (!items_changed && items_.size() > 0 && use_items.size() > 0) {
        // 比较前几个 item 的 label，如果相同则认为内容未变化
        bool content_same = true;
        size_t compare_count =
            std::min({items_.size(), use_items.size(), size_t(5)}); // 比较前 5 个
        for (size_t i = 0; i < compare_count; ++i) {
            if (items_[i].label != use_items[i].label) {
                content_same = false;
                break;
            }
        }
        items_changed = !content_same;
    }

    // 检查屏幕尺寸是否变化（需要重新计算位置）
    bool screen_changed = (screen_width_ != screen_width || screen_height_ != screen_height);

    // 检查锚点位置是否变化（需要重新计算位置）
    bool anchor_changed =
        (anchor_screen_y_ != anchor_screen_y || anchor_screen_x_ != anchor_screen_x);

    items_ = use_items;
    current_query_ = query;
    anchor_screen_y_ = anchor_screen_y;
    anchor_screen_x_ = anchor_screen_x;
    screen_width_ = screen_width;
    screen_height_ = screen_height;
    content_top_ = content_top;
    content_bottom_ = content_bottom;
    selected_index_ = 0;
    visible_ = !items_.empty();

    if (visible_ && (items_changed || screen_changed || anchor_changed || !was_visible)) {
        calculatePopupPosition();
    }
}

void CompletionPopup::updateCursorPosition(int row, int col, int screen_width, int screen_height) {
    // 增强的阈值机制，进一步减少不必要的弹窗位置更新
    int row_diff = std::abs(row - anchor_screen_y_);
    int col_diff = std::abs(col - anchor_screen_x_);
    bool screen_changed = (screen_width_ != screen_width || screen_height_ != screen_height);

    // 增大阈值以大幅减少抖动：
    // 行变化 >= 3，列变化 >= 8
    // 这样可以确保只有在锚点发生明显移动时才更新弹窗位置
    bool needs_update = screen_changed || row_diff >= 3 || col_diff >= 8;

    anchor_screen_y_ = row;
    anchor_screen_x_ = col;
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

void CompletionPopup::updateItem(size_t index, const features::CompletionItem& item) {
    if (index < items_.size()) {
        items_[index] = item;
    }
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
        // Neovim 风格：紧凑宽度，约屏幕 40%
        popup_width_ = std::min(60, std::max(45, (screen_width_ * 40) / 100));
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

    // 高度：选中项占两行时需额外一行显示描述
    size_t display_count = std::min(items_.size(), max_items_);
    int new_height = static_cast<int>(display_count) + 1; // +1 为选中项的详情行
    if (popup_height_ == 0) {
        popup_height_ = new_height;
        size_changed = true;
    } else if (std::abs(new_height - popup_height_) > 5) {
        popup_height_ = new_height;
        size_changed = true;
    }

    // ========== 位置计算：参考 VSCode 的行为 ==========
    // 目标：始终尽量在光标所在列的右侧显示（即与光标对齐左边界），
    // 在光标行的下方一行优先显示；若下方空间不足则显示在上方一行。
    // 同时确保不超出屏幕边界，并尽量减少抖动（但阈值更小以提升对齐准确性）。

    // 计算水平目标位置：优先在锚点右侧显示
    int desired_x = anchor_screen_x_;
    int right_limit = std::max(0, screen_width_ - 2);
    if (desired_x + popup_width_ > right_limit) {
        int left_of_anchor = anchor_screen_x_ - popup_width_;
        if (left_of_anchor >= 0) {
            desired_x = left_of_anchor;
        } else {
            desired_x = std::max(0, right_limit - popup_width_);
        }
    }

    // 只使用屏幕边界，不再按内容区做 clamp
    int anchor_screen_y = std::clamp(anchor_screen_y_, 0, std::max(0, screen_height_ - 1));
    int desired_y_below = anchor_screen_y + 4; // 临时加大间距：在锚点下方四行显示，便于验证视觉偏移
    int desired_y_above = anchor_screen_y - popup_height_; // 在锚点上一行显示

    bool can_show_below = (desired_y_below + popup_height_ <= screen_height_ - 1);
    bool can_show_above = (desired_y_above >= 0);

    int desired_y;
    std::string vertical_reason;
    if (can_show_below) {
        desired_y = desired_y_below;
        vertical_reason = "below";
    } else if (can_show_above) {
        desired_y = desired_y_above;
        vertical_reason = "above";
    } else {
        int space_below = std::max(0, screen_height_ - (anchor_screen_y + 1));
        int space_above = std::max(0, anchor_screen_y);
        if (space_below >= space_above) {
            desired_y = std::max(0, screen_height_ - popup_height_ - 1);
            vertical_reason = "clamp_bottom";
        } else {
            desired_y = 0;
            vertical_reason = "clamp_top";
        }
    }

    if (size_changed) {
        popup_x_ = desired_x;
        popup_y_ = desired_y;
        return;
    }

    if (popup_x_ == 0 && popup_y_ == 0) {
        popup_x_ = desired_x;
        popup_y_ = desired_y;
        return;
    }

    int x_diff = std::abs(desired_x - popup_x_);
    int y_diff = std::abs(desired_y - popup_y_);

    if (x_diff > 2) {
        popup_x_ = desired_x;
    }

    if (y_diff > 0) {
        popup_y_ = desired_y;
    }

    // 边界检查与修正（以防外部改动导致越界）
    if (popup_x_ + popup_width_ > screen_width_ - 2) {
        popup_x_ = screen_width_ - popup_width_ - 2;
        if (popup_x_ < 0)
            popup_x_ = 0;
    }
    if (popup_x_ < 0) {
        popup_x_ = 0;
    }

    popup_y_ = std::clamp(popup_y_, 0, std::max(0, screen_height_ - popup_height_ - 1));
}

std::string CompletionPopup::getKindIcon(const std::string& kind) const {
    // LSP CompletionItemKind 细化图标
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
            return icons::LSP_METHOD; // Method
        case 3:
            return icons::LSP_FUNCTION; // Function
        case 4:
            return icons::LSP_CONSTRUCTOR; // Constructor
        case 5:
            return icons::LSP_FIELD; // Field
        case 6:
            return icons::LSP_VARIABLE; // Variable
        case 7:
            return icons::LSP_CLASS; // Class
        case 8:
            return icons::LSP_INTERFACE; // Interface
        case 9:
            return icons::LSP_MODULE; // Module
        case 10:
            return icons::LSP_PROPERTY; // Property
        case 11:
            return icons::LSP_UNIT; // Unit
        case 12:
            return icons::LSP_VALUE; // Value
        case 13:
            return icons::LSP_ENUM; // Enum
        case 14:
            return icons::LSP_KEYWORD; // Keyword
        case 15:
            return icons::LSP_SNIPPET; // Snippet
        case 16:
            return icons::LSP_COLOR; // Color
        case 17:
            return icons::FILE; // File
        case 18:
            return icons::LSP_REFERENCE; // Reference
        case 19:
            return icons::FOLDER; // Folder
        case 20:
            return icons::LSP_ENUMMEMBER; // EnumMember
        case 21:
            return icons::LSP_CONSTANT; // Constant
        case 22:
            return icons::LSP_STRUCT; // Struct
        case 23:
            return icons::LSP_EVENT; // Event
        case 24:
            return icons::LSP_OPERATOR; // Operator
        case 25:
            return icons::LSP_TYPEPARAM; // TypeParameter
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
        case 5:
        case 6:
        case 10: // Field, Variable, Property
            return Color::Default;
        case 7:
        case 8:
        case 22: // Class, Interface, Struct
            return Color::Blue;
        case 9:
        case 19: // Module, Folder
            return Color::Blue;
        case 13:
        case 20: // Enum, EnumMember
            return Color::Green;
        case 14: // Keyword
            return Color::Magenta;
        case 15: // Snippet
            return Color::Cyan;
        case 16: // Color
            return Color::Magenta;
        case 17: // File
            return Color::Yellow;
        case 21: // Constant
            return Color::Yellow;
        case 24: // Operator
            return Color::Default;
        default:
            return Color::Default;
    }
}

Element CompletionPopup::renderItem(const features::CompletionItem& item, bool is_selected,
                                    const ui::Theme& theme, const std::string& query) const {
    const auto& colors = theme.getColors();

    // Neovim Pmenu 风格：细化图标 + 标签
    std::string icon = getKindIcon(item.kind);
    Color icon_color = getKindColor(item.kind);
    if (item.isSnippet) {
        icon = icons::LSP_SNIPPET;
        icon_color = Color::Cyan;
    }

    std::string label = item.label;
    size_t max_label_width = static_cast<size_t>(popup_width_) - 8;
    if (label.length() > max_label_width) {
        label = label.substr(0, max_label_width - 3) + "...";
    }

    Element label_elem = renderHighlightedLabel(label, query, is_selected, colors);
    // 确保 1-2 字符的标签有最小显示宽度，避免被压缩不可见
    if (label.length() <= 2) {
        label_elem = label_elem | size(WIDTH, GREATER_THAN, 2);
    }

    // 行布局：图标(2) + 空格(1) + 标签
    Element line1 =
        hbox({text(icon.empty() ? " " : icon) | color(icon_color) | size(WIDTH, EQUAL, 2),
              text(" "), label_elem, filler()});

    // 选中项高亮当前行：current_line 背景 + 加粗，与编辑器当前行风格一致
    if (is_selected) {
        line1 = line1 | bgcolor(colors.current_line) | color(colors.foreground) | bold;
    } else {
        line1 = line1 | bgcolor(colors.background);
    }

    // 选中时第二行：detail/documentation，延续当前行高亮
    if (is_selected && (!item.detail.empty() || !item.documentation.empty())) {
        // 优先显示 documentation（通常更详细），若为空则用 detail（如返回类型）
        std::string desc = !item.documentation.empty() ? item.documentation : item.detail;
        size_t nl = desc.find('\n');
        if (nl != std::string::npos)
            desc = desc.substr(0, nl);
        if (desc.length() > static_cast<size_t>(popup_width_ - 8)) {
            desc = desc.substr(0, static_cast<size_t>(popup_width_ - 11)) + "...";
        }
        Element line2 =
            hbox({text("  "), text(desc) | color(colors.comment)}) | bgcolor(colors.current_line);
        return vbox({line1, line2});
    }

    return line1;
}

Element CompletionPopup::render(const ui::Theme& theme, int content_origin_x,
                                int content_origin_y) const {
    if (!visible_ || items_.empty()) {
        return text("");
    }

    const auto& colors = theme.getColors();

    // 计算显示范围
    size_t display_start = getDisplayStart();
    size_t display_end = getDisplayEnd();

    Elements lines;

    // Neovim Pmenu：无标题栏，紧凑列表
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

    // 与代码区统一：使用 background 主题色，与编辑器背景一致
    Element popup = vbox(lines) | borderNeovim(colors.dialog_border) | bgcolor(colors.background);

    popup = popup | size(WIDTH, EQUAL, popup_width_) | size(HEIGHT, EQUAL, popup_height_);

    int relative_x = popup_x_ - content_origin_x;
    int relative_y = popup_y_ - content_origin_y;
    if (relative_x < 0)
        relative_x = 0;
    if (relative_y < 0)
        relative_y = 0;

    Elements positioned_lines;

    for (int i = 0; i < relative_y; ++i) {
        positioned_lines.push_back(text("") | size(HEIGHT, EQUAL, 1));
    }

    std::string left_padding(relative_x > 0 ? static_cast<size_t>(relative_x) : 0, ' ');
    positioned_lines.push_back(hbox({text(left_padding), popup}));

    return vbox(positioned_lines);
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

    // 参考 file_browser_view.cpp：保持选中项在可见区域 1/3 处，确保行光标不被底部裁剪
    size_t available_height = max_items_;
    size_t ideal_position = available_height / 3; // 选中项距顶部 1/3
    size_t target_start;

    if (selected_index_ < ideal_position) {
        target_start = 0;
    } else if (selected_index_ >= items_.size() - (available_height - ideal_position)) {
        // 选中接近底部时：留一行底部余量，避免选中项贴底导致行光标被裁剪（对应 file_browser_view
        // 的向下微调）
        target_start =
            (items_.size() > available_height) ? items_.size() - available_height + 1 : 0;
        if (target_start > items_.size()) {
            target_start = 0;
        }
    } else {
        target_start = selected_index_ - ideal_position;
    }

    return target_start;
}

size_t CompletionPopup::getDisplayEnd() const {
    size_t start = getDisplayStart();
    return std::min(start + max_items_, items_.size());
}

} // namespace ui
} // namespace pnana
