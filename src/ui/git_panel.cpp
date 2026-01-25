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
    // 延迟加载git数据，只在面板显示时才加载
    // Initialize cache timestamps
    last_repo_display_update_ = std::chrono::steady_clock::now() - repo_display_cache_timeout_;
    last_branch_update_ = std::chrono::steady_clock::now() - branch_cache_timeout_;
    last_branch_status_update_ = std::chrono::steady_clock::now() - branch_status_cache_timeout_;
}

Component GitPanel::getComponent() {
    // 只在初次创建、模式切换或需要重建时重新构建组件
    if (!main_component_ || component_needs_rebuild_) {
        main_component_ = buildMainComponent();
        component_needs_rebuild_ = false; // Reset the flag after rebuild
        pnana::utils::Logger::getInstance().log(
            "GitPanel::getComponent - Component rebuilt, needs_rebuild was: " +
            std::string(component_needs_rebuild_ ? "true" : "false"));
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
        pnana::utils::Logger::getInstance().log("GitPanel::onShow - Starting async data loading");

        // 异步加载数据，不阻塞UI
        std::thread([this]() {
            auto start_time = std::chrono::high_resolution_clock::now();
            pnana::utils::Logger::getInstance().log(
                "GitPanel::onShow - ASYNC: Starting data loading");

            refreshData();

            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            pnana::utils::Logger::getInstance().log(
                "GitPanel::onShow - ASYNC: Data loading completed - " +
                std::to_string(duration.count()) + "ms");
        }).detach(); // 分离线程，让它在后台运行
    }
}

void GitPanel::onHide() {
    // Cleanup if needed
}

bool GitPanel::onKeyPress(Event event) {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::onKeyPress - START");

    if (!visible_) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitPanel::onKeyPress - END (not visible) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    if (event == Event::Escape) {
        auto escape_start = std::chrono::high_resolution_clock::now();
        hide();
        auto escape_end = std::chrono::high_resolution_clock::now();
        auto escape_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(escape_end - escape_start);
        pnana::utils::Logger::getInstance().log("GitPanel::hide() took " +
                                                std::to_string(escape_duration.count()) + "ms");

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitPanel::onKeyPress - END (escape) - " +
                                                std::to_string(duration.count()) + "ms");
        return true;
    }

    bool handled = false;
    auto handler_start = std::chrono::high_resolution_clock::now();
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
    }
    auto handler_end = std::chrono::high_resolution_clock::now();
    auto handler_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(handler_end - handler_start);
    pnana::utils::Logger::getInstance().log(
        "GitPanel::handleKey(mode=" + std::to_string(static_cast<int>(current_mode_)) + ") took " +
        std::to_string(handler_duration.count()) + "ms");

    // 对于导航键（箭头键、翻页键等），标记为需要重绘
    // 对于Git操作，GitPanelHandler会处理重绘
    if (handled && isNavigationKey(event)) {
        needs_redraw_ = true;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log(
        "GitPanel::onKeyPress - END (handled: " + std::string(handled ? "true" : "false") + ") - " +
        std::to_string(duration.count()) + "ms");

    return handled;
}

void GitPanel::refreshStatusOnly() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::refreshStatusOnly - START");

    if (data_loading_) {
        pnana::utils::Logger::getInstance().log(
            "GitPanel::refreshStatusOnly - END (already loading)");
        return; // 如果正在加载，忽略请求
    }

    data_loading_ = true;

    // 异步只刷新状态数据，不刷新分支数据
    auto future = std::async(std::launch::async, [this]() {
        auto async_start = std::chrono::high_resolution_clock::now();
        pnana::utils::Logger::getInstance().log("GitPanel::refreshStatusOnly - ASYNC START");

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

            auto async_end = std::chrono::high_resolution_clock::now();
            auto async_duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(async_end - async_start);
            pnana::utils::Logger::getInstance().log(
                "GitPanel::refreshStatusOnly - ASYNC END (success) - " +
                std::to_string(async_duration.count()) +
                "ms, files: " + std::to_string(files_.size()));
        } catch (...) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            data_loading_ = false;
            error_message_ = "Failed to load git data";
            auto async_end = std::chrono::high_resolution_clock::now();
            auto async_duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(async_end - async_start);
            pnana::utils::Logger::getInstance().log(
                "GitPanel::refreshStatusOnly - ASYNC END (exception) - " +
                std::to_string(async_duration.count()) + "ms");
        }
    });

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log(
        "GitPanel::refreshStatusOnly - END (launched async) - " + std::to_string(duration.count()) +
        "ms");
}

