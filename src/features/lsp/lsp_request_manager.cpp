#include "features/lsp/lsp_request_manager.h"
#include "utils/logger.h"
#include <chrono>
#include <sstream>

namespace pnana {
namespace features {

LspRequestManager::LspRequestManager() : running_(true), next_id_(1) {
    worker_thread_ = std::thread(&LspRequestManager::workerLoop, this);
    // Check env var for JSON perf logging
    const char* env = std::getenv("PNANA_PERF_JSON");
    json_perf_enabled_ = (env && std::string(env) == "1");
    // Request manager started
}

LspRequestManager::~LspRequestManager() {
    stop();
}

int LspRequestManager::postRequest(Priority priority, RequestTask task, CancelCallback on_cancel) {
    Request req;
    req.id = next_id_.fetch_add(1);
    req.priority = priority;
    req.task = std::move(task);
    req.on_cancel = std::move(on_cancel);
    req.enqueue_time = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(req);
        active_map_.emplace(req.id, req);
    }
    cv_.notify_one();

    if (json_perf_enabled_) {
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                      req.enqueue_time.time_since_epoch())
                      .count();
        LOG(std::string("{\"event\":\"post\",\"id\":") + std::to_string(req.id) + ",\"priority\":" +
            std::to_string(static_cast<int>(priority)) + ",\"ts\":" + std::to_string(ts) + "}");
    }
    return req.id;
}

bool LspRequestManager::cancelRequest(int request_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_map_.find(request_id);
    if (it == active_map_.end())
        return false;
    // Mark cancel by calling cancel callback if provided
    if (it->second.on_cancel) {
        try {
            it->second.on_cancel();
        } catch (...) {
            LOG_WARNING("Exception in cancel callback for request id=" +
                        std::to_string(request_id));
        }
    }
    // If this request had a dedup_key, remove mapping
    if (!it->second.dedup_key.empty()) {
        auto dik = dedup_map_.find(it->second.dedup_key);
        if (dik != dedup_map_.end() && dik->second == request_id) {
            dedup_map_.erase(dik);
        }
    }
    active_map_.erase(it);
    // Note: cannot remove from priority_queue efficiently; workerLoop will skip entries not in
    // active_map_ Request cancelled
    return true;
}

int LspRequestManager::postOrReplace(const std::string& dedup_key, Priority priority,
                                     RequestTask task, CancelCallback on_cancel) {
    std::lock_guard<std::mutex> lock(mutex_);
    // If a request with same key exists, cancel it
    auto existing_it = dedup_map_.find(dedup_key);
    if (existing_it != dedup_map_.end()) {
        int old_id = existing_it->second;
        // cancelRequest will remove from active_map_ and dedup_map_ entry
        cancelRequest(old_id);
    }

    Request req;
    req.id = next_id_.fetch_add(1);
    req.priority = priority;
    req.task = std::move(task);
    req.on_cancel = std::move(on_cancel);
    req.enqueue_time = std::chrono::steady_clock::now();
    req.dedup_key = dedup_key;

    queue_.push(req);
    active_map_.emplace(req.id, req);
    dedup_map_[dedup_key] = req.id;
    cv_.notify_one();

    // Request posted or replaced
    return req.id;
}

void LspRequestManager::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false))
        return;
    cv_.notify_all();
    if (worker_thread_.joinable())
        worker_thread_.join();
}

bool LspRequestManager::isRunning() const {
    return running_.load();
}

void LspRequestManager::workerLoop() {
    while (running_) {
        Request req;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return !queue_.empty() || !running_;
            });
            if (!running_ && queue_.empty())
                break;

            // Pop until we find a request that still exists in active_map_
            while (!queue_.empty()) {
                req = queue_.top();
                queue_.pop();
                auto it = active_map_.find(req.id);
                if (it != active_map_.end()) {
                    // use the canonical request stored in map (in case moved)
                    req = it->second;
                    break;
                }
                // else skip stale/cancelled request
            }
            // If queue exhausted, continue
            if (req.task == nullptr) {
                continue;
            }
            // remove from active_map_ to mark executing
            active_map_.erase(req.id);
            // if request was deduped, remove dedup map entry (pending -> executing)
            if (!req.dedup_key.empty()) {
                auto dik = dedup_map_.find(req.dedup_key);
                if (dik != dedup_map_.end() && dik->second == req.id) {
                    dedup_map_.erase(dik);
                }
            }
        }

        // Execute outside lock and measure duration
        try {
            auto start = std::chrono::steady_clock::now();
            req.task();
            auto end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            // Request executed successfully
        } catch (const std::exception& e) {
            LOG_WARNING(std::string("Exception executing LSP request id=") +
                        std::to_string(req.id) + ": " + e.what());
        } catch (...) {
            LOG_WARNING("Unknown exception executing LSP request id=" + std::to_string(req.id));
        }
    }
}

} // namespace features
} // namespace pnana
