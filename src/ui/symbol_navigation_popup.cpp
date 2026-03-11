#include "ui/symbol_navigation_popup.h"
#include "ui/icons.h"
#include "utils/match_highlight.h"
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace ftxui;
using namespace pnana::ui::icons;

namespace pnana {
namespace ui {

SymbolNavigationPopup::SymbolNavigationPopup(Theme& theme)
    : theme_(theme), selected_index_(0), visible_(false), search_cursor_pos_(0),
      cursor_blink_rate_ms_(800), jump_callback_(nullptr) {
    cursor_config_.style = CursorStyle::BLOCK;
    cursor_config_.color = theme.getColors().foreground;
    cursor_config_.blink_enabled = false;
}

void SymbolNavigationPopup::setSymbols(
    const std::vector<pnana::features::DocumentSymbol>& symbols) {
    symbols_ = symbols;
    flattened_symbols_.clear();
    flattenSymbols(symbols);
    search_input_.clear();
    search_cursor_pos_ = 0;
    updateFilteredIndices();
    selected_index_ = 0;
}

void SymbolNavigationPopup::setCursorConfig(const CursorConfig& config, int blink_rate_ms) {
    cursor_config_ = config;
    cursor_blink_rate_ms_ = blink_rate_ms <= 0 ? 800 : blink_rate_ms;
}

void SymbolNavigationPopup::flattenSymbols(
    const std::vector<pnana::features::DocumentSymbol>& symbols, int depth) {
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
    search_input_.clear();
    search_cursor_pos_ = 0;
    updateFilteredIndices();
    selected_index_ = 0;
}

void SymbolNavigationPopup::updateFilteredIndices() {
    filtered_indices_.clear();
    if (search_input_.empty()) {
        for (size_t i = 0; i < flattened_symbols_.size(); ++i)
            filtered_indices_.push_back(i);
        return;
    }
    for (size_t i = 0; i < flattened_symbols_.size(); ++i) {
        if (fuzzyMatch(flattened_symbols_[i].name, search_input_))
            filtered_indices_.push_back(i);
    }
    if (selected_index_ >= filtered_indices_.size())
        selected_index_ = filtered_indices_.empty() ? 0 : filtered_indices_.size() - 1;
}

bool SymbolNavigationPopup::fuzzyMatch(const std::string& name, const std::string& query) {
    if (query.empty())
        return true;
    size_t ni = 0;
    for (char q : query) {
        char ql = static_cast<char>(std::tolower(static_cast<unsigned char>(q)));
        bool found = false;
        for (; ni < name.size(); ++ni) {
            if (static_cast<char>(std::tolower(static_cast<unsigned char>(name[ni]))) == ql) {
                found = true;
                ++ni;
                break;
            }
        }
        if (!found)
            return false;
    }
    return true;
}

void SymbolNavigationPopup::hide() {
    visible_ = false;
}

bool SymbolNavigationPopup::isVisible() const {
    return visible_;
}

void SymbolNavigationPopup::selectNext() {
    if (!filtered_indices_.empty()) {
        selected_index_ = (selected_index_ + 1) % filtered_indices_.size();
        if (jump_callback_ && selected_index_ < filtered_indices_.size()) {
            jump_callback_(flattened_symbols_[filtered_indices_[selected_index_]]);
        }
    }
}

void SymbolNavigationPopup::selectPrevious() {
    if (!filtered_indices_.empty()) {
        selected_index_ =
            (selected_index_ + filtered_indices_.size() - 1) % filtered_indices_.size();
        if (jump_callback_ && selected_index_ < filtered_indices_.size()) {
            jump_callback_(flattened_symbols_[filtered_indices_[selected_index_]]);
        }
    }
}

void SymbolNavigationPopup::selectFirst() {
    selected_index_ = 0;
    if (jump_callback_ && !filtered_indices_.empty()) {
        jump_callback_(flattened_symbols_[filtered_indices_[selected_index_]]);
    }
}

void SymbolNavigationPopup::selectLast() {
    selected_index_ = filtered_indices_.empty() ? 0 : filtered_indices_.size() - 1;
    if (jump_callback_ && !filtered_indices_.empty()) {
        jump_callback_(flattened_symbols_[filtered_indices_[selected_index_]]);
    }
}

const pnana::features::DocumentSymbol* SymbolNavigationPopup::getSelectedSymbol() const {
    if (filtered_indices_.empty() || selected_index_ >= filtered_indices_.size()) {
        return nullptr;
    }
    return &flattened_symbols_[filtered_indices_[selected_index_]];
}

void SymbolNavigationPopup::setJumpCallback(
    std::function<void(const pnana::features::DocumentSymbol&)> callback) {
    jump_callback_ = std::move(callback);
}

bool SymbolNavigationPopup::handleInput(ftxui::Event event) {
    if (!visible_) {
        return false;
    }

    // 模糊搜索输入框：可打印字符、退格、左右移动光标
    if (event.is_character()) {
        std::string input = event.character();
        if (!input.empty() && search_cursor_pos_ <= search_input_.size()) {
            unsigned char c = static_cast<unsigned char>(input[0]);
            // 只接受可打印 ASCII（空格 32 到 ~ 126），拒绝换行等控制字符
            if (c >= 32 && c < 127) {
                search_input_.insert(search_cursor_pos_, input);
                search_cursor_pos_ += input.size();
                updateFilteredIndices();
                if (jump_callback_ && !filtered_indices_.empty()) {
                    jump_callback_(flattened_symbols_[filtered_indices_[selected_index_]]);
                }
                return true;
            }
        }
    }
    if (event == Event::Backspace && search_cursor_pos_ > 0) {
        search_input_.erase(search_cursor_pos_ - 1, 1);
        search_cursor_pos_--;
        updateFilteredIndices();
        if (jump_callback_ && !filtered_indices_.empty()) {
            jump_callback_(flattened_symbols_[filtered_indices_[selected_index_]]);
        }
        return true;
    }
    if (event == Event::ArrowLeft && search_cursor_pos_ > 0) {
        search_cursor_pos_--;
        return true;
    }
    if (event == Event::ArrowRight && search_cursor_pos_ < search_input_.size()) {
        search_cursor_pos_++;
        return true;
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

    // 块状光标：使用与编辑器一致的光标配置
    CursorRenderer cursor_renderer;
    cursor_renderer.setConfig(cursor_config_);
    cursor_renderer.setBlinkRate(cursor_blink_rate_ms_);
    cursor_renderer.updateCursorState();

    Elements content;

    // 标题栏
    Elements title_elements;
    title_elements.push_back(text(CODE) | color(colors.function));
    title_elements.push_back(text(" Symbol Navigation ") | color(colors.foreground) | bold);
    content.push_back(hbox(title_elements) | center);

    content.push_back(separator());

    // 模糊搜索输入框（块状光标 + 主题色）
    Elements search_row;
    search_row.push_back(text(" Filter: ") | color(colors.comment));
    if (search_cursor_pos_ <= search_input_.size()) {
        std::string before = search_input_.substr(0, search_cursor_pos_);
        std::string cursor_char = search_cursor_pos_ < search_input_.size()
                                      ? search_input_.substr(search_cursor_pos_, 1)
                                      : " ";
        std::string after = search_cursor_pos_ < search_input_.size()
                                ? search_input_.substr(search_cursor_pos_ + 1)
                                : "";
        if (!before.empty())
            search_row.push_back(text(before) | color(colors.foreground));
        search_row.push_back(cursor_renderer.renderCursorElement(
            cursor_char, search_cursor_pos_, search_input_.size(), colors.foreground,
            colors.background));
        if (!after.empty())
            search_row.push_back(text(after) | color(colors.foreground));
    } else {
        search_row.push_back(text(search_input_) | color(colors.foreground));
    }
    content.push_back(hbox(search_row));

    content.push_back(separator());

    // 符号列表（按过滤结果）
    Elements items;
    size_t max_display = 12;
    size_t total = filtered_indices_.size();

    size_t start_idx = 0;
    if (selected_index_ >= max_display) {
        start_idx = selected_index_ - max_display + 1;
    }
    size_t end_idx = std::min(start_idx + max_display, total);

    for (size_t i = start_idx; i < end_idx; ++i) {
        bool is_selected = (i == selected_index_);
        items.push_back(renderSymbolItem(flattened_symbols_[filtered_indices_[i]], is_selected));
    }

    content.push_back(vbox(items) | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 15));

    // 统计信息
    std::string stats = std::to_string(filtered_indices_.size()) + " / " +
                        std::to_string(flattened_symbols_.size()) + " symbols";
    content.push_back(separator());
    content.push_back(text(stats) | dim | center);

    // 帮助信息
    content.push_back(separator());
    content.push_back(text("Type to filter | ↑↓ Navigate | Enter Jump | Esc Close") | dim | center);

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

    // 符号名称（匹配搜索高亮）
    Color name_color = is_selected ? colors.foreground : kind_color;
    line_elements.push_back(
        pnana::utils::highlightMatch(symbol.name, search_input_, name_color, colors.keyword));

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
    // LSP SymbolKind：支持字符串名与数字（1-based），按类型选用区分度高的图标
    if (kind == "Function" || kind == "12") {
        return FUNCTION; // 函数
    }
    if (kind == "Method" || kind == "6") {
        return FUNCTION; // 方法
    }
    if (kind == "Class" || kind == "5") {
        return CODE; // 类
    }
    if (kind == "Namespace" || kind == "3") {
        return FOLDER_OPEN; // 命名空间（容器）
    }
    if (kind == "Module" || kind == "2") {
        return PACKAGE; // 模块
    }
    if (kind == "Package" || kind == "4") {
        return PACKAGE; // 包
    }
    if (kind == "Variable" || kind == "13") {
        return SELECT; // 变量
    }
    if (kind == "Property" || kind == "7") {
        return PENCIL; // 属性
    }
    if (kind == "Field" || kind == "8") {
        return SELECT; // 字段
    }
    if (kind == "Constructor" || kind == "9") {
        return WRENCH; // 构造函数
    }
    if (kind == "Enum" || kind == "10") {
        return LIST; // 枚举（列表）
    }
    if (kind == "EnumMember" || kind == "22") {
        return STAR; // 枚举成员
    }
    if (kind == "Struct" || kind == "23") {
        return DATABASE; // 结构体（数据结构）
    }
    if (kind == "Interface" || kind == "11") {
        return LINK; // 接口（契约）
    }
    if (kind == "Constant" || kind == "14") {
        return KEY; // 常量
    }
    if (kind == "Event" || kind == "24") {
        return BELL; // 事件
    }
    if (kind == "Operator" || kind == "25") {
        return COGS; // 运算符
    }
    if (kind == "TypeParameter" || kind == "26") {
        return INFO; // 类型参数
    }
    if (kind == "File" || kind == "1") {
        return icons::FILE; // 文件（避免与 C 标准库 FILE 类型歧义）
    }
    if (kind == "String" || kind == "15" || kind == "Number" || kind == "16" || kind == "Boolean" ||
        kind == "17" || kind == "Array" || kind == "18" || kind == "Object" || kind == "19" ||
        kind == "Key" || kind == "20") {
        return CODE; // 基础类型/字面量
    }
    return CODE; // 默认
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
