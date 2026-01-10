#include "vgit/git_panel.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <future>
#include <mutex>
#include <sstream>

using namespace ftxui;
using namespace pnana::ui::icons;

namespace pnana {
namespace vgit {

GitPanel::GitPanel(ui::Theme& theme, const std::string& repo_path)
    : theme_(theme), git_manager_(std::make_unique<GitManager>(repo_path)) {
    // 延迟加载git数据，只在面板显示时才加载
}

Component GitPanel::getComponent() {
    // 总是重新构建组件以确保状态变化被反映
    main_component_ = buildMainComponent();
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
        hide();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitPanel::onKeyPress - END (escape) - " +
                                                std::to_string(duration.count()) + "ms");
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
    }

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

            // 在主线程中更新UI数据
            std::lock_guard<std::mutex> lock(data_mutex_);
            files_ = std::move(files);
            error_message_ = std::move(error);

            // 分支数据变化较少，只有在第一次加载或明确需要时才获取
            if (branches_.empty() || branch_data_stale_) {
                auto branches = git_manager_->getBranches();
                branches_ = std::move(branches);
                branch_data_stale_ = false;
            }

            data_loading_ = false;
            data_loaded_ = true;
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
    } else if (mode == GitPanelMode::BRANCH) {
        branch_name_.clear();
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

    bool success = true;
    for (size_t index : selected_files_) {
        if (index < files_.size()) {
            if (!git_manager_->stageFile(files_[index].path)) {
                success = false;
                error_message_ = git_manager_->getLastError();
                break;
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
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performStageSelected - Marked data as stale, will refresh on next access");
    }

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

    bool success = true;
    for (size_t index : selected_files_) {
        if (index < files_.size()) {
            if (!git_manager_->unstageFile(files_[index].path)) {
                success = false;
                error_message_ = git_manager_->getLastError();
                break;
            }
        }
    }
    if (success) {
        // 延迟刷新状态，避免频繁的Git命令调用
        // refreshStatusOnly();
        clearSelection();

        // 标记数据已过期，下次需要时再刷新
        data_loaded_ = false;
        pnana::utils::Logger::getInstance().log(
            "GitPanel::performUnstageSelected - Marked data as stale, will refresh on next access");
    }

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
        refreshData(); // commit后需要刷新所有数据，包括分支
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
    std::string repo_path =
        git_manager_->getRepositoryRoot().empty() ? "." : git_manager_->getRepositoryRoot();
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
        makeTab("Remote", GitPanelMode::REMOTE, current_mode_ == GitPanelMode::REMOTE)};
    return hbox(std::move(elements)) | center;
}

Element GitPanel::renderStatusPanel() {
    auto& colors = theme_.getColors();

    // 如果数据正在加载，显示加载指示器
    if (data_loading_) {
        return vbox({text("Loading git status...") | color(colors.comment) | center,
                     text("Please wait...") | color(colors.menubar_fg) | center}) |
               center | size(HEIGHT, EQUAL, 10);
    }

    Elements file_elements;

    // Enhanced header with status summary
    size_t staged_count = 0, unstaged_count = 0;
    for (const auto& file : files_) {
        if (file.staged) {
            staged_count++;
        } else {
            unstaged_count++;
        }
    }

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
    size_t start = scroll_offset_;
    size_t visible_count = 25; // Show more files at once for better overview
    size_t end = std::min(start + visible_count, files_.size());

    for (size_t i = start; i < end; ++i) {
        bool is_selected =
            std::find(selected_files_.begin(), selected_files_.end(), i) != selected_files_.end();
        bool is_highlighted = (i == selected_index_);
        file_elements.push_back(renderFileItem(files_[i], i, is_selected, is_highlighted));
    }

    if (files_.empty()) {
        Elements empty_elements = {
            text(pnana::ui::icons::CHECK_CIRCLE) | color(colors.success) | bold,
            text(" Working directory clean") | color(colors.success),
            text(" - no changes to commit") | color(colors.comment)};
        file_elements.push_back(hbox(std::move(empty_elements)) | center);
    }

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

    // Staged files summary with better visualization
    size_t staged_count = 0, unstaged_count = 0;
    for (const auto& file : files_) {
        if (file.staged) {
            staged_count++;
        } else {
            unstaged_count++;
        }
    }

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
    std::string current_branch = git_manager_->getCurrentBranch();
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
    std::string current_branch = git_manager_->getCurrentBranch();
    if (!current_branch.empty()) {
        Elements branch_elements = {text(pnana::ui::icons::GIT_BRANCH) | color(colors.keyword),
                                    text(" Current branch: ") | color(colors.menubar_fg),
                                    text(current_branch) | color(colors.foreground) | bold};
        elements.push_back(hbox(std::move(branch_elements)));
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

Element GitPanel::renderFileItem(const GitFile& file, size_t /*index*/, bool is_selected,
                                 bool is_highlighted) {
    auto& colors = theme_.getColors();

    // Enhanced git file item styling inspired by file_browser_view and neovim git plugins
    Color item_color = colors.foreground;
    std::string status_icon = getStatusIcon(file.status);
    std::string status_text = getStatusText(file.status);

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
    // [space][staged_indicator][space][status_icon][space][filename][space][metadata][space]
    Elements row_elements = {
        text(" "), // Leading space for visual breathing room
        text(staged_indicator) | color(staged_color) | bold, // Staged status (prominent)
        text(" "),                                           // Space separator
        text(status_icon) | color(status_color) | bold, // Status icon with status-specific color
        text(" "),                                      // Space separator
        text(display_name) | color(item_color),         // File path/name
        text(" "),                                      // Space separator
        text(metadata) | color(colors.comment)          // Status metadata
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
        item_text = item_text | bgcolor(colors.selection) | color(colors.background) | bold;
    } else if (is_highlighted) {
        item_text = item_text | bgcolor(colors.selection) | color(colors.background) | bold;
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
                text("Modes: [c]ommit/[b]ranch/[r]emote") | color(colors.comment),
                text(" | ") | color(colors.comment), text("Refresh: R/F5") | color(colors.comment),
                text(" | ") | color(colors.comment), text("Exit: ESC") | color(colors.comment)};

            footer_elements.push_back(hbox(line1));
            footer_elements.push_back(hbox(line2));
            break;
        }
        case GitPanelMode::COMMIT: {
            Elements commit_help = {text("Commit: Enter") | color(colors.success) | bold,
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
                                    text("New: n") | color(colors.comment),
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
                                    text("Back: ESC") | color(colors.comment)};
            footer_elements.push_back(hbox(remote_help));
            break;
        }
    }

    // Add scroll indicator if needed
    if (current_mode_ == GitPanelMode::STATUS && files_.size() > 25) {
        size_t total_pages = (files_.size() + 24) / 25; // Ceiling division
        size_t current_page = scroll_offset_ / 25 + 1;
        std::string scroll_info =
            " [" + std::to_string(current_page) + "/" + std::to_string(total_pages) + "]";
        footer_elements.push_back(text(scroll_info) | color(colors.menubar_fg));
    } else if (current_mode_ == GitPanelMode::BRANCH && branches_.size() > 18) {
        size_t total_pages = (branches_.size() + 17) / 18; // Ceiling division
        size_t current_page = scroll_offset_ / 18 + 1;
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
        }

        if (!error_message_.empty()) {
            content_elements.push_back(separatorLight());
            content_elements.push_back(renderError());
        }

        content_elements.push_back(separator());
        content_elements.push_back(renderFooter());

        Element dialog_content = vbox(std::move(content_elements));

        // Use window style like other dialogs with proper sizing
        return window(text("Git Panel"), dialog_content) | size(WIDTH, GREATER_THAN, 75) |
               size(HEIGHT, GREATER_THAN, 28) | bgcolor(colors.background) | border;
    });
}

