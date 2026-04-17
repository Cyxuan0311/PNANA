#ifndef PNANA_UI_COMPLETION_POPUP_H
#define PNANA_UI_COMPLETION_POPUP_H

#include "features/lsp/lsp_client.h"
#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

/**
 * 代码补全弹出窗口
 * 显示 LSP 返回的代码补全建议
 * 参考 VSCode 和 Neovim 的设计
 */
class CompletionPopup {
  public:
    CompletionPopup();

    // 显示补全列表
    // content_top/content_bottom 表示编辑区在屏幕中的垂直边界（含 tabbar/statusbar/helpbar 等
    // chrome）
    void show(const std::vector<features::CompletionItem>& items, int anchor_screen_y,
              int anchor_screen_x, int screen_width, int screen_height, int content_top = 0,
              int content_bottom = -1, const std::string& query = "");

    // 隐藏补全列表
    void hide();

    // 检查是否可见
    bool isVisible() const {
        return visible_;
    }

    // 选择下一个/上一个
    void selectNext();
    void selectPrevious();

    // 获取当前选中的补全项
    const features::CompletionItem* getSelectedItem() const;

    // 更新指定索引的补全项（用于 resolve 后刷新 detail/documentation）
    void updateItem(size_t index, const features::CompletionItem& item);

    // 获取选中的索引
    size_t getSelectedIndex() const {
        return selected_index_;
    }

    // 渲染补全列表（返回元素和位置信息）
    // content_origin_x/content_origin_y 为编辑区在屏幕中的原点偏移
    ftxui::Element render(const ui::Theme& theme, int content_origin_x = 0,
                          int content_origin_y = 0) const;

    // 获取弹窗位置（用于定位）
    int getPopupX() const {
        return popup_x_;
    }
    int getPopupY() const {
        return popup_y_;
    }
    int getPopupWidth() const {
        return popup_width_;
    }
    int getPopupHeight() const {
        return popup_height_;
    }

    // 应用选中的补全项（返回要插入的文本）
    std::string applySelected() const;

    // 设置最大显示项数
    void setMaxItems(size_t max) {
        max_items_ = max;
    }

    // 更新光标位置（用于重新计算弹窗位置）
    void updateCursorPosition(int row, int col, int screen_width, int screen_height);

  private:
    bool visible_;
    std::vector<features::CompletionItem> items_;
    std::string current_query_; // 当前的查询前缀，用于匹配高亮
    size_t selected_index_;
    size_t max_items_;
    int anchor_screen_y_;
    int anchor_screen_x_;
    int screen_width_;
    int screen_height_;
    int content_top_;
    int content_bottom_;

    // 弹窗位置和尺寸
    int popup_x_;
    int popup_y_;
    int popup_width_;
    int popup_height_;

    // 用于避免抖动的状态跟踪
    size_t last_items_size_;

    // 计算显示范围
    size_t getDisplayStart() const;
    size_t getDisplayEnd() const;

    // 计算弹窗位置
    void calculatePopupPosition();

    // 获取补全项类型图标
    std::string getKindIcon(const std::string& kind) const;

    // 获取补全项类型颜色
    ftxui::Color getKindColor(const std::string& kind) const;

    // 渲染单个补全项（带匹配高亮）
    ftxui::Element renderItem(const features::CompletionItem& item, bool is_selected,
                              const ui::Theme& theme, const std::string& query = "") const;

    // 渲染带高亮的标签文本
    ftxui::Element renderHighlightedLabel(const std::string& label, const std::string& query,
                                          bool is_selected, const ui::ThemeColors& colors) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_COMPLETION_POPUP_H
