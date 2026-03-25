#include "ui/git_panel.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <future>
#include <mutex>
#include <sstream>

using namespace ftxui;
using namespace pnana::ui::icons;

namespace fs = std::filesystem;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace vgit {

GitPanel::GitPanel(ui::Theme& theme, const std::string& repo_path)
    : theme_(theme), git_manager_(std::make_unique<GitManager>(repo_path)), icon_mapper_() {
    last_repo_display_update_ = std::chrono::steady_clock::now() - repo_display_cache_timeout_;
    last_branch_update_ = std::chrono::steady_clock::now() - branch_cache_timeout_;
    last_branch_status_update_ = std::chrono::steady_clock::now() - branch_status_cache_timeout_;
}

void GitPanel::setRemoteExecutor(vgit::GitManager::RemoteExecutor executor,
                                 const std::string& label, const std::string& remote_path) {
    git_manager_->setRemoteExecutor(std::move(executor), label, remote_path);
    // 清除所有 UI 缓存，让面板重新拉取远程数据
    data_loaded_ = false;
    data_loading_ = false;
    files_.clear();
    branches_.clear();
    graph_commits_.clear();
    cached_current_branch_.clear();
    cached_repo_path_display_.clear();
    stats_cache_valid_ = false;
    last_branch_update_ = std::chrono::steady_clock::now() - branch_cache_timeout_;
    last_repo_display_update_ = std::chrono::steady_clock::now() - repo_display_cache_timeout_;
    last_branch_status_update_ = std::chrono::steady_clock::now() - branch_status_cache_timeout_;
}

void GitPanel::clearRemoteContext(const std::string& local_path) {
    git_manager_->clearRemoteContext(local_path);
    data_loaded_ = false;
    data_loading_ = false;
    files_.clear();
    branches_.clear();
    graph_commits_.clear();
    cached_current_branch_.clear();
    cached_repo_path_display_.clear();
    stats_cache_valid_ = false;
    last_branch_update_ = std::chrono::steady_clock::now() - branch_cache_timeout_;
    last_repo_display_update_ = std::chrono::steady_clock::now() - repo_display_cache_timeout_;
    last_branch_status_update_ = std::chrono::steady_clock::now() - branch_status_cache_timeout_;
}

bool GitPanel::isRemote() const {
    return git_manager_ && git_manager_->isRemote();
}

const std::string& GitPanel::getRemoteLabel() const {
    static const std::string empty;
    return git_manager_ ? git_manager_->getRemoteLabel() : empty;
}

Component GitPanel::getComponent() {
    // 只在初次创建、模式切换或需要重建时重新构建组件
    if (!main_component_ || component_needs_rebuild_) {
        main_component_ = buildMainComponent();
        component_needs_rebuild_ = false; // Reset the flag after rebuild
    }
    return main_component_;
}

void GitPanel::onShow() {
    // 立即设置UI状态，允许用户开始交互
    selected_index_ = 0;
    scroll_offset_ = 0;
    clearSelection();

    // 如果还没加载过数据，开始异步加载（不阻塞UI）
    if (!data_loaded_ && !data_loading_) {
        // 异步加载数据，不阻塞UI
        std::thread([this]() {
            refreshData();
        }).detach(); // 分离线程，让它在后台运行
    }
}

void GitPanel::onHide() {
    // Cleanup if needed
}

bool GitPanel::onKeyPress(Event event) {
    if (!visible_) {
        return false;
    }

    if (event == Event::Escape) {
        hide();
        return true;
    }

    bool handled = false;
    switch (current_mode_) {
        case GitPanelMode::STATUS:
            handled = handleStatusModeKey(event);
            break;
        case GitPanelMode::COMMIT:
            handled = handleCommitModeKey(event);
            break;
        case GitPanelMode::BRANCH:
            handled = handleBranchModeKey(event);
            break;
        case GitPanelMode::REMOTE:
            handled = handleRemoteModeKey(event);
            break;
        case GitPanelMode::CLONE:
            handled = handleCloneModeKey(event);
            break;
        case GitPanelMode::DIFF:
            handled = handleDiffModeKey(event);
            break;
        case GitPanelMode::GRAPH:
            handled = handleGraphModeKey(event);
            break;
    }

    // 对于导航键（箭头键、翻页键等），标记为需要重绘
    // 对于Git操作，GitPanelHandler会处理重绘
    if (handled && isNavigationKey(event)) {
        needs_redraw_ = true;
    }

    return handled;
}

void GitPanel::refreshStatusOnly() {
    if (data_loading_) {
        return; // 如果正在加载，忽略请求
    }

    data_loading_ = true;

    // 异步只刷新状态数据，不刷新分支数据
    auto future = std::async(std::launch::async, [this]() {
        try {
            git_manager_->refreshStatusForced();
            auto files = git_manager_->getStatus();
            auto error = git_manager_->getLastError();
            git_manager_->clearError();

            // 在主线程中更新UI数据
            std::lock_guard<std::mutex> lock(data_mutex_);
            files_ = std::move(files);
            error_message_ = std::move(error);
            data_loading_ = false;
            stats_cache_valid_ = false; // Invalidate stats cache when data changes
        } catch (...) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            data_loading_ = false;
            error_message_ = "Failed to load git data";
        }
    });
}

void GitPanel::refreshData() {
    if (data_loading_) {
        return; // 如果正在加载，忽略请求
    }

    data_loading_ = true;
    last_refresh_time_ = std::chrono::steady_clock::now();

    // 异步加载git数据，使用更高效的方式
    auto future = std::async(std::launch::async, [this]() {
        try {
            // 强制刷新状态，确保获取最新数据
            git_manager_->refreshStatusForced();

            auto files = git_manager_->getStatus();
            auto error = git_manager_->getLastError();
            git_manager_->clearError();

            // 分支数据变化较少，只有在第一次加载或明确需要时才获取
            bool need_branches = branches_.empty() || branch_data_stale_;
            std::vector<GitBranch> branches;
            if (need_branches) {
                branches = git_manager_->getBranches();
            }

            // 在主线程中更新UI数据
            std::lock_guard<std::mutex> lock(data_mutex_);
            files_ = std::move(files);
            error_message_ = std::move(error);
            stats_cache_valid_ = false; // Invalidate stats cache when data changes

            if (need_branches) {
                branches_ = std::move(branches);
                branch_data_stale_ = false;
            }

            data_loading_ = false;
            data_loaded_ = true;

            // Ensure indices are valid after data update
            ensureValidIndices();
        } catch (...) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            data_loading_ = false;
            error_message_ = "Failed to load git data";
        }
    });
}

void GitPanel::switchMode(GitPanelMode mode) {
    current_mode_ = mode;
    selected_index_ = 0;
    scroll_offset_ = 0;
    clearSelection();

    if (mode == GitPanelMode::COMMIT) {
        commit_message_.clear();
        commit_cursor_position_ = 0;
    } else if (mode == GitPanelMode::BRANCH) {
        branch_name_.clear();
        branch_cursor_position_ = 0;
    } else if (mode == GitPanelMode::CLONE) {
        clone_url_.clear();
        clone_path_ = git_manager_->getRepositoryRoot().empty() ? fs::current_path().string()
                                                                : git_manager_->getRepositoryRoot();
        clone_focus_on_url_ = true;      // Default focus on URL
        clone_state_ = CloneState::IDLE; // Reset clone state when switching to clone mode
        clone_success_message_.clear();
    } else if (mode == GitPanelMode::GRAPH) {
        // Load graph commits when switching to graph mode
        if (graph_commits_.empty() || !data_loaded_) {
            std::thread([this]() {
                auto commits = git_manager_->getGraphCommits(100);
                std::lock_guard<std::mutex> lock(data_mutex_);
                graph_commits_ = std::move(commits);
            }).detach();
        }
    }

    // Ensure indices are valid for the new mode
    ensureValidIndices();
}

GitPanelMode GitPanel::getNextMode(GitPanelMode current) {
    switch (current) {
        case GitPanelMode::STATUS:
            return GitPanelMode::COMMIT;
        case GitPanelMode::COMMIT:
            return GitPanelMode::BRANCH;
        case GitPanelMode::BRANCH:
            return GitPanelMode::REMOTE;
        case GitPanelMode::REMOTE:
            return GitPanelMode::CLONE;
        case GitPanelMode::CLONE:
            return GitPanelMode::DIFF;
        case GitPanelMode::DIFF:
            return GitPanelMode::GRAPH;
        case GitPanelMode::GRAPH:
            return GitPanelMode::STATUS;
        default:
            return GitPanelMode::STATUS;
    }
}

void GitPanel::toggleFileSelection(size_t index) {
    if (index >= files_.size())
        return;

    auto it = std::find(selected_files_.begin(), selected_files_.end(), index);
    if (it != selected_files_.end()) {
        selected_files_.erase(it);
    } else {
        selected_files_.push_back(index);
    }
}

void GitPanel::clearSelection() {
    selected_files_.clear();
}

void GitPanel::selectAll() {
    selected_files_.clear();
    for (size_t i = 0; i < files_.size(); ++i) {
        selected_files_.push_back(i);
    }
}

void GitPanel::performStageSelected() {
    if (selected_files_.empty()) {
        return; // 没有选中文件，直接返回
    }

    bool success = true;
    for (size_t index : selected_files_) {
        if (index < files_.size()) {
            if (!git_manager_->stageFile(files_[index].path)) {
                success = false;
                error_message_ = git_manager_->getLastError();
                break;
            } else {
                // Immediately reflect staged state in UI so color/indicator update without waiting
                {
                    std::lock_guard<std::mutex> lock(data_mutex_);
                    if (index < files_.size()) {
                        files_[index].staged = true;
                        // Invalidate cached stats so header will update
                        stats_cache_valid_ = false;
                    }
                }
                // Request a rebuild/redraw so changes are visible immediately
                component_needs_rebuild_ = true;
                needs_redraw_ = true;
            }
        }
    }

    if (success) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // 用户可以通过F5或R键手动刷新，或者等待下次自动刷新
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
        // Invalidate cached stats so UI will recalculate staged/unstaged counts
        stats_cache_valid_ = false;
    }
}

void GitPanel::performUnstageSelected() {
    if (selected_files_.empty()) {
        return; // 没有选中文件，直接返回
    }

    bool success = true;
    for (size_t index : selected_files_) {
        if (index < files_.size()) {
            if (!git_manager_->unstageFile(files_[index].path)) {
                success = false;
                error_message_ = git_manager_->getLastError();
                break;
            } else {
                // Immediately reflect unstaged state in UI so color/indicator update without
                // waiting
                {
                    std::lock_guard<std::mutex> lock(data_mutex_);
                    if (index < files_.size()) {
                        files_[index].staged = false;
                        // Invalidate cached stats so header will update
                        stats_cache_valid_ = false;
                    }
                }
                component_needs_rebuild_ = true;
                needs_redraw_ = true;
            }
        }
    }

    if (success) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
    }
}

void GitPanel::performStageAll() {
    if (git_manager_->stageAll()) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
    } else {
        error_message_ = git_manager_->getLastError();
    }
}

void GitPanel::performUnstageAll() {
    if (git_manager_->unstageAll()) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
    } else {
        error_message_ = git_manager_->getLastError();
    }
}

void GitPanel::performCommit() {
    if (commit_message_.empty())
        return;

    if (git_manager_->commit(commit_message_)) {
        commit_message_.clear();
        commit_cursor_position_ = 0;
        // Refresh data and ensure UI updates immediately
        refreshData(); // commit后需要刷新所有数据，包括分支
        // Invalidate cached stats to force recalculation
        stats_cache_valid_ = false;
        // Clear selection and request rebuild so Status panel reflects new state
        clearSelection();
        component_needs_rebuild_ = true;
        needs_redraw_ = true;
        switchMode(GitPanelMode::STATUS);
    } else {
        error_message_ = git_manager_->getLastError();
    }
}

bool GitPanel::performPush() {
    if (git_manager_->push()) {
        refreshData(); // push后可能需要刷新分支和状态信息
        return true;
    }
    error_message_ = git_manager_->getLastError();
    return false;
}

bool GitPanel::performPull() {
    if (git_manager_->pull()) {
        refreshData(); // pull后需要刷新所有数据
        return true;
    }
    error_message_ = git_manager_->getLastError();
    return false;
}

void GitPanel::performCreateBranch() {
    if (branch_name_.empty())
        return;

    if (git_manager_->createBranch(branch_name_)) {
        branch_name_.clear();
        branch_cursor_position_ = 0;
        // Clear branch cache since current branch might have changed
        cached_current_branch_.clear();
        last_branch_update_ = std::chrono::steady_clock::now() - branch_cache_timeout_;
        refreshData(); // 分支操作后需要刷新分支数据
        switchMode(GitPanelMode::STATUS);
    } else {
        error_message_ = git_manager_->getLastError();
    }
}

