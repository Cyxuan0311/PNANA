#include "features/vgit/git_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>

namespace fs = std::filesystem;

namespace pnana {
namespace vgit {

GitManager::GitManager(const std::string& repo_path) : repo_path_(repo_path) {
    // Find repository root
    repo_root_ = getRepositoryRoot();
    // Initialize cache timestamps
    last_repo_check_ =
        std::chrono::steady_clock::now() - repo_cache_timeout_; // Force initial check
    last_status_refresh_ =
        std::chrono::steady_clock::now() - status_cache_timeout_; // Force initial refresh
}

void GitManager::invalidateRepoStatusCache() {
    repo_status_cached_ = false;
    repo_root_cached_.clear();
    last_repo_check_ = std::chrono::steady_clock::now() - repo_cache_timeout_;
    pnana::utils::Logger::getInstance().log(
        "GitManager::invalidateRepoStatusCache - Repository status cache invalidated");
}

bool GitManager::isGitRepository() const {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_check =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_repo_check_);

    // Use cached result if within timeout
    if (repo_status_cached_ && time_since_last_check < repo_cache_timeout_) {
        pnana::utils::Logger::getInstance().log(
            "GitManager::isGitRepository - Using cached repo status (age: " +
            std::to_string(time_since_last_check.count()) + "ms)");
        return is_git_repo_cached_;
    }

    // Cache miss - execute git command
    pnana::utils::Logger::getInstance().log(
        "GitManager::isGitRepository - Cache miss, executing git command");
    auto check_start = std::chrono::high_resolution_clock::now();

    std::string cmd = "git -C \"" + repo_path_ + "\" rev-parse --git-dir 2>/dev/null";
    std::string result = executeGitCommand(cmd);
    bool is_git_repo = !result.empty() && result.find("fatal") == std::string::npos;

    auto check_end = std::chrono::high_resolution_clock::now();
    auto check_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(check_end - check_start);

    // Update cache
    repo_status_cached_ = true;
    is_git_repo_cached_ = is_git_repo;
    last_repo_check_ = now;

    pnana::utils::Logger::getInstance().log("GitManager::isGitRepository - Cache updated (took " +
                                            std::to_string(check_duration.count()) +
                                            "ms, result: " + (is_git_repo ? "true" : "false") +
                                            ")");

    return is_git_repo;
}

bool GitManager::initRepository() {
    std::string cmd = "git -C \"" + repo_path_ + "\" init";
    std::string result = executeGitCommand(cmd);
    if (!result.empty()) {
        last_error_ = "Failed to initialize git repository";
        return false;
    }
    repo_root_ = repo_path_;
    // Repository state has changed, invalidate cache
    invalidateRepoStatusCache();
    return true;
}

std::string GitManager::getRepositoryRoot() const {
    // Use cached repo root if available and repo status is valid
    if (repo_status_cached_ && is_git_repo_cached_ && !repo_root_cached_.empty()) {
        return repo_root_cached_;
    }

    if (!isGitRepository()) {
        repo_root_cached_ = "";
        return "";
    }

    std::string cmd = "git -C \"" + repo_path_ + "\" rev-parse --show-toplevel 2>/dev/null";
    std::string result = executeGitCommand(cmd);
    if (!result.empty()) {
        // Remove trailing newline
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        repo_root_cached_ = result;
        return result;
    }
    repo_root_cached_ = "";
    return "";
}

std::vector<GitFile> GitManager::getStatus() {
    if (!isGitRepository()) {
        return {};
    }

    refreshStatus();
    return current_status_;
}

bool GitManager::refreshStatus() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitManager::refreshStatus - START");