void GitPanel::refreshData() {
    auto refresh_start = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::refreshData - START");

    if (data_loading_) {
        pnana::utils::Logger::getInstance().log("GitPanel::refreshData - END (already loading)");
        return; // 如果正在加载，忽略请求
    }

    data_loading_ = true;
    last_refresh_time_ = std::chrono::steady_clock::now();

    // 异步加载git数据，使用更高效的方式
    auto future = std::async(std::launch::async, [this]() {
        auto async_start = std::chrono::high_resolution_clock::now();
        pnana::utils::Logger::getInstance().log("GitPanel::refreshData - ASYNC START");

        try {
            // 强制刷新状态，确保获取最新数据
            auto status_refresh_start = std::chrono::high_resolution_clock::now();
            git_manager_->refreshStatusForced();
            auto status_refresh_end = std::chrono::high_resolution_clock::now();
            auto status_refresh_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                status_refresh_end - status_refresh_start);
            pnana::utils::Logger::getInstance().log(
                "GitManager::refreshStatusForced took " +
                std::to_string(status_refresh_duration.count()) + "ms");

            auto files = git_manager_->getStatus();
            auto error = git_manager_->getLastError();
            git_manager_->clearError();

            // 分支数据变化较少，只有在第一次加载或明确需要时才获取
            bool need_branches = branches_.empty() || branch_data_stale_;
            std::vector<GitBranch> branches;
            if (need_branches) {
                auto branch_fetch_start = std::chrono::high_resolution_clock::now();
                branches = git_manager_->getBranches();
                auto branch_fetch_end = std::chrono::high_resolution_clock::now();
                auto branch_fetch_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    branch_fetch_end - branch_fetch_start);
                pnana::utils::Logger::getInstance().log(
                    "GitManager::getBranches took " +
                    std::to_string(branch_fetch_duration.count()) + "ms (" +
                    std::to_string(branches.size()) + " branches)");
            }

            // 在主线程中更新UI数据
            auto ui_update_start = std::chrono::high_resolution_clock::now();
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

            auto ui_update_end = std::chrono::high_resolution_clock::now();
            auto ui_update_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                ui_update_end - ui_update_start);
            pnana::utils::Logger::getInstance().log(
                "UI data update took " + std::to_string(ui_update_duration.count()) + "ms (" +
                std::to_string(files_.size()) + " files)");

            auto async_end = std::chrono::high_resolution_clock::now();
            auto async_duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(async_end - async_start);
            pnana::utils::Logger::getInstance().log("GitPanel::refreshData - ASYNC END - " +
                                                    std::to_string(async_duration.count()) + "ms");
        } catch (...) {
            std::lock_guard<std::mutex> lock(data_mutex_);
            data_loading_ = false;
            error_message_ = "Failed to load git data";
            auto async_end = std::chrono::high_resolution_clock::now();
            auto async_duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(async_end - async_start);
            pnana::utils::Logger::getInstance().log(
                "GitPanel::refreshData - ASYNC END (exception) - " +
                std::to_string(async_duration.count()) + "ms");
        }
    });

    auto refresh_end = std::chrono::high_resolution_clock::now();
    auto refresh_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(refresh_end - refresh_start);
    pnana::utils::Logger::getInstance().log("GitPanel::refreshData - END (launched async) - " +
                                            std::to_string(refresh_duration.count()) + "ms");
}

void GitPanel::switchMode(GitPanelMode mode) {
    current_mode_ = mode;
    selected_index_ = 0;
    scroll_offset_ = 0;
    clearSelection();

    if (mode == GitPanelMode::COMMIT) {
        commit_message_.clear();
    } else if (mode == GitPanelMode::BRANCH) {
        branch_name_.clear();
    } else if (mode == GitPanelMode::CLONE) {
        clone_url_.clear();
        clone_path_ = git_manager_->getRepositoryRoot().empty() ? fs::current_path().string()
                                                                : git_manager_->getRepositoryRoot();
        clone_focus_on_url_ = true; // Default focus on URL
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
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::performStageSelected - START - selected: " +
                                            std::to_string(selected_files_.size()));

    if (selected_files_.empty()) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performStageSelected - END (no selection) - " +
            std::to_string(duration.count()) + "ms");
        return; // 没有选中文件，直接返回
    }

    auto git_ops_start = std::chrono::high_resolution_clock::now();
    bool success = true;
    size_t staged_count = 0;
    for (size_t index : selected_files_) {
        if (index < files_.size()) {
            auto single_stage_start = std::chrono::high_resolution_clock::now();
            if (!git_manager_->stageFile(files_[index].path)) {
                success = false;
                error_message_ = git_manager_->getLastError();
                auto single_stage_end = std::chrono::high_resolution_clock::now();
                auto single_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    single_stage_end - single_stage_start);
                pnana::utils::Logger::getInstance().log(
                    "GitPanel::stageFile failed for '" + files_[index].path + "' after " +
                    std::to_string(single_duration.count()) + "ms");
                break;
            } else {
                staged_count++;
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
                auto single_stage_end = std::chrono::high_resolution_clock::now();
                auto single_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    single_stage_end - single_stage_start);
                pnana::utils::Logger::getInstance().log(
                    "GitPanel::stageFile succeeded for '" + files_[index].path + "' in " +
                    std::to_string(single_duration.count()) + "ms");
            }
        }
    }
    auto git_ops_end = std::chrono::high_resolution_clock::now();
    auto git_ops_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(git_ops_end - git_ops_start);
    pnana::utils::Logger::getInstance().log(
        "Git operations completed: " + std::to_string(staged_count) + "/" +
        std::to_string(selected_files_.size()) + " files staged in " +
        std::to_string(git_ops_duration.count()) + "ms");

    auto post_ops_start = std::chrono::high_resolution_clock::now();
    if (success) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // 用户可以通过F5或R键手动刷新，或者等待下次自动刷新
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
        // Invalidate cached stats so UI will recalculate staged/unstaged counts
        stats_cache_valid_ = false;
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performStageSelected - Marked data as stale, will refresh on next access");
    }
    auto post_ops_end = std::chrono::high_resolution_clock::now();
    auto post_ops_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(post_ops_end - post_ops_start);
    pnana::utils::Logger::getInstance().log("Post-operations took " +
                                            std::to_string(post_ops_duration.count()) + "ms");

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitPanel::performStageSelected - END (success: " +
                                            std::string(success ? "true" : "false") + ") - " +
                                            std::to_string(duration.count()) + "ms");
}

