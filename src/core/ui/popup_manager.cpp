#include "core/ui/popup_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace core {
namespace ui {

namespace {

// 颜色名称映射表
std::map<std::string, Color> createColorMap() {
    std::map<std::string, Color> color_map;
    color_map["black"] = Color::Black;
    color_map["red"] = Color::Red;
    color_map["green"] = Color::Green;
    color_map["yellow"] = Color::Yellow;
    color_map["blue"] = Color::Blue;
    color_map["magenta"] = Color::Magenta;
    color_map["cyan"] = Color::Cyan;
    color_map["white"] = Color::White;
    return color_map;
}

const std::map<std::string, Color>& getColorMap() {
    static std::map<std::string, Color> color_map = createColorMap();
    return color_map;
}

} // namespace

// 解析颜色字符串为 FTXUI Color
Color PopupManager::parseColor(const std::string& color_str) const {
    if (color_str.empty()) {
        return theme_ ? theme_->getColors().foreground : Color::Default;
    }

    std::string color = color_str;
    // 移除空格
    color.erase(std::remove(color.begin(), color.end(), ' '), color.end());

    // 转换为小写
    std::transform(color.begin(), color.end(), color.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    // 检查是否是颜色名称
    const auto& color_map = getColorMap();
    auto it = color_map.find(color);
    if (it != color_map.end()) {
        return it->second;
    }

    // 检查是否是十六进制颜色 #RRGGBB
    if (color.size() == 7 && color[0] == '#') {
        try {
            int r = std::stoi(color.substr(1, 2), nullptr, 16);
            int g = std::stoi(color.substr(3, 2), nullptr, 16);
            int b = std::stoi(color.substr(5, 2), nullptr, 16);
            return Color::RGB(r, g, b);
        } catch (...) {
            // 解析失败，继续尝试其他格式
        }
    }

    // 检查是否是 RGB 格式 "R,G,B"
    if (color.find(',') != std::string::npos) {
        std::istringstream iss(color);
        std::string token;
        std::vector<int> values;

        while (std::getline(iss, token, ',')) {
            try {
                int value = std::stoi(token);
                if (value < 0)
                    value = 0;
                if (value > 255)
                    value = 255;
                values.push_back(value);
            } catch (...) {
                return theme_ ? theme_->getColors().foreground : Color::Default;
            }
        }

        if (values.size() >= 3) {
            return Color::RGB(values[0], values[1], values[2]);
        }
    }

    // 默认返回主题前景色
    return theme_ ? theme_->getColors().foreground : Color::Default;
}

// 应用颜色装饰器到元素
Element PopupManager::applyColorDecorators(
    Element elem, const std::map<std::string, std::string>& color_config) const {
    if (color_config.empty()) {
        return elem;
    }

    auto it = color_config.find("fg");
    if (it != color_config.end()) {
        elem = elem | color(parseColor(it->second));
    }

    it = color_config.find("bg");
    if (it != color_config.end()) {
        elem = elem | bgcolor(parseColor(it->second));
    }

    it = color_config.find("bold");
    if (it != color_config.end() && it->second == "true") {
        elem = elem | bold;
    }

    it = color_config.find("underlined");
    if (it != color_config.end() && it->second == "true") {
        elem = elem | underlined;
    }

    it = color_config.find("dim");
    if (it != color_config.end() && it->second == "true") {
        elem = elem | dim;
    }

    it = color_config.find("inverted");
    if (it != color_config.end() && it->second == "true") {
        elem = elem | inverted;
    }

    return elem;
}

namespace {

// 从查询字符串中提取搜索文本（去除前导的提示符和尾部的光标）
std::string extractSearchText(const std::string& input_line) {
    std::string text = input_line;

    // 移除前导的提示符："> " 或 "❯ " (注意：❯ 是 UTF-8 多字节字符)
    if (!text.empty()) {
        // 检查是否以 "> " 开头
        if (text.size() >= 2 && text.substr(0, 2) == "> ") {
            text = text.substr(2);
        }
        // 检查是否以 "❯ " 开头（❯ 是 3 字节 UTF-8 字符，加上空格共 4 字节）
        else if (text.size() >= 4 && text.substr(0, 4) == "❯ ") {
            text = text.substr(4);
        }
    }

    // 去除首尾空格
    size_t start = text.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = text.find_last_not_of(" \t");
    text = text.substr(start, end - start + 1);

    // 移除尾部的非字母数字字符（如光标符号 █ 等）
    while (!text.empty() && !std::isalnum(static_cast<unsigned char>(text.back()))) {
        text.pop_back();
    }

    return text;
}

// 在文本中查找并高亮搜索词
Element highlightSearchInText(const std::string& text, const std::string& search_text,
                              const Color& normal_color, const Color& highlight_color) {
    if (search_text.empty()) {
        return ::ftxui::text(text) | color(normal_color);
    }

    // 查找搜索词在文本中的位置
    size_t pos = text.find(search_text);
    if (pos == std::string::npos) {
        return ::ftxui::text(text) | color(normal_color);
    }

    Elements result;

    // 添加搜索词前的文本
    if (pos > 0) {
        result.push_back(::ftxui::text(text.substr(0, pos)) | color(normal_color));
    }

    // 添加高亮的搜索词
    result.push_back(::ftxui::text(search_text) | color(highlight_color) | bold);

    // 添加搜索词后的文本
    size_t end_pos = pos + search_text.length();
    if (end_pos < text.size()) {
        result.push_back(::ftxui::text(text.substr(end_pos)) | color(normal_color));
    }

    // 组合所有元素
    if (result.empty()) {
        return ::ftxui::text("");
    } else if (result.size() == 1) {
        return result[0];
    } else {
        return hbox(std::move(result));
    }
}

// 在文本中查找并高亮搜索词，同时为当前行标记（▶）设置特殊颜色
Element highlightSearchInTextWithMarker(const std::string& text, const std::string& search_text,
                                        const Color& normal_color, const Color& highlight_color,
                                        const Color& marker_color) {
    if (search_text.empty()) {
        // 没有搜索词时，检查是否有当前行标记
        size_t marker_pos = text.find("▶");
        if (marker_pos != std::string::npos) {
            Elements result;
            // 标记前的文本
            if (marker_pos > 0) {
                result.push_back(::ftxui::text(text.substr(0, marker_pos)) | color(normal_color));
            }
            // 当前行标记使用特殊颜色
            result.push_back(::ftxui::text("▶") | color(marker_color) | bold);
            // 标记后的文本
            if (marker_pos + 1 < text.size()) {
                result.push_back(::ftxui::text(text.substr(marker_pos + 1)) | color(normal_color));
            }
            if (result.size() == 1) {
                return result[0];
            } else {
                return hbox(std::move(result));
            }
        }
        return ::ftxui::text(text) | color(normal_color);
    }

    // 有搜索词时，需要同时处理搜索词高亮和标记颜色
    Elements result;
    size_t search_pos = text.find(search_text);
    size_t marker_pos = text.find("▶");

    // 按位置顺序处理所有需要特殊样式的部分
    std::vector<std::pair<size_t, std::string>> special_ranges;

    // 添加搜索词范围
    if (search_pos != std::string::npos) {
        special_ranges.push_back({search_pos, "search"});
    }

    // 添加标记范围
    if (marker_pos != std::string::npos) {
        special_ranges.push_back({marker_pos, "marker"});
    }

    // 按位置排序
    std::sort(special_ranges.begin(), special_ranges.end());

    size_t current_pos = 0;
    for (const auto& range : special_ranges) {
        size_t start = range.first;
        const std::string& type = range.second;

        // 添加特殊部分前的普通文本
        if (start > current_pos) {
            result.push_back(::ftxui::text(text.substr(current_pos, start - current_pos)) |
                             color(normal_color));
        }

        // 添加特殊部分
        if (type == "marker") {
            result.push_back(::ftxui::text("▶") | color(marker_color) | bold);
            current_pos = start + 1;
        } else if (type == "search") {
            size_t search_len = search_text.length();
            result.push_back(::ftxui::text(search_text) | color(highlight_color) | bold);
            current_pos = start + search_len;
        }
    }

    // 添加剩余的普通文本
    if (current_pos < text.size()) {
        result.push_back(::ftxui::text(text.substr(current_pos)) | color(normal_color));
    }

    // 组合所有元素
    if (result.empty()) {
        return ::ftxui::text("");
    } else if (result.size() == 1) {
        return result[0];
    } else {
        return hbox(std::move(result));
    }
}

void collectFocusableIds(const WidgetSpec& node, std::vector<std::string>& ids) {
    if (node.focusable && !node.id.empty()) {
        ids.push_back(node.id);
    }
    for (const auto& child : node.children) {
        collectFocusableIds(child, ids);
    }
}
} // namespace

PopupManager::PopupManager(::pnana::ui::Theme* theme) : theme_(theme), next_handle_(1) {}

PopupHandle PopupManager::openPopup(const PopupSpec& spec, PopupCallbacks callbacks) {
    PopupState state;
    state.handle = next_handle_++;
    state.spec = spec;
    state.callbacks = std::move(callbacks);
    state.z_index = static_cast<int>(z_order_.size());
    state.live_input_value = spec.default_value;
    state.selected_index = 0;
    state.focus_index = 0;

    popups_[state.handle] = std::move(state);
    z_order_.push_back(state.handle);
    rebuildFocusChain(popups_[state.handle]);
    return state.handle;
}

bool PopupManager::updatePopup(PopupHandle handle, const PopupSpec& patch) {
    auto it = popups_.find(handle);
    if (it == popups_.end()) {
        return false;
    }

    if (!patch.title.empty())
        it->second.spec.title = patch.title;
    if (!patch.message.empty())
        it->second.spec.message = patch.message;
    if (!patch.prompt.empty())
        it->second.spec.prompt = patch.prompt;
    if (!patch.default_value.empty()) {
        it->second.spec.default_value = patch.default_value;
        it->second.live_input_value = patch.default_value;
    }
    if (!patch.items.empty())
        it->second.spec.items = patch.items;
    if (patch.width > 0)
        it->second.spec.width = patch.width;
    if (patch.height > 0)
        it->second.spec.height = patch.height;

    if (!patch.root.children.empty() || patch.root.type != WidgetType::TEXT) {
        it->second.spec.root = patch.root;
    }

    if (patch.component_mode) {
        it->second.spec.component_mode = true;
        it->second.spec.component_lines = patch.component_lines;
        if (!patch.component_input_line.empty()) {
            it->second.spec.component_input_line = patch.component_input_line;
        }
        if (!patch.component_left_title.empty()) {
            it->second.spec.component_left_title = patch.component_left_title;
        }
        if (!patch.component_right_title.empty()) {
            it->second.spec.component_right_title = patch.component_right_title;
        }
        if (!patch.component_left_lines.empty()) {
            it->second.spec.component_left_lines = patch.component_left_lines;
        }
        if (!patch.component_right_lines.empty()) {
            it->second.spec.component_right_lines = patch.component_right_lines;
        }
        if (!patch.component_help_lines.empty()) {
            it->second.spec.component_help_lines = patch.component_help_lines;
        }
        // 更新颜色配置
        if (!patch.component_left_line_colors.empty()) {
            it->second.spec.component_left_line_colors = patch.component_left_line_colors;
        }
        if (!patch.component_right_line_colors.empty()) {
            it->second.spec.component_right_line_colors = patch.component_right_line_colors;
        }
    }

    // 样式 token 增量覆盖
    for (const auto& [k, v] : patch.style_tokens) {
        it->second.spec.style_tokens[k] = v;
    }

    rebuildFocusChain(it->second);
    return true;
}

bool PopupManager::closePopup(PopupHandle handle, bool accepted, const std::string& value,
                              std::size_t selected) {
    auto it = popups_.find(handle);
    if (it == popups_.end()) {
        return false;
    }

    PopupCallbacks callbacks = it->second.callbacks;
    popups_.erase(it);
    z_order_.erase(std::remove(z_order_.begin(), z_order_.end(), handle), z_order_.end());

    if (callbacks.on_input_result) {
        callbacks.on_input_result(accepted, value);
    }
    if (callbacks.on_bool_result) {
        callbacks.on_bool_result(accepted);
    }
    if (callbacks.on_select_result) {
        callbacks.on_select_result(accepted, selected);
    }

    return true;
}

bool PopupManager::isVisible() const {
    return !z_order_.empty();
}

bool PopupManager::hasPopup(PopupHandle handle) const {
    return popups_.find(handle) != popups_.end();
}

bool PopupManager::bringToFront(PopupHandle handle) {
    auto it = popups_.find(handle);
    if (it == popups_.end()) {
        return false;
    }

    // 从当前位置移除
    z_order_.erase(std::remove(z_order_.begin(), z_order_.end(), handle), z_order_.end());
    // 添加到最前面
    z_order_.push_back(handle);
    // 更新 z-index
    it->second.z_index = static_cast<int>(z_order_.size()) - 1;

    return true;
}

std::optional<std::reference_wrapper<PopupManager::PopupState>> PopupManager::topPopup() {
    if (z_order_.empty()) {
        return std::nullopt;
    }
    PopupHandle top = z_order_.back();
    auto it = popups_.find(top);
    if (it == popups_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::reference_wrapper<const PopupManager::PopupState>> PopupManager::topPopup()
    const {
    if (z_order_.empty()) {
        return std::nullopt;
    }
    PopupHandle top = z_order_.back();
    auto it = popups_.find(top);
    if (it == popups_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<std::reference_wrapper<PopupManager::PopupState>> PopupManager::getPopup(
    PopupHandle handle) {
    auto it = popups_.find(handle);
    if (it == popups_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void PopupManager::rebuildFocusChain(PopupState& state) {
    state.focus_chain.clear();

    WidgetSpec root = state.spec.root;
    if (root.children.empty()) {
        root = buildDefaultWidgetTree(state);
    }

    collectFocusableIds(root, state.focus_chain);
    if (state.focus_chain.empty()) {
        state.focus_index = 0;
    } else if (state.focus_index >= static_cast<int>(state.focus_chain.size())) {
        state.focus_index = 0;
    }
}

void PopupManager::focusNext(PopupState& state, bool backwards) {
    if (state.focus_chain.empty()) {
        return;
    }

    if (backwards) {
        state.focus_index = (state.focus_index - 1 + static_cast<int>(state.focus_chain.size())) %
                            static_cast<int>(state.focus_chain.size());
    } else {
        state.focus_index = (state.focus_index + 1) % static_cast<int>(state.focus_chain.size());
    }
}

bool PopupManager::dispatchAction(PopupState& state, const WidgetSpec* focused_widget) {
    if (focused_widget && state.callbacks.on_widget_action && !focused_widget->id.empty()) {
        state.callbacks.on_widget_action(focused_widget->id + ":press");
    }

    if (!state.spec.items.empty()) {
        return closePopup(state.handle, true, state.live_input_value,
                          static_cast<std::size_t>(state.selected_index + 1));
    }
    return closePopup(state.handle, true, state.live_input_value, 0);
}

WidgetSpec PopupManager::buildDefaultWidgetTree(const PopupState& state) const {
    WidgetSpec root;
    root.type = WidgetType::WINDOW;
    root.id = "window";
    root.label = state.spec.title;

    if (!state.spec.message.empty()) {
        WidgetSpec message;
        message.type = WidgetType::TEXT;
        message.id = "message";
        message.label = state.spec.message;
        root.children.push_back(message);
    }

    if (!state.spec.prompt.empty()) {
        WidgetSpec input;
        input.type = WidgetType::INPUT;
        input.id = "input";
        input.label = state.spec.prompt;
        input.value = state.live_input_value;
        input.focusable = true;
        root.children.push_back(input);
    }

    if (!state.spec.items.empty()) {
        WidgetSpec list;
        list.type = WidgetType::LIST;
        list.id = "list";
        list.items = state.spec.items;
        list.selected_index = state.selected_index;
        list.focusable = true;
        root.children.push_back(list);
    }

    WidgetSpec ok_button;
    ok_button.type = WidgetType::BUTTON;
    ok_button.id = "ok";
    ok_button.label = "OK";
    ok_button.focusable = true;
    root.children.push_back(ok_button);

    WidgetSpec cancel_button;
    cancel_button.type = WidgetType::BUTTON;
    cancel_button.id = "cancel";
    cancel_button.label = "Cancel";
    cancel_button.focusable = true;
    root.children.push_back(cancel_button);

    return root;
}

const WidgetSpec* PopupManager::findWidgetById(const WidgetSpec& root,
                                               const std::string& id) const {
    if (root.id == id) {
        return &root;
    }
    for (const auto& child : root.children) {
        if (const WidgetSpec* found = findWidgetById(child, id)) {
            return found;
        }
    }
    return nullptr;
}

Element PopupManager::renderWidgetTree(const PopupState& state, const LayoutNode& node) const {
    Color fg = Color::White;
    Color bg = Color::Black;
    Color border_color = Color::GrayDark;
    Color focus_bg = Color::Blue;

    if (theme_) {
        const auto& c = theme_->getColors();
        fg = c.foreground;
        bg = c.background;
        border_color = c.dialog_border;
    }

    auto is_focused = [&state](const std::string& id) {
        return !state.focus_chain.empty() && state.focus_index >= 0 &&
               state.focus_index < static_cast<int>(state.focus_chain.size()) &&
               state.focus_chain[state.focus_index] == id;
    };

    switch (node.widget.type) {
        case WidgetType::WINDOW: {
            Elements content;
            content.push_back(text(" " + node.widget.label) | bold | color(fg));
            content.push_back(separator());
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | bgcolor(bg) | border | color(border_color);
        }
        case WidgetType::TEXT:
            return text(node.widget.label) | color(fg);
        case WidgetType::INPUT: {
            std::string value = state.live_input_value.empty() ? " " : state.live_input_value;
            Element e =
                vbox({text(node.widget.label) | color(fg), text(value) | color(Color::White)});
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg);
            } else {
                e = e | bgcolor(Color::GrayDark);
            }
            return e;
        }
        case WidgetType::BUTTON: {
            std::string caption = "[ " + node.widget.label + " ]";
            Element e = text(caption) | color(fg);
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg) | bold;
            }
            return e;
        }
        case WidgetType::LIST: {
            Elements rows;
            int start = std::max(0, node.widget.scroll_offset);
            int end =
                std::min(static_cast<int>(node.widget.items.size()), start + node.rect.height);
            for (int i = start; i < end; ++i) {
                bool selected = i == state.selected_index;
                Element row = text((selected ? "> " : "  ") + node.widget.items[i]);
                if (selected) {
                    row = row | bgcolor(Color::GrayDark) | color(Color::White);
                } else {
                    row = row | color(fg);
                }
                rows.push_back(row);
            }
            Element list_box = vbox(std::move(rows));
            if (is_focused(node.widget.id)) {
                list_box = list_box | bgcolor(focus_bg);
            }
            return list_box;
        }
        // 基础显示组件
        case WidgetType::PARAGRAPH: {
            // 自动换行段落 - 简化实现
            return paragraph(node.widget.label) | color(fg);
        }
        case WidgetType::SEPARATOR: {
            return separator() | color(border_color);
        }
        case WidgetType::CANVAS: {
            // 2D 绘图画布 - 简化实现
            return text("[canvas]") | color(fg);
        }
        case WidgetType::SPINNER: {
            // 加载动画 - 简化实现
            static const char* spinners[] = {"|", "/", "-", "\\"};
            static int frame = 0;
            return text("[" + std::string(spinners[frame++ % 4]) + "]") | color(fg);
        }
        case WidgetType::IMAGE: {
            // 终端图片 - 简化实现
            return text("[image: " + node.widget.label + "]") | color(fg);
        }
        case WidgetType::ANIMATION: {
            // 自定义动画 - 简化实现
            return text("[animation]") | color(fg);
        }
        case WidgetType::BULLET: {
            return text("• " + node.widget.label) | color(fg);
        }
        case WidgetType::LINK: {
            // 超链接样式 - 使用下划线
            return text(node.widget.label) | underlined | color(Color::Blue);
        }

        // 基础交互组件
        case WidgetType::TEXTAREA: {
            // 多行文本输入 - 简化实现
            std::string value = node.widget.value.empty() ? " " : node.widget.value;
            Element e = vbox({text(node.widget.label) | color(fg),
                              text(value) | color(Color::White) | bgcolor(Color::GrayDark)});
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg);
            }
            return e;
        }
        case WidgetType::CHECKBOX: {
            std::string check = node.widget.selected_index > 0 ? "[X]" : "[ ]";
            Element e = text(check + " " + node.widget.label) | color(fg);
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg);
            }
            return e;
        }
        case WidgetType::RADIOBOX: {
            std::string radio = node.widget.selected_index > 0 ? "(X)" : "( )";
            Element e = text(radio + " " + node.widget.label) | color(fg);
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg);
            }
            return e;
        }
        case WidgetType::TOGGLE: {
            std::string toggle = node.widget.selected_index > 0 ? "[ON]" : "[OFF]";
            Element e = text(node.widget.label + ": " + toggle) | color(fg);
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg);
            }
            return e;
        }
        case WidgetType::SLIDER: {
            // 滑动条 - 简化实现
            int value = node.widget.selected_index;
            int max = std::max(1, static_cast<int>(node.widget.items.size()));
            int filled = (value * 20) / max;
            std::string bar = "[" + std::string(filled, '=') + std::string(20 - filled, '-') + "]";
            Element e = text(node.widget.label + ": " + bar) | color(fg);
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg);
            }
            return e;
        }
        case WidgetType::DROPDOWN: {
            // 下拉选择框 - 简化实现
            std::string selected =
                node.widget.selected_index >= 0 &&
                        node.widget.selected_index < static_cast<int>(node.widget.items.size())
                    ? node.widget.items[node.widget.selected_index]
                    : "Select...";
            Element e = text(node.widget.label + ": [" + selected + " ▼]") | color(fg);
            if (is_focused(node.widget.id)) {
                e = e | bgcolor(focus_bg);
            }
            return e;
        }
        case WidgetType::MENU: {
            // 列表菜单 - 类似 LIST 但不同样式
            Elements rows;
            int start = std::max(0, node.widget.scroll_offset);
            int end =
                std::min(static_cast<int>(node.widget.items.size()), start + node.rect.height);
            for (int i = start; i < end; ++i) {
                bool selected = i == state.selected_index;
                std::string prefix = selected ? "→ " : "  ";
                Element row = text(prefix + node.widget.items[i]);
                if (selected) {
                    row = row | bgcolor(focus_bg) | color(Color::White) | bold;
                } else {
                    row = row | color(fg);
                }
                rows.push_back(row);
            }
            return vbox(std::move(rows));
        }
        case WidgetType::COLOR_PICKER: {
            // 颜色选择器 - 简化实现
            return text("[color picker: " + node.widget.value + "]") | color(fg);
        }
        case WidgetType::FILE_PICKER: {
            // 文件选择器 - 简化实现
            return text("[file: " + node.widget.value + "]") | color(fg);
        }
        case WidgetType::GAUGE: {
            // 进度条/仪表盘
            int value = node.widget.selected_index;
            int max = std::max(1, static_cast<int>(node.widget.items.size()));
            int percent = (value * 100) / max;
            int filled = (percent * 20) / 100;
            std::string bar = "[" + std::string(filled, '#') + std::string(20 - filled, '-') + "]";
            return text(node.widget.label + " " + bar + " " + std::to_string(percent) + "%") |
                   color(fg);
        }

        // 容器组件
        case WidgetType::GROUP: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | border | color(border_color);
        }
        case WidgetType::DBOX: {
            // 深度层叠容器 - 简化实现
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return dbox(std::move(content)) | color(fg);
        }
        case WidgetType::RESIZABLE_SPLIT: {
            // 可拖动分割面板 - 简化实现
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return hbox(std::move(content)) | color(fg);
        }
        case WidgetType::GRID: {
            // 网格布局 - 简化实现
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | color(fg);
        }
        case WidgetType::FRAME: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | frame | color(fg);
        }
        case WidgetType::YFRAME: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | yframe | color(fg);
        }
        case WidgetType::XFRAME: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return hbox(std::move(content)) | xframe | color(fg);
        }
        case WidgetType::VSCROLL: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | vscroll_indicator | color(fg);
        }
        case WidgetType::HSCROLL: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return hbox(std::move(content)) | hscroll_indicator | color(fg);
        }

        // 弹窗/模态组件
        case WidgetType::MODAL: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | bgcolor(Color::RGBA(0, 0, 0, 180)) | color(fg);
        }
        case WidgetType::POPUP: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | border | color(border_color);
        }
        case WidgetType::NOTIFICATION: {
            return text("🔔 " + node.widget.label) | border | color(fg);
        }

        // 原有组件兼容
        case WidgetType::SCROLL: {
            Elements rows;
            rows.push_back(text("[scroll]"));
            return vbox(std::move(rows)) | color(fg);
        }
        // 新增布局容器类型
        case WidgetType::CONTAINER: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            Element container = vbox(std::move(content));
            if (node.widget.border_style != "none") {
                container = container | border;
            }
            return container | color(fg);
        }
        case WidgetType::HBOX: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            Element hbox_elem = hbox(std::move(content));
            if (node.widget.spacing > 0) {
                hbox_elem = hbox_elem | xflex;
            }
            if (node.widget.border_style != "none") {
                hbox_elem = hbox_elem | border;
            }
            return hbox_elem | color(fg);
        }
        case WidgetType::VBOX: {
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            Element vbox_elem = vbox(std::move(content));
            if (node.widget.border_style != "none") {
                vbox_elem = vbox_elem | border;
            }
            return vbox_elem | color(fg);
        }
        case WidgetType::SPLIT: {
            // 分割布局 - 简化实现
            Elements content;
            for (const auto& child : node.children) {
                content.push_back(renderWidgetTree(state, child));
            }
            return vbox(std::move(content)) | color(fg);
        }
        case WidgetType::TABS: {
            // 标签页 - 简化实现，只显示第一个子元素
            if (!node.children.empty()) {
                return renderWidgetTree(state, node.children[0]);
            }
            return text("[tabs]") | color(fg);
        }
    }

    return text("");
}

