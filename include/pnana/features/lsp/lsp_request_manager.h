#ifndef PNANA_FEATURES_LSP_LSP_REQUEST_MANAGER_H
#define PNANA_FEATURES_LSP_LSP_REQUEST_MANAGER_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace pnana {
namespace features {

class LspRequestManager {
  public:
    enum class Priority { HIGH = 0, NORMAL = 1, LOW = 2 };

    using RequestTask = std::function<void()>;
    using CancelCallback = std::function<void()>;

    struct Request {
        int id;
        Priority priority;
        RequestTask task;
        CancelCallback on_cancel;
        std::chrono::steady_clock::time_point enqueue_time;
        std::string dedup_key; // optional dedup key for replace semantics
    };

    LspRequestManager();
    ~LspRequestManager();

    // Non-copyable
    LspRequestManager(const LspRequestManager&) = delete;
    LspRequestManager& operator=(const LspRequestManager&) = delete;

    // Post a request, returns request id
    int postRequest(Priority priority, RequestTask task, CancelCallback on_cancel = nullptr);

    // Post a request but replace any existing pending request with the same key.
    // Returns the new request id.
    int postOrReplace(const std::string& dedup_key, Priority priority, RequestTask task,
                      CancelCallback on_cancel = nullptr);

    // Cancel a pending request by id. Returns true if cancelled.
    bool cancelRequest(int request_id);

    // Stop the manager and worker thread
    void stop();
    bool isRunning() const;

  private:
    void workerLoop();
    struct QueueCompare {
        bool operator()(const Request& a, const Request& b) const {
            if (a.priority != b.priority)
                return a.priority > b.priority;
            return a.enqueue_time > b.enqueue_time;
        }
    };

    std::priority_queue<Request, std::vector<Request>, QueueCompare> queue_;
    std::map<int, Request> active_map_;
    std::unordered_map<std::string, int> dedup_map_; // dedup_key -> request id
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_;
    std::atomic<int> next_id_;
    bool json_perf_enabled_; // format perf logs as JSON-lines when true
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_LSP_REQUEST_MANAGER_H