void GitPanel::performSwitchBranch() {
    if (selected_index_ >= branches_.size())
        return;

    if (git_manager_->switchBranch(branches_[selected_index_].name)) {
        // Clear branch cache since current branch has changed
        cached_current_branch_.clear();
        last_branch_update_ = std::chrono::steady_clock::now() - branch_cache_timeout_;
        refreshData(); // 分支切换后需要刷新所有数据
        switchMode(GitPanelMode::STATUS);
    } else {
        error_message_ = git_manager_->getLastError();
    }
}

Element GitPanel::renderHeader() {
    auto& colors = theme_.getColors();

    Elements header_elements;
    header_elements.push_back(text(pnana::ui::icons::GIT) | color(colors.function));
    header_elements.push_back(text(" Git") | color(colors.foreground) | bold);

    // 远程模式：显示 SSH 主机标识
    if (isRemote()) {
        const std::string& label = getRemoteLabel();
        if (!label.empty()) {
            header_elements.push_back(text(" @ ") | color(colors.comment));
            header_elements.push_back(text(label) | color(colors.string) | bold);
            header_elements.push_back(text(" [remote]") | color(colors.comment) | dim);
        }
    }

    header_elements.push_back(text(" │ ") | color(colors.comment));
    header_elements.push_back(text(getModeTitle(current_mode_)) | color(colors.keyword) | bold);

    // Right aligned repository label (keeps top bar readable on narrow terminals)
    std::string repo_path = getCachedRepoPathDisplay();
    Element left = hbox(std::move(header_elements));
    Element right = hbox(
        {text("Repo: ") | color(colors.menubar_fg), text(repo_path) | color(colors.foreground)});

    return hbox({left, filler(), right}) | bgcolor(colors.menubar_bg);
}

Element GitPanel::renderTabs() {
    auto& colors = theme_.getColors();

    auto makeTab = [&](const std::string& label, GitPanelMode /*mode*/, bool active) {
        if (active) {
            return text("[" + label + "]") | bgcolor(colors.comment) | color(colors.background) |
                   bold;
        } else {
            return text(" " + label + " ") | color(colors.menubar_fg);
        }
    };

    Elements elements = {
        text(" ") | color(colors.menubar_fg),
        makeTab("Status", GitPanelMode::STATUS, current_mode_ == GitPanelMode::STATUS),
        text(" ") | color(colors.comment),
        makeTab("Commit", GitPanelMode::COMMIT, current_mode_ == GitPanelMode::COMMIT),
        text(" ") | color(colors.comment),
        makeTab("Branch", GitPanelMode::BRANCH, current_mode_ == GitPanelMode::BRANCH),
        text(" ") | color(colors.comment),
        makeTab("Remote", GitPanelMode::REMOTE, current_mode_ == GitPanelMode::REMOTE),
        text(" ") | color(colors.comment),
        makeTab("Clone", GitPanelMode::CLONE, current_mode_ == GitPanelMode::CLONE),
        text(" ") | color(colors.comment),
        makeTab("Diff", GitPanelMode::DIFF, current_mode_ == GitPanelMode::DIFF),
        text(" ") | color(colors.comment),
        makeTab("Graph", GitPanelMode::GRAPH, current_mode_ == GitPanelMode::GRAPH),
        filler(),
        text("Tab: switch") | color(colors.comment),
        text("  Esc: close") | color(colors.comment),
        text(" ") | color(colors.menubar_fg),
    };
    return hbox(std::move(elements)) | bgcolor(colors.helpbar_bg);
}

Element GitPanel::renderStatusPanel() {
    auto& colors = theme_.getColors();

    // 如果数据正在加载，显示加载指示器
    if (data_loading_) {
        return vbox({text("Loading git status...") | color(colors.comment) | center,
                     text("Please wait...") | color(colors.menubar_fg) | center}) |
               center | size(HEIGHT, EQUAL, 10);
    }

    Elements list_elements;

    // Enhanced header with status summary (use cached stats for performance)
    if (!stats_cache_valid_) {
        updateCachedStats();
    }

    size_t staged_count = cached_staged_count_;
    size_t unstaged_count = cached_unstaged_count_;

    Elements header_elements = {
        text(pnana::ui::icons::GIT) | color(colors.function),
        text(" Git Status") | color(colors.foreground) | bold,
        text(" | "),
        text(std::to_string(files_.size()) + " files") | color(colors.menubar_fg),
        text(" (") | color(colors.comment),
        text(std::to_string(staged_count)) | color(colors.success),
        text(" staged, ") | color(colors.comment),
        text(std::to_string(unstaged_count)) | color(colors.warning),
        text(" unstaged") | color(colors.comment),
        text(")") | color(colors.comment)};
    list_elements.push_back(hbox(std::move(header_elements)));
    list_elements.push_back(separator());

    // File list with improved scrolling and display
    const size_t MAX_VISIBLE_FILES = 25; // Fixed maximum visible files for better UX

    // Ensure scroll_offset_ is within valid range
    if (scroll_offset_ >= files_.size()) {
        scroll_offset_ = files_.size() > MAX_VISIBLE_FILES ? files_.size() - MAX_VISIBLE_FILES : 0;
    }

    // Ensure selected_index_ is within valid range
    if (selected_index_ >= files_.size() && !files_.empty()) {
        selected_index_ = files_.size() - 1;
    }

    // Adjust scroll_offset_ to keep selected item visible
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_FILES) {
        scroll_offset_ = selected_index_ - MAX_VISIBLE_FILES + 1;
    }

    // Ensure scroll_offset_ doesn't show empty space at the end
    if (scroll_offset_ + MAX_VISIBLE_FILES > files_.size()) {
        scroll_offset_ = files_.size() > MAX_VISIBLE_FILES ? files_.size() - MAX_VISIBLE_FILES : 0;
    }

    size_t start = scroll_offset_;
    size_t end = std::min(start + MAX_VISIBLE_FILES, files_.size());

    for (size_t i = start; i < end; ++i) {
        bool is_selected =
            std::find(selected_files_.begin(), selected_files_.end(), i) != selected_files_.end();
        bool is_highlighted = (i == selected_index_);
        list_elements.push_back(renderFileItem(files_[i], i, is_selected, is_highlighted));
    }

    if (files_.empty()) {
        Elements empty_elements = {
            text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success) | bold,
            text(" Working directory clean") | color(colors.success),
            text(" - no changes to commit") | color(colors.comment)};
        list_elements.push_back(hbox(std::move(empty_elements)) | center);
    }

    // Borderless split panes: keep only the outer "Git Panel" window border.
    Element left_title = hbox({text(pnana::ui::icons::LIST) | color(colors.comment),
                               text(" Changes") | color(colors.foreground) | bold}) |
                         bgcolor(colors.background);
    Element left = vbox({
                       left_title,
                       separator(),
                       vbox(std::move(list_elements)) | frame | vscroll_indicator | flex,
                   }) |
                   flex;

    // Right details pane (modern TUI split view: list + detail)
    Elements detail;
    const int kDetailPaneWidth = 44;

    auto truncate_middle = [](const std::string& s, size_t max_len) -> std::string {
        if (s.size() <= max_len)
            return s;
        if (max_len <= 5)
            return s.substr(0, max_len);
        size_t keep = (max_len - 3) / 2;
        size_t tail = max_len - 3 - keep;
        return s.substr(0, keep) + "..." + s.substr(s.size() - tail);
    };

    if (files_.empty()) {
        detail.push_back(text("No changes selected.") | color(colors.comment));
        detail.push_back(separator());
        detail.push_back(text("Tips:") | color(colors.menubar_fg) | bold);
        detail.push_back(text("  - Press R/F5 to refresh") | color(colors.comment));
        detail.push_back(text("  - Use Space to select files") | color(colors.comment));
    } else {
        const size_t idx = std::min(selected_index_, files_.size() - 1);
        const auto& f = files_[idx];

        // Keep a stable pane width and wrap long paths inside it.
        const std::string display_path = truncate_middle(f.path, 96);
        detail.push_back(hbox({text(pnana::ui::icons::FILE) | color(colors.function),
                               text(" ") | color(colors.comment),
                               paragraph(display_path) | color(colors.foreground) | bold}) |
                         xflex);

        detail.push_back(
            hbox({text("Status: ") | color(colors.menubar_fg),
                  text(getStatusText(f.status)) | color(getStatusColor(f.status)) | bold}));
        detail.push_back(hbox({text("Staged: ") | color(colors.menubar_fg),
                               text(f.staged ? "yes" : "no") |
                                   color(f.staged ? colors.success : colors.warning) | bold}));

        if (!f.old_path.empty() && f.old_path != f.path) {
            detail.push_back(hbox({text("Renamed: ") | color(colors.menubar_fg),
                                   paragraph(f.old_path) | color(colors.comment) | dim}));
        }

        detail.push_back(separator());
        detail.push_back(text("Shortcuts:") | color(colors.menubar_fg) | bold);
        detail.push_back(text("  - s/S: stage selected/all") | color(colors.comment));
        detail.push_back(text("  - u/U: unstage selected/all") | color(colors.comment));
        detail.push_back(text("  - Tab: switch panel") | color(colors.comment));
    }

    Element right = vbox({
                        hbox({text(pnana::ui::icons::INFO_CIRCLE) | color(colors.comment),
                              text(" Details") | color(colors.foreground) | bold}),
                        separator(),
                        vbox(std::move(detail)) | flex,
                    }) |
                    size(WIDTH, EQUAL, kDetailPaneWidth) | flex;

    Elements panes;
    panes.push_back(left | flex);
    // `separator()` adapts: vertical in hbox, horizontal in vbox.
    panes.push_back(separator() | color(colors.comment) | dim);
    panes.push_back(right | flex);
    return hbox(std::move(panes)) | borderWithColor(colors.dialog_border) | flex;
}