    // Check cache validity
    auto cache_check_start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_refresh =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_status_refresh_);
    auto cache_check_end = std::chrono::high_resolution_clock::now();
    auto cache_check_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(cache_check_end - cache_check_start);

    if (time_since_last_refresh < status_cache_timeout_ && !current_status_.empty()) {
        pnana::utils::Logger::getInstance().log(
            "GitManager::refreshStatus - Using cached status (age: " +
            std::to_string(time_since_last_refresh.count()) +
            "ms, cache check: " + std::to_string(cache_check_duration.count()) + "ms)");
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::refreshStatus - END (cached) - " +
                                                std::to_string(duration.count()) + "ms");
        return true;
    }

    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitManager::refreshStatus - END (not git repo) - " + std::to_string(duration.count()) +
            "ms");
        return false;
    }

    clearError();

    // Get porcelain status output - using v2 for better performance
    auto git_cmd_start = std::chrono::high_resolution_clock::now();
    std::string cmd = "git -C \"" + repo_root_ + "\" status --porcelain=v2";
    pnana::utils::Logger::getInstance().log(
        "GitManager::refreshStatus - Executing git status command (porcelain v2)");
    auto lines = executeGitCommandLines(cmd);
    auto git_cmd_end = std::chrono::high_resolution_clock::now();
    auto git_cmd_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(git_cmd_end - git_cmd_start);
    pnana::utils::Logger::getInstance().log("Git status command took " +
                                            std::to_string(git_cmd_duration.count()) + "ms (" +
                                            std::to_string(lines.size()) + " lines returned)");

    auto parse_start = std::chrono::high_resolution_clock::now();
    current_status_.clear();

    for (const auto& line : lines) {
        if (line.length() >= 3) {
            parseStatusLine(line, current_status_);
        }
    }
    auto parse_end = std::chrono::high_resolution_clock::now();
    auto parse_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(parse_end - parse_start);
    pnana::utils::Logger::getInstance().log("Status line parsing took " +
                                            std::to_string(parse_duration.count()) + "ms (" +
                                            std::to_string(lines.size()) + " lines, " +
                                            std::to_string(current_status_.size()) + " files)");

    // Update cache timestamp
    last_status_refresh_ = std::chrono::steady_clock::now();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitManager::refreshStatus - END - " +
                                            std::to_string(duration.count()) + "ms, " +
                                            std::to_string(lines.size()) + " lines");

    return true;
}

bool GitManager::refreshStatusForced() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log(
        "GitManager::refreshStatusForced - START (forced refresh)");

    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitManager::refreshStatusForced - END (not git repo) - " +
            std::to_string(duration.count()) + "ms");
        return false;
    }

    clearError();

    // Get porcelain status output - forced refresh, no cache
    std::string cmd = "git -C \"" + repo_root_ + "\" status --porcelain=v2";
    pnana::utils::Logger::getInstance().log(
        "GitManager::refreshStatusForced - Executing git status command (forced)");
    auto lines = executeGitCommandLines(cmd);

    current_status_.clear();

    for (const auto& line : lines) {
        if (line.length() >= 3) {
            parseStatusLine(line, current_status_);
        }
    }

    // Update cache timestamp
    last_status_refresh_ = std::chrono::steady_clock::now();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitManager::refreshStatusForced - END - " +
                                            std::to_string(duration.count()) + "ms, " +
                                            std::to_string(lines.size()) + " lines");

    return true;
}

bool GitManager::stageFile(const std::string& path) {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitManager::stageFile - START - file: " + path);

    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::stageFile - END (not git repo) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    auto cmd_prep_start = std::chrono::high_resolution_clock::now();
    std::string escaped_path = escapePath(path);
    std::string cmd = "git -C \"" + repo_root_ + "\" add \"" + escaped_path + "\"";
    auto cmd_prep_end = std::chrono::high_resolution_clock::now();
    auto cmd_prep_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(cmd_prep_end - cmd_prep_start);
    pnana::utils::Logger::getInstance().log("Command preparation took " +
                                            std::to_string(cmd_prep_duration.count()) + "ms");

    // 对于单个文件操作，使用更快的同步执行
    auto exec_start = std::chrono::high_resolution_clock::now();
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute git add command";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitManager::stageFile - END (failed to execute) - " +
            std::to_string(duration.count()) + "ms");
        return false;
    }

    // 简单等待命令完成，不读取输出（add命令通常没有输出）
    int status = pclose(pipe);
    auto exec_end = std::chrono::high_resolution_clock::now();
    auto exec_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(exec_end - exec_start);
    pnana::utils::Logger::getInstance().log("Git add command execution took " +
                                            std::to_string(exec_duration.count()) +
                                            "ms (exit code: " + std::to_string(status) + ")");

    if (status != 0) {
        last_error_ = "Failed to stage file (exit code: " + std::to_string(status) + ")";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::stageFile - END (failed) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    // 不在这里调用refreshStatus，让调用者决定何时刷新
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitManager::stageFile - END (success) - " +
                                            std::to_string(duration.count()) + "ms");

    return true;
}

