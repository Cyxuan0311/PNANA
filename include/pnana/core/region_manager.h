#ifndef PNANA_CORE_REGION_MANAGER_H
#define PNANA_CORE_REGION_MANAGER_H

#include <string>

namespace pnana {
namespace core {

// 编辑器区域类型
enum class EditorRegion {
    CODE_AREA,    // 代码编辑区
    TAB_AREA,     // 文件标签栏
    FILE_BROWSER, // 文件浏览器
    TERMINAL,     // 终端区域
    GIT_PANEL,    // Git 面板
    HELP_WINDOW   // 帮助窗口（模态，不参与导航）
};

// 区域管理器
class RegionManager {
  public:
    RegionManager();

    // 获取当前区域
    EditorRegion getCurrentRegion() const {
        return current_region_;
    }

    // 设置当前区域
    void setRegion(EditorRegion region);

    // 根据方向键导航到下一个区域
    bool navigateUp();
    bool navigateDown();
    bool navigateLeft();
    bool navigateRight();

    // 获取区域名称（用于状态栏显示）
    std::string getRegionName() const;
    std::string getRegionName(EditorRegion region) const;

    // 检查特定区域是否可用
    void setTabAreaEnabled(bool enabled) {
        tab_area_enabled_ = enabled;
    }
    void setFileBrowserEnabled(bool enabled) {
        file_browser_enabled_ = enabled;
    }
    void setTerminalEnabled(bool enabled) {
        terminal_enabled_ = enabled;
    }
    void setGitPanelEnabled(bool enabled) {
        git_panel_enabled_ = enabled;
    }
    void setHelpWindowEnabled(bool enabled) {
        help_window_enabled_ = enabled;
    }

    bool isTabAreaEnabled() const {
        return tab_area_enabled_;
    }
    bool isFileBrowserEnabled() const {
        return file_browser_enabled_;
    }
    bool isTerminalEnabled() const {
        return terminal_enabled_;
    }
    bool isGitPanelEnabled() const {
        return git_panel_enabled_;
    }
    bool isHelpWindowEnabled() const {
        return help_window_enabled_;
    }

    // 标签栏选择
    void setTabIndex(int index) {
        tab_index_ = index;
    }
    int getTabIndex() const {
        return tab_index_;
    }
    void nextTab() {
        tab_index_++;
    }
    void previousTab() {
        if (tab_index_ > 0)
            tab_index_--;
    }

  private:
    EditorRegion current_region_;
    EditorRegion previous_region_;

    // 区域可用性标志
    bool tab_area_enabled_;
    bool file_browser_enabled_;
    bool terminal_enabled_;
    bool git_panel_enabled_;
    bool help_window_enabled_;

    // 标签栏状态
    int tab_index_;

    // 辅助方法
    bool canNavigateTo(EditorRegion region) const;
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_REGION_MANAGER_H
