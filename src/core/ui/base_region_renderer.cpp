#include "core/ui/base_region_renderer.h"
#include "core/editor.h"
#include "core/ui/border_manager.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace core {
namespace ui {

ftxui::Element BaseRegionRenderer::renderWithBorder(Editor* editor, bool is_active) {
    ftxui::Element content = render(editor);

    // 使用 BorderManager 应用边框
    BorderManager border_manager;
    return border_manager.applyBorder(content, getRegionType(), is_active, editor->getTheme());
}

ftxui::Color BaseRegionRenderer::getBorderColor(bool is_active, Editor* editor) const {
    BorderManager border_manager;
    if (is_active) {
        return border_manager.getActiveBorderColor(editor->getTheme());
    } else {
        return border_manager.getInactiveBorderColor(editor->getTheme());
    }
}

} // namespace ui
} // namespace core
} // namespace pnana