bool GitManager::unstageFile(const std::string& path) {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitManager::unstageFile - START - file: " + path);

    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::unstageFile - END (not git repo) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    auto cmd_prep_start = std::chrono::high_resolution_clock::now();
    std::string escaped_path = escapePath(path);
    std::string cmd = "git -C \"" + repo_root_ + "\" reset HEAD \"" + escaped_path + "\"";
    auto cmd_prep_end = std::chrono::high_resolution_clock::now();
    auto cmd_prep_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(cmd_prep_end - cmd_prep_start);
    pnana::utils::Logger::getInstance().log("Command preparation took " +
                                            std::to_string(cmd_prep_duration.count()) + "ms");

    // 使用更快的同步执行
    auto exec_start = std::chrono::high_resolution_clock::now();
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute git reset command";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitManager::unstageFile - END (failed to execute) - " +
            std::to_string(duration.count()) + "ms");
        return false;
    }

    // 简单等待命令完成
    int status = pclose(pipe);
    auto exec_end = std::chrono::high_resolution_clock::now();
    auto exec_duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(exec_end - exec_start);
    pnana::utils::Logger::getInstance().log("Git reset command execution took " +
                                            std::to_string(exec_duration.count()) +
                                            "ms (exit code: " + std::to_string(status) + ")");

    if (status != 0) {
        last_error_ = "Failed to unstage file (exit code: " + std::to_string(status) + ")";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::unstageFile - END (failed) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitManager::unstageFile - END (success) - " +
                                            std::to_string(duration.count()) + "ms");

    return true;
}

bool GitManager::stageAll() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitManager::stageAll - START");

    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::stageAll - END (not git repo) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" add .";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute git add command";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitManager::stageAll - END (failed to execute) - " + std::to_string(duration.count()) +
            "ms");
        return false;
    }

    int status = pclose(pipe);
    if (status != 0) {
        last_error_ = "Failed to stage all files (exit code: " + std::to_string(status) + ")";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::stageAll - END (failed) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitManager::stageAll - END (success) - " +
                                            std::to_string(duration.count()) + "ms");

    return true;
}

bool GitManager::unstageAll() {
    auto start_time = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log("GitManager::unstageAll - START");

    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::unstageAll - END (not git repo) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" reset HEAD";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute git reset command";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log(
            "GitManager::unstageAll - END (failed to execute) - " +
            std::to_string(duration.count()) + "ms");
        return false;
    }

    int status = pclose(pipe);
    if (status != 0) {
        last_error_ = "Failed to unstage all files (exit code: " + std::to_string(status) + ")";
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        pnana::utils::Logger::getInstance().log("GitManager::unstageAll - END (failed) - " +
                                                std::to_string(duration.count()) + "ms");
        return false;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    pnana::utils::Logger::getInstance().log("GitManager::unstageAll - END (success) - " +
                                            std::to_string(duration.count()) + "ms");

    return true;
}

bool GitManager::commit(const std::string& message) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    if (message.empty()) {
        last_error_ = "Commit message cannot be empty";
        return false;
    }

    // Escape single quotes in message
    std::string escaped_message = message;
    size_t pos = 0;
    while ((pos = escaped_message.find("'", pos)) != std::string::npos) {
        escaped_message.replace(pos, 1, "'\\''");
        pos += 4;
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" commit -m '" + escaped_message + "'";
    std::string result = executeGitCommand(cmd);

    if (!result.empty()) {
        last_error_ = "Failed to commit: " + result;
        return false;
    }

    refreshStatus();
    return true;
}