Element GitPanel::renderCommitPanel() {
    auto& colors = theme_.getColors();

    Elements elements;

    // Enhanced header
    Elements header_elements = {text(pnana::ui::icons::GIT_COMMIT) | color(colors.function),
                                text(" Commit Changes") | color(colors.foreground) | bold};
    elements.push_back(hbox(std::move(header_elements)));
    elements.push_back(separator());

    // Staged files summary with better visualization (use cached stats for performance)
    if (!stats_cache_valid_) {
        updateCachedStats();
    }
    size_t staged_count = cached_staged_count_;
    size_t unstaged_count = cached_unstaged_count_;

    Elements summary_elements = {text(pnana::ui::icons::SAVED) | color(colors.success),
                                 text(" Staged: ") | color(colors.menubar_fg),
                                 text(std::to_string(staged_count)) | color(colors.success) | bold,
                                 text(" files") | color(colors.menubar_fg),
                                 text(" | ") | color(colors.comment),
                                 text(pnana::ui::icons::UNSAVED) | color(colors.warning),
                                 text(" Unstaged: ") | color(colors.menubar_fg),
                                 text(std::to_string(unstaged_count)) | color(colors.warning),
                                 text(" files") | color(colors.menubar_fg)};
    elements.push_back(hbox(std::move(summary_elements)));
    elements.push_back(separator());

    auto truncate_middle = [](const std::string& s, size_t max_len) -> std::string {
        if (s.size() <= max_len)
            return s;
        if (max_len <= 5)
            return s.substr(0, max_len);
        size_t keep = (max_len - 3) / 2;
        size_t tail = max_len - 3 - keep;
        return s.substr(0, keep) + "..." + s.substr(s.size() - tail);
    };

    // Message editor (multi-line) + staged preview, modern split layout.
    auto render_message_editor = [&]() -> Element {
        // Normalize cursor
        size_t cursor = std::min(commit_cursor_position_, commit_message_.size());

        // Split message into lines and compute cursor line/column.
        std::vector<std::string> lines;
        lines.reserve(8);
        size_t line_start = 0;
        size_t cursor_line = 0;
        size_t cursor_col = 0;
        for (size_t i = 0; i <= commit_message_.size(); ++i) {
            bool at_end = (i == commit_message_.size());
            bool is_nl = (!at_end && commit_message_[i] == '\n');
            if (at_end || is_nl) {
                lines.push_back(commit_message_.substr(line_start, i - line_start));
                if (cursor >= line_start && cursor <= i) {
                    cursor_line = lines.size() - 1;
                    cursor_col = cursor - line_start;
                }
                line_start = i + 1;
            }
        }
        if (lines.empty()) {
            lines.push_back("");
            cursor_line = 0;
            cursor_col = 0;
        }

        Elements rendered_lines;
        rendered_lines.reserve(lines.size());
        for (size_t li = 0; li < lines.size(); ++li) {
            const std::string& s = lines[li];
            if (li == cursor_line) {
                const size_t safe_col = std::min(cursor_col, s.size());
                const std::string before = s.substr(0, safe_col);
                const std::string cursor_char = (safe_col < s.size()) ? s.substr(safe_col, 1) : " ";
                const std::string after = (safe_col < s.size()) ? s.substr(safe_col + 1) : "";

                Elements row;
                if (!before.empty())
                    row.push_back(text(before) | color(colors.foreground));
                row.push_back(text(cursor_char) | bgcolor(colors.selection) |
                              color(colors.background) | bold);
                if (!after.empty())
                    row.push_back(text(after) | color(colors.foreground));
                rendered_lines.push_back(hbox(std::move(row)));
            } else {
                rendered_lines.push_back(text(s) | color(colors.foreground));
            }
        }

        // Ensure we show a cursor even for an empty message.
        if (commit_message_.empty()) {
            rendered_lines.clear();
            rendered_lines.push_back(
                hbox({text(" ") | bgcolor(colors.selection) | color(colors.background) | bold}) |
                xflex);
        }

        Element editor_body = vbox(std::move(rendered_lines)) | frame | vscroll_indicator | flex;
        Element title = hbox({
            text(pnana::ui::icons::FILE_EDIT) | color(colors.keyword),
            text(" Message") | color(colors.foreground) | bold,
            filler(),
            text("Ctrl+Enter: newline") | color(colors.comment) | dim,
            text("  Enter: commit") | color(colors.comment) | dim,
        });
        return vbox({title, separator(), editor_body}) | border | flex;
    };

    auto render_staged_preview = [&]() -> Element {
        Elements rows;
        rows.push_back(
            hbox({text(pnana::ui::icons::SAVED) | color(colors.success),
                  text(" Staged") | color(colors.foreground) | bold,
                  text(" (" + std::to_string(staged_count) + ")") | color(colors.comment)}));
        rows.push_back(separator());

        if (staged_count == 0) {
            rows.push_back(text("Stage files in Status tab first.") | color(colors.comment));
        } else {
            const size_t MAX_STAGED_SHOW = 12;
            size_t shown = 0;
            for (const auto& f : files_) {
                if (!f.staged)
                    continue;
                std::string display = truncate_middle(f.path, 52);
                rows.push_back(hbox({text(pnana::ui::icons::FILE) | color(colors.function),
                                     text(" ") | color(colors.comment),
                                     text(display) | color(colors.success)}));
                if (++shown >= MAX_STAGED_SHOW)
                    break;
            }
            if (shown < staged_count) {
                rows.push_back(text("...") | color(colors.comment));
            }
        }
        return vbox(std::move(rows)) | border | flex;
    };

    // Status line
    Elements status_line;
    if (staged_count == 0) {
        status_line = {text(pnana::ui::icons::WARNING) | color(colors.error),
                       text(" No staged changes to commit") | color(colors.error)};
    } else if (commit_message_.empty()) {
        status_line = {text(pnana::ui::icons::INFO_CIRCLE) | color(colors.warning),
                       text(" Commit message is required") | color(colors.warning)};
    } else {
        status_line = {text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success),
                       text(" Ready to commit") | color(colors.success)};
    }

    std::string char_count = std::to_string(commit_message_.length()) + " chars";
    Element status_bar = hbox({
                             hbox(status_line),
                             filler(),
                             text(char_count) | color(colors.comment) | dim,
                         }) |
                         bgcolor(colors.background);

    Element panes = hbox({render_message_editor() | flex, separator() | color(colors.comment) | dim,
                          render_staged_preview() | flex}) |
                    flex;

    elements.push_back(panes);
    elements.push_back(separator());
    elements.push_back(status_bar);

    return vbox(std::move(elements));
}

Element GitPanel::renderBranchPanel() {
    auto& colors = theme_.getColors();

    // 如果数据正在加载，显示加载指示器
    if (data_loading_) {
        return vbox({text("Loading branches...") | color(colors.comment) | center,
                     text("Please wait...") | color(colors.menubar_fg) | center}) |
               center | size(HEIGHT, EQUAL, 10);
    }

    Elements branch_elements;

    // Enhanced header with branch statistics
    size_t local_branches = 0, remote_branches = 0;
    for (const auto& branch : branches_) {
        if (branch.is_remote) {
            remote_branches++;
        } else {
            local_branches++;
        }
    }

    Elements header_elements = {
        text(pnana::ui::icons::GIT_BRANCH) | color(colors.function),
        text(" Branches") | color(colors.foreground) | bold,
        text(" | ") | color(colors.comment),
        text(std::to_string(branches_.size()) + " total") | color(colors.menubar_fg),
        text(" (") | color(colors.comment),
        text(std::to_string(local_branches)) | color(colors.foreground),
        text(" local, ") | color(colors.comment),
        text(std::to_string(remote_branches)) | color(colors.keyword),
        text(" remote") | color(colors.comment),
        text(")") | color(colors.comment)};
    branch_elements.push_back(hbox(header_elements));
    branch_elements.push_back(separator());

    // Current branch with enhanced display
    std::string current_branch = getCachedCurrentBranch();
    if (!current_branch.empty()) {
        Elements current_elements = {text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success),
                                     text(" Current branch: ") | color(colors.menubar_fg),
                                     text(current_branch) | color(colors.success) | bold,
                                     text(" (HEAD)") | color(colors.comment)};
        branch_elements.push_back(hbox(current_elements));
        branch_elements.push_back(separator());
    }

    // Branch list with improved display
    size_t start = scroll_offset_;
    size_t visible_count = 18; // Show more branches
    size_t end = std::min(start + visible_count, branches_.size());

    for (size_t i = start; i < end; ++i) {
        bool is_highlighted = (i == selected_index_);
        branch_elements.push_back(renderBranchItem(branches_[i], i, is_highlighted));
    }

    if (branches_.empty()) {
        Elements empty_elements = {text(pnana::ui::icons::WARNING) | color(colors.warning),
                                   text(" No branches found") | color(colors.warning)};
        branch_elements.push_back(hbox(empty_elements) | center);
    }

    // New branch input with better styling
    branch_elements.push_back(separator());
    Elements input_elements = {text(pnana::ui::icons::FILE_PLUS) | color(colors.success),
                               text(" Create new branch:") | color(colors.menubar_fg)};
    branch_elements.push_back(hbox(input_elements));

    auto render_branch_input = [&]() -> Element {
        size_t cursor = std::min(branch_cursor_position_, branch_name_.size());
        const size_t safe_col = std::min(cursor, branch_name_.size());
        const std::string before = branch_name_.substr(0, safe_col);
        const std::string cursor_char =
            (safe_col < branch_name_.size()) ? branch_name_.substr(safe_col, 1) : " ";
        const std::string after =
            (safe_col < branch_name_.size()) ? branch_name_.substr(safe_col + 1) : "";

        Elements row;
        if (!before.empty()) {
            row.push_back(text(before) | color(colors.foreground));
        }
        row.push_back(text(cursor_char) | bgcolor(colors.selection) | color(colors.background) |
                      bold);
        if (!after.empty()) {
            row.push_back(text(after) | color(colors.foreground));
        }
        if (branch_name_.empty()) {
            row.clear();
            row.push_back(text(" ") | bgcolor(colors.selection) | color(colors.background) | bold);
        }
        return hbox(std::move(row)) | xflex;
    };

    branch_elements.push_back(render_branch_input() | border | bgcolor(colors.background));

    return vbox(std::move(branch_elements));
}

Element GitPanel::renderRemotePanel() {
    auto& colors = theme_.getColors();

    Elements elements;

    // Enhanced header
    Elements header_elements = {text(pnana::ui::icons::GIT_REMOTE) | color(colors.function),
                                text(" Remote Operations") | color(colors.foreground) | bold};
    elements.push_back(hbox(std::move(header_elements)));
    elements.push_back(separator());

    std::string current_branch = getCachedCurrentBranch();
    GitBranchStatus branch_status = getCachedBranchStatus();

    auto build_status_card = [&]() -> Element {
        Elements rows;
        rows.push_back(hbox({text(pnana::ui::icons::INFO_CIRCLE) | color(colors.keyword),
                             text(" Status") | color(colors.foreground) | bold}));
        rows.push_back(separator());

        if (!current_branch.empty()) {
            rows.push_back(hbox({text("Branch: ") | color(colors.menubar_fg),
                                 text(current_branch) | color(colors.foreground) | bold}));
        } else {
            rows.push_back(text("Branch: (unknown)") | color(colors.comment));
        }

        if (branch_status.has_remote_tracking) {
            if (branch_status.ahead > 0 && branch_status.behind == 0) {
                rows.push_back(hbox({text(pnana::ui::icons::UPLOAD) | color(colors.success),
                                     text(" Ahead of remote") | color(colors.success) | bold,
                                     text(" (" + std::to_string(branch_status.ahead) + ")") |
                                         color(colors.success)}));
            } else if (branch_status.ahead == 0 && branch_status.behind > 0) {
                rows.push_back(hbox({text(pnana::ui::icons::DOWNLOAD) | color(colors.warning),
                                     text(" Behind remote") | color(colors.warning) | bold,
                                     text(" (" + std::to_string(branch_status.behind) + ")") |
                                         color(colors.warning)}));
            } else if (branch_status.ahead > 0 && branch_status.behind > 0) {
                rows.push_back(hbox({text(pnana::ui::icons::WARNING) | color(colors.warning),
                                     text(" Diverged") | color(colors.warning) | bold,
                                     text(" (" + std::to_string(branch_status.ahead) + " ahead, " +
                                          std::to_string(branch_status.behind) + " behind)") |
                                         color(colors.warning)}));
            } else {
                rows.push_back(hbox({text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success),
                                     text(" In sync") | color(colors.success)}));
            }
        } else if (!current_branch.empty()) {
            rows.push_back(hbox({text(pnana::ui::icons::WARNING) | color(colors.warning),
                                 text(" No remote tracking branch") | color(colors.comment)}));
        } else {
            rows.push_back(text("Remote status unavailable") | color(colors.comment));
        }

        return vbox(std::move(rows)) | border | flex;
    };

    auto build_ops_card = [&]() -> Element {
        Elements rows;
        rows.push_back(hbox({text(pnana::ui::icons::GIT_REMOTE) | color(colors.function),
                             text(" Operations") | color(colors.foreground) | bold}));
        rows.push_back(separator());

        rows.push_back(hbox({text("[p]") | color(colors.success) | bold | bgcolor(colors.selection),
                             text(" ") | color(colors.background),
                             text(pnana::ui::icons::UPLOAD) | color(colors.success),
                             text(" Push") | color(colors.foreground), filler(),
                             text("upload commits") | color(colors.comment)}));

        rows.push_back(hbox({text("[l]") | color(colors.warning) | bold | bgcolor(colors.selection),
                             text(" ") | color(colors.background),
                             text(pnana::ui::icons::DOWNLOAD) | color(colors.warning),
                             text(" Pull") | color(colors.foreground), filler(),
                             text("merge remote changes") | color(colors.comment)}));

        rows.push_back(hbox({text("[f]") | color(colors.keyword) | bold | bgcolor(colors.selection),
                             text(" ") | color(colors.background),
                             text(pnana::ui::icons::REFRESH) | color(colors.keyword),
                             text(" Fetch") | color(colors.foreground), filler(),
                             text("download without merge") | color(colors.comment)}));

        rows.push_back(separatorLight());
        rows.push_back(hbox({text(pnana::ui::icons::INFO_CIRCLE) | color(colors.comment),
                             text(" Tip: fetch before pull if unsure") | color(colors.comment)}));

        return vbox(std::move(rows)) | border | flex;
    };

    Element cards = hbox(
        {build_status_card(), separator() | color(colors.comment) | dim, build_ops_card() | flex});

    elements.push_back(cards | flex);

    return vbox(std::move(elements));
}

Element GitPanel::renderDiffPanel() {
    auto& colors = theme_.getColors();

    // 如果数据正在加载，显示加载指示器
    if (data_loading_) {
        return vbox({text("Loading files for diff...") | color(colors.comment) | center,
                     text("Please wait...") | color(colors.menubar_fg) | center}) |
               center | size(HEIGHT, EQUAL, 10);
    }

    Elements diff_elements;

    // Enhanced header
    Elements header_elements = {text(pnana::ui::icons::GIT_DIFF) | color(colors.function),
                                text(" Diff View") | color(colors.foreground) | bold};
    diff_elements.push_back(hbox(std::move(header_elements)));
    diff_elements.push_back(separator());

    // File list with improved display
    Elements file_header = {text(pnana::ui::icons::FILE) | color(colors.function),
                            text(" Files with changes:") | color(colors.menubar_fg)};
    diff_elements.push_back(hbox(std::move(file_header)));
    // Removed separator line between header and file list for cleaner diff panel UI

    // File list
    const size_t MAX_VISIBLE_FILES = 20; // Show more files in diff panel
    size_t start = scroll_offset_;
    size_t end = std::min(start + MAX_VISIBLE_FILES, files_.size());

    for (size_t i = start; i < end; ++i) {
        bool is_highlighted = (i == selected_index_);
        diff_elements.push_back(renderDiffFileItem(files_[i], i, is_highlighted));
    }

    if (files_.empty()) {
        Elements empty_elements = {text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success),
                                   text(" No files with changes") | color(colors.success)};
        diff_elements.push_back(hbox(std::move(empty_elements)) | center);
    }

    return vbox(std::move(diff_elements));
}

