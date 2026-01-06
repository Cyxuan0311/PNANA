#ifndef PNANA_FEATURES_LSP_LSP_WORKER_POOL_H
#define PNANA_FEATURES_LSP_LSP_WORKER_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace pnana {
namespace features {

class LspWorkerPool {
  public:
    explicit LspWorkerPool(size_t num_threads = 0);
    ~LspWorkerPool();

    // Non-copyable
    LspWorkerPool(const LspWorkerPool&) = delete;
    LspWorkerPool& operator=(const LspWorkerPool&) = delete;

    // Post a new task to be executed by the pool
    void postTask(std::function<void()> task);

    // Stop the pool and join threads
    void stop();
    bool isRunning() const;

  private:
    void workerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_LSP_WORKER_POOL_H
