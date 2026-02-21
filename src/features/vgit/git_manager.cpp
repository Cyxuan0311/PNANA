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
#include <sys/wait.h>
#include <thread>

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
}

bool GitManager::isGitRepository() const {
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_check =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_repo_check_);

    // Use cached result if within timeout
    if (repo_status_cached_ && time_since_last_check < repo_cache_timeout_) {
        return is_git_repo_cached_;
    }

    // Cache miss - execute git command

    std::string cmd = "git -C \"" + repo_path_ + "\" rev-parse --git-dir 2>/dev/null";
    std::string result = executeGitCommand(cmd);
    bool is_git_repo = !result.empty() && result.find("fatal") == std::string::npos;

    // Update cache
    repo_status_cached_ = true;
    is_git_repo_cached_ = is_git_repo;
    last_repo_check_ = now;

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

bool GitManager::clone(const std::string& url, const std::string& path) {
    if (url.empty()) {
        last_error_ = "Repository URL cannot be empty";
        return false;
    }

    if (path.empty()) {
        last_error_ = "Clone path cannot be empty";
        return false;
    }

    // Check if target directory already exists and is not empty
    if (fs::exists(path) && !fs::is_empty(path)) {
        last_error_ = "Target directory is not empty: " + path;
        return false;
    }

    clearError();

    // Construct git clone command
    // Use --quiet flag to suppress progress output, redirect stderr to stdout to capture errors
    std::string escaped_url = escapePath(url);
    std::string escaped_path = escapePath(path);
    std::string cmd = "git clone --quiet \"" + escaped_url + "\" \"" + escaped_path + "\" 2>&1";

    // Execute clone command
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute git clone command";
        return false;
    }

    // Read error output (stderr redirected to stdout, stdout redirected to /dev/null)
    std::array<char, 256> buffer;
    std::string error_output;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        error_output += buffer.data();
    }

    int status = pclose(pipe);

    if (status != 0) {
        // Extract meaningful error message from git output
        std::string error_msg;
        if (!error_output.empty()) {
            // Clean up error output: remove newlines and extract key error message
            std::string cleaned = error_output;
            // Remove trailing newlines
            while (!cleaned.empty() && (cleaned.back() == '\n' || cleaned.back() == '\r')) {
                cleaned.pop_back();
            }

            // Try to extract the most relevant error line (usually the last line with "fatal" or
            // "error")
            std::istringstream iss(cleaned);
            std::string line;
            std::string last_error_line;
            while (std::getline(iss, line)) {
                std::string lower = line;
                std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
                    return std::tolower(c);
                });
                if (lower.find("fatal") != std::string::npos ||
                    lower.find("error") != std::string::npos ||
                    lower.find("failed") != std::string::npos) {
                    last_error_line = line;
                }
            }

            if (!last_error_line.empty()) {
                // Remove "fatal: " prefix if present
                if (last_error_line.find("fatal: ") == 0) {
                    error_msg = last_error_line.substr(7);
                } else {
                    error_msg = last_error_line;
                }
            } else {
                // Use first non-empty line or full output if no error keywords found
                std::istringstream iss2(cleaned);
                if (std::getline(iss2, line)) {
                    error_msg = line;
                } else {
                    error_msg = cleaned;
                }
            }
        }

        if (error_msg.empty()) {
            last_error_ = "Clone failed with exit code: " + std::to_string(status);
        } else {
            // Clean up the error message
            while (!error_msg.empty() && (error_msg.back() == '\n' || error_msg.back() == '\r')) {
                error_msg.pop_back();
            }
            last_error_ = error_msg;
        }

        return false;
    }

    // Clone successful
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
    // Check cache validity
    auto now = std::chrono::steady_clock::now();
    auto time_since_last_refresh =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_status_refresh_);

    if (time_since_last_refresh < status_cache_timeout_ && !current_status_.empty()) {
        return true;
    }

    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    clearError();

    // Get porcelain status output - using v2 for better performance
    std::string cmd = "git -C \"" + repo_root_ + "\" status --porcelain=v2";
    auto lines = executeGitCommandLines(cmd);

    current_status_.clear();

    for (const auto& line : lines) {
        if (line.length() >= 3) {
            parseStatusLine(line, current_status_);
        }
    }

    // Update cache timestamp
    last_status_refresh_ = std::chrono::steady_clock::now();

    return true;
}

bool GitManager::refreshStatusForced() {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    clearError();

    // Get porcelain status output - forced refresh, no cache
    std::string cmd = "git -C \"" + repo_root_ + "\" status --porcelain=v2";
    auto lines = executeGitCommandLines(cmd);

    current_status_.clear();

    for (const auto& line : lines) {
        if (line.length() >= 3) {
            parseStatusLine(line, current_status_);
        }
    }

    // Update cache timestamp
    last_status_refresh_ = std::chrono::steady_clock::now();

    return true;
}