Element PopupManager::renderPopupLayer(const PopupState& state, int screen_w, int screen_h) const {
    auto rect =
        layout_engine_.computeCenteredRect(screen_w, screen_h, state.spec.width, state.spec.height);

    if (state.spec.component_mode) {
        Color fg = Color::White;
        Color bg = Color::Black;
        Color border_color = Color::GrayDark;
        Color selection_bg = Color::GrayDark;
        Color help_fg = Color::GrayLight;

        if (theme_) {
            const auto& c = theme_->getColors();
            fg = c.foreground;
            bg = c.dialog_bg;
            border_color = c.dialog_border;
            selection_bg = c.selection;
            help_fg = c.helpbar_fg;
        }

        auto get_style = [&](const std::string& key) -> std::string {
            auto it = state.spec.style_tokens.find(key);
            return it == state.spec.style_tokens.end() ? std::string("") : it->second;
        };

        if (!get_style("popup.fg").empty()) {
            fg = parseColor(get_style("popup.fg"));
        }
        if (!get_style("popup.bg").empty()) {
            bg = parseColor(get_style("popup.bg"));
        }
        if (!get_style("popup.border").empty()) {
            border_color = parseColor(get_style("popup.border"));
        }
        if (!get_style("popup.selection_bg").empty()) {
            selection_bg = parseColor(get_style("popup.selection_bg"));
        }
        if (!get_style("popup.help_fg").empty()) {
            help_fg = parseColor(get_style("popup.help_fg"));
        }

        Elements left_rows;
        for (size_t i = 0; i < state.spec.component_left_lines.size(); i++) {
            const auto& line = state.spec.component_left_lines[i];
            Element line_elem = text(line);

            // 检查是否有颜色配置
            if (i < state.spec.component_left_line_colors.size()) {
                const auto& colors = state.spec.component_left_line_colors[i];
                line_elem = applyColorDecorators(line_elem, colors);
            } else {
                // 没有颜色配置时使用默认逻辑
                if (line.rfind(">>", 0) == 0) {
                    line_elem = text(line.substr(2)) | bgcolor(selection_bg) | color(fg) | bold;
                } else {
                    line_elem = text(line) | color(fg);
                }
            }

            left_rows.push_back(line_elem);
        }
        if (left_rows.empty()) {
            left_rows.push_back(text(" ") | color(fg));
        }

        Elements right_rows;
        // 使用主题中的 warning 颜色高亮匹配文本（如果没有主题，使用默认黄色）
        Color highlight_color = theme_ ? theme_->getColors().warning : Color::RGB(255, 200, 0);
        // 使用主题中的 success 颜色高亮当前行标记（如果没有主题，使用默认绿色）
        Color current_line_marker_color =
            theme_ ? theme_->getColors().success : Color::RGB(100, 200, 100);

        // 从输入行中提取搜索文本
        std::string search_text = extractSearchText(state.spec.component_input_line);

        for (size_t i = 0; i < state.spec.component_right_lines.size(); i++) {
            const auto& line = state.spec.component_right_lines[i];

            // 检查是否有颜色配置
            if (i < state.spec.component_right_line_colors.size()) {
                const auto& colors = state.spec.component_right_line_colors[i];

                // 从颜色配置中提取前景色用于搜索高亮
                Color line_color = fg; // 默认使用前景色
                auto fg_it = colors.find("fg");
                if (fg_it != colors.end()) {
                    line_color = parseColor(fg_it->second);
                }

                // 检查是否包含当前行标记 ▶ (U+25B6)
                bool is_current_line = line.find("▶") != std::string::npos;

                Element line_elem;
                if (is_current_line) {
                    // 当前行：使用颜色配置中的前景色，同时高亮搜索词和标记
                    line_elem = highlightSearchInTextWithMarker(
                        line, search_text, line_color, highlight_color, current_line_marker_color);
                    // 应用其他颜色配置（如 bg、bold、dim 等）
                    line_elem = applyColorDecorators(line_elem, colors);
                } else {
                    // 非当前行：使用颜色配置中的前景色高亮搜索词
                    line_elem =
                        highlightSearchInText(line, search_text, line_color, highlight_color);
                    // 应用其他颜色配置（如 bg、bold、dim 等，但不包括 fg）
                    std::map<std::string, std::string> other_colors = colors;
                    other_colors.erase("fg"); // 移除 fg，因为已经在 highlightSearchInText 中应用了
                    line_elem = applyColorDecorators(line_elem, other_colors);
                }
                right_rows.push_back(line_elem);
            } else {
                // 没有颜色配置时使用默认逻辑
                // 检查是否包含当前行标记 ▶ (U+25B6)
                bool is_current_line = line.find("▶") != std::string::npos;

                if (is_current_line) {
                    // 对当前行：使用新函数同时处理搜索词高亮和标记颜色
                    right_rows.push_back(highlightSearchInTextWithMarker(
                        line, search_text, fg, highlight_color, current_line_marker_color));
                } else {
                    // 非当前行使用普通颜色
                    right_rows.push_back(
                        highlightSearchInText(line, search_text, fg, highlight_color) | dim);
                }
            }
        }
        if (right_rows.empty()) {
            right_rows.push_back(text(" ") | color(fg));
        }

        Elements help_rows;
        for (const auto& line : state.spec.component_help_lines) {
            help_rows.push_back(text(line) | color(help_fg) | dim);
        }

        Element input_line =
            text(state.spec.component_input_line.empty() ? std::string(" >")
                                                         : state.spec.component_input_line) |
            color(fg) | bold;

        const int input_h = 3;
        const int help_h = help_rows.empty() ? 0 : 4;
        const int body_h = std::max(3, rect.height - input_h - help_h - 2);

        Element left_panel = window(text(" " + state.spec.component_left_title + " ") | bold,
                                    vbox(std::move(left_rows)) | yframe | yflex) |
                             size(WIDTH, EQUAL, std::max(10, rect.width / 2 - 1)) |
                             size(HEIGHT, EQUAL, body_h) | color(border_color);

        Element right_panel = window(text(" " + state.spec.component_right_title + " ") | bold,
                                     vbox(std::move(right_rows)) | yframe | yflex) |
                              size(HEIGHT, EQUAL, body_h) | xflex | color(border_color);

        Elements layout_rows;
        layout_rows.push_back(
            (window(text(" Query ") | bold, input_line | xframe) | color(border_color)) |
            size(HEIGHT, EQUAL, input_h));
        layout_rows.push_back(hbox({left_panel, right_panel}) | size(HEIGHT, EQUAL, body_h));
        if (!help_rows.empty()) {
            layout_rows.push_back(
                (window(text(" Help ") | bold, vbox(std::move(help_rows))) | color(border_color)) |
                size(HEIGHT, EQUAL, help_h));
        }

        // 渲染标题（支持装饰器 + style token）
        Element title_elem = text(" " + state.spec.title + " ");

        bool title_bold = state.spec.window_title_decorators.bold;
        bool title_inverted = state.spec.window_title_decorators.inverted;
        bool title_dim = state.spec.window_title_decorators.dim;
        bool title_underlined = state.spec.window_title_decorators.underlined;
        std::string title_color = state.spec.window_title_decorators.color;

        auto bool_style_true = [&](const std::string& key) {
            auto it = state.spec.style_tokens.find(key);
            return it != state.spec.style_tokens.end() &&
                   (it->second == "true" || it->second == "1" || it->second == "yes");
        };
        auto style_value = [&](const std::string& key) -> std::string {
            auto it = state.spec.style_tokens.find(key);
            return it == state.spec.style_tokens.end() ? std::string("") : it->second;
        };

        if (bool_style_true("title.bold"))
            title_bold = true;
        if (bool_style_true("title.inverted"))
            title_inverted = true;
        if (bool_style_true("title.dim"))
            title_dim = true;
        if (bool_style_true("title.underlined"))
            title_underlined = true;
        if (!style_value("title.color").empty())
            title_color = style_value("title.color");

        if (title_bold) {
            title_elem = title_elem | bold;
        }
        if (title_inverted) {
            title_elem = title_elem | inverted;
        }
        if (title_dim) {
            title_elem = title_elem | dim;
        }
        if (title_underlined) {
            title_elem = title_elem | underlined;
        }
        if (!title_color.empty()) {
            title_elem = title_elem | color(parseColor(title_color));
        }
        return window(title_elem, vbox(std::move(layout_rows)) | bgcolor(bg) | yframe) |
               size(WIDTH, EQUAL, rect.width) | size(HEIGHT, EQUAL, rect.height) |
               color(border_color) | center;
    }

    WidgetSpec root = state.spec.root;
    if (root.children.empty()) {
        root = buildDefaultWidgetTree(state);
    }
    if (root.label.empty()) {
        root.label = state.spec.title;
    }

    auto tree = layout_engine_.buildLayoutTree(root, rect);
    return renderWidgetTree(state, tree) | size(WIDTH, EQUAL, rect.width) |
           size(HEIGHT, EQUAL, rect.height) | center;
}