void GitPanel::performUnstageSelected() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log(
        "GitPanel::performUnstageSelected - START - selected: " +
        std::to_string(selected_files_.size()));

    if (selected_files_.empty()) {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performUnstageSelected - END (no selection) - " +
            std::to_string(duration.count()) + "ms");
        return; // 没有选中文件，直接返回
    }

    auto git_ops_start = std::chrono::high_resolution_clock::now();
    bool success = true;
    size_t unstaged_count = 0;
    for (size_t index : selected_files_) {
        if (index < files_.size()) {
            auto single_unstage_start = std::chrono::high_resolution_clock::now();
            if (!git_manager_->unstageFile(files_[index].path)) {
                success = false;
                error_message_ = git_manager_->getLastError();
                auto single_unstage_end = std::chrono::high_resolution_clock::now();
                auto single_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    single_unstage_end - single_unstage_start);
                pnana::utils::Logger::getInstance().log(
                    "GitPanel::unstageFile failed for '" + files_[index].path + "' after " +
                    std::to_string(single_duration.count()) + "ms");
                break;
            } else {
                unstaged_count++;
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
                auto single_unstage_end = std::chrono::high_resolution_clock::now();
                auto single_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    single_unstage_end - single_unstage_start);
                pnana::utils::Logger::getInstance().log(
                    "GitPanel::unstageFile succeeded for '" + files_[index].path + "' in " +
                    std::to_string(single_duration.count()) + "ms");
            }
        }
    }
    auto git_ops_end = std::chrono::high_resolution_clock::now();
    auto git_ops_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(git_ops_end - git_ops_start);
    pnana::utils::Logger::getInstance().log(
        "Git unstage operations completed: " + std::to_string(unstaged_count) + "/" +
        std::to_string(selected_files_.size()) + " files unstaged in " +
        std::to_string(git_ops_duration.count()) + "ms");

    auto post_ops_start = std::chrono::high_resolution_clock::now();
    if (success) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performUnstageSelected - Marked data as stale, will refresh on next access");
    }
    auto post_ops_end = std::chrono::high_resolution_clock::now();
    auto post_ops_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(post_ops_end - post_ops_start);
    pnana::utils::Logger::getInstance().log("Post-operations took " +
                                            std::to_string(post_ops_duration.count()) + "ms");

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitPanel::performUnstageSelected - END (success: " +
                                            std::string(success ? "true" : "false") + ") - " +
                                            std::to_string(duration.count()) + "ms");
}

void GitPanel::performStageAll() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::performStageAll - START");

    if (git_manager_->stageAll()) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performStageAll - Marked data as stale, will refresh on next access");

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitPanel::performStageAll - END (success) - " +
                                                std::to_string(duration.count()) + "ms");
    } else {
        error_message_ = git_manager_->getLastError();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitPanel::performStageAll - END (failed) - " +
                                                std::to_string(duration.count()) + "ms");
    }
}

void GitPanel::performUnstageAll() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::performUnstageAll - START");

    if (git_manager_->unstageAll()) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performUnstageAll - Marked data as stale, will refresh on next access");

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitPanel::performUnstageAll - END (success) - " +
                                                std::to_string(duration.count()) + "ms");
    } else {
        error_message_ = git_manager_->getLastError();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitPanel::performUnstageAll - END (failed) - " +
                                                std::to_string(duration.count()) + "ms");
    }
}