bool GitManager::stageFile(const std::string& path) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string escaped_path = escapePath(path);
    std::string cmd = "git -C \"" + repo_root_ + "\" add \"" + escaped_path + "\"";

    // 对于单个文件操作，尝试在遇到 index.lock 时重试几次（处理并发 git 进程的短暂冲突）
    const int max_attempts = 6;
    std::string last_output;
    bool ok = false;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        std::string cmd_with_err = cmd + " 2>&1";
        last_output = executeGitCommand(cmd_with_err);

        if (last_output.empty()) {
            ok = true;
            break;
        }

        std::string lower = last_output;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        // If index.lock or unable to create lockfile, wait and retry (transient)
        if (lower.find("index.lock") != std::string::npos ||
            lower.find("unable to create") != std::string::npos ||
            lower.find("another git process") != std::string::npos) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150 * (attempt + 1)));
            continue;
        }

        // Other errors - do not retry
        last_error_ = "Failed to stage file: " + last_output;
        return false;
    }

    if (!ok) {
        last_error_ = "Failed to stage file after retries: " + last_output;
        return false;
    }

    // 不在这里调用refreshStatus，让调用者决定何时刷新
    return true;
}

bool GitManager::unstageFile(const std::string& path) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string escaped_path = escapePath(path);
    std::string cmd = "git -C \"" + repo_root_ + "\" reset HEAD \"" + escaped_path + "\"";

    // 使用更快的同步执行
    {
        // Run command and capture both stdout and stderr to diagnose failures
        std::string cmd_with_err = cmd + " 2>&1";
        std::string output = executeGitCommand(cmd_with_err);
        if (!output.empty()) {
            // Some git commands print warnings to stdout/stderr but still succeed.
            // Attempt to detect common success messages; otherwise treat as error.
            // If output contains 'fatal' or 'error', consider it a failure.
            std::string lower = output;
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            if (lower.find("fatal") != std::string::npos ||
                lower.find("error") != std::string::npos) {
                last_error_ = "Failed to unstage file: " + output;
                return false;
            }
        }
    }

    return true;
}

bool GitManager::stageAll() {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" add .";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute git add command";
        return false;
    }

    int status = pclose(pipe);
    if (status != 0) {
        last_error_ = "Failed to stage all files (exit code: " + std::to_string(status) + ")";
        return false;
    }

    return true;
}

bool GitManager::unstageAll() {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string cmd = "git -C \"" + repo_root_ + "\" reset HEAD";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        last_error_ = "Failed to execute git reset command";
        return false;
    }

    int status = pclose(pipe);
    if (status != 0) {
        last_error_ = "Failed to unstage all files (exit code: " + std::to_string(status) + ")";
        return false;
    }

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
    // Capture output (stdout/stderr) to decide success: git prints summary on success.
    std::string output = executeGitCommand(cmd + " 2>&1");
    std::string lower = output;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    // If output contains fatal or error, treat as failure
    if (lower.find("fatal") != std::string::npos || lower.find("error") != std::string::npos ||
        lower.find("aborting") != std::string::npos) {
        last_error_ = "Failed to commit: " + output;
        return false;
    }

    // Success - refresh status and return true. Keep output (summary) in logs if needed.
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