Element GitPanel::renderDiffFileItem(const GitFile& file, size_t /*index*/, bool is_highlighted) {
    auto& colors = theme_.getColors();

    std::string status_icon = getStatusIcon(file.status);
    std::string status_text = getStatusText(file.status);
    Color status_color = getStatusColor(file.status);

    // File path/name - handle long paths
    std::string display_name = file.path;
    size_t last_slash = display_name.find_last_of("/\\");
    if (last_slash != std::string::npos && last_slash > 0) {
        std::string path_part = display_name.substr(0, last_slash);
        std::string name_part = display_name.substr(last_slash + 1);

        // Truncate path if too long
        if (path_part.length() > 25) {
            path_part = "..." + path_part.substr(path_part.length() - 22);
        }

        display_name = path_part + "/" + name_part;
    }

    Elements row_elements = {text(" "), // Leading space
                             text(status_icon) | color(status_color) | bold,
                             text(" "), // Space separator
                             text(display_name) | color(colors.foreground),
                             text(" "), // Space separator
                             text("(" + status_text + ")") | color(colors.comment)};

    auto item_text = hbox(std::move(row_elements));

    if (is_highlighted) {
        item_text = item_text | bgcolor(colors.selection) | color(colors.background) | bold;
    } else {
        item_text = item_text | bgcolor(colors.background);
    }

    return item_text;
}

Element GitPanel::renderFileItem(const GitFile& file, size_t /*index*/, bool is_selected,
                                 bool is_highlighted) {
    auto& colors = theme_.getColors();

    // Enhanced git file item styling inspired by file_browser_view and neovim git plugins
    // Use a different color for staged files to make them stand out per theme
    Color item_color = file.staged ? colors.success : colors.foreground;
    std::string status_text = getStatusText(file.status);

    // Get file type icon (use the mapper only; remove the old file icon to avoid duplication)
    std::string file_type_icon = icon_mapper_.getIcon(getFileExtension(file.path));

    // Staged status indicator - more prominent than file browser
    std::string staged_indicator;
    Color staged_color = colors.comment;
    if (file.staged) {
        staged_indicator = "●"; // Solid circle for staged files
        staged_color = colors.success;
    } else {
        staged_indicator = "○"; // Outline circle for unstaged files
        staged_color = colors.comment;
    }

    // Status icon with color coding based on git status
    Color status_color = getStatusColor(file.status);

    // Special highlighting for conflicted files
    bool is_conflicted = (file.status == GitFileStatus::UPDATED_BUT_UNMERGED);
    Color background_color = colors.background;
    if (is_conflicted && !is_selected && !is_highlighted) {
        background_color = Color::RGB(139, 69, 19); // Saddle brown background for conflicts
    }

    // File path/name - handle long/strange git output by sanitizing and showing just filename when
    // appropriate. Examples to handle: lines like "100644 100644 100644 51f6b69f... .gitignore"
    // which may appear when parsing raw git output.
    std::string display_name = file.path;
    std::string file_path = file.path;

    // If the path looks like a raw git index line (contains spaces, no directory separators, and is
    // long), assume the real filename is the last token and use that for display.
    if (display_name.find(' ') != std::string::npos &&
        display_name.find('/') == std::string::npos && display_name.length() > 30) {
        size_t last_space = display_name.find_last_of(' ');
        if (last_space != std::string::npos && last_space + 1 < display_name.size()) {
            display_name = display_name.substr(last_space + 1);
            file_path = display_name; // keep file_path consistent with display_name for later use
        }
    }

    // If path contains directory separators, show only filename with dimmed path
    size_t last_slash = display_name.find_last_of("/\\");
    if (last_slash != std::string::npos && last_slash > 0) {
        std::string path_part = display_name.substr(0, last_slash);
        std::string name_part = display_name.substr(last_slash + 1);

        // Truncate path if too long
        if (path_part.length() > 20) {
            path_part = "..." + path_part.substr(path_part.length() - 17);
        }

        display_name = path_part + "/" + name_part;
    }

    // Enhanced status info with additional context
    std::string metadata = status_text;
    // Sanitize old_path display similar to path, to avoid showing raw index lines in rename
    // metadata.
    std::string old_path_display = file.old_path;
    if (!old_path_display.empty() && old_path_display.find(' ') != std::string::npos &&
        old_path_display.find('/') == std::string::npos && old_path_display.length() > 30) {
        size_t last_space = old_path_display.find_last_of(' ');
        if (last_space != std::string::npos && last_space + 1 < old_path_display.size()) {
            old_path_display = old_path_display.substr(last_space + 1);
        }
    }
    if (old_path_display != file.path && !old_path_display.empty()) {
        // Show rename information using sanitized old path
        metadata += " → " + old_path_display;
    }

    // Build enhanced row elements with better visual hierarchy:
    // [space][staged_indicator][space][file_type_icon][space][filename][space][metadata][space]
    Elements row_elements = {
        text(" "), // Leading space for visual breathing room
        text(staged_indicator) | color(staged_color) | bold, // Staged status (prominent)
        text(" "),                                           // Space separator
        // Use file type icon (new) and keep status text in metadata to avoid duplicate icons
        text(file_type_icon) | color(colors.function), // File type icon
        text(" "),                                     // Space separator
        text(display_name) | color(item_color),        // File path/name
        text(" "),                                     // Space separator
        text(metadata) |
            color(status_color) // Status metadata (use status_color to avoid unused variable)
    };

    // For renamed files, add extra visual indicator
    if (!old_path_display.empty() && old_path_display != file.path) {
        row_elements.push_back(text(" "));
        row_elements.push_back(text("↳") | color(colors.comment));
        row_elements.push_back(text(" "));
        row_elements.push_back(text(old_path_display) | color(colors.comment) | dim);
    }

    auto item_text = hbox(std::move(row_elements));

    // Selection and highlight styling with clearer distinctions:
    // - Highlighted (cursor) items show selection background and bold text.
    // - User-selected items (toggled) show a dimmed background and a visible marker.
    // - If both, combine both visual hints.
    // Add a small selection marker to the left for toggled selection.
    std::string selection_marker = is_selected ? "[*]" : "[ ]";

    // Prepend selection marker visually (as separate element)
    item_text = hbox(text(selection_marker) | color(is_selected ? colors.success : colors.comment),
                     item_text);

    if (is_highlighted && is_selected) {
        // Show selection background and bold, but DO NOT override child element colors
        item_text = item_text | bgcolor(colors.selection) | bold;
    } else if (is_highlighted) {
        // Highlighted (cursor) - show selection background but preserve element colors.
        item_text = item_text | bgcolor(colors.selection) | bold;
    } else if (is_selected) {
        // dim background for selected but not highlighted
        item_text = item_text | bgcolor(Color::RGB(30, 30, 30));
    } else {
        item_text = item_text | bgcolor(background_color);
        // Add special styling for conflicted files
        if (is_conflicted) {
            item_text = item_text | color(Color::White); // White text on brown background
        }
    }

    return item_text;
}

Element GitPanel::renderBranchItem(const GitBranch& branch, size_t /*index*/, bool is_selected) {
    auto& colors = theme_.getColors();

    // Strict file browser-like styling for branches
    Color item_color = colors.foreground;

    // Branch status indicator (like expand icon in file browser)
    std::string branch_indicator;
    if (branch.is_current) {
        branch_indicator = pnana::ui::icons::CHECK_CIRCLE; // Current branch indicator
    } else if (branch.is_remote) {
        branch_indicator = pnana::ui::icons::GIT_REMOTE; // Remote branch indicator
    } else {
        branch_indicator = pnana::ui::icons::GIT_BRANCH; // Local branch indicator
    }

    // Branch icon (like file icon in file browser)
    std::string branch_icon = ""; // Git branch icon

    // Branch name (primary element like file name)
    std::string display_name = branch.name;

    // Branch metadata (like file info in file browser)
    std::string metadata = branch.is_remote ? "remote" : "local";

    // Build row elements following file browser exact pattern:
    // [space][branch_indicator][space][branch_icon][space][branch_name][space][metadata]
    Elements row_elements = {
        text(" "), // Leading space (like file browser)
        text(branch_indicator) |
            color(branch.is_current ? colors.success : colors.comment), // Branch status indicator
        text(" "),                                                      // Space (like file browser)
        text(branch_icon) | color(item_color),  // Branch icon (like file icon)
        text(" "),                              // Space (like file browser)
        text(display_name) | color(item_color), // Branch name (like file name)
        text(" "),                              // Space (like file browser)
        text(metadata) | color(colors.comment)  // Branch metadata (like file info)
    };

    auto item_text = hbox(std::move(row_elements));

    // Selection highlighting exactly like file browser
    if (is_selected) {
        item_text = item_text | bgcolor(colors.selection) | bold;
    } else {
        item_text = item_text | bgcolor(colors.background);
    }

    return item_text;
}

Element GitPanel::renderClonePanel() {
    auto& colors = theme_.getColors();

    Elements elements;

    // Enhanced header
    Elements header_elements = {text(pnana::ui::icons::DOWNLOAD) | color(colors.success),
                                text(" Clone Repository") | color(colors.foreground) | bold};
    elements.push_back(hbox(std::move(header_elements)));
    elements.push_back(separator());

    // Show clone state status
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        if (clone_state_ == CloneState::CLONING) {
            Elements cloning_elements = {
                text(pnana::ui::icons::REFRESH) | color(colors.keyword),
                text(" Cloning repository...") | color(colors.keyword) | bold};
            elements.push_back(hbox(std::move(cloning_elements)));
            elements.push_back(separatorLight());
        } else if (clone_state_ == CloneState::SUCCESS) {
            // Show success message for 5 seconds
            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - clone_state_time_);

            if (elapsed.count() < 5000) { // Show for 5 seconds
                Elements success_elements = {
                    text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success),
                    text(" ") | color(colors.background),
                    text(clone_success_message_) | color(colors.success) | bold};
                elements.push_back(hbox(std::move(success_elements)));
                elements.push_back(separatorLight());
            } else {
                // Auto-reset after 5 seconds
                clone_state_ = CloneState::IDLE;
                clone_url_.clear();
                clone_path_ = git_manager_->getRepositoryRoot().empty()
                                  ? fs::current_path().string()
                                  : git_manager_->getRepositoryRoot();
                clone_focus_on_url_ = true;
                clone_success_message_.clear();
            }
        } else if (clone_state_ == CloneState::FAILED && !error_message_.empty()) {
            Elements error_elements = {
                text(pnana::ui::icons::ERROR) | color(colors.error),
                text(" ") | color(colors.background),
                text("Clone failed: " + error_message_) | color(colors.error) | bold};
            elements.push_back(hbox(std::move(error_elements)));
            elements.push_back(separatorLight());
        }
    }

    // Repository URL input
    Elements url_header = {text(pnana::ui::icons::LINK) | color(colors.keyword),
                           text(" Repository URL (HTTPS/SSH):") | color(colors.menubar_fg)};
    elements.push_back(hbox(std::move(url_header)));

    // URL input with focus indication
    auto url_input = text(clone_url_) | color(colors.foreground);
    if (clone_focus_on_url_ && clone_state_ != CloneState::CLONING) {
        url_input = url_input | border | bgcolor(colors.selection) | color(colors.background);
    } else {
        url_input = url_input | border | bgcolor(colors.background);
    }
    elements.push_back(url_input);
    elements.push_back(separatorLight());

    // Clone path input
    Elements path_header = {text(pnana::ui::icons::FOLDER) | color(colors.function),
                            text(" Clone to path:") | color(colors.menubar_fg)};
    elements.push_back(hbox(std::move(path_header)));

    // Path input with focus indication
    auto path_input = text(clone_path_) | color(colors.foreground);
    if (!clone_focus_on_url_ && clone_state_ != CloneState::CLONING) {
        path_input = path_input | border | bgcolor(colors.selection) | color(colors.background);
    } else {
        path_input = path_input | border | bgcolor(colors.background);
    }
    elements.push_back(path_input);
    elements.push_back(separatorLight());

    // Instructions (hide when cloning)
    if (clone_state_ != CloneState::CLONING) {
        Elements instructions = {
            text(pnana::ui::icons::INFO_CIRCLE) | color(colors.info),
            text(" Enter repository URL and destination path, then press Enter to clone") |
                color(colors.comment)};
        elements.push_back(hbox(std::move(instructions)));

        elements.push_back(separatorLight());

        // Examples
        Elements examples_header = {text("Examples:") | color(colors.menubar_fg)};
        elements.push_back(hbox(std::move(examples_header)));

        Elements https_example = {text("HTTPS: ") | color(colors.comment),
                                  text("https://github.com/user/repo.git") | color(colors.string)};
        elements.push_back(hbox(std::move(https_example)));

        Elements ssh_example = {text("SSH:   ") | color(colors.comment),
                                text("git@github.com:user/repo.git") | color(colors.string)};
        elements.push_back(hbox(std::move(ssh_example)));
    }

    return vbox(std::move(elements));
}

