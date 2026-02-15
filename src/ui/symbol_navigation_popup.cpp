#include "ui/symbol_navigation_popup.h"
#include "ui/icons.h"
#include <algorithm>
#include <sstream>

using namespace ftxui;
using namespace pnana::ui::icons;

namespace pnana {
namespace ui {

SymbolNavigationPopup::SymbolNavigationPopup(Theme& theme)
    : theme_(theme), selected_index_(0), visible_(false), jump_callback_(nullptr) {}

void SymbolNavigationPopup::setSymbols(const std::vector<pnana::features::DocumentSymbol>& symbols) {
    symbols_ = symbols;
    flattened_symbols_.clear();
    flattenSymbols(symbols);
    selected_index_ = flattened_symbols_.empty() ? 0 : 0;
}

void SymbolNavigationPopup::flattenSymbols(const std::vector<pnana::features::DocumentSymbol>& symbols,
                                           int depth) {
    for (const auto& symbol : symbols) {
        pnana::features::DocumentSymbol flat_symbol = symbol;
        flat_symbol.depth = depth;
        flattened_symbols_.push_back(flat_symbol);
        
        // 递归处理子符号
        if (!symbol.children.empty()) {
            flattenSymbols(symbol.children, depth + 1);
        }
    }
}

void SymbolNavigationPopup::show() {
    visible_ = true;
    selected_index_ = flattened_symbols_.empty() ? 0 : 0;
}

void SymbolNavigationPopup::hide() {
    visible_ = false;
}

bool SymbolNavigationPopup::isVisible() const {
    return visible_;
}

void SymbolNavigationPopup::selectNext() {
    if (!flattened_symbols_.empty()) {
        selected_index_ = (selected_index_ + 1) % flattened_symbols_.size();
        // 触发预览跳转
        if (jump_callback_ && selected_index_ < flattened_symbols_.size()) {
            jump_callback_(flattened_symbols_[selected_index_]);
        }
    }
}

void SymbolNavigationPopup::selectPrevious() {
    if (!flattened_symbols_.empty()) {
        selected_index_ = (selected_index_ + flattened_symbols_.size() - 1) % flattened_symbols_.size();
        // 触发预览跳转
        if (jump_callback_ && selected_index_ < flattened_symbols_.size()) {
            jump_callback_(flattened_symbols_[selected_index_]);
        }
    }
}

void SymbolNavigationPopup::selectFirst() {
    selected_index_ = flattened_symbols_.empty() ? 0 : 0;
    if (jump_callback_ && !flattened_symbols_.empty()) {
        jump_callback_(flattened_symbols_[selected_index_]);
    }
}

void SymbolNavigationPopup::selectLast() {
    selected_index_ = flattened_symbols_.empty() ? 0 : flattened_symbols_.size() - 1;
    if (jump_callback_ && !flattened_symbols_.empty()) {
        jump_callback_(flattened_symbols_[selected_index_]);
    }
}

const pnana::features::DocumentSymbol* SymbolNavigationPopup::getSelectedSymbol() const {
    if (flattened_symbols_.empty() || selected_index_ >= flattened_symbols_.size()) {
        return nullptr;
    }
    return &flattened_symbols_[selected_index_];
}

void SymbolNavigationPopup::setJumpCallback(
    std::function<void(const pnana::features::DocumentSymbol&)> callback) {
    jump_callback_ = std::move(callback);
}

bool SymbolNavigationPopup::handleInput(ftxui::Event event) {
    if (!visible_) {
        return false;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        selectPrevious();
        return true;
    }

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        selectNext();
        return true;
    }

    if (event == Event::Home) {
        selectFirst();
        return true;
    }

    if (event == Event::End) {
        selectLast();
        return true;
    }

    if (event == Event::Return || event == Event::Character('\n')) {
        // Enter键确认选择，由Editor处理跳转和关闭
        return true;
    }

    if (event == Event::Escape) {
        hide();
        return true;
    }

    return false;
}

Element SymbolNavigationPopup::render() const {
    if (!visible_ || flattened_symbols_.empty()) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements content;

    // 标题栏
    Elements title_elements;
    title_elements.push_back(text(CODE) | color(colors.function));
    title_elements.push_back(text(" Symbol Navigation ") | color(colors.foreground) | bold);
    content.push_back(hbox(title_elements) | center);

    content.push_back(separator());

    // 符号列表
    Elements items;
    size_t max_display = 12;

    // 如果选中项不在显示范围内，滚动显示
    size_t start_idx = 0;
    if (selected_index_ >= max_display) {
        start_idx = selected_index_ - max_display + 1;
    }

    size_t end_idx = std::min(start_idx + max_display, flattened_symbols_.size());

    for (size_t i = start_idx; i < end_idx; ++i) {
        bool is_selected = (i == selected_index_);
        items.push_back(renderSymbolItem(flattened_symbols_[i], is_selected));
    }

    content.push_back(vbox(items) | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 15));