std::vector<GitCommit> GitManager::getGraphCommits(int count) {
    if (!isGitRepository()) {
        return {};
    }

    // Use git log with graph format to get commit history
    // We'll use a simpler format that's easier to parse
    std::string cmd = "git -C \"" + repo_root_ + "\" log --oneline --all --decorate -n " +
                      std::to_string(count) + " --pretty=format:\"%H|%s|%an|%ad\" --date=short";
    auto lines = executeGitCommandLines(cmd);

    std::vector<GitCommit> commits;

    for (const auto& line : lines) {
        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        // Parse format: hash|message|author|date
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
    std::string output = executeGitCommand(cmd + " 2>&1");
    std::string lower = output;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    if (lower.find("fatal") != std::string::npos || lower.find("error") != std::string::npos ||
        lower.find("aborting") != std::string::npos) {
        last_error_ = "Failed to push: " + output;
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
    std::string output = executeGitCommand(cmd + " 2>&1");
    std::string lower = output;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    if (lower.find("fatal") != std::string::npos || lower.find("error") != std::string::npos ||
        lower.find("aborting") != std::string::npos) {
        last_error_ = "Failed to pull: " + output;
        return false;
    }
    refreshStatus();
    return true;
}

size_t GitManager::getStagedCount() const {
    if (!isGitRepository()) {
        return 0;
    }

    // Use git diff --cached --name-only to list staged files and count lines
    std::string cmd = "git -C \"" + repo_root_ + "\" diff --cached --name-only";
    auto lines = executeGitCommandLines(cmd);
    return lines.size();
}

bool GitManager::fetch(const std::string& remote) {
    if (!isGitRepository()) {
        last_error_ = "Not a git repository";
        return false;
    }

    std::string target_remote = remote.empty() ? "origin" : remote;
    std::string cmd = "git -C \"" + repo_root_ + "\" fetch \"" + target_remote + "\"";
    std::string output = executeGitCommand(cmd + " 2>&1");
    std::string lower = output;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    if (lower.find("fatal") != std::string::npos || lower.find("error") != std::string::npos ||
        lower.find("aborting") != std::string::npos) {
        last_error_ = "Failed to fetch: " + output;
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
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        return "Failed to execute command";
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

std::vector<std::string> GitManager::executeGitCommandLines(const std::string& command) const {
    std::array<char, 128> buffer;
    std::vector<std::string> lines;
    std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        return lines;
    }

    std::string current_line;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        current_line += buffer.data();

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

    return lines;
}

GitFileStatus GitManager::parseStatusChar(char status_char) const {
    switch (status_char) {
        case ' ':
            return GitFileStatus::UNMODIFIED;
        case '.':
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
    if (line.empty())
        return;

    // Support both porcelain v1 (two-column XY at positions 0/1) and porcelain v2 formats.
    // Porcelain v2 common prefixes:
    //  - '1' entries: "1 XY SUB M H I W path"
    //  - '2' entries: "2 XY SCORE path orig-path"
    //  - 'u' entries: unmerged entries start with 'u' then space then XY...
    //  - '?' and '!' entries: "? path" or "! path"
    char index_status = ' ';
    char worktree_status = ' ';
    std::string remaining;

    if (line[0] == '1' || line[0] == '2' || line[0] == 'u') {
        // porcelain v2 style: "1 XY ...", extract XY at positions 2 and 3
        if (line.size() < 4)
            return;
        index_status = line[2];
        worktree_status = line[3];
        // remaining content starts after "1 XY " (5 chars)
        if (line.size() > 5)
            remaining = line.substr(5);
        else
            remaining.clear();
    } else if (line[0] == '?' || line[0] == '!') {
        // untracked or ignored (porcelain v2 or v1 style)
        index_status = line[0];
        worktree_status = ' ';
        if (line.size() > 2)
            remaining = line.substr(2);
        else
            remaining.clear();
    } else {
        // Assume porcelain v1 style "XY path..."
        if (line.length() < 3)
            return;
        index_status = line[0];
        worktree_status = line[1];
        remaining = line.substr(2);
    }

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
    // In porcelain v2, index_status '.' means no index change -> not staged.
    bool staged = (index_status != ' ' && index_status != '?' && index_status != '.');

    GitFileStatus status;
    if (index_status != ' ' && worktree_status != ' ') {
        // Both staged and unstaged indicators present.
        // Favor the index (staged) status when it indicates a real change (not '.'),
        // otherwise fall back to worktree status.
        if (index_status != '.' && index_status != ' ') {
            status = parseStatusChar(index_status);
        } else {
            status = (worktree_status == 'M') ? GitFileStatus::MODIFIED
                                              : parseStatusChar(worktree_status);
        }
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

GitBranchStatus GitManager::getBranchStatus(const std::string& branch) {
    std::string branch_name = branch.empty() ? getCurrentBranch() : branch;

    if (branch_name.empty()) {
        last_error_ = "No current branch";
        return GitBranchStatus();
    }

    // Get remote tracking branch
    std::string remote_cmd = "git -C \"" + repo_path_ + "\" rev-parse --abbrev-ref " + branch_name +
                             "@{upstream} 2>/dev/null";
    std::string remote_branch = executeGitCommand(remote_cmd);

    if (remote_branch.empty() || remote_branch.find("fatal") != std::string::npos) {
        // No remote tracking branch
        return GitBranchStatus(0, 0, "", false);
    }

    // Remove trailing newline
    if (!remote_branch.empty() && remote_branch.back() == '\n') {
        remote_branch.pop_back();
    }

    // Get ahead/behind count using rev-list
    std::string count_cmd = "git -C \"" + repo_path_ + "\" rev-list --left-right --count " +
                            branch_name + "..." + remote_branch + " 2>/dev/null";
    std::string count_output = executeGitCommand(count_cmd);

    if (count_output.empty() || count_output.find("fatal") != std::string::npos) {
        // Cannot determine ahead/behind status
        return GitBranchStatus(0, 0, remote_branch, true);
    }

    // Parse the output (format: "ahead\tbehind")
    size_t tab_pos = count_output.find('\t');
    if (tab_pos == std::string::npos) {
        return GitBranchStatus(0, 0, remote_branch, true);
    }

    try {
        int ahead = std::stoi(count_output.substr(0, tab_pos));
        int behind = std::stoi(count_output.substr(tab_pos + 1));

        return GitBranchStatus(ahead, behind, remote_branch, true);
    } catch (const std::exception&) {
        // Parse error
        return GitBranchStatus(0, 0, remote_branch, true);
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
