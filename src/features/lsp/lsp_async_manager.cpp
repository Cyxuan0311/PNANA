#include "features/lsp/lsp_async_manager.h"
#include "features/lsp/lsp_client.h"
#include "utils/logger.h"
#include <chrono>
#include <stdexcept>

namespace pnana {
namespace features {

LspAsyncManager::LspAsyncManager() : running_(true) {
    worker_thread_ = std::thread(&LspAsyncManager::workerThread, this);
}

LspAsyncManager::~LspAsyncManager() {
    stop();
}

void LspAsyncManager::requestCompletionAsync(LspClient* client, const std::string& uri,
                                             const LspPosition& position,
                                             CompletionCallback on_success,
                                             ErrorCallback on_error) {
    LOG("[COMPLETION] [AsyncManager] requestCompletionAsync() called");

    if (!client || !running_) {
        LOG("[COMPLETION] [AsyncManager] Client is null or manager is stopped");
        if (on_error) {
            on_error("Client is null or manager is stopped");
        }
        return;
    }

    RequestTask task;
    task.type = RequestTask::COMPLETION;
    task.client = client;
    task.uri = uri;
    task.position = position;
    task.completion_callback = on_success;
    task.error_callback = on_error;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        request_queue_.push(task);
        LOG("[COMPLETION] [AsyncManager] Task queued (queue size: " +
            std::to_string(request_queue_.size()) + ")");
    }
    queue_cv_.notify_one();
    LOG("[COMPLETION] [AsyncManager] Worker thread notified");
}

void LspAsyncManager::workerThread() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] {
            return !request_queue_.empty() || !running_;
        });

        if (!running_ && request_queue_.empty()) {
            break;
        }

        if (!request_queue_.empty()) {
            RequestTask task = request_queue_.front();
            request_queue_.pop();
            lock.unlock();

            try {
                if (task.type == RequestTask::COMPLETION) {
                    auto worker_start = std::chrono::steady_clock::now();
                    LOG("[COMPLETION] [AsyncManager] Worker thread processing completion "
                        "request...");

                    if (task.client && task.client->isConnected()) {
                        LOG("[COMPLETION] [AsyncManager] Calling client->completion()...");
                        auto items = task.client->completion(task.uri, task.position);
                        auto worker_end = std::chrono::steady_clock::now();
                        auto worker_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                            worker_end - worker_start);
                        LOG("[COMPLETION] [AsyncManager] client->completion() returned " +
                            std::to_string(items.size()) + " items (took " +
                            std::to_string(worker_time.count()) + " ms)");

                        if (task.completion_callback) {
                            auto callback_start = std::chrono::steady_clock::now();
                            LOG("[COMPLETION] [AsyncManager] Executing completion callback...");
                            task.completion_callback(items);
                            auto callback_end = std::chrono::steady_clock::now();
                            auto callback_time =
                                std::chrono::duration_cast<std::chrono::microseconds>(
                                    callback_end - callback_start);
                            LOG("[COMPLETION] [AsyncManager] Callback executed (took " +
                                std::to_string(callback_time.count() / 1000.0) + " ms)");
                        }
                    } else {
                        LOG("[COMPLETION] [AsyncManager] Client not connected");
                        if (task.error_callback) {
                            task.error_callback("LSP client is not connected");
                        }
                    }
                }
            } catch (const std::exception& e) {
                LOG_ERROR("LspAsyncManager: Exception in worker thread: " + std::string(e.what()));
                if (task.error_callback) {
                    task.error_callback(e.what());
                }
            } catch (...) {
                LOG_ERROR("LspAsyncManager: Unknown exception in worker thread");
                if (task.error_callback) {
                    task.error_callback("Unknown error occurred");
                }
            }
        }
    }
}

void LspAsyncManager::cancelPendingRequests() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!request_queue_.empty()) {
        request_queue_.pop();
    }
}

void LspAsyncManager::stop() {
    if (running_) {
        running_ = false;
        queue_cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }
}

} // namespace features
} // namespace pnana
