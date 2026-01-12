#ifndef PNANA_VGIT_GIT_MANAGER_H
#define PNANA_VGIT_GIT_MANAGER_H

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace vgit {

enum class GitFileStatus {
    UNMODIFIED = 0,
    MODIFIED = 1,
    ADDED = 2,
    DELETED = 3,
    RENAMED = 4,
    COPIED = 5,
    UPDATED_BUT_UNMERGED = 6,
    UNTRACKED = 7,
    IGNORED = 8
};

struct GitFile {
    std::string path;
    std::string old_path; // for renamed files
    GitFileStatus status;
    bool staged = false;

    GitFile(const std::string& p, GitFileStatus s, bool st = false)
        : path(p), status(s), staged(st) {}
    GitFile(const std::string& p, const std::string& op, GitFileStatus s, bool st = false)
        : path(p), old_path(op), status(s), staged(st) {}
};

struct GitBranch {
    std::string name;
    bool is_current = false;
    bool is_remote = false;

    GitBranch(const std::string& n, bool current = false, bool remote = false)
        : name(n), is_current(current), is_remote(remote) {}
};

struct GitCommit {
    std::string hash;
    std::string message;
    std::string author;
    std::string date;

    GitCommit(const std::string& h, const std::string& msg, const std::string& auth,
              const std::string& dt)
        : hash(h), message(msg), author(auth), date(dt) {}
};

class GitManager {
  public:
    GitManager(const std::string& repo_path = ".");
    ~GitManager() = default;

    // Repository operations
    bool isGitRepository() const;
    bool initRepository();
    std::string getRepositoryRoot() const;

    // Status operations
    std::vector<GitFile> getStatus();
    bool refreshStatus();
    bool refreshStatusForced(); // Force refresh, ignore cache

    // Staging operations
    bool stageFile(const std::string& path);
    bool unstageFile(const std::string& path);
    bool stageAll();
    bool unstageAll();

    // Commit operations
    bool commit(const std::string& message);
    std::vector<GitCommit> getRecentCommits(int count = 10);

    // Branch operations
    std::vector<GitBranch> getBranches();
    bool createBranch(const std::string& name);
    bool switchBranch(const std::string& name);
    bool deleteBranch(const std::string& name, bool force = false);
    std::string getCurrentBranch();

    // Remote operations
    bool push(const std::string& remote = "origin", const std::string& branch = "");
    bool pull(const std::string& remote = "origin", const std::string& branch = "");
    bool fetch(const std::string& remote = "origin");
    std::vector<std::string> getRemotes();

    // Utility functions
    std::string getLastError() const {
        return last_error_;
    }
    void clearError() {
        last_error_.clear();
    }

  private:
    std::string repo_path_;
    std::string repo_root_;
    std::vector<GitFile> current_status_;
    std::string last_error_;

    // Status caching for performance optimization
    std::chrono::steady_clock::time_point last_status_refresh_;
    std::chrono::milliseconds status_cache_timeout_{2000}; // 2 seconds cache

    // Helper functions
    std::string executeGitCommand(const std::string& command) const;
    std::vector<std::string> executeGitCommandLines(const std::string& command) const;
    GitFileStatus parseStatusChar(char status_char) const;
    void parseStatusLine(const std::string& line, std::vector<GitFile>& files);
    std::string escapePath(const std::string& path) const;
};

} // namespace vgit
} // namespace pnana

#endif // PNANA_VGIT_GIT_MANAGER_H