bool GitPanel::handleCloneModeKey(Event event) {
    // Tab navigation between modes
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    // Don't allow input when cloning
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        if (clone_state_ == CloneState::CLONING) {
            // Only allow ESC to cancel (though we can't actually cancel the async operation)
            if (event == Event::Escape) {
                // Note: The clone operation will continue in background
                // User can switch modes, but clone state will update when done
                return false; // Let ESC bubble up to switch mode
            }
            return true; // Consume other events while cloning
        }
    }

    // Up/Down arrow keys to switch between input fields
    if (event == Event::ArrowUp || event == Event::ArrowDown) {
        clone_focus_on_url_ = !clone_focus_on_url_;
        component_needs_rebuild_ = true;
        return true;
    }

    if (event == Event::Return) {
        performClone();
        return true;
    }
    if (event == Event::Escape) {
        // Reset clone state when escaping
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            if (clone_state_ != CloneState::CLONING) {
                clone_state_ = CloneState::IDLE;
                clone_success_message_.clear();
            }
        }
        switchMode(GitPanelMode::STATUS);
        return true;
    }

    // Handle text input based on current focus
    if (event.is_character()) {
        if (clone_focus_on_url_) {
            clone_url_ += event.character();
        } else {
            clone_path_ += event.character();
        }
        component_needs_rebuild_ = true;
        return true;
    }
    if (event == Event::Backspace) {
        if (clone_focus_on_url_ && !clone_url_.empty()) {
            clone_url_.pop_back();
        } else if (!clone_focus_on_url_ && !clone_path_.empty()) {
            clone_path_.pop_back();
        }
        component_needs_rebuild_ = true;
        return true;
    }

    return false;
}

void GitPanel::performClone() {
    if (clone_url_.empty()) {
        error_message_ = "Repository URL cannot be empty";
        clone_state_ = CloneState::FAILED;
        clone_state_time_ = std::chrono::steady_clock::now();
        return;
    }

    if (clone_path_.empty()) {
        error_message_ = "Clone path cannot be empty";
        clone_state_ = CloneState::FAILED;
        clone_state_time_ = std::chrono::steady_clock::now();
        return;
    }

    // Set cloning state
    {
        std::lock_guard<std::mutex> lock(data_mutex_);
        clone_state_ = CloneState::CLONING;
        clone_state_time_ = std::chrono::steady_clock::now();
        error_message_.clear();
        clone_success_message_.clear();
        component_needs_rebuild_ = true; // Force UI update to show cloning state
    }

    // Start async clone operation
    std::thread([this]() {
        std::string url_to_clone = clone_url_;
        std::string path_to_clone = clone_path_;

        GitManager temp_manager(path_to_clone);
        bool success = temp_manager.clone(url_to_clone, path_to_clone);

        // Update state in main thread
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            if (success) {
                clone_state_ = CloneState::SUCCESS;
                clone_state_time_ = std::chrono::steady_clock::now();

                // Build success message with repository name only
                std::string repo_name = url_to_clone;
                // Extract repo name from URL (e.g., "user/repo.git" or "user/repo")
                size_t last_slash = repo_name.find_last_of('/');
                if (last_slash != std::string::npos) {
                    repo_name = repo_name.substr(last_slash + 1);
                    // Remove .git suffix if present
                    if (repo_name.length() > 4 &&
                        repo_name.substr(repo_name.length() - 4) == ".git") {
                        repo_name = repo_name.substr(0, repo_name.length() - 4);
                    }
                }

                clone_success_message_ = "Repository '" + repo_name + "' cloned successfully!";
                error_message_.clear(); // Clear any previous errors

                // Clear inputs on success (after a delay, so user can see the success message)
                // We'll clear them after 3 seconds or when user switches mode
            } else {
                clone_state_ = CloneState::FAILED;
                clone_state_time_ = std::chrono::steady_clock::now();

                // Only set error message if there's an actual error (consistent with remote
                // operations)
                std::string error = temp_manager.getLastError();
                if (!error.empty()) {
                    error_message_ = error;
                } else {
                    error_message_ = "Clone operation failed";
                }
            }
            component_needs_rebuild_ = true; // Force UI update to show result
        }
    }).detach();
}

Element GitPanel::renderDiffViewer() {
    if (!diff_viewer_visible_) {
        return emptyElement();
    }

    auto& colors = theme_.getColors();

    Elements viewer_elements;

    // Header: file path + stats + controls
    std::string dir_part;
    std::string base_part = current_diff_file_;
    try {
        fs::path p(current_diff_file_);
        base_part = p.filename().string();
        dir_part =
            p.has_parent_path() ? (p.parent_path().string() + fs::path::preferred_separator) : "";
    } catch (...) {
        // best-effort only
    }

    // Truncate directory path if too long
    if (dir_part.size() > 48) {
        dir_part = "..." + dir_part.substr(dir_part.size() - 45);
    }

    // Diff stats (added/removed)
    size_t added = 0;
    size_t removed = 0;
    for (const auto& l : diff_content_) {
        if (!l.empty() && l[0] == '+' && l.rfind("+++", 0) != 0) {
            added++;
        } else if (!l.empty() && l[0] == '-' && l.rfind("---", 0) != 0) {
            removed++;
        }
    }

    Elements header_row = {text(pnana::ui::icons::GIT_DIFF) | color(colors.function),
                           text(" ") | color(colors.foreground),
                           text(dir_part) | color(colors.comment) | dim,
                           text(base_part) | color(colors.foreground) | bold, text("  ")};

    if (!diff_content_.empty()) {
        header_row.push_back(text("+") | color(colors.success) | bold);
        header_row.push_back(text(std::to_string(added)) | color(colors.success));
        header_row.push_back(text(" ") | color(colors.comment));
        header_row.push_back(text("-") | color(colors.error) | bold);
        header_row.push_back(text(std::to_string(removed)) | color(colors.error));
        header_row.push_back(text("  ") | color(colors.comment));
    }

    header_row.push_back(filler());
    header_row.push_back(text("PgUp/PgDn") | color(colors.keyword) | bold);
    header_row.push_back(text(":↑↓scroll  ") | color(colors.comment));
    header_row.push_back(text("Tab") | color(colors.keyword) | bold);
    header_row.push_back(text(":⇄scroll  ") | color(colors.comment));
    header_row.push_back(text("ESC") | color(colors.keyword) | bold);
    header_row.push_back(text(":close") | color(colors.comment));

    viewer_elements.push_back(hbox(std::move(header_row)) | bgcolor(colors.dialog_title_bg) |
                              color(colors.dialog_title_fg));
    viewer_elements.push_back(separatorLight());

    // Diff content with enhanced neovim-like styling
    size_t start_line = diff_scroll_offset_;
    size_t end_line = std::min(start_line + DIFF_VISIBLE_LINES, diff_content_.size());

    if (diff_content_.empty()) {
        viewer_elements.push_back(text("No diff content available") | color(colors.comment) |
                                  center);
    } else {
        // Calculate line number width
        int line_num_width = std::to_string(std::max(diff_content_.size(), size_t(1))).length();

        for (size_t i = start_line; i < end_line; ++i) {
            const std::string& line_content = diff_content_[i];

            // Determine line type and styling
            bool is_addition = !line_content.empty() && line_content[0] == '+';
            bool is_deletion = !line_content.empty() && line_content[0] == '-';
            bool is_hunk_header = !line_content.empty() && line_content.substr(0, 2) == "@@";
            bool is_file_header = !line_content.empty() && ((line_content.substr(0, 3) == "+++") ||
                                                            (line_content.substr(0, 3) == "---"));

            // Create line elements
            Elements line_elements;

            // Line number (only for non-file headers)
            if (!is_file_header) {
                std::string line_num = std::to_string(i + 1);
                std::string padded_line_num =
                    std::string(line_num_width - line_num.length(), ' ') + line_num;
                Color line_num_color = is_hunk_header ? colors.keyword : colors.comment;
                line_elements.push_back(text(padded_line_num) | color(line_num_color) | dim);
                line_elements.push_back(text("│") | color(colors.comment) | dim);
            } else {
                line_elements.push_back(text(std::string(line_num_width, ' ')));
                line_elements.push_back(text(" ") | color(colors.comment));
            }

            // Line content with appropriate styling and horizontal scroll
            std::string visible_content = line_content;
            if (diff_h_offset_ < visible_content.size()) {
                visible_content = visible_content.substr(diff_h_offset_);
            } else {
                visible_content.clear();
            }

            Element content_element;
            if (is_addition) {
                // Additions: green foreground + subtle background (skip +++ header)
                if (!is_file_header) {
                    content_element = text(visible_content) | color(colors.success) |
                                      bgcolor(Color::RGB(20, 60, 30));
                } else {
                    content_element = text(visible_content) | color(colors.success) | bold;
                }
            } else if (is_deletion) {
                // Deletions: red foreground + subtle background (skip --- header)
                if (!is_file_header) {
                    content_element = text(visible_content) | color(colors.error) |
                                      bgcolor(Color::RGB(60, 20, 20));
                } else {
                    content_element = text(visible_content) | color(colors.error) | bold;
                }
            } else if (is_hunk_header) {
                // Blue for hunk headers
                content_element = text(visible_content) | color(colors.keyword) | bold |
                                  bgcolor(Color::RGB(20, 30, 60));
            } else if (is_file_header) {
                // File headers with appropriate colors
                Color header_color =
                    (line_content.substr(0, 3) == "+++") ? colors.success : colors.error;
                content_element = text(visible_content) | color(header_color) | bold;
            } else {
                // Context lines - normal color
                content_element = text(visible_content) | color(colors.foreground);
            }

            line_elements.push_back(content_element | flex_grow);
            Element row = hbox(std::move(line_elements));
            // Add gentle row padding for readability
            viewer_elements.push_back(row);
        }

        // Bottom status line (page + range + horizontal scroll)
        if (diff_content_.size() > DIFF_VISIBLE_LINES || diff_h_offset_ > 0) {
            size_t from = start_line + 1;
            size_t to = end_line;
            std::string info = " " + std::to_string(from) + "-" + std::to_string(to) + "/" +
                               std::to_string(diff_content_.size()) + " ";

            if (diff_h_offset_ > 0) {
                info += " | Col: " + std::to_string(diff_h_offset_ + 1);
            }

            viewer_elements.push_back(separatorLight());
            viewer_elements.push_back(text(info) | color(colors.comment) | center | dim);
        }
    }

    // Modal-like viewer with fixed size (similar to fzf popup)
    // 使用简单边框，标题已在 header_row 中显示
    return vbox(std::move(viewer_elements)) | size(WIDTH, EQUAL, 80) |
           size(HEIGHT, EQUAL, DIFF_VISIBLE_LINES + 6) | borderWithColor(colors.dialog_border) |
           flex | bgcolor(colors.background);
}

Color GitPanel::getDiffLineColor(const std::string& line) {
    auto& colors = theme_.getColors();

    if (line.empty()) {
        return colors.foreground;
    }

    if (line[0] == '+') {
        return colors.success; // Green for additions
    } else if (line[0] == '-') {
        return colors.error; // Red for deletions
    } else if (line[0] == '@') {
        return colors.keyword; // Blue for hunk headers
    } else if (line.substr(0, 3) == "+++") {
        return colors.success;
    } else if (line.substr(0, 3) == "---") {
        return colors.error;
    }

    return colors.comment; // Gray for context lines
}

