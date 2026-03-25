#include "core/ui/layout_engine.h"
#include <algorithm>

namespace pnana {
namespace core {
namespace ui {

LayoutRect LayoutEngine::computeCenteredRect(int screen_w, int screen_h, int desired_w,
                                             int desired_h) const {
    int width = std::max(10, std::min(desired_w, screen_w - 2));
    int height = std::max(6, std::min(desired_h, screen_h - 2));

    LayoutRect rect;
    rect.width = width;
    rect.height = height;
    rect.x = std::max(0, (screen_w - width) / 2);
    rect.y = std::max(0, (screen_h - height) / 2);
    return rect;
}

std::size_t LayoutEngine::estimateContentHeight(const WidgetSpec& root) const {
    std::size_t height = 0;

    switch (root.type) {
        // 容器组件
        case WidgetType::WINDOW:
        case WidgetType::CONTAINER:
        case WidgetType::GROUP:
        case WidgetType::HBOX:
        case WidgetType::VBOX:
        case WidgetType::DBOX:
        case WidgetType::SPLIT:
        case WidgetType::RESIZABLE_SPLIT:
        case WidgetType::TABS:
        case WidgetType::GRID:
        case WidgetType::FRAME:
        case WidgetType::YFRAME:
        case WidgetType::XFRAME:
        case WidgetType::VSCROLL:
        case WidgetType::HSCROLL:
        case WidgetType::MODAL:
        case WidgetType::POPUP:
        case WidgetType::NOTIFICATION:
            height += 2;
            break;
        // 基础显示组件
        case WidgetType::TEXT:
        case WidgetType::BULLET:
        case WidgetType::LINK:
        case WidgetType::SEPARATOR:
        case WidgetType::SPINNER:
            height += 1;
            break;
        case WidgetType::PARAGRAPH:
            height += 3;
            break;
        case WidgetType::CANVAS:
        case WidgetType::IMAGE:
        case WidgetType::ANIMATION:
            height += 5;
            break;
        // 基础交互组件
        case WidgetType::INPUT:
            height += 2;
            break;
        case WidgetType::TEXTAREA:
            height += 5;
            break;
        case WidgetType::BUTTON:
        case WidgetType::CHECKBOX:
        case WidgetType::RADIOBOX:
        case WidgetType::TOGGLE:
        case WidgetType::SLIDER:
        case WidgetType::DROPDOWN:
        case WidgetType::GAUGE:
            height += 1;
            break;
        case WidgetType::MENU:
        case WidgetType::LIST:
            height += std::max<std::size_t>(1, root.items.size());
            break;
        case WidgetType::SCROLL:
            height += std::max<std::size_t>(3, root.items.size());
            break;
        case WidgetType::COLOR_PICKER:
        case WidgetType::FILE_PICKER:
            height += 3;
            break;
    }

    for (const auto& child : root.children) {
        height += estimateContentHeight(child);
    }

    return height;
}

LayoutNode LayoutEngine::buildLayoutTree(const WidgetSpec& root, const LayoutRect& bounds) const {
    LayoutNode node;
    node.widget = root;
    node.rect = bounds;

    for (const auto& child : root.children) {
        LayoutNode c;
        c.widget = child;
        node.children.push_back(std::move(c));
    }

    layoutChildren(node);
    return node;
}

void LayoutEngine::layoutChildren(LayoutNode& node) const {
    if (node.children.empty()) {
        return;
    }

    // 应用内边距
    int padding = node.widget.padding;
    int x = node.rect.x + 1 + padding;
    int y = node.rect.y + 1 + padding;
    int w = std::max(1, node.rect.width - 2 - padding * 2);
    int h = std::max(1, node.rect.height - 2 - padding * 2);

    // 根据布局方向处理
    bool is_horizontal = (node.widget.layout_direction == LayoutDirection::HORIZONTAL) ||
                         (node.widget.type == WidgetType::HBOX);

    if (is_horizontal) {
        layoutHorizontal(node, x, y, w, h);
    } else {
        layoutVertical(node, x, y, w, h);
    }
}

void LayoutEngine::layoutHorizontal(LayoutNode& node, int x, int y, int w, int h) const {
    int spacing = node.widget.spacing;
    int child_count = static_cast<int>(node.children.size());
    if (child_count == 0)
        return;

    // 计算总间距
    int total_spacing = spacing * (child_count - 1);
    int available_width = std::max(1, w - total_spacing);

    // 计算 flex 总值
    int total_flex = 0;
    int fixed_width = 0;
    for (const auto& child : node.children) {
        if (child.widget.flex > 0) {
            total_flex += child.widget.flex;
        } else {
            int child_w = child.widget.min_width;
            if (child_w <= 0)
                child_w = available_width / child_count;
            fixed_width += child_w;
        }
    }

    int remaining_width = std::max(1, available_width - fixed_width);
    int current_x = x;

    for (auto& child : node.children) {
        int child_w;
        if (child.widget.flex > 0) {
            child_w = (remaining_width * child.widget.flex) / total_flex;
        } else {
            child_w = child.widget.min_width;
            if (child_w <= 0)
                child_w = available_width / child_count;
        }

        int child_h = h;
        if (child.widget.min_height > 0) {
            child_h = std::min(h, child.widget.min_height);
        }

        // 应用对齐
        int child_y = y;
        if (node.widget.alignment == Alignment::CENTER) {
            child_y = y + (h - child_h) / 2;
        } else if (node.widget.alignment == Alignment::END) {
            child_y = y + h - child_h;
        }

        child.rect = LayoutRect{current_x, child_y, child_w, child_h};
        current_x += child_w + spacing;

        layoutChildren(child);
    }
}

void LayoutEngine::layoutVertical(LayoutNode& node, int x, int y, int w, int h) const {
    int spacing = node.widget.spacing;
    int child_count = static_cast<int>(node.children.size());
    if (child_count == 0)
        return;

    // 计算总间距
    int total_spacing = spacing * (child_count - 1);
    int available_height = std::max(1, h - total_spacing);

    // 计算 flex 总值和固定高度
    int total_flex = 0;
    int fixed_height = 0;
    for (const auto& child : node.children) {
        if (child.widget.flex > 0) {
            total_flex += child.widget.flex;
        } else {
            int child_h = estimateWidgetHeight(child.widget);
            fixed_height += child_h;
        }
    }

    int remaining_height = std::max(1, available_height - fixed_height);
    int current_y = y;

    for (auto& child : node.children) {
        int child_h;
        if (child.widget.flex > 0) {
            child_h = (remaining_height * child.widget.flex) / total_flex;
        } else {
            child_h = estimateWidgetHeight(child.widget);
        }

        int child_w = w;
        if (child.widget.min_width > 0 && node.widget.alignment != Alignment::STRETCH) {
            child_w = std::min(w, child.widget.min_width);
        }

        // 应用对齐
        int child_x = x;
        if (node.widget.alignment == Alignment::CENTER) {
            child_x = x + (w - child_w) / 2;
        } else if (node.widget.alignment == Alignment::END) {
            child_x = x + w - child_w;
        }

        child.rect = LayoutRect{child_x, current_y, child_w, child_h};
        current_y += child_h + spacing;

        layoutChildren(child);
    }
}

int LayoutEngine::estimateWidgetHeight(const WidgetSpec& widget) const {
    if (widget.min_height > 0) {
        return widget.min_height;
    }

    switch (widget.type) {
        // 基础显示组件
        case WidgetType::TEXT:
        case WidgetType::BULLET:
        case WidgetType::LINK:
        case WidgetType::SEPARATOR:
            return 1;
        case WidgetType::PARAGRAPH:
            return 3; // 段落默认3行
        case WidgetType::CANVAS:
        case WidgetType::IMAGE:
        case WidgetType::ANIMATION:
            return 5; // 画布/图片默认5行
        case WidgetType::SPINNER:
            return 1;

        // 基础交互组件
        case WidgetType::INPUT:
            return 2;
        case WidgetType::TEXTAREA:
            return 5; // 多行文本域默认5行
        case WidgetType::BUTTON:
        case WidgetType::CHECKBOX:
        case WidgetType::RADIOBOX:
        case WidgetType::TOGGLE:
        case WidgetType::SLIDER:
        case WidgetType::DROPDOWN:
        case WidgetType::GAUGE:
            return 1;
        case WidgetType::MENU:
        case WidgetType::LIST:
        case WidgetType::SCROLL:
            return std::max(3, static_cast<int>(widget.items.size()));
        case WidgetType::COLOR_PICKER:
        case WidgetType::FILE_PICKER:
            return 3;

        // 容器组件
        case WidgetType::WINDOW:
        case WidgetType::CONTAINER:
        case WidgetType::GROUP:
        case WidgetType::VBOX:
        case WidgetType::HBOX:
        case WidgetType::DBOX:
        case WidgetType::SPLIT:
        case WidgetType::RESIZABLE_SPLIT:
        case WidgetType::TABS:
        case WidgetType::GRID:
        case WidgetType::FRAME:
        case WidgetType::YFRAME:
        case WidgetType::XFRAME:
        case WidgetType::VSCROLL:
        case WidgetType::HSCROLL:
            return 3; // 容器默认最小高度

        // 弹窗/模态组件
        case WidgetType::MODAL:
        case WidgetType::POPUP:
        case WidgetType::NOTIFICATION:
            return 3;

        default:
            return 1;
    }
}

} // namespace ui
} // namespace core
} // namespace pnana