bool PopupManager::handleInput(Event event) {
    auto top_opt = topPopup();
    if (!top_opt.has_value()) {
        return false;
    }

    PopupState& popup = top_opt->get();

    if (popup.spec.component_mode && popup.callbacks.on_component_event) {
        std::string event_name;
        std::string payload;

        // 基础事件映射（保持兼容性）
        if (event == Event::ArrowUp) {
            event_name = "arrow_up";
        } else if (event == Event::ArrowDown) {
            event_name = "arrow_down";
        } else if (event == Event::ArrowLeft) {
            event_name = "arrow_left";
        } else if (event == Event::ArrowRight) {
            event_name = "arrow_right";
        } else if (event == Event::PageUp) {
            event_name = "pageup";
        } else if (event == Event::PageDown) {
            event_name = "pagedown";
        } else if (event == Event::Home) {
            event_name = "home";
        } else if (event == Event::End) {
            event_name = "end";
        } else if (event == Event::Return) {
            event_name = "enter";
        } else if (event == Event::Escape) {
            event_name = "escape";
        } else if (event == Event::Backspace) {
            event_name = "backspace";
        } else if (event == Event::Delete) {
            event_name = "delete";
        } else if (event == Event::Insert) {
            event_name = "insert";
        } else if (event.is_character()) {
            event_name = "char";
            payload = event.character();
        } else {
            // 其他事件传递原始字符串，让 Lua 侧自行解析
            event_name = "raw";
            payload = event.input();
        }

        if (!event_name.empty()) {
            bool handled = popup.callbacks.on_component_event(event_name, payload);
            if (event_name == "escape") {
                closePopup(popup.handle, false, "", 0);
                return true;
            }
            return handled;
        }
    }

    if (event == Event::Tab) {
        focusNext(popup, false);
        return true;
    }

    if (event == Event::TabReverse) {
        focusNext(popup, true);
        return true;
    }

    if (event == Event::Escape) {
        return closePopup(popup.handle, false, "", 0);
    }

    if (event == Event::Return) {
        const WidgetSpec* focused_widget = nullptr;
        if (!popup.focus_chain.empty()) {
            const std::string& focused = popup.focus_chain[popup.focus_index];
            if (focused == "cancel") {
                return closePopup(popup.handle, false, "", 0);
            }
            if (focused == "ok") {
                return dispatchAction(popup, nullptr);
            }
            focused_widget = findWidgetById(popup.spec.root, focused);
        }
        return dispatchAction(popup, focused_widget);
    }

    if (!popup.spec.items.empty()) {
        if (event == Event::ArrowUp) {
            popup.selected_index = std::max(0, popup.selected_index - 1);
            return true;
        }
        if (event == Event::ArrowDown) {
            popup.selected_index =
                std::min(static_cast<int>(popup.spec.items.size()) - 1, popup.selected_index + 1);
            return true;
        }
    }

    if (event == Event::Backspace) {
        if (!popup.live_input_value.empty()) {
            popup.live_input_value.pop_back();
        }
        return true;
    }

    if (event.is_character()) {
        popup.live_input_value += event.character();
        return true;
    }

    return true;
}

