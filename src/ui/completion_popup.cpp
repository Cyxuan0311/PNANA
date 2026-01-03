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
                           int cursor_col, int screen_width, int screen_height) {
    auto show_start = std::chrono::steady_clock::now();
    LOG("[COMPLETION] [Popup] show() called with " + std::to_string(items.size()) + " items");

    // 参考 VSCode：优化响应速度，减少不必要的更新
    bool was_visible = visible_;

    // 快速比较：检查内容是否真正变化
    bool items_changed = (items_.size() != items.size());
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
    // 参考 Neovim：使用阈值机制，减少频繁的位置更新
    int row_diff = std::abs(row - cursor_row_);
    int col_diff = std::abs(col - cursor_col_);
    bool screen_changed = (screen_width_ != screen_width || screen_height_ != screen_height);

    // 只在位置变化超过阈值或屏幕尺寸变化时才更新
    // 阈值：行变化 >= 2，列变化 >= 5（增大阈值以减少抖动）
    bool needs_update = screen_changed || row_diff >= 2 || col_diff >= 5;

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

    // ========== 位置计算：锚点策略（参考 Neovim） ==========
    // Neovim 使用"锚点"机制：位置相对于光标，但只在光标移动超过阈值时更新
    // 这样可以避免光标微动时导致的抖动

    // 计算目标位置（相对于光标）
    int target_x = cursor_col_;
    if (target_x + popup_width_ > screen_width_ - 2) {
        target_x = screen_width_ - popup_width_ - 2;
        if (target_x < 0) {
            target_x = 0;
        }
    }

    // 计算目标 Y 位置
    int available_height = screen_height_ - 6;
    int cursor_screen_y = cursor_row_;
    int target_y;
    if (cursor_screen_y + popup_height_ + 1 < available_height) {
        target_y = cursor_screen_y + 1;
    } else if (cursor_screen_y > popup_height_ + 1) {
        target_y = cursor_screen_y - popup_height_ - 1;
    } else {
        target_y = cursor_screen_y + 1;
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

    // 策略3：使用阈值机制，减少频繁更新
    // X 方向：只在变化超过 5 个字符时更新（增大阈值以减少抖动）
    int x_diff = std::abs(target_x - popup_x_);
    if (x_diff > 5) {
        popup_x_ = target_x;
    }

    // Y 方向：只在变化超过 3 行时更新（增大阈值以减少抖动）
    int y_diff = std::abs(target_y - popup_y_);
    if (y_diff > 3) {
        popup_y_ = target_y;
    }

    // 策略4：如果光标移动导致 popup 超出屏幕，强制更新
    if (popup_x_ + popup_width_ > screen_width_ - 2 || popup_x_ < 0) {
        popup_x_ = target_x;
    }
    if (popup_y_ + popup_height_ > screen_height_ - 2 || popup_y_ < 0) {
        popup_y_ = target_y;
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
                                    const ui::Theme& theme) const {
    const auto& colors = theme.getColors();

    Elements item_elements;

    // 图标（参考neovim，使用更简洁的样式）
    std::string icon = getKindIcon(item.kind);
    Color icon_color = getKindColor(item.kind);
    item_elements.push_back(text(icon.empty() ? " " : icon) | color(icon_color) |
                            size(WIDTH, EQUAL, 2));
    item_elements.push_back(text(" ")); // 图标和文本之间的间距

    // 标签（主要文本）
    std::string label = item.label;
    size_t max_label_width = popup_width_ - 25; // 预留空间给图标、detail等
    if (label.length() > max_label_width) {
        label = label.substr(0, max_label_width - 3) + "...";
    }

    Element label_elem = text(label);
    if (is_selected) {
        // 选中项：使用主题的当前行背景色，参考neovim
        label_elem = label_elem | color(colors.foreground) | bold;
    } else {
        label_elem = label_elem | color(colors.foreground);
    }
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

    // 显示补全项
    for (size_t i = display_start; i < display_end && i < items_.size(); ++i) {
        const auto& item = items_[i];
        bool is_selected = (i == selected_index_);
        lines.push_back(renderItem(item, is_selected, theme));
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
