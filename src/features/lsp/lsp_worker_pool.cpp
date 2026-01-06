#include "features/lsp/lsp_worker_pool.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>

namespace pnana {
namespace features {

LspWorkerPool::LspWorkerPool(size_t num_threads) : running_(true) {
    if (num_threads == 0) {
        num_threads = std::max<size_t>(1, std::thread::hardware_concurrency());
    }
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this, i]() {
            this->workerLoop();
        });
    }
}

LspWorkerPool::~LspWorkerPool() {
    stop();
}

void LspWorkerPool::postTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void LspWorkerPool::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false))
        return;
    cv_.notify_all();
    for (auto& t : workers_) {
        if (t.joinable())
            t.join();
    }
}

bool LspWorkerPool::isRunning() const {
    return running_.load();
}

void LspWorkerPool::workerLoop() {
    while (running_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return !tasks_.empty() || !running_;
            });
            if (!running_ && tasks_.empty())
                break;
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                LOG_WARNING(std::string("Exception in LspWorkerPool task: ") + e.what());
            } catch (...) {
                LOG_WARNING("Unknown exception in LspWorkerPool task");
            }
        }
    }
}

} // namespace features
} // namespace pnana
