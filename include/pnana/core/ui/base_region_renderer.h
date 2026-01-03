#ifndef PNANA_CORE_UI_BASE_REGION_RENDERER_H
#define PNANA_CORE_UI_BASE_REGION_RENDERER_H

#include "core/region_manager.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace core {

// 前向声明
class Editor;

namespace ui {

// 区域渲染器基类
class BaseRegionRenderer {
  public:
    virtual ~BaseRegionRenderer() = default;

    // 渲染区域内容
    virtual ftxui::Element render(Editor* editor) = 0;

    // 渲染带边框的区域（边框颜色根据是否激活变化）
    ftxui::Element renderWithBorder(Editor* editor, bool is_active);

    // 获取区域类型
    virtual EditorRegion getRegionType() const = 0;

  protected:
    // 获取边框颜色（激活时使用高亮色，非激活时使用默认色）
    ftxui::Color getBorderColor(bool is_active, Editor* editor) const;
};

} // namespace ui
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_UI_BASE_REGION_RENDERER_H