std::vector<GitCommit> GitManager::getRecentCommits(int count) {
    if (!isGitRepository()) {
        return {};
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" log --oneline -n " + std::to_string(count) +
                      " --pretty=format:\"%H|%s|%an|%ad\" --date=short";
    auto lines = executeGitCommandLines(cmd);

    std::vector<GitCommit> commits;

    for (const auto& line : lines) {
        size_t pos1 = line.find('|');
        if (pos1 == std::string::npos)
            continue;

        size_t pos2 = line.find('|', pos1 + 1);
        if (pos2 == std::string::npos)
            continue;

        size_t pos3 = line.find('|', pos2 + 1);
        if (pos3 == std::string::npos)
            continue;

        std::string hash = line.substr(0, pos1);
        std::string message = line.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string author = line.substr(pos2 + 1, pos3 - pos2 - 1);
        std::string date = line.substr(pos3 + 1);

        commits.emplace_back(hash, message, author, date);
    }

    return commits;
}

std::vector<GitBranch> GitManager::getBranches() {
    if (!isGitRepository()) {
        return {};
    }

    std::string cmd =
        "git -C \"" + repo_root_ + "\" branch -a --format=\"%(refname:short)|%(HEAD)\"";
    auto lines = executeGitCommandLines(cmd);

    std::vector<GitBranch> branches;

    for (const auto& line : lines) {
        size_t sep_pos = line.find('|');
        if (sep_pos == std::string::npos)
            continue;

        std::string name = line.substr(0, sep_pos);
        std::string head_marker = line.substr(sep_pos + 1);

        bool is_current = (head_marker == "*");
        bool is_remote = name.find("remotes/") == 0;

        // Remove remotes/ prefix for cleaner display
        if (is_remote) {
            name = name.substr(8);
        }

        branches.emplace_back(name, is_current, is_remote);
    }

    return branches;
}

bool GitManager::createBranch(const std::string& name) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    if (name.empty()) {
        last_error_ = "Branch name cannot be empty";
        return false;
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" checkout -b \"" + name + "\"";
    std::string result = executeGitCommand(cmd);

    if (!result.empty()) {
        last_error_ = "Failed to create branch: " + result;
        return false;
    }

    return true;
}

bool GitManager::switchBranch(const std::string& name) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" checkout \"" + name + "\"";
    std::string result = executeGitCommand(cmd);

    if (!result.empty()) {
        last_error_ = "Failed to switch branch: " + result;
        return false;
    }

    return true;
}

bool GitManager::deleteBranch(const std::string& name, bool force) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string cmd =
        "git -C \"" + repo_root_ + "\" branch " + (force ? "-D " : "-d ") + "\"" + name + "\"";
    std::string result = executeGitCommand(cmd);

    if (!result.empty()) {
        last_error_ = "Failed to delete branch: " + result;
        return false;
    }

    return true;
}

std::string GitManager::getCurrentBranch() {
    if (!isGitRepository()) {
        return "";
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" branch --show-current";
    std::string result = executeGitCommand(cmd);

    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

bool GitManager::push(const std::string& remote, const std::string& branch) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string current_branch = getCurrentBranch();
    std::string target_branch = branch.empty() ? current_branch : branch;
    std::string target_remote = remote.empty() ? "origin" : remote;

    std::string cmd =
        "git -C \"" + repo_root_ + "\" push \"" + target_remote + "\" \"" + target_branch + "\"";
    std::string result = executeGitCommand(cmd);

    if (!result.empty()) {
        last_error_ = "Failed to push: " + result;
        return false;
    }

    return true;
}

bool GitManager::pull(const std::string& remote, const std::string& branch) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string current_branch = getCurrentBranch();
    std::string target_branch = branch.empty() ? current_branch : branch;
    std::string target_remote = remote.empty() ? "origin" : remote;

    std::string cmd =
        "git -C \"" + repo_root_ + "\" pull \"" + target_remote + "\" \"" + target_branch + "\"";
    std::string result = executeGitCommand(cmd);

    if (!result.empty()) {
        last_error_ = "Failed to pull: " + result;
        return false;
    }

    refreshStatus();
    return true;
}