Element GitPanel::renderFooter() {
    auto& colors = theme_.getColors();

    Elements footer_elements;

    switch (current_mode_) {
        case GitPanelMode::STATUS: {
            Elements line1 = {text("Navigation: ↑↓/PgUp/PgDn/Home/End") | color(colors.comment),
                              text(" | ") | color(colors.comment),
                              text("Select: Space/a/A") | color(colors.comment),
                              text(" | ") | color(colors.comment),
                              text("Stage: s/S/u/U") | color(colors.comment)};
            Elements line2 = {
                text("Switch tab: Tab") | color(colors.comment),
                text(" | ") | color(colors.comment), text("Refresh: R/F5") | color(colors.comment),
                text(" | ") | color(colors.comment), text("Exit: ESC") | color(colors.comment)};

            footer_elements.push_back(hbox(line1));
            footer_elements.push_back(hbox(line2));
            break;
        }
        case GitPanelMode::DIFF: {
            Elements diff_help = {text("Navigation: ↑↓") | color(colors.comment),
                                  text(" | ") | color(colors.comment),
                                  text("View diff: Enter") | color(colors.success) | bold,
                                  text(" | ") | color(colors.comment),
                                  text("Switch tab: Tab") | color(colors.comment),
                                  text(" | ") | color(colors.comment),
                                  text("Back: ESC") | color(colors.comment)};
            footer_elements.push_back(hbox(diff_help));
            break;
        }
        case GitPanelMode::COMMIT: {
            Elements commit_help = {text("Commit: Enter") | color(colors.success) | bold,
                                    text(" | ") | color(colors.comment),
                                    text("Switch mode: Tab") | color(colors.comment),
                                    text(" | ") | color(colors.comment),
                                    text("Back: ESC") | color(colors.comment)};
            footer_elements.push_back(hbox(commit_help));
            break;
        }
        case GitPanelMode::BRANCH: {
            Elements branch_help = {text("Navigate: ↑↓") | color(colors.comment),
                                    text(" | ") | color(colors.comment),
                                    text("Switch: Enter") | color(colors.success) | bold,
                                    text(" | ") | color(colors.comment),
                                    text("New: Ctrl+N") | color(colors.comment),
                                    text(" | ") | color(colors.comment),
                                    text("Switch mode: Tab") | color(colors.comment),
                                    text(" | ") | color(colors.comment),
                                    text("Back: ESC") | color(colors.comment)};
            footer_elements.push_back(hbox(branch_help));
            break;
        }
        case GitPanelMode::REMOTE: {
            Elements remote_help = {text("Push: [p]") | color(colors.success) | bold,
                                    text(" | ") | color(colors.comment),
                                    text("Pull: [l]") | color(colors.warning) | bold,
                                    text(" | ") | color(colors.comment),
                                    text("Fetch: [f]") | color(colors.keyword) | bold,
                                    text(" | ") | color(colors.comment),
                                    text("Switch mode: Tab") | color(colors.comment),
                                    text(" | ") | color(colors.comment),
                                    text("Back: ESC") | color(colors.comment)};
            footer_elements.push_back(hbox(remote_help));
            break;
        }
        case GitPanelMode::CLONE: {
            Elements clone_help = {text("Clone: Enter") | color(colors.success) | bold,
                                   text(" | ") | color(colors.comment),
                                   text("Switch field: ↑↓") | color(colors.comment),
                                   text(" | ") | color(colors.comment),
                                   text("Switch mode: Tab") | color(colors.comment),
                                   text(" | ") | color(colors.comment),
                                   text("Back: ESC") | color(colors.comment)};
            footer_elements.push_back(hbox(clone_help));
            break;
        }
        case GitPanelMode::GRAPH: {
            Elements graph_help = {
                text("Navigation: ↑↓/PgUp/PgDn/Home/End") | color(colors.comment),
                text(" | ") | color(colors.comment),
                text("Refresh: R/F5") | color(colors.comment),
                text(" | ") | color(colors.comment),
                text("Switch mode: Tab") | color(colors.comment),
                text(" | ") | color(colors.comment),
                text("Back: ESC") | color(colors.comment)};
            footer_elements.push_back(hbox(graph_help));
            break;
        }
    }

    // Add scroll indicator if needed
    if (current_mode_ == GitPanelMode::STATUS && files_.size() > 25) {
        size_t total_pages = (files_.size() + 24) / 25; // Ceiling division
        size_t current_page = scroll_offset_ / 25 + 1;
        (void)current_page;
        (void)total_pages;
        // 移除页码显示
    } else if (current_mode_ == GitPanelMode::BRANCH && branches_.size() > 18) {
        size_t total_pages = (branches_.size() + 17) / 18; // Ceiling division
        size_t current_page = scroll_offset_ / 18 + 1;
        (void)current_page;
        (void)total_pages;
        // 移除页码显示
    } else if (current_mode_ == GitPanelMode::DIFF && files_.size() > 20) {
        size_t total_pages = (files_.size() + 19) / 20; // Ceiling division
        size_t current_page = scroll_offset_ / 20 + 1;
        (void)current_page;
        (void)total_pages;
        // 移除页码显示
    }

    return vbox(std::move(footer_elements)) | bgcolor(colors.menubar_bg);
}

Element GitPanel::renderError() {
    if (error_message_.empty())
        return emptyElement();

    return text("Error: " + error_message_) | color(Color::Red) | border;
}

Element GitPanel::separatorLight() {
    // Avoid fixed-width separators that can force the UI to grow wider.
    return separator() | color(theme_.getColors().comment);
}

// Component builders

Component GitPanel::buildMainComponent() {
    return Renderer([this] {
               if (!visible_)
                   return emptyElement();

               auto& colors = theme_.getColors();

               Elements content_elements;
               content_elements.push_back(renderHeader());
               content_elements.push_back(renderTabs());
               content_elements.push_back(separatorLight());

               switch (current_mode_) {
                   case GitPanelMode::STATUS:
                       content_elements.push_back(renderStatusPanel());
                       break;
                   case GitPanelMode::COMMIT:
                       content_elements.push_back(renderCommitPanel());
                       break;
                   case GitPanelMode::BRANCH:
                       content_elements.push_back(renderBranchPanel());
                       break;
                   case GitPanelMode::REMOTE:
                       content_elements.push_back(renderRemotePanel());
                       break;
                   case GitPanelMode::CLONE:
                       content_elements.push_back(renderClonePanel());
                       break;
                   case GitPanelMode::DIFF:
                       content_elements.push_back(renderDiffPanel());
                       break;
                   case GitPanelMode::GRAPH:
                       content_elements.push_back(renderGraphPanel());
                       break;
               }

               if (!error_message_.empty()) {
                   content_elements.push_back(separator());
                   content_elements.push_back(renderError());
               }

               content_elements.push_back(separatorLight());
               content_elements.push_back(renderFooter());

               Element dialog_content = vbox(std::move(content_elements));

               // Use window style like other dialogs with proper sizing
               Element main_panel = window(text("Git Panel"), dialog_content) |
                                    size(WIDTH, GREATER_THAN, 75) | size(HEIGHT, GREATER_THAN, 28) |
                                    bgcolor(colors.background) |
                                    borderWithColor(colors.dialog_border);

               // If diff viewer is visible, render it on top
               if (diff_viewer_visible_) {
                   return dbox({main_panel, renderDiffViewer()});
               }

               return main_panel;
           }) |
           CatchEvent([this](Event event) {
               // Handle diff viewer events first if visible
               if (diff_viewer_visible_) {
                   if (event == Event::Escape) {
                       hideDiffViewer();
                       component_needs_rebuild_ =
                           true;    // Force component rebuild after hiding viewer
                       return true; // Consume ESC event, don't let it bubble up
                   }
                   if (event == Event::PageUp) {
                       if (diff_scroll_offset_ > 0) {
                           diff_scroll_offset_ = (diff_scroll_offset_ > DIFF_VISIBLE_LINES)
                                                     ? diff_scroll_offset_ - DIFF_VISIBLE_LINES
                                                     : 0;
                           component_needs_rebuild_ =
                               true; // Force component rebuild after scrolling
                       }
                       return true;
                   }
                   if (event == Event::PageDown) {
                       size_t max_offset = diff_content_.size() > DIFF_VISIBLE_LINES
                                               ? diff_content_.size() - DIFF_VISIBLE_LINES
                                               : 0;
                       if (diff_scroll_offset_ < max_offset) {
                           diff_scroll_offset_ += DIFF_VISIBLE_LINES;
                           if (diff_scroll_offset_ > max_offset) {
                               diff_scroll_offset_ = max_offset;
                           }
                           component_needs_rebuild_ =
                               true; // Force component rebuild after scrolling
                       }
                       return true;
                   }
                   // Tab: horizontal scroll (similar to fzf popup preview)
                   if (event == Event::Tab) {
                       if (!diff_content_.empty()) {
                           // Calculate max line length
                           size_t max_len = 0;
                           for (const auto& line : diff_content_) {
                               max_len = std::max(max_len, line.size());
                           }

                           if (max_len <= DIFF_H_SCROLL_STEP) {
                               diff_h_offset_ = 0;
                           } else if (diff_h_offset_ + DIFF_H_SCROLL_STEP >= max_len) {
                               diff_h_offset_ = 0; // Reset to beginning when reaching end
                           } else {
                               diff_h_offset_ += DIFF_H_SCROLL_STEP;
                           }
                           component_needs_rebuild_ = true;
                       }
                       return true;
                   }
                   return true; // Consume all events when diff viewer is open
               }

               // Handle navigation events directly without rebuilding component
               if (event == Event::ArrowUp || event == Event::ArrowDown || event == Event::PageUp ||
                   event == Event::PageDown) {
                   switch (current_mode_) {
                       case GitPanelMode::STATUS:
                           return handleStatusModeKey(event);
                       case GitPanelMode::BRANCH:
                           return handleBranchModeKey(event);
                       case GitPanelMode::COMMIT:
                           return handleCommitModeKey(event);
                       case GitPanelMode::REMOTE:
                           return handleRemoteModeKey(event);
                       case GitPanelMode::CLONE:
                           return handleCloneModeKey(event);
                       case GitPanelMode::DIFF:
                           return handleDiffModeKey(event);
                       case GitPanelMode::GRAPH:
                           return handleGraphModeKey(event);
                   }
               }
               return false;
           });
}

// Key handlers

bool GitPanel::handleStatusModeKey(Event event) {
    const size_t MAX_VISIBLE_FILES = 25; // Must match renderStatusPanel

    // Tab navigation between modes (must be first)
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    // Ensure we have files to work with (but still handle Tab navigation)
    if (files_.empty()) {
        // Still allow tab navigation even if no files
        return false;
    }

    // Single item navigation
    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
            // Adjust scroll to keep selected item visible
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (selected_index_ < files_.size() - 1) {
            selected_index_++;
            // Adjust scroll to keep selected item visible
            if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_FILES) {
                scroll_offset_ = selected_index_ - MAX_VISIBLE_FILES + 1;
            }
        }
        return true;
    }

    // Page navigation (scroll by page size)
    if (event == Event::PageUp) {
        if (selected_index_ > 0) {
            size_t page_size = MAX_VISIBLE_FILES;
            if (selected_index_ >= page_size) {
                selected_index_ -= page_size;
            } else {
                selected_index_ = 0;
            }
            // Center the selected item in the visible area
            scroll_offset_ =
                (selected_index_ > page_size / 2) ? selected_index_ - page_size / 2 : 0;
        }
        return true;
    }
    if (event == Event::PageDown) {
        if (selected_index_ < files_.size() - 1) {
            size_t page_size = MAX_VISIBLE_FILES;
            size_t max_index = files_.size() - 1;
            if (selected_index_ + page_size < max_index) {
                selected_index_ += page_size;
            } else {
                selected_index_ = max_index;
            }
            // Center the selected item in the visible area
            size_t new_scroll =
                (selected_index_ > page_size / 2) ? selected_index_ - page_size / 2 : 0;
            // Don't scroll past the end
            if (new_scroll + MAX_VISIBLE_FILES > files_.size()) {
                new_scroll =
                    files_.size() > MAX_VISIBLE_FILES ? files_.size() - MAX_VISIBLE_FILES : 0;
            }
            scroll_offset_ = new_scroll;
        }
        return true;
    }

    // Home/End navigation
    if (event == Event::Home) {
        selected_index_ = 0;
        scroll_offset_ = 0;
        return true;
    }
    if (event == Event::End) {
        selected_index_ = files_.size() - 1;
        // Scroll to show the last page
        scroll_offset_ =
            (files_.size() > MAX_VISIBLE_FILES) ? files_.size() - MAX_VISIBLE_FILES : 0;
        return true;
    }

    // Selection operations
    if (event == Event::Character(' ')) { // Space - toggle selection
        toggleFileSelection(selected_index_);
        return true;
    }
    if (event == Event::Character('a')) { // Select all
        selectAll();
        return true;
    }
    if (event == Event::Character('A')) { // Select all (shift+A)
        clearSelection();
        return true;
    }

    // Git operations
    if (event == Event::Character('s')) { // Stage selected or toggle detailed stats / unstage
        if (selected_files_.empty()) {
            // No files selected, toggle detailed stats view
            show_detailed_stats_ = !show_detailed_stats_;
            return true;
        } else {
            // If all selected files are already staged, unstage them; otherwise stage selected
            bool all_staged = true;
            for (size_t idx : selected_files_) {
                if (idx < files_.size()) {
                    if (!files_[idx].staged) {
                        all_staged = false;
                        break;
                    }
                }
            }
            if (all_staged) {
                performUnstageSelected();
            } else {
                performStageSelected();
            }
            return true;
        }
    }
    if (event == Event::Character('u')) { // Unstage selected
        performUnstageSelected();
        return true;
    }
    if (event == Event::Character('S')) { // Stage all
        performStageAll();
        return true;
    }
    if (event == Event::Character('U')) { // Unstage all
        performUnstageAll();
        return true;
    }

    // Refresh data
    if (event == Event::Character('R') || event == Event::F5) { // Refresh/Ctrl+R or F5
        refreshData();
        return true;
    }

    // Quick actions on current file
    if (event == Event::Return) { // Enter - stage/unstage current file
        if (files_[selected_index_].staged) {
            performUnstageSelected();
        } else {
            performStageSelected();
        }
        return true;
    }

    // Refresh data
    if (event == Event::Character('R') || event == Event::F5) { // Refresh/Ctrl+R or F5
        refreshData();
        return true;
    }

    return false;
}

