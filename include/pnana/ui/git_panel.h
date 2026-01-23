#ifndef PNANA_VGIT_GIT_PANEL_H
#define PNANA_VGIT_GIT_PANEL_H

#include "features/vgit/git_manager.h"
#include "ui/theme.h"
#include "utils/file_type_icon_mapper.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace pnana {
namespace vgit {

enum class GitPanelMode { STATUS, COMMIT, BRANCH, REMOTE, CLONE, DIFF };

class GitPanel {
  public:
    GitPanel(ui::Theme& theme, const std::string& repo_path = ".");
    ~GitPanel() = default;

    // UI methods
    ftxui::Component getComponent();
    bool isVisible() const {
        return visible_;
    }
    void show() {
        visible_ = true;
    }
    void hide() {
        visible_ = false;
    }
    void toggle() {
        visible_ = !visible_;
    }

    // Diff viewer state getters
    bool isDiffViewerVisible() const {
        return diff_viewer_visible_;
    }

    // Diff viewer content access (for input handler)
    const std::vector<std::string>& getDiffContent() const {
        return diff_content_;
    }
    size_t getDiffScrollOffset() const {
        return diff_scroll_offset_;
    }
    void setDiffScrollOffset(size_t offset) {
        diff_scroll_offset_ = offset;
    }

    // Event handlers
    void onShow();
    void onHide();

    // Data management
    void refreshData();
    void performClone();

    // Key handlers
    bool onKeyPress(ftxui::Event event);

  private:
    ui::Theme& theme_;
    std::unique_ptr<GitManager> git_manager_;
    bool visible_ = false;
    bool data_loaded_ = false;  // 标记数据是否已加载
    bool data_loading_ = false; // 标记数据是否正在加载
    std::mutex data_mutex_;     // 保护数据访问的互斥锁

    utils::FileTypeIconMapper icon_mapper_; // 文件类型图标映射器

    // UI state
    GitPanelMode current_mode_ = GitPanelMode::STATUS;
    std::vector<GitFile> files_;
    std::vector<GitBranch> branches_;
    size_t selected_index_ = 0;
    size_t scroll_offset_ = 0;
    std::string commit_message_;
    std::string branch_name_;
    std::string clone_url_;
    std::string clone_path_;
    bool clone_focus_on_url_ = true; // true for URL, false for path
    std::string error_message_;

    // Diff viewer state
    bool diff_viewer_visible_ = false;
    std::vector<std::string> diff_content_;
    size_t diff_scroll_offset_ = 0;
    std::string current_diff_file_;

    // UI components
    ftxui::Component main_component_;
    ftxui::Component file_list_component_;
    ftxui::Component commit_input_component_;
    ftxui::Component branch_list_component_;

    // Selection state
    std::vector<size_t> selected_files_; // indices of selected files

    // Performance optimization
    bool branch_data_stale_ = true;                           // 是否需要刷新分支数据
    bool needs_redraw_ = false;                               // 是否需要重绘UI
    bool component_needs_rebuild_ = true;                     // 是否需要重新构建组件
    std::chrono::steady_clock::time_point last_refresh_time_; // 最后刷新时间
    std::chrono::milliseconds refresh_cooldown_{500};         // 刷新冷却时间(ms)

    // Cached statistics for performance
    size_t cached_staged_count_ = 0;
    size_t cached_unstaged_count_ = 0;
    bool stats_cache_valid_ = false;

    // Cached repository display info to avoid frequent git calls during rendering
    std::string cached_repo_path_display_;
    std::chrono::steady_clock::time_point last_repo_display_update_;
    std::chrono::milliseconds repo_display_cache_timeout_{10000}; // 10 seconds for repo display

    // Cached current branch info to avoid frequent git calls during rendering
    std::string cached_current_branch_;
    std::chrono::steady_clock::time_point last_branch_update_;
    std::chrono::milliseconds branch_cache_timeout_{15000}; // 15 seconds for branch info

    // Private methods
    void switchMode(GitPanelMode mode);
    GitPanelMode getNextMode(GitPanelMode current);
    void toggleFileSelection(size_t index);
    void clearSelection();
    void selectAll();
    void performStageSelected();
    void performUnstageSelected();
    void performStageAll();
    void performUnstageAll();
    void performCommit();
    bool performPush();
    bool performPull();
    void performCreateBranch();
    void performSwitchBranch();
    void refreshStatusOnly();
    void updateCachedStats(); // Update cached statistics for performance
    void showDiffViewer(const std::string& file_path);
    void hideDiffViewer();

  public:
    void handleDiffViewerEscape();

    // UI rendering
    ftxui::Element renderHeader();
    ftxui::Element renderTabs();
    ftxui::Element renderStatusPanel();
    ftxui::Element renderCommitPanel();
    ftxui::Element renderBranchPanel();
    ftxui::Element renderRemotePanel();
    ftxui::Element renderClonePanel();
    ftxui::Element renderDiffPanel();
    ftxui::Element renderDiffViewer();
    ftxui::Element renderDiffFileItem(const GitFile& file, size_t index, bool is_highlighted);
    ftxui::Element renderFileItem(const GitFile& file, size_t index, bool is_selected,
                                  bool is_highlighted);
    ftxui::Element renderBranchItem(const GitBranch& branch, size_t index, bool is_selected);
    ftxui::Element renderFooter();
    ftxui::Element renderError();
    ftxui::Element separatorLight();

    // Component builders
    ftxui::Component buildMainComponent();
    ftxui::Component buildFileListComponent();
    ftxui::Component buildCommitInputComponent();
    ftxui::Component buildBranchListComponent();

    // Key handlers
    bool handleStatusModeKey(ftxui::Event event);
    bool handleCommitModeKey(ftxui::Event event);
    bool handleBranchModeKey(ftxui::Event event);
    bool handleRemoteModeKey(ftxui::Event event);
    bool handleCloneModeKey(ftxui::Event event);
    bool handleDiffModeKey(ftxui::Event event);

    // Utility methods
    std::string getStatusIcon(GitFileStatus status) const;
    std::string getStatusText(GitFileStatus status) const;
    std::string getModeTitle(GitPanelMode mode) const;
    ftxui::Color getStatusColor(GitFileStatus status) const;
    ftxui::Color getStatusColor(GitFileStatus status, bool is_staged) const;
    ftxui::Color getDiffLineColor(const std::string& line);
    bool hasStagedChanges() const;
    bool hasUnstagedChanges() const;
    bool isNavigationKey(ftxui::Event event) const;
    std::string getCachedRepoPathDisplay();
    std::string getCachedCurrentBranch();
    std::string getFileExtension(const std::string& filename) const;
    void ensureValidIndices();
};

} // namespace vgit
} // namespace pnana

#endif // PNANA_VGIT_GIT_PANEL_H