bool GitManager::fetch(const std::string& remote) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string target_remote = remote.empty() ? "origin" : remote;
    std::string cmd = "git -C \"" + repo_root_ + "\" fetch \"" + target_remote + "\"";
    std::string result = executeGitCommand(cmd);

    if (!result.empty()) {
        last_error_ = "Failed to fetch: " + result;
        return false;
    }

    return true;
}

std::vector<std::string> GitManager::getRemotes() {
    if (!isGitRepository()) {
        return {};
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" remote";
    auto lines = executeGitCommandLines(cmd);

    return lines;
}

std::vector<std::string> GitManager::getDiff(const std::string& path) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return {};
    }

    clearError();

    std::string escaped_path = escapePath(path);

    // Try unstaged changes first
    std::string cmd = "git -C \"" + repo_root_ + "\" diff \"" + escaped_path + "\"";
    auto lines = executeGitCommandLines(cmd);

    // If no unstaged diff, try staged changes
    if (lines.empty()) {
        std::string staged_cmd =
            "git -C \"" + repo_root_ + "\" diff --cached \"" + escaped_path + "\"";
        lines = executeGitCommandLines(staged_cmd);
    }

    // If still no output, set error message
    if (lines.empty()) {
        std::string status_cmd =
            "git -C \"" + repo_root_ + "\" status --porcelain \"" + escaped_path + "\"";
        auto status_lines = executeGitCommandLines(status_cmd);

        if (!status_lines.empty()) {
            last_error_ = "File has changes but no diff output. Status: " + status_lines[0];
        } else {
            last_error_ = "File not found in git status";
        }
    }

    return lines;
}

// Private helper methods

std::string GitManager::executeGitCommand(const std::string& command) const {
    auto cmd_start = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log(
        "GitManager::executeGitCommand - START: " +
        command.substr(0, std::min(size_t(100), command.size())) +
        (command.size() > 100 ? "..." : ""));

    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        auto cmd_end = std::chrono::high_resolution_clock::now();
        auto cmd_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(cmd_end - cmd_start);
        pnana::utils::Logger::getInstance().log(
            "GitManager::executeGitCommand - END (pipe failed) - " +
            std::to_string(cmd_duration.count()) + "ms");
        return "Failed to execute command";
    }

    size_t read_calls = 0;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
        read_calls++;
    }

    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    auto cmd_end = std::chrono::high_resolution_clock::now();
    auto cmd_duration = std::chrono::duration_cast<std::chrono::milliseconds>(cmd_end - cmd_start);
    pnana::utils::Logger::getInstance().log(
        "GitManager::executeGitCommand - END - " + std::to_string(cmd_duration.count()) + "ms (" +
        std::to_string(read_calls) + " reads, " + std::to_string(result.size()) + " bytes)");

    return result;
}

