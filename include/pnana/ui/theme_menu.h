#ifndef PNANA_UI_THEME_MENU_H
#define PNANA_UI_THEME_MENU_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 主题菜单组件
class ThemeMenu {
  public:
    explicit ThemeMenu(Theme& theme);

    // 设置可用主题列表
    void setAvailableThemes(const std::vector<std::string>& themes);

    // 设置当前选中的主题索引
    void setSelectedIndex(size_t index);

    // 设置光标颜色获取器（用于输入框光标，跟随编辑器光标配置）
    void setCursorColorGetter(std::function<ftxui::Color()> getter);

    // 获取当前选中的主题索引
    size_t getSelectedIndex() const {
        return selected_index_;
    }

    // 获取可用主题列表（返回过滤后的列表）
    const std::vector<std::string>& getAvailableThemes() const {
        return filtered_themes_;
    }

    // 获取所有可用主题列表（未过滤）
    const std::vector<std::string>& getAllThemes() const {
        return available_themes_;
    }

    // 获取当前主题名称
    std::string getCurrentThemeName() const;

    // 获取当前选中的主题名称（支持过滤后的列表）
    std::string getSelectedThemeName() const;

    // 处理输入事件
    bool handleInput(ftxui::Event event);

    // 渲染主题菜单
    ftxui::Element render();

  private:
    Theme& theme_;
    std::vector<std::string> available_themes_;
    size_t selected_index_;

    // 搜索功能
    std::string search_input_;                          // 搜索输入
    size_t search_cursor_pos_;                          // 搜索框光标位置
    std::vector<std::string> filtered_themes_;          // 过滤后的主题列表
    std::vector<size_t> filtered_indices_;              // 过滤后主题对应的原始索引
    std::function<ftxui::Color()> cursor_color_getter_; // 输入框光标颜色

    // 辅助方法
    void updateFilteredThemes();               // 更新过滤后的主题列表
    ftxui::Element renderSearchBox() const;    // 渲染搜索框
    ftxui::Element renderThemeList() const;    // 渲染主题列表
    ftxui::Element renderColorPreview() const; // 渲染右侧颜色预览
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_THEME_MENU_H
