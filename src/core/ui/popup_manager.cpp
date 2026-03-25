#include "core/ui/popup_manager.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace core {
namespace ui {

namespace {
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
    (void)focused_widget; // 暂时未使用
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
        if (!popup.focus_chain.empty()) {
            const std::string& focused = popup.focus_chain[popup.focus_index];
            if (focused == "cancel") {
                return closePopup(popup.handle, false, "", 0);
            }
            if (focused == "ok") {
                return dispatchAction(popup, nullptr);
            }
        }
        return dispatchAction(popup, nullptr);
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
