#ifndef PNANA_UI_SYMBOL_NAVIGATION_POPUP_H
#define PNANA_UI_SYMBOL_NAVIGATION_POPUP_H

#include "features/lsp/lsp_types.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

class SymbolNavigationPopup {
  public:
    SymbolNavigationPopup(Theme& theme);

    // 设置符号列表
    void setSymbols(const std::vector<pnana::features::DocumentSymbol>& symbols);

    // 显示/隐藏弹窗
    void show();
    void hide();
    bool isVisible() const;

    // 导航
    void selectNext();
    void selectPrevious();
    void selectFirst();
    void selectLast();

    // 获取当前选中的符号
    const pnana::features::DocumentSymbol* getSelectedSymbol() const;

    // 设置跳转回调（用于预览跳转）
    void setJumpCallback(std::function<void(const pnana::features::DocumentSymbol&)> callback);

    // 输入处理
    bool handleInput(ftxui::Event event);

    // 渲染
    ftxui::Element render() const;

    // 获取符号数量
    size_t getSymbolCount() const;

  private:
    Theme& theme_;
    std::vector<pnana::features::DocumentSymbol> symbols_;
    std::vector<pnana::features::DocumentSymbol> flattened_symbols_; // 扁平化的符号列表（包含嵌套）
    size_t selected_index_;
    bool visible_;

    // 跳转回调函数
    std::function<void(const pnana::features::DocumentSymbol&)> jump_callback_;

    // 扁平化符号列表（将嵌套符号展开）
    void flattenSymbols(const std::vector<pnana::features::DocumentSymbol>& symbols, int depth = 0);

    // 渲染单个符号项
    ftxui::Element renderSymbolItem(const pnana::features::DocumentSymbol& symbol,
                                    bool is_selected) const;

    // 获取符号类型图标
    std::string getKindIcon(const std::string& kind) const;

    // 获取符号类型颜色
    ftxui::Color getKindColor(const std::string& kind) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SYMBOL_NAVIGATION_POPUP_H
