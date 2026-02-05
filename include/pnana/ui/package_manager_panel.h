#ifndef PNANA_UI_PACKAGE_MANAGER_PANEL_H
#define PNANA_UI_PACKAGE_MANAGER_PANEL_H

#include "features/package_manager/package_manager_base.h"
#include "features/package_manager/package_manager_registry.h"
#include "ui/package_detail_dialog.h"
#include "ui/package_install_dialog.h"
#include "ui/theme.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 包管理器标签页模式（动态，基于注册的管理器）

// 包管理器面板
class PackageManagerPanel {
  public:
    PackageManagerPanel(Theme& theme);
    ~PackageManagerPanel() = default;

    // UI 渲染
    ftxui::Element render();
    ftxui::Component getComponent();

    // 面板控制
    void show();
    void hide();
    bool isVisible() const {
        return visible_;
    }

    // 输入处理
    bool handleInput(ftxui::Event event);

  private:
    Theme& theme_;

    bool visible_;
    bool search_focused_;              // 搜索框是否获得焦点
    std::string current_manager_name_; // 当前选中的管理器名称
    size_t selected_index_;
    size_t scroll_offset_;
    std::string search_filter_;

    // 包详情弹窗
    PackageDetailDialog detail_dialog_;

    // 安装包对话框
    PackageInstallDialog install_dialog_;

    // 性能优化：不再在这里缓存，直接使用 PackageManagerRegistry 的缓存（30秒）

    // 性能优化：缓存过滤后的包列表（mutable 允许在 const 方法中修改）
    mutable std::vector<features::package_manager::Package> cached_filtered_packages_;
    mutable std::string cached_filter_key_; // 管理器名称 + 搜索过滤器
    mutable std::chrono::steady_clock::time_point cached_filter_timestamp_;
    static constexpr std::chrono::milliseconds FILTER_CACHE_TIMEOUT_{100}; // 100ms缓存

    // UI 组件
    ftxui::Component main_component_;

    // 渲染方法
    ftxui::Element renderHeader() const;
    ftxui::Element renderTabs() const;
    ftxui::Element renderSearchBox() const;
    ftxui::Element renderCurrentTab() const;
    ftxui::Element renderPackageList(
        std::shared_ptr<features::package_manager::PackageManagerBase> manager) const;
    ftxui::Element renderPackageItem(const features::package_manager::Package& pkg, size_t index,
                                     bool is_selected) const;
    ftxui::Element renderHelpBar() const;
    ftxui::Element renderStatusBar() const;

    // 辅助方法
    void switchTab(const std::string& manager_name);
    void filterPackages();
    void updateScrollOffset();
    std::vector<features::package_manager::Package> getFilteredPackages(
        std::shared_ptr<features::package_manager::PackageManagerBase> manager) const;
    std::shared_ptr<features::package_manager::PackageManagerBase> getCurrentManager() const;

    // 性能优化辅助方法
    std::vector<std::shared_ptr<features::package_manager::PackageManagerBase>>
    getCachedAvailableManagers() const;
    void invalidateManagersCache();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_PACKAGE_MANAGER_PANEL_H