Element PopupManager::render(Element base_ui, int screen_w, int screen_h) {
    if (z_order_.empty()) {
        return base_ui;
    }

    Element combined = base_ui;

    bool has_modal = false;
    for (PopupHandle handle : z_order_) {
        auto it = popups_.find(handle);
        if (it != popups_.end() && it->second.spec.modal) {
            has_modal = true;
            break;
        }
    }
    if (has_modal) {
        combined = dbox({combined, filler() | bgcolor(Color::RGBA(0, 0, 0, 100))});
    }

    std::vector<PopupHandle> ordered = z_order_;
    std::sort(ordered.begin(), ordered.end(), [this](PopupHandle a, PopupHandle b) {
        const auto ia = popups_.find(a);
        const auto ib = popups_.find(b);
        if (ia == popups_.end() || ib == popups_.end()) {
            return a < b;
        }
        if (ia->second.z_index == ib->second.z_index) {
            return ia->second.handle < ib->second.handle;
        }
        return ia->second.z_index < ib->second.z_index;
    });

    for (PopupHandle handle : ordered) {
        auto it = popups_.find(handle);
        if (it == popups_.end()) {
            continue;
        }
        combined = dbox({combined, renderPopupLayer(it->second, screen_w, screen_h)});
    }

    return combined;
}

PopupHandle PopupManager::openInput(const std::string& title, const std::string& prompt,
                                    const std::string& default_value,
                                    std::function<void(bool, const std::string&)> on_result) {
    PopupSpec spec;
    spec.title = title;
    spec.prompt = prompt;
    spec.default_value = default_value;

    PopupCallbacks cb;
    cb.on_input_result = std::move(on_result);
    return openPopup(spec, std::move(cb));
}

PopupHandle PopupManager::openConfirm(const std::string& title, const std::string& message,
                                      std::function<void(bool)> on_result) {
    PopupSpec spec;
    spec.title = title;
    spec.message = message;

    PopupCallbacks cb;
    cb.on_bool_result = std::move(on_result);
    return openPopup(spec, std::move(cb));
}

PopupHandle PopupManager::openSelect(const std::string& title, const std::string& prompt,
                                     const std::vector<std::string>& items,
                                     std::function<void(bool, std::size_t)> on_result) {
    PopupSpec spec;
    spec.title = title;
    spec.prompt = prompt;
    spec.items = items;

    PopupCallbacks cb;
    cb.on_select_result = std::move(on_result);
    return openPopup(spec, std::move(cb));
}

} // namespace ui
} // namespace core
} // namespace pnana