bool GitPanel::handleCommitModeKey(Event event) {
    // Tab navigation between modes
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    auto is_ctrl_modified_return = [&]() -> bool {
        if (!(event == Event::Return || event == Event::CtrlM)) {
            return false;
        }
        std::string in = event.input();
        std::transform(in.begin(), in.end(), in.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return in.find("ctrl") != std::string::npos;
    };

    // Ctrl+Enter: insert newline inside commit message (multi-line commit).
    // Fall back: Ctrl+J is commonly delivered as newline on terminals.
    if (is_ctrl_modified_return() || event == Event::CtrlJ) {
        if (commit_cursor_position_ > commit_message_.length()) {
            commit_cursor_position_ = commit_message_.length();
        }
        commit_message_.insert(commit_cursor_position_, "\n");
        commit_cursor_position_ += 1;
        return true;
    }

    // Enter: commit
    if (event == Event::Return) {
        performCommit();
        return true;
    }
    if (event == Event::Escape) {
        switchMode(GitPanelMode::STATUS);
        return true;
    }

    // Cursor movement
    if (event == Event::ArrowLeft) {
        if (commit_cursor_position_ > 0) {
            commit_cursor_position_--;
        }
        return true;
    }
    if (event == Event::ArrowRight) {
        if (commit_cursor_position_ < commit_message_.length()) {
            commit_cursor_position_++;
        }
        return true;
    }
    if (event == Event::Home) {
        commit_cursor_position_ = 0;
        return true;
    }
    if (event == Event::End) {
        commit_cursor_position_ = commit_message_.length();
        return true;
    }

    // Handle text input for commit message
    if (event.is_character()) {
        // Insert character at cursor position
        std::string char_str = event.character();
        if (!char_str.empty()) {
            if (commit_cursor_position_ >= commit_message_.length()) {
                commit_message_ += char_str;
            } else {
                commit_message_.insert(commit_cursor_position_, char_str);
            }
            commit_cursor_position_ += char_str.length();
        }
        return true;
    }
    if (event == Event::Backspace) {
        if (commit_cursor_position_ > 0) {
            if (commit_cursor_position_ >= commit_message_.length()) {
                commit_message_.pop_back();
            } else {
                commit_message_.erase(commit_cursor_position_ - 1, 1);
            }
            commit_cursor_position_--;
        }
        return true;
    }
    if (event == Event::Delete) {
        if (commit_cursor_position_ < commit_message_.length()) {
            commit_message_.erase(commit_cursor_position_, 1);
        }
        return true;
    }

    return false;
}

bool GitPanel::handleBranchModeKey(Event event) {
    // Tab navigation between modes
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    const size_t visible_items = 15; // Number of visible branch items

    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;

            // Auto-scroll up if needed
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (selected_index_ < branches_.size() - 1) {
            selected_index_++;

            // Auto-scroll down if needed
            if (selected_index_ >= scroll_offset_ + visible_items - 2) {
                scroll_offset_ = selected_index_ - visible_items + 3;
            }
        }
        return true;
    }
    if (event == Event::Return) {
        performSwitchBranch();
        return true;
    }
    // Ctrl+N to create new branch (avoid plain 'n' to reduce accidental creates)
    if (event == Event::CtrlN) {
        branch_name_.clear();
        branch_cursor_position_ = 0;
        return true;
    }
    // Ctrl+D to delete selected branch
    if (event == Event::CtrlD) {
        if (selected_index_ < branches_.size() && !branches_[selected_index_].is_current) {
            git_manager_->deleteBranch(branches_[selected_index_].name);
            refreshData();
        }
        return true;
    }
    if (event == Event::Escape) {
        switchMode(GitPanelMode::STATUS);
        return true;
    }

    // Handle text input for new branch name (always allow input for simplicity)
    if (event.is_character()) {
        std::string char_str = event.character();
        if (!char_str.empty()) {
            if (branch_cursor_position_ >= branch_name_.length()) {
                branch_name_ += char_str;
            } else {
                branch_name_.insert(branch_cursor_position_, char_str);
            }
            branch_cursor_position_ += char_str.length();
        }
        return true;
    }
    if (event == Event::Backspace) {
        if (branch_cursor_position_ > 0 && !branch_name_.empty()) {
            if (branch_cursor_position_ >= branch_name_.length()) {
                branch_name_.pop_back();
            } else {
                branch_name_.erase(branch_cursor_position_ - 1, 1);
            }
            branch_cursor_position_--;
        }
        return true;
    }
    if (event == Event::Delete) {
        if (branch_cursor_position_ < branch_name_.length()) {
            branch_name_.erase(branch_cursor_position_, 1);
        }
        return true;
    }
    if (event == Event::ArrowLeft) {
        if (branch_cursor_position_ > 0) {
            branch_cursor_position_--;
        }
        return true;
    }
    if (event == Event::ArrowRight) {
        if (branch_cursor_position_ < branch_name_.length()) {
            branch_cursor_position_++;
        }
        return true;
    }
    if (event == Event::Home) {
        branch_cursor_position_ = 0;
        return true;
    }
    if (event == Event::End) {
        branch_cursor_position_ = branch_name_.length();
        return true;
    }
    if (event == Event::Return && !branch_name_.empty()) {
        // Create new branch when Enter is pressed
        performCreateBranch();
        return true;
    }

    return false;
}

bool GitPanel::handleRemoteModeKey(Event event) {
    // Tab navigation between modes
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    // Handle both lowercase and uppercase letters
    if (event == Event::Character('p') || event == Event::Character('P')) {
        if (performPush()) {
            error_message_.clear(); // Clear any previous errors
        } else {
            error_message_ = git_manager_->getLastError();
        }
        return true;
    }
    if (event == Event::Character('l') || event == Event::Character('L')) {
        if (performPull()) {
            error_message_.clear(); // Clear any previous errors
        } else {
            error_message_ = git_manager_->getLastError();
        }
        return true;
    }
    if (event == Event::Character('f') || event == Event::Character('F')) {
        if (git_manager_->fetch()) {
            refreshData();
            error_message_.clear(); // Clear any previous errors
        } else {
            error_message_ = git_manager_->getLastError();
        }
        return true;
    }
    if (event == Event::Escape) {
        switchMode(GitPanelMode::STATUS);
        return true;
    }

    return false;
}

bool GitPanel::handleDiffModeKey(Event event) {
    // 如果 diff viewer 打开，Tab 键用于水平滚动，不切换模式
    if (diff_viewer_visible_ && event == Event::Tab) {
        if (!diff_content_.empty()) {
            // Calculate max line length
            size_t max_len = 0;
            for (const auto& line : diff_content_) {
                max_len = std::max(max_len, line.size());
            }

            if (max_len <= DIFF_H_SCROLL_STEP) {
                diff_h_offset_ = 0;
            } else if (diff_h_offset_ + DIFF_H_SCROLL_STEP >= max_len) {
                diff_h_offset_ = 0; // Reset to beginning when reaching end
            } else {
                diff_h_offset_ += DIFF_H_SCROLL_STEP;
            }
            component_needs_rebuild_ = true;
        }
        return true;
    }

    // Tab navigation between modes (only when diff viewer is not open)
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    const size_t MAX_VISIBLE_FILES = 20; // Must match renderDiffPanel

    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
            // Adjust scroll to keep selected item visible
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (selected_index_ < files_.size() - 1) {
            selected_index_++;
            // Adjust scroll to keep selected item visible
            if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_FILES) {
                scroll_offset_ = selected_index_ - MAX_VISIBLE_FILES + 1;
            }
        }
        return true;
    }

    if (event == Event::Return) {
        // Show diff viewer for selected file
        if (selected_index_ < files_.size()) {
            showDiffViewer(files_[selected_index_].path);
        }
        return true;
    }

    if (event == Event::Escape) {
        switchMode(GitPanelMode::STATUS);
        return true;
    }

    return false;
}

Element GitPanel::renderGraphPanel() {
    auto& colors = theme_.getColors();

    // 如果数据正在加载，显示加载指示器
    if (data_loading_ && graph_commits_.empty()) {
        return vbox({text("Loading git graph...") | color(colors.comment) | center,
                     text("Please wait...") | color(colors.menubar_fg) | center}) |
               center | size(HEIGHT, EQUAL, 10);
    }

    Elements graph_elements;

    // Enhanced header
    Elements header_elements = {
        text(pnana::ui::icons::GIT_BRANCH) | color(colors.function),
        text(" Git Graph") | color(colors.foreground) | bold, text(" | ") | color(colors.comment),
        text(std::to_string(graph_commits_.size()) + " commits") | color(colors.menubar_fg)};
    graph_elements.push_back(hbox(std::move(header_elements)));
    graph_elements.push_back(separator());

    // Commit list with improved display
    const size_t MAX_VISIBLE_COMMITS = 25;
    size_t start = scroll_offset_;
    size_t end = std::min(start + MAX_VISIBLE_COMMITS, graph_commits_.size());

    // Ensure selected_index_ is within valid range
    if (selected_index_ >= graph_commits_.size() && !graph_commits_.empty()) {
        selected_index_ = graph_commits_.size() - 1;
    }

    // Adjust scroll_offset_ to keep selected item visible
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
        start = scroll_offset_;
        end = std::min(start + MAX_VISIBLE_COMMITS, graph_commits_.size());
    } else if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_COMMITS) {
        scroll_offset_ = selected_index_ - MAX_VISIBLE_COMMITS + 1;
        start = scroll_offset_;
        end = std::min(start + MAX_VISIBLE_COMMITS, graph_commits_.size());
    }

    for (size_t i = start; i < end; ++i) {
        bool is_highlighted = (i == selected_index_);
        std::string pref = graph_commits_[i].graph_prefix;
        graph_elements.push_back(renderGraphCommitItem(graph_commits_[i], i, is_highlighted, pref));
    }

    // If we've rendered to the end of currently loaded commits, try to load more asynchronously
    if (end == graph_commits_.size() && !graph_loading_) {
        // Prevent duplicate concurrent loads
        graph_loading_ = true;
        std::thread([this]() {
            try {
                // Fetch next page of commits using skip = current size
                size_t skip = 0;
                {
                    std::lock_guard<std::mutex> lock(data_mutex_);
                    skip = graph_commits_.size();
                }
                // Load 100 more commits per page
                auto more = git_manager_->getGraphCommits(100, static_cast<int>(skip));
                if (!more.empty()) {
                    std::lock_guard<std::mutex> lock(data_mutex_);
                    // Append new commits
                    graph_commits_.insert(graph_commits_.end(), more.begin(), more.end());
                }
            } catch (...) {
                // ignore errors; UI will remain with existing commits
            }
            graph_loading_ = false;
        }).detach();
    }

    if (graph_commits_.empty()) {
        Elements empty_elements = {text(pnana::ui::icons::WARNING) | color(colors.warning),
                                   text(" No commits found") | color(colors.warning)};
        graph_elements.push_back(hbox(std::move(empty_elements)) | center);
    }

    return vbox(std::move(graph_elements));
}