void GitPanel::performCommit() {
    if (commit_message_.empty())
        return;

    if (git_manager_->commit(commit_message_)) {
        commit_message_.clear();
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
    header_elements.push_back(text(" │ ") | color(colors.comment));
    header_elements.push_back(text(getModeTitle(current_mode_)) | color(colors.keyword) | bold);
    header_elements.push_back(text(" │ ") | color(colors.comment));
    header_elements.push_back(text("Repository: ") | color(colors.menubar_fg));

    // Use cached repo path display to avoid frequent git calls during rendering
    std::string repo_path = getCachedRepoPathDisplay();
    header_elements.push_back(text(repo_path) | color(colors.foreground));

    return hbox(std::move(header_elements)) | bgcolor(colors.menubar_bg);
}

Element GitPanel::renderTabs() {
    auto& colors = theme_.getColors();

    auto makeTab = [&](const std::string& label, GitPanelMode /*mode*/, bool active) {
        if (active) {
            return text("[" + label + "]") | bgcolor(colors.selection) | color(colors.foreground) |
                   bold;
        } else {
            return text(" " + label + " ") | color(colors.menubar_fg);
        }
    };

    Elements elements = {
        makeTab("Status", GitPanelMode::STATUS, current_mode_ == GitPanelMode::STATUS),
        text(" │ ") | color(colors.comment),
        makeTab("Commit", GitPanelMode::COMMIT, current_mode_ == GitPanelMode::COMMIT),
        text(" │ ") | color(colors.comment),
        makeTab("Branch", GitPanelMode::BRANCH, current_mode_ == GitPanelMode::BRANCH),
        text(" │ ") | color(colors.comment),
        makeTab("Remote", GitPanelMode::REMOTE, current_mode_ == GitPanelMode::REMOTE),
        text(" │ ") | color(colors.comment),
        makeTab("Clone", GitPanelMode::CLONE, current_mode_ == GitPanelMode::CLONE),
        text(" │ ") | color(colors.comment),
        makeTab("Diff", GitPanelMode::DIFF, current_mode_ == GitPanelMode::DIFF)};
    return hbox(std::move(elements)) | center;
}

Element GitPanel::renderStatusPanel() {
    auto render_start = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::renderStatusPanel - START");

    auto& colors = theme_.getColors();

    // 如果数据正在加载，显示加载指示器
    if (data_loading_) {
        auto render_end = std::chrono::high_resolution_clock::now();
        auto render_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(render_end - render_start);
        pnana::utils::Logger::getInstance().log("GitPanel::renderStatusPanel - END (loading) - " +
                                                std::to_string(render_duration.count()) + "ms");
        return vbox({text("Loading git status...") | color(colors.comment) | center,
                     text("Please wait...") | color(colors.menubar_fg) | center}) |
               center | size(HEIGHT, EQUAL, 10);
    }

    Elements file_elements;

    // Enhanced header with status summary (use cached stats for performance)
    auto stats_start = std::chrono::high_resolution_clock::now();
    if (!stats_cache_valid_) {
        updateCachedStats();
    }
    auto stats_end = std::chrono::high_resolution_clock::now();
    auto stats_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(stats_end - stats_start);
    if (!stats_cache_valid_) {
        pnana::utils::Logger::getInstance().log("updateCachedStats took " +
                                                std::to_string(stats_duration.count()) + "ms");
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
    file_elements.push_back(hbox(std::move(header_elements)));
    file_elements.push_back(separator());

    // File list with improved scrolling and display
    auto list_prep_start = std::chrono::high_resolution_clock::now();
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
    size_t visible_count = end - start;
    auto list_prep_end = std::chrono::high_resolution_clock::now();
    auto list_prep_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(list_prep_end - list_prep_start);
    pnana::utils::Logger::getInstance().log(
        "File list preparation took " + std::to_string(list_prep_duration.count()) +
        "ms (showing " + std::to_string(visible_count) + " of " + std::to_string(files_.size()) +
        " files, scroll_offset=" + std::to_string(scroll_offset_) +
        ", selected_index=" + std::to_string(selected_index_) + ")");

    auto render_items_start = std::chrono::high_resolution_clock::now();
    for (size_t i = start; i < end; ++i) {
        bool is_selected =
            std::find(selected_files_.begin(), selected_files_.end(), i) != selected_files_.end();
        bool is_highlighted = (i == selected_index_);
        file_elements.push_back(renderFileItem(files_[i], i, is_selected, is_highlighted));
    }
    auto render_items_end = std::chrono::high_resolution_clock::now();
    auto render_items_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        render_items_end - render_items_start);
    if (end > start) {
        pnana::utils::Logger::getInstance().log(
            "Rendering " + std::to_string(end - start) + " file items took " +
            std::to_string(render_items_duration.count()) + "ms (avg: " +
            std::to_string(render_items_duration.count() / (end - start)) + "ms per item)");
    }

    if (files_.empty()) {
        Elements empty_elements = {
            text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success) | bold,
            text(" Working directory clean") | color(colors.success),
            text(" - no changes to commit") | color(colors.comment)};
        file_elements.push_back(hbox(std::move(empty_elements)) | center);
    }

    auto render_end = std::chrono::high_resolution_clock::now();
    auto render_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(render_end - render_start);
    pnana::utils::Logger::getInstance().log("GitPanel::renderStatusPanel - END - " +
                                            std::to_string(render_duration.count()) + "ms");

    return vbox(std::move(file_elements));
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

    // Commit message input with better styling
    Elements input_header = {text(pnana::ui::icons::FILE_EDIT) | color(colors.keyword),
                             text(" Commit message:") | color(colors.menubar_fg)};
    elements.push_back(hbox(std::move(input_header)));

    // Show staged file list (immediate from files_ so user sees what will be committed)
    if (staged_count > 0) {
        Elements staged_list_header = {text(pnana::ui::icons::SAVED) | color(colors.success),
                                       text(" Staged files:") | color(colors.menubar_fg)};
        elements.push_back(hbox(std::move(staged_list_header)));

        // List staged file names (limit visual lines to avoid overflow)
        const size_t MAX_STAGED_SHOW = 10;
        size_t shown = 0;
        for (const auto& f : files_) {
            if (f.staged) {
                Elements row = {text("  ") | color(colors.background),
                                text(pnana::ui::icons::FILE) | color(colors.function), text(" "),
                                text(f.path) | color(colors.success)};
                elements.push_back(hbox(std::move(row)));
                shown++;
                if (shown >= MAX_STAGED_SHOW)
                    break;
            }
        }
        if (shown < staged_count) {
            elements.push_back(text("  ...") | color(colors.comment));
        }
        elements.push_back(separator());
    }

    // Show character count and validation
    std::string char_count = "(" + std::to_string(commit_message_.length()) + " chars)";
    elements.push_back(text(commit_message_) | color(colors.foreground) | border |
                       bgcolor(colors.background));
    elements.push_back(text(char_count) | color(colors.comment) | dim);

    // Validation and status messages
    if (staged_count == 0) {
        Elements warning_elements = {text(pnana::ui::icons::WARNING) | color(colors.error),
                                     text(" No staged changes to commit") | color(colors.error)};
        elements.push_back(hbox(warning_elements));
    } else if (commit_message_.empty()) {
        Elements info_elements = {text(pnana::ui::icons::INFO_CIRCLE) | color(colors.warning),
                                  text(" Commit message is required") | color(colors.warning)};
        elements.push_back(hbox(info_elements));
    } else {
        Elements ready_elements = {text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success),
                                   text(" Ready to commit") | color(colors.success)};
        elements.push_back(hbox(ready_elements));
    }

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
    branch_elements.push_back(text(branch_name_) | color(colors.foreground) | border |
                              bgcolor(colors.background));

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

    // Current branch and remote info
    std::string current_branch = getCachedCurrentBranch();
    if (!current_branch.empty()) {
        Elements branch_elements = {text(pnana::ui::icons::GIT_BRANCH) | color(colors.keyword),
                                    text(" Current branch: ") | color(colors.menubar_fg),
                                    text(current_branch) | color(colors.foreground) | bold};
        elements.push_back(hbox(std::move(branch_elements)));
        elements.push_back(separator());
    }

    // Branch status info (push/sync status)
    GitBranchStatus branch_status = getCachedBranchStatus();
    if (branch_status.has_remote_tracking) {
        Elements status_elements;

        // Status icon and text
        status_elements.push_back(text(pnana::ui::icons::REFRESH) | color(colors.comment));

        if (branch_status.ahead > 0 && branch_status.behind == 0) {
            // Ahead of remote - ready to push
            status_elements.push_back(text(" Local commits ready to push") | color(colors.success) |
                                      bold);
            status_elements.push_back(
                text(" (" + std::to_string(branch_status.ahead) + " commits ahead)") |
                color(colors.success));
        } else if (branch_status.ahead == 0 && branch_status.behind > 0) {
            // Behind remote - need to pull
            status_elements.push_back(text(" Remote has new commits") | color(colors.warning) |
                                      bold);
            status_elements.push_back(
                text(" (" + std::to_string(branch_status.behind) + " commits behind)") |
                color(colors.warning));
        } else if (branch_status.ahead > 0 && branch_status.behind > 0) {
            // Diverged - need to handle merge/rebase
            status_elements.push_back(text(" Branch diverged from remote") | color(colors.warning) |
                                      bold);
            status_elements.push_back(text(" (" + std::to_string(branch_status.ahead) + " ahead, " +
                                           std::to_string(branch_status.behind) + " behind)") |
                                      color(colors.warning));
        } else {
            // In sync
            status_elements.push_back(text(" Branch is in sync with remote") |
                                      color(colors.comment));
        }

        elements.push_back(hbox(std::move(status_elements)));
        elements.push_back(separator());
    } else if (!current_branch.empty()) {
        // No remote tracking branch
        Elements no_remote_elements = {
            text(pnana::ui::icons::WARNING) | color(colors.warning),
            text(" No remote tracking branch configured") | color(colors.comment)};
        elements.push_back(hbox(std::move(no_remote_elements)));
        elements.push_back(separator());
    }

    // Available operations with enhanced visual design
    elements.push_back(text("Available operations:") | color(colors.menubar_fg));
    elements.push_back(separatorLight());

    // Push operation
    Elements push_elements = {
        text("  ") | color(colors.background),
        text("[p]") | color(colors.success) | bold | bgcolor(colors.selection),
        text(" ") | color(colors.background),
        text(pnana::ui::icons::UPLOAD) | color(colors.success),
        text(" Push to remote") | color(colors.foreground),
        text(" - Upload local commits") | color(colors.comment)};
    elements.push_back(hbox(push_elements));

    // Pull operation
    Elements pull_elements = {
        text("  ") | color(colors.background),
        text("[l]") | color(colors.warning) | bold | bgcolor(colors.selection),
        text(" ") | color(colors.background),
        text(pnana::ui::icons::DOWNLOAD) | color(colors.warning),
        text(" Pull from remote") | color(colors.foreground),
        text(" - Download and merge remote changes") | color(colors.comment)};
    elements.push_back(hbox(pull_elements));

    // Fetch operation
    Elements fetch_elements = {
        text("  ") | color(colors.background),
        text("[f]") | color(colors.keyword) | bold | bgcolor(colors.selection),
        text(" ") | color(colors.background),
        text(pnana::ui::icons::REFRESH) | color(colors.keyword),
        text(" Fetch from remote") | color(colors.foreground),
        text(" - Download remote changes without merging") | color(colors.comment)};
    elements.push_back(hbox(fetch_elements));

    elements.push_back(separatorLight());

    // Remote status info
    Elements status_elements = {
        text(pnana::ui::icons::INFO_CIRCLE) | color(colors.comment),
        text(" Use the operations above to sync with remote repositories") | color(colors.comment)};
    elements.push_back(hbox(status_elements));

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

    // Repository URL input
    Elements url_header = {text(pnana::ui::icons::LINK) | color(colors.keyword),
                           text(" Repository URL (HTTPS/SSH):") | color(colors.menubar_fg)};
    elements.push_back(hbox(std::move(url_header)));

    // URL input with focus indication
    auto url_input = text(clone_url_) | color(colors.foreground);
    if (clone_focus_on_url_) {
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
    if (!clone_focus_on_url_) {
        path_input = path_input | border | bgcolor(colors.selection) | color(colors.background);
    } else {
        path_input = path_input | border | bgcolor(colors.background);
    }
    elements.push_back(path_input);
    elements.push_back(separatorLight());

    // Instructions
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

    return vbox(std::move(elements));
}

