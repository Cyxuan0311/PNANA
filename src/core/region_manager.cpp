#include "core/region_manager.h"

namespace pnana {
namespace core {

RegionManager::RegionManager()
    : current_region_(EditorRegion::CODE_AREA), previous_region_(EditorRegion::CODE_AREA),
      tab_area_enabled_(false), file_browser_enabled_(false), terminal_enabled_(false),
      git_panel_enabled_(false), help_window_enabled_(false), tab_index_(0) {}

void RegionManager::setRegion(EditorRegion region) {
    if (canNavigateTo(region)) {
        previous_region_ = current_region_;
        current_region_ = region;

        // 切换到标签区时重置索引
        if (region == EditorRegion::TAB_AREA) {
            tab_index_ = 0;
        }
    }
}

bool RegionManager::navigateUp() {
    switch (current_region_) {
        case EditorRegion::CODE_AREA:
            // 从代码区向上导航到标签区
            if (tab_area_enabled_) {
                setRegion(EditorRegion::TAB_AREA);
                return true;
            }
            break;

        case EditorRegion::TAB_AREA:
            // 标签区已经在最上方
            return false;

        case EditorRegion::FILE_BROWSER:
            // 文件浏览器向上导航到标签区
            if (tab_area_enabled_) {
                setRegion(EditorRegion::TAB_AREA);
                return true;
            }
            break;

        case EditorRegion::TERMINAL:
            // 从终端向上导航到代码区
            setRegion(EditorRegion::CODE_AREA);
            return true;

        case EditorRegion::GIT_PANEL:
            // 从Git面板向上导航到标签区
            if (tab_area_enabled_) {
                setRegion(EditorRegion::TAB_AREA);
                return true;
            }
            break;

        case EditorRegion::HELP_WINDOW:
            // 帮助窗口是模态的，不导航
            return false;
    }

    return false;
}

bool RegionManager::navigateDown() {
    switch (current_region_) {
        case EditorRegion::CODE_AREA:
            // 从代码区向下导航到终端（如果打开）
            if (terminal_enabled_) {
                setRegion(EditorRegion::TERMINAL);
                return true;
            }
            return false;

        case EditorRegion::TAB_AREA:
            // 从标签区向下导航到代码区
            setRegion(EditorRegion::CODE_AREA);
            return true;

        case EditorRegion::FILE_BROWSER:
            // 文件浏览器向下导航到代码区
            setRegion(EditorRegion::CODE_AREA);
            return true;

        case EditorRegion::TERMINAL:
            // 终端已经在最下方
            return false;

        case EditorRegion::GIT_PANEL:
            // 从Git面板向下导航到终端（如果打开）
            if (terminal_enabled_) {
                setRegion(EditorRegion::TERMINAL);
                return true;
            }
            return false;

        case EditorRegion::HELP_WINDOW:
            // 帮助窗口是模态的，不导航
            return false;
    }

    return false;
}

bool RegionManager::navigateLeft() {
    switch (current_region_) {
        case EditorRegion::CODE_AREA:
            // 从代码区向左导航到文件浏览器
            if (file_browser_enabled_) {
                setRegion(EditorRegion::FILE_BROWSER);
                return true;
            }
            break;

        case EditorRegion::TAB_AREA:
            // 标签区左右键用于切换标签，不切换区域
            previousTab();
            return true;

        case EditorRegion::FILE_BROWSER:
            // 文件浏览器已经在最左侧
            return false;

        case EditorRegion::TERMINAL:
            // 终端向左导航到代码区（如果文件浏览器未打开）
            // 如果文件浏览器打开，则导航到文件浏览器
            if (file_browser_enabled_) {
                setRegion(EditorRegion::FILE_BROWSER);
                return true;
            } else {
                setRegion(EditorRegion::CODE_AREA);
                return true;
            }

        case EditorRegion::GIT_PANEL:
            // 从Git面板向左导航到代码区
            setRegion(EditorRegion::CODE_AREA);
            return true;

        case EditorRegion::HELP_WINDOW:
            // 帮助窗口是模态的，不导航
            return false;
    }

    return false;
}

bool RegionManager::navigateRight() {
    switch (current_region_) {
        case EditorRegion::CODE_AREA:
            // 代码区向右导航到Git面板（如果启用）
            if (git_panel_enabled_) {
                setRegion(EditorRegion::GIT_PANEL);
                return true;
            }
            return false;

        case EditorRegion::TAB_AREA:
            // 标签区左右键用于切换标签，不切换区域
            nextTab();
            return true;

        case EditorRegion::FILE_BROWSER:
            // 从文件浏览器向右导航到代码区
            setRegion(EditorRegion::CODE_AREA);
            return true;

        case EditorRegion::TERMINAL:
            // 终端向右导航到代码区
            setRegion(EditorRegion::CODE_AREA);
            return true;

        case EditorRegion::GIT_PANEL:
            // 从Git面板向左导航到代码区
            setRegion(EditorRegion::CODE_AREA);
            return true;

        case EditorRegion::HELP_WINDOW:
            // 帮助窗口是模态的，不导航
            return false;
    }

    return false;
}

std::string RegionManager::getRegionName() const {
    return getRegionName(current_region_);
}

std::string RegionManager::getRegionName(EditorRegion region) const {
    switch (region) {
        case EditorRegion::CODE_AREA:
            return "Code Editor";
        case EditorRegion::TAB_AREA:
            return "Tab Bar";
        case EditorRegion::FILE_BROWSER:
            return "File Browser";
        case EditorRegion::TERMINAL:
            return "Terminal";
        case EditorRegion::GIT_PANEL:
            return "Git Panel";
        case EditorRegion::HELP_WINDOW:
            return "Help Window";
        default:
            return "Unknown Region";
    }
}

bool RegionManager::canNavigateTo(EditorRegion region) const {
    switch (region) {
        case EditorRegion::CODE_AREA:
            return true; // 代码区始终可用

        case EditorRegion::TAB_AREA:
            return tab_area_enabled_;

        case EditorRegion::FILE_BROWSER:
            return file_browser_enabled_;

        case EditorRegion::TERMINAL:
            return terminal_enabled_;

        case EditorRegion::GIT_PANEL:
            return git_panel_enabled_;

        case EditorRegion::HELP_WINDOW:
            return help_window_enabled_;

        default:
            return false;
    }
}

} // namespace core
} // namespace pnana