// Key handlers

bool GitPanel::handleStatusModeKey(Event event) {
    const size_t visible_items = 25; // Match renderStatusPanel visible count

    // Fast navigation - minimal scrolling logic for better performance
    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
            // Only scroll when necessary
            if (selected_index_ < scroll_offset_) {
                scroll_offset_ = selected_index_;
            }
        }
        return true;
    }
    if (event == Event::ArrowDown) {
        if (selected_index_ < files_.size() - 1) {
            selected_index_++;
            // Only scroll when necessary
            if (selected_index_ >= scroll_offset_ + visible_items) {
                scroll_offset_ = selected_index_ - visible_items + 1;
            }
        }
        return true;
    }

    // Page navigation
    if (event == Event::PageUp) {
        if (selected_index_ >= visible_items) {
            selected_index_ -= visible_items;
        } else {
            selected_index_ = 0;
        }
        scroll_offset_ =
            (selected_index_ > visible_items / 2) ? selected_index_ - visible_items / 2 : 0;
        return true;
    }
    if (event == Event::PageDown) {
        if (selected_index_ + visible_items < files_.size()) {
            selected_index_ += visible_items;
        } else {
            selected_index_ = files_.size() - 1;
        }
        scroll_offset_ =
            (selected_index_ > visible_items / 2) ? selected_index_ - visible_items / 2 : 0;
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
        scroll_offset_ = (files_.size() > visible_items) ? files_.size() - visible_items : 0;
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
    if (event == Event::Character('s')) { // Stage selected
        performStageSelected();
        return true;
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

    // Mode switching
    if (event == Event::Character('c')) { // Commit mode
        switchMode(GitPanelMode::COMMIT);
        return true;
    }
    if (event == Event::Character('b')) { // Branch mode
        switchMode(GitPanelMode::BRANCH);
        return true;
    }
    if (event == Event::Character('r')) { // Remote mode
        switchMode(GitPanelMode::REMOTE);
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
    if (event == Event::Character('n')) {
        // Start creating new branch - prepare input mode
        branch_name_.clear();
        // Note: In a full implementation, we would switch to an input mode
        // For now, we'll just clear the branch name to indicate we're ready for input
        return true;
    }
    if (event == Event::Character('d')) {
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

bool GitPanel::isNavigationKey(Event event) const {
    return event == Event::ArrowUp || event == Event::ArrowDown || event == Event::PageUp ||
           event == Event::PageDown || event == Event::Home || event == Event::End;
}

} // namespace vgit
} // namespace pnana
