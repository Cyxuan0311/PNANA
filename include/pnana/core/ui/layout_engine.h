#ifndef PNANA_CORE_UI_LAYOUT_ENGINE_H
#define PNANA_CORE_UI_LAYOUT_ENGINE_H

#include "core/ui/widget.h"
#include <cstddef>
#include <vector>

namespace pnana {
namespace core {
namespace ui {

struct LayoutRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct LayoutNode {
    WidgetSpec widget;
    LayoutRect rect;
    std::vector<LayoutNode> children;
};

class LayoutEngine {
  public:
    LayoutEngine() = default;
    ~LayoutEngine() = default;

    LayoutRect computeCenteredRect(int screen_w, int screen_h, int desired_w, int desired_h) const;
    std::size_t estimateContentHeight(const WidgetSpec& root) const;

    LayoutNode buildLayoutTree(const WidgetSpec& root, const LayoutRect& bounds) const;

  private:
    void layoutChildren(LayoutNode& node) const;
    void layoutHorizontal(LayoutNode& node, int x, int y, int w, int h) const;
    void layoutVertical(LayoutNode& node, int x, int y, int w, int h) const;
    int estimateWidgetHeight(const WidgetSpec& widget) const;
};

} // namespace ui
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_UI_LAYOUT_ENGINE_H