    // 统计信息
    std::string stats = std::to_string(flattened_symbols_.size()) + " symbols";
    content.push_back(separator());
    content.push_back(text(stats) | dim | center);

    // 帮助信息
    content.push_back(separator());
    content.push_back(text("↑↓ Navigate | Enter Jump | Esc Close") | dim | center);

    Element dialog_content = vbox(content);

    return window(text("Symbol Navigation"), dialog_content) | size(WIDTH, GREATER_THAN, 70) |
           size(HEIGHT, GREATER_THAN, 15) | bgcolor(colors.background) |
           color(colors.dialog_border) | border;
}

Element SymbolNavigationPopup::renderSymbolItem(const pnana::features::DocumentSymbol& symbol,
                                               bool is_selected) const {
    auto& colors = theme_.getColors();

    // 获取符号类型图标和颜色
    std::string icon = getKindIcon(symbol.kind);
    Color kind_color = getKindColor(symbol.kind);

    // 构建缩进（根据嵌套深度）
    std::string indent;
    for (int i = 0; i < symbol.depth; ++i) {
        indent += "  "; // 每层缩进2个空格
    }

    // 构建位置信息
    std::string location = "[" + std::to_string(symbol.range.start.line + 1) + ":" +
                           std::to_string(symbol.range.start.character + 1) + "]";

    // 构建显示文本
    Elements line_elements;
    
    // 缩进
    if (!indent.empty()) {
        line_elements.push_back(text(indent));
    }
    
    // 图标
    line_elements.push_back(text(icon) | color(kind_color));
    line_elements.push_back(text(" "));
    
    // 符号名称
    line_elements.push_back(text(symbol.name) | color(is_selected ? colors.foreground : kind_color));
    
    // 详细信息（如果有）
    if (!symbol.detail.empty()) {
        std::string detail = symbol.detail;
        if (detail.length() > 40) {
            detail = detail.substr(0, 37) + "...";
        }
        line_elements.push_back(text(" " + detail) | dim);
    }
    
    // 位置信息（右对齐）
    line_elements.push_back(text("") | flex);
    line_elements.push_back(text(location) | dim);

    Element element = hbox(line_elements);

    // 应用选中样式
    if (is_selected) {
        element = element | bgcolor(colors.current_line) | color(colors.foreground);
    }

    return element;
}

std::string SymbolNavigationPopup::getKindIcon(const std::string& kind) const {
    // 根据符号类型返回对应的图标
    if (kind == "Function" || kind == "12") {
        return FUNCTION;
    } else if (kind == "Method" || kind == "6") {
        return FUNCTION; // 方法也用函数图标
    } else if (kind == "Class" || kind == "5") {
        return CODE; // 类用代码图标
    } else if (kind == "Namespace" || kind == "3") {
        return TAB; // 命名空间用标签图标
    } else if (kind == "Variable" || kind == "13") {
        return SELECT; // 变量用选择图标
    } else if (kind == "Enum" || kind == "10") {
        return CODE; // 枚举用代码图标
    } else if (kind == "Struct" || kind == "23") {
        return CODE; // 结构体用代码图标
    } else if (kind == "Interface" || kind == "11") {
        return CODE; // 接口用代码图标
    } else if (kind == "Constant" || kind == "14") {
        return LOCATION; // 常量用位置图标
    } else {
        return CODE; // 默认用代码图标
    }
}

Color SymbolNavigationPopup::getKindColor(const std::string& kind) const {
    auto& colors = theme_.getColors();
    
    // 根据符号类型返回对应的颜色
    if (kind == "Function" || kind == "12" || kind == "Method" || kind == "6") {
        return colors.function; // 函数和方法用函数色
    } else if (kind == "Class" || kind == "5" || kind == "Struct" || kind == "23" ||
               kind == "Interface" || kind == "11") {
        return colors.info; // 类、结构体、接口用信息色
    } else if (kind == "Namespace" || kind == "3") {
        return colors.warning; // 命名空间用警告色
    } else if (kind == "Variable" || kind == "13") {
        return colors.success; // 变量用成功色
    } else if (kind == "Enum" || kind == "10") {
        return colors.info; // 枚举用信息色
    } else if (kind == "Constant" || kind == "14") {
        return colors.success; // 常量用成功色
    } else {
        return colors.foreground; // 默认用前景色
    }
}

size_t SymbolNavigationPopup::getSymbolCount() const {
    return flattened_symbols_.size();
}

} // namespace ui
} // namespace pnana