std::vector<std::string> GitManager::executeGitCommandLines(const std::string& command) const {
    auto cmd_start = std::chrono::high_resolution_clock::now();
    pnana::utils::Logger::getInstance().log(
        "GitManager::executeGitCommandLines - START: " +
        command.substr(0, std::min(size_t(100), command.size())) +
        (command.size() > 100 ? "..." : ""));

    std::array<char, 128> buffer;
    std::vector<std::string> lines;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        auto cmd_end = std::chrono::high_resolution_clock::now();
        auto cmd_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(cmd_end - cmd_start);
        pnana::utils::Logger::getInstance().log(
            "GitManager::executeGitCommandLines - END (pipe failed) - " +
            std::to_string(cmd_duration.count()) + "ms");
        return lines;
    }

    size_t read_calls = 0;
    size_t total_bytes = 0;
    std::string current_line;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        current_line += buffer.data();
        read_calls++;
        total_bytes += strlen(buffer.data());

        // Check if we have a complete line
        if (!current_line.empty() && current_line.back() == '\n') {
            // Remove trailing newline
            current_line.pop_back();
            if (!current_line.empty()) {
                lines.push_back(current_line);
            }
            current_line.clear();
        }
    }

    // Handle last line if it doesn't end with newline
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }

    auto cmd_end = std::chrono::high_resolution_clock::now();
    auto cmd_duration = std::chrono::duration_cast<std::chrono::milliseconds>(cmd_end - cmd_start);
    pnana::utils::Logger::getInstance().log(
        "GitManager::executeGitCommandLines - END - " + std::to_string(cmd_duration.count()) +
        "ms (" + std::to_string(read_calls) + " reads, " + std::to_string(total_bytes) +
        " bytes, " + std::to_string(lines.size()) + " lines)");

    return lines;
}

GitFileStatus GitManager::parseStatusChar(char status_char) const {
    switch (status_char) {
        case ' ':
            return GitFileStatus::UNMODIFIED;
        case 'M':
            return GitFileStatus::MODIFIED;
        case 'A':
            return GitFileStatus::ADDED;
        case 'D':
            return GitFileStatus::DELETED;
        case 'R':
            return GitFileStatus::RENAMED;
        case 'C':
            return GitFileStatus::COPIED;
        case 'U':
            return GitFileStatus::UPDATED_BUT_UNMERGED;
        case '?':
            return GitFileStatus::UNTRACKED;
        case '!':
            return GitFileStatus::IGNORED;
        default:
            return GitFileStatus::UNMODIFIED;
    }
}

void GitManager::parseStatusLine(const std::string& line, std::vector<GitFile>& files) {
    if (line.length() < 3)
        return;

    char index_status = line[0];
    char worktree_status = line[1];
    std::string remaining = line.substr(2);

    // Skip leading spaces
    size_t content_start = remaining.find_first_not_of(" \t");
    if (content_start != std::string::npos) {
        remaining = remaining.substr(content_start);
    }

    // Extract the actual file path - for porcelain format, the file path is the last token
    // Format can be: "XY file_path" or "XY ...metadata... file_path"
    std::string path;
    std::string::size_type last_space = remaining.find_last_of(" \t");
    if (last_space != std::string::npos) {
        // Check if this looks like a file path (contains directory separators or common file
        // extensions)
        std::string potential_path = remaining.substr(last_space + 1);
        if (potential_path.find('/') != std::string::npos ||
            potential_path.find('\\') != std::string::npos ||
            potential_path.find('.') != std::string::npos) {
            path = potential_path;
        } else {
            // Fallback to the whole remaining string
            path = remaining;
        }
    } else {
        path = remaining;
    }

    // Handle renamed files (format: "R  old_name -> new_name")
    std::string old_path;
    size_t arrow_pos = path.find(" -> ");
    if (arrow_pos != std::string::npos) {
        old_path = path.substr(0, arrow_pos);
        path = path.substr(arrow_pos + 4);
    }

    // Determine if file is staged
    bool staged = (index_status != ' ' && index_status != '?');

    GitFileStatus status;
    if (index_status != ' ' && worktree_status != ' ') {
        // Both staged and unstaged changes
        status =
            (worktree_status == 'M') ? GitFileStatus::MODIFIED : parseStatusChar(worktree_status);
    } else if (index_status != ' ') {
        // Only staged changes
        status = parseStatusChar(index_status);
    } else {
        // Only unstaged changes
        status = parseStatusChar(worktree_status);
    }

    if (!old_path.empty()) {
        files.emplace_back(path, old_path, status, staged);
    } else {
        files.emplace_back(path, status, staged);
    }
}

std::string GitManager::escapePath(const std::string& path) const {
    std::string escaped = path;
    // Escape quotes
    size_t pos = 0;
    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return escaped;
}

} // namespace vgit
} // namespace pnana