bool GitPanel::handleCloneModeKey(Event event) {
    // Tab navigation between modes
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    // Up/Down arrow keys to switch between input fields
    if (event == Event::ArrowUp || event == Event::ArrowDown) {
        clone_focus_on_url_ = !clone_focus_on_url_;
        return true;
    }

    if (event == Event::Return) {
        performClone();
        return true;
    }
    if (event == Event::Escape) {
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
        return true;
    }
    if (event == Event::Backspace) {
        if (clone_focus_on_url_ && !clone_url_.empty()) {
            clone_url_.pop_back();
        } else if (!clone_focus_on_url_ && !clone_path_.empty()) {
            clone_path_.pop_back();
        }
        return true;
    }

    return false;
}

void GitPanel::performClone() {
    if (clone_url_.empty()) {
        error_message_ = "Repository URL cannot be empty";
        return;
    }

    if (clone_path_.empty()) {
        error_message_ = "Clone path cannot be empty";
        return;
    }

    // Start async clone operation
    std::thread([this]() {
        auto start_time = std::chrono::high_resolution_clock::now();
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performClone - Starting async clone operation");

        GitManager temp_manager(clone_path_);
        bool success = temp_manager.clone(clone_url_, clone_path_);

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        if (success) {
            error_message_.clear();
            pnana::utils::Logger::getInstance().log(
                "GitPanel::performClone - Clone completed successfully in " +
                std::to_string(duration.count()) + "ms");

            // Clear inputs on success
            clone_url_.clear();
            clone_path_ = git_manager_->getRepositoryRoot().empty()
                              ? fs::current_path().string()
                              : git_manager_->getRepositoryRoot();
            clone_focus_on_url_ = true; // Reset focus to URL
        } else {
            error_message_ = temp_manager.getLastError();
            pnana::utils::Logger::getInstance().log("GitPanel::performClone - Clone failed after " +
                                                    std::to_string(duration.count()) +
                                                    "ms: " + error_message_);
        }
    }).detach();
}