Element GitPanel::renderGraphCommitItem(const GitCommit& commit, size_t /*index*/,
                                        bool is_highlighted, const std::string& prefix) {
    auto& colors = theme_.getColors();

    // Truncate hash to 7 characters for display
    std::string short_hash = commit.hash.length() > 7 ? commit.hash.substr(0, 7) : commit.hash;

    // Truncate message if too long
    std::string display_message = commit.message;
    if (display_message.length() > 50) {
        display_message = display_message.substr(0, 47) + "...";
    }

    // Build row elements
    // Build row elements: prefix (graph), then commit info
    Elements row_elements;
    const size_t prefix_min_width = 8;
    // Render prefix one character at a time to avoid trimming/collapsing by the renderer.
    // Ensure we pad to `prefix_min_width` characters.
    std::string pref_display = prefix;
    if (pref_display.size() < prefix_min_width) {
        pref_display += std::string(prefix_min_width - pref_display.size(), ' ');
    }

    // Define branch colors for different columns in the graph
    // Each column index gets a distinct color for better branch visualization
    std::vector<Color> branch_colors = {
        colors.keyword,  // Column 0: keyword color (typically purple/blue)
        colors.success,  // Column 1: success color (typically green)
        colors.warning,  // Column 2: warning color (typically orange/yellow)
        colors.type,     // Column 3: type color (typically cyan)
        colors.number,   // Column 4: number color (typically orange)
        colors.function, // Column 5: function color (typically light blue)
        colors.string,   // Column 6: string color (typically yellow/green)
        colors.error,    // Column 7: error color (typically red)
    };

    // Map ASCII graph chars to more visible Unicode variants to avoid terminal/font
    // rendering issues with '/' and '\'. Keep space as-is.
    // Track which column we're in to assign colors
    Elements prefix_chars;
    size_t column_index = 0;
    for (size_t i = 0; i < pref_display.size(); ++i) {
        unsigned char c = pref_display[i];
        std::string s;
        if (c == '/') {
            s = "╱"; // U+2571
        } else if (c == '\\') {
            s = "╲"; // U+2572
        } else if (c == '|') {
            s = "│"; // U+2502
        } else if (c == '*') {
            s = "●"; // U+25CF - commit node
        } else if (c == '+') {
            s = "┼"; // U+253C - merge point
        } else {
            s = std::string(1, static_cast<char>(c));
        }

        // Determine color based on character type and column
        Color char_color = colors.foreground;
        if (c == '*' || c == '+') {
            // Commit nodes and merge points use current column color
            char_color = branch_colors[column_index % branch_colors.size()];
        } else if (c == '|' || c == '/' || c == '\\') {
            // Branch lines use column-specific color
            char_color = branch_colors[column_index % branch_colors.size()];
            // Advance column index after vertical lines
            if (c == '|') {
                column_index++;
            }
        } else if (c == ' ') {
            // Spaces don't advance column
            char_color = colors.foreground;
        } else {
            // Other characters (like '-')
            char_color = branch_colors[column_index % branch_colors.size()];
        }

        prefix_chars.push_back(text(s) | color(char_color));
    }
    row_elements.push_back(hbox(std::move(prefix_chars)));
    row_elements.push_back(text(short_hash) | color(colors.keyword) | bold);
    row_elements.push_back(text(" ") | color(colors.background));
    row_elements.push_back(text(display_message) | color(colors.foreground));
    row_elements.push_back(text(" ") | color(colors.background));
    row_elements.push_back(text("(" + commit.author + ")") | bgcolor(colors.comment) |
                           color(colors.background) | bold);
    row_elements.push_back(text(" ") | color(colors.background));
    row_elements.push_back(text(commit.date) | bgcolor(colors.success) | dim);

    auto item_text = hbox(std::move(row_elements));

    if (is_highlighted) {
        item_text = item_text | bgcolor(colors.selection) | bold;
    } else {
        item_text = item_text | bgcolor(colors.background);
    }

    return item_text;
}

bool GitPanel::handleGraphModeKey(Event event) {
    // Tab navigation between modes
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    const size_t MAX_VISIBLE_COMMITS = 25; // Must match renderGraphPanel

    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
            // Adjust scroll to keep selected item visible
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (selected_index_ < graph_commits_.size() - 1) {
            selected_index_++;
            // Adjust scroll to keep selected item visible
            if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_COMMITS) {
                scroll_offset_ = selected_index_ - MAX_VISIBLE_COMMITS + 1;
            }
        }
        return true;
    }

    // Page navigation
    if (event == Event::PageUp) {
        if (selected_index_ > 0) {
            size_t page_size = MAX_VISIBLE_COMMITS;
            if (selected_index_ >= page_size) {
                selected_index_ -= page_size;
            } else {
                selected_index_ = 0;
            }
            scroll_offset_ =
                (selected_index_ > page_size / 2) ? selected_index_ - page_size / 2 : 0;
        }
        return true;
    }
    if (event == Event::PageDown) {
        if (selected_index_ < graph_commits_.size() - 1) {
            size_t page_size = MAX_VISIBLE_COMMITS;
            size_t max_index = graph_commits_.size() - 1;
            if (selected_index_ + page_size < max_index) {
                selected_index_ += page_size;
            } else {
                selected_index_ = max_index;
            }
            size_t new_scroll =
                (selected_index_ > page_size / 2) ? selected_index_ - page_size / 2 : 0;
            if (new_scroll + MAX_VISIBLE_COMMITS > graph_commits_.size()) {
                new_scroll = graph_commits_.size() > MAX_VISIBLE_COMMITS
                                 ? graph_commits_.size() - MAX_VISIBLE_COMMITS
                                 : 0;
            }
            scroll_offset_ = new_scroll;
        }
        return true;
    }

    // Home/End navigation
    if (event == Event::Home) {
        selected_index_ = 0;
        scroll_offset_ = 0;
        return true;
    }
    if (event == Event::End) {
        selected_index_ = graph_commits_.size() > 0 ? graph_commits_.size() - 1 : 0;
        scroll_offset_ = (graph_commits_.size() > MAX_VISIBLE_COMMITS)
                             ? graph_commits_.size() - MAX_VISIBLE_COMMITS
                             : 0;
        return true;
    }

    // Refresh data
    if (event == Event::Character('R') || event == Event::F5) {
        graph_commits_.clear();
        switchMode(GitPanelMode::GRAPH); // This will reload the commits
        return true;
    }

    if (event == Event::Escape) {
        switchMode(GitPanelMode::STATUS);
        return true;
    }

    return false;
}

// Utility methods

std::string GitPanel::getStatusIcon(GitFileStatus status) const {
    switch (status) {
        case GitFileStatus::MODIFIED:
            return MODIFIED;
        case GitFileStatus::ADDED:
            return SAVED;
        case GitFileStatus::DELETED:
            return CLOSE;
        case GitFileStatus::RENAMED:
            return ARROW_RIGHT;
        case GitFileStatus::COPIED:
            return COPY;
        case GitFileStatus::UNTRACKED:
            return UNSAVED;
        case GitFileStatus::IGNORED:
            return LOCK;
        default:
            return pnana::ui::icons::FILE;
    }
}

std::string GitPanel::getStatusText(GitFileStatus status) const {
    switch (status) {
        case GitFileStatus::MODIFIED:
            return "modified";
        case GitFileStatus::ADDED:
            return "added";
        case GitFileStatus::DELETED:
            return "deleted";
        case GitFileStatus::RENAMED:
            return "renamed";
        case GitFileStatus::COPIED:
            return "copied";
        case GitFileStatus::UPDATED_BUT_UNMERGED:
            return "unmerged";
        case GitFileStatus::UNMODIFIED:
            return "unmodified";
        case GitFileStatus::UNTRACKED:
            return "untracked";
        case GitFileStatus::IGNORED:
            return "ignored";
        default:
            return "unknown";
    }
}

std::string GitPanel::getModeTitle(GitPanelMode mode) const {
    switch (mode) {
        case GitPanelMode::STATUS:
            return "Status";
        case GitPanelMode::COMMIT:
            return "Commit";
        case GitPanelMode::BRANCH:
            return "Branch";
        case GitPanelMode::REMOTE:
            return "Remote";
        case GitPanelMode::CLONE:
            return "Clone";
        case GitPanelMode::DIFF:
            return "Diff";
        case GitPanelMode::GRAPH:
            return "Graph";
        default:
            return "";
    }
}

ftxui::Color GitPanel::getStatusColor(GitFileStatus status) const {
    auto& colors = theme_.getColors();

    switch (status) {
        case GitFileStatus::MODIFIED:
            return colors.warning; // Yellow/orange for modified
        case GitFileStatus::ADDED:
            return colors.success; // Green for added
        case GitFileStatus::DELETED:
            return colors.error; // Red for deleted
        case GitFileStatus::RENAMED:
            return colors.keyword; // Blue for renamed
        case GitFileStatus::COPIED:
            return colors.function; // Purple/cyan for copied
        case GitFileStatus::UPDATED_BUT_UNMERGED:
            return colors.error; // Red for conflicts
        case GitFileStatus::UNTRACKED:
            return colors.comment; // Gray for untracked
        case GitFileStatus::IGNORED:
            return colors.comment; // Gray for ignored
        default:
            return colors.foreground;
    }
}

bool GitPanel::hasStagedChanges() const {
    return std::any_of(files_.begin(), files_.end(), [](const GitFile& f) {
        return f.staged;
    });
}

bool GitPanel::hasUnstagedChanges() const {
    return std::any_of(files_.begin(), files_.end(), [](const GitFile& f) {
        return !f.staged;
    });
}

// Utility methods

void GitPanel::updateCachedStats() {
    // Use git to get authoritative staged count (handles cached index reliably)
    try {
        cached_staged_count_ = git_manager_->getStagedCount();
    } catch (...) {
        // Fallback: count from parsed files_
        cached_staged_count_ = 0;
        for (const auto& file : files_) {
            if (file.staged) {
                cached_staged_count_++;
            }
        }
    }

    // Unstaged count is approximate: remaining files that are not staged
    cached_unstaged_count_ =
        (files_.size() > cached_staged_count_) ? (files_.size() - cached_staged_count_) : 0;

    stats_cache_valid_ = true;
}

bool GitPanel::isNavigationKey(Event event) const {
    return event == Event::ArrowUp || event == Event::ArrowDown || event == Event::PageUp ||
           event == Event::PageDown || event == Event::Home || event == Event::End;
}

std::string GitPanel::getCachedRepoPathDisplay() {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_update =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_repo_display_update_);

    // Use cached result if within timeout
    if (!cached_repo_path_display_.empty() &&
        time_since_last_update < repo_display_cache_timeout_) {
        return cached_repo_path_display_;
    }

    // Update cache
    std::string repo_root = git_manager_->getRepositoryRoot();
    cached_repo_path_display_ = repo_root.empty() ? "." : repo_root;
    last_repo_display_update_ = now;

    return cached_repo_path_display_;
}

std::string GitPanel::getCachedCurrentBranch() {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_update =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_branch_update_);

    // Use cached result if within timeout
    if (!cached_current_branch_.empty() && time_since_last_update < branch_cache_timeout_) {
        return cached_current_branch_;
    }

    // Update cache
    cached_current_branch_ = git_manager_->getCurrentBranch();
    last_branch_update_ = now;

    return cached_current_branch_;
}

GitBranchStatus GitPanel::getCachedBranchStatus() {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_update =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_branch_status_update_);

    // Use cached result if within timeout
    if (cached_branch_status_.has_remote_tracking &&
        time_since_last_update < branch_status_cache_timeout_) {
        return cached_branch_status_;
    }

    // Update cache
    cached_branch_status_ = git_manager_->getBranchStatus();
    last_branch_status_update_ = now;

    return cached_branch_status_;
}

std::string GitPanel::getFileExtension(const std::string& filename) const {
    try {
        std::filesystem::path file_path(filename);
        std::string extension = file_path.extension().string();
        // 移除扩展名前的点
        if (!extension.empty() && extension[0] == '.') {
            return extension.substr(1);
        }
        return extension;
    } catch (...) {
        return "";
    }
}

void GitPanel::ensureValidIndices() {
    const size_t MAX_VISIBLE_FILES = 25;

    // Ensure selected_index_ is within valid range
    if (files_.empty()) {
        selected_index_ = 0;
        scroll_offset_ = 0;
        return;
    }

    if (selected_index_ >= files_.size()) {
        selected_index_ = files_.size() - 1;
    }

    // Ensure scroll_offset_ is valid
    if (scroll_offset_ >= files_.size()) {
        scroll_offset_ = files_.size() > MAX_VISIBLE_FILES ? files_.size() - MAX_VISIBLE_FILES : 0;
    }

    // Ensure selected item is visible
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + MAX_VISIBLE_FILES) {
        scroll_offset_ = selected_index_ - MAX_VISIBLE_FILES + 1;
        // Don't scroll past the end
        if (scroll_offset_ + MAX_VISIBLE_FILES > files_.size()) {
            scroll_offset_ =
                files_.size() > MAX_VISIBLE_FILES ? files_.size() - MAX_VISIBLE_FILES : 0;
        }
    }
}

void GitPanel::showDiffViewer(const std::string& file_path) {
    current_diff_file_ = file_path;
    diff_content_ = git_manager_->getDiff(file_path);
    diff_scroll_offset_ = 0;
    diff_h_offset_ = 0; // 重置水平滚动
    diff_viewer_visible_ = true;
    error_message_ = git_manager_->getLastError();
    git_manager_->clearError();
}

void GitPanel::hideDiffViewer() {
    diff_viewer_visible_ = false;
    diff_content_.clear();
    current_diff_file_.clear();
    diff_scroll_offset_ = 0;
    diff_h_offset_ = 0; // 重置水平滚动
}

void GitPanel::handleDiffViewerEscape() {
    hideDiffViewer();
    component_needs_rebuild_ = true; // Force component rebuild after hiding viewer
}

} // namespace vgit
} // namespace pnana