Element GitPanel::renderDiffViewer() {
    if (!diff_viewer_visible_) {
        return emptyElement();
    }

    auto& colors = theme_.getColors();

    Elements viewer_elements;

    // Header with file name and controls
    Elements header_elements = {text(pnana::ui::icons::FILE) | color(colors.function),
                                text(" ") | color(colors.foreground),
                                text(current_diff_file_) | color(colors.foreground) | bold,
                                text(" │ ") | color(colors.comment),
                                text("ESC") | color(colors.keyword) | bold,
                                text(":close ") | color(colors.comment),
                                text("PgUp/PgDn") | color(colors.keyword) | bold,
                                text(":scroll") | color(colors.comment)};
    viewer_elements.push_back(hbox(std::move(header_elements)));
    viewer_elements.push_back(separator());

    // Diff content with enhanced neovim-like styling
    const size_t VISIBLE_LINES = 25;
    size_t start_line = diff_scroll_offset_;
    size_t end_line = std::min(start_line + VISIBLE_LINES, diff_content_.size());

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

            // Line content with appropriate styling
            Element content_element;
            if (is_addition) {
                // Green for additions, like neovim
                content_element = text(line_content) | color(colors.success);
            } else if (is_deletion) {
                // Red for deletions, like neovim
                content_element = text(line_content) | color(colors.error);
            } else if (is_hunk_header) {
                // Blue for hunk headers
                content_element = text(line_content) | color(colors.keyword) | bold;
            } else if (is_file_header) {
                // File headers with appropriate colors
                Color header_color =
                    (line_content.substr(0, 3) == "+++") ? colors.success : colors.error;
                content_element = text(line_content) | color(header_color) | bold;
            } else {
                // Context lines - normal color
                content_element = text(line_content) | color(colors.foreground);
            }

            line_elements.push_back(content_element);
            viewer_elements.push_back(hbox(std::move(line_elements)));
        }

        // Add scroll indicator if needed (no dashed separator)
        if (diff_content_.size() > VISIBLE_LINES) {
            size_t total_pages = (diff_content_.size() + VISIBLE_LINES - 1) / VISIBLE_LINES;
            size_t current_page = diff_scroll_offset_ / VISIBLE_LINES + 1;
            std::string scroll_info =
                "[" + std::to_string(current_page) + "/" + std::to_string(total_pages) + "]";
            viewer_elements.push_back(text(scroll_info) | color(colors.comment) | center);
        }
    }

    return window(text("Diff Viewer"), vbox(std::move(viewer_elements))) |
           size(WIDTH, GREATER_THAN, 100) | size(HEIGHT, GREATER_THAN, 30) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border);
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
    }

    // Add scroll indicator if needed
    if (current_mode_ == GitPanelMode::STATUS && files_.size() > 25) {
        size_t total_pages = (files_.size() + 24) / 25; // Ceiling division
        size_t current_page = scroll_offset_ / 40 + 1;
        std::string scroll_info =
            " [" + std::to_string(current_page) + "/" + std::to_string(total_pages) + "]";
        footer_elements.push_back(text(scroll_info) | color(colors.menubar_fg));
    } else if (current_mode_ == GitPanelMode::BRANCH && branches_.size() > 18) {
        size_t total_pages = (branches_.size() + 17) / 18; // Ceiling division
        size_t current_page = scroll_offset_ / 18 + 1;
        std::string scroll_info =
            " [" + std::to_string(current_page) + "/" + std::to_string(total_pages) + "]";
        footer_elements.push_back(text(scroll_info) | color(colors.menubar_fg));
    } else if (current_mode_ == GitPanelMode::DIFF && files_.size() > 20) {
        size_t total_pages = (files_.size() + 19) / 20; // Ceiling division
        size_t current_page = scroll_offset_ / 20 + 1;
        std::string scroll_info =
            " [" + std::to_string(current_page) + "/" + std::to_string(total_pages) + "]";
        footer_elements.push_back(text(scroll_info) | color(colors.menubar_fg));
    }

    return vbox(std::move(footer_elements)) | bgcolor(colors.menubar_bg);
}

Element GitPanel::renderError() {
    if (error_message_.empty())
        return emptyElement();

    return text("Error: " + error_message_) | color(Color::Red) | border;
}

Element GitPanel::separatorLight() {
    return text(std::string(80, '-')) | color(theme_.getColors().comment);
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
               content_elements.push_back(separator());

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
               }

               if (!error_message_.empty()) {
                   content_elements.push_back(separatorLight());
                   content_elements.push_back(renderError());
               }

               content_elements.push_back(separator());
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
                   pnana::utils::Logger::getInstance().log(
                       "GitPanel::CatchEvent - Diff viewer visible, handling event");
                   if (event == Event::Escape) {
                       pnana::utils::Logger::getInstance().log(
                           "GitPanel::CatchEvent - ESC pressed in diff viewer, hiding viewer");
                       hideDiffViewer();
                       component_needs_rebuild_ =
                           true;    // Force component rebuild after hiding viewer
                       return true; // Consume ESC event, don't let it bubble up
                   }
                   if (event == Event::PageUp) {
                       pnana::utils::Logger::getInstance().log(
                           "GitPanel::CatchEvent - PageUp in diff viewer");
                       const size_t VISIBLE_LINES = 25; // Must match renderDiffViewer
                       if (diff_scroll_offset_ > 0) {
                           size_t old_offset = diff_scroll_offset_;
                           diff_scroll_offset_ = (diff_scroll_offset_ > VISIBLE_LINES)
                                                     ? diff_scroll_offset_ - VISIBLE_LINES
                                                     : 0;
                           pnana::utils::Logger::getInstance().log(
                               "GitPanel::CatchEvent - Scrolled up from " +
                               std::to_string(old_offset) + " to " +
                               std::to_string(diff_scroll_offset_));
                           component_needs_rebuild_ =
                               true; // Force component rebuild after scrolling
                       }
                       return true;
                   }
                   if (event == Event::PageDown) {
                       pnana::utils::Logger::getInstance().log(
                           "GitPanel::CatchEvent - PageDown in diff viewer");
                       const size_t VISIBLE_LINES = 25; // Must match renderDiffViewer
                       size_t max_offset = diff_content_.size() > VISIBLE_LINES
                                               ? diff_content_.size() - VISIBLE_LINES
                                               : 0;
                       if (diff_scroll_offset_ < max_offset) {
                           size_t old_offset = diff_scroll_offset_;
                           diff_scroll_offset_ += VISIBLE_LINES;
                           if (diff_scroll_offset_ > max_offset) {
                               diff_scroll_offset_ = max_offset;
                           }
                           pnana::utils::Logger::getInstance().log(
                               "GitPanel::CatchEvent - Scrolled down from " +
                               std::to_string(old_offset) + " to " +
                               std::to_string(diff_scroll_offset_));
                           component_needs_rebuild_ =
                               true; // Force component rebuild after scrolling
                       }
                       return true;
                   }
                   pnana::utils::Logger::getInstance().log(
                       "GitPanel::CatchEvent - Consuming event in diff viewer");
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
            pnana::utils::Logger::getInstance().log(
                "GitPanel::handleStatusModeKey - Toggled detailed stats: " +
                std::string(show_detailed_stats_ ? "on" : "off"));
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

    return false;
}

bool GitPanel::handleCommitModeKey(Event event) {
    // Tab navigation between modes
    if (event == Event::Tab) {
        switchMode(getNextMode(current_mode_));
        return true;
    }

    if (event == Event::Return) {
        performCommit();
        return true;
    }
    if (event == Event::Escape) {
        switchMode(GitPanelMode::STATUS);
        return true;
    }

    // Handle text input for commit message
    if (event.is_character()) {
        commit_message_ += event.character();
        return true;
    }
    if (event == Event::Backspace && !commit_message_.empty()) {
        commit_message_.pop_back();
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
        branch_name_ += event.character();
        return true;
    }
    if (event == Event::Backspace && !branch_name_.empty()) {
        branch_name_.pop_back();
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
    // Tab navigation between modes
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
    auto stats_start = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitPanel::updateCachedStats - START (" +
                                            std::to_string(files_.size()) + " files)");

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

    auto stats_end = std::chrono::high_resolution_clock::now();
    auto stats_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(stats_end - stats_start);
    pnana::utils::Logger::getInstance().log("GitPanel::updateCachedStats - END - " +
                                            std::to_string(stats_duration.count()) + "ms (" +
                                            std::to_string(cached_staged_count_) + " staged, " +
                                            std::to_string(cached_unstaged_count_) + " unstaged)");
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

    pnana::utils::Logger::getInstance().log("GitPanel::getCachedRepoPathDisplay - Cache updated: " +
                                            cached_repo_path_display_);

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

    pnana::utils::Logger::getInstance().log("GitPanel::getCachedCurrentBranch - Cache updated: " +
                                            cached_current_branch_);

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

    std::string log_msg = "GitPanel::getCachedBranchStatus - Cache updated: ahead=" +
                          std::to_string(cached_branch_status_.ahead) +
                          ", behind=" + std::to_string(cached_branch_status_.behind) +
                          ", remote=" + cached_branch_status_.remote_branch;
    pnana::utils::Logger::getInstance().log(log_msg);

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
    diff_viewer_visible_ = true;
    error_message_ = git_manager_->getLastError();
    git_manager_->clearError();
}

void GitPanel::hideDiffViewer() {
    diff_viewer_visible_ = false;
    diff_content_.clear();
    current_diff_file_.clear();
    diff_scroll_offset_ = 0;
}

void GitPanel::handleDiffViewerEscape() {
    hideDiffViewer();
    component_needs_rebuild_ = true; // Force component rebuild after hiding viewer
}

} // namespace vgit
} // namespace pnana
