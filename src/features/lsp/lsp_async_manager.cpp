#include "features/lsp/lsp_async_manager.h"
#include "features/lsp/lsp_client.h"
#include "utils/logger.h"
#include <chrono>
#include <future>
#include <stdexcept>

namespace pnana {
namespace features {

LspAsyncManager::LspAsyncManager() : running_(true) {
    worker_thread_ = std::thread(&LspAsyncManager::workerThread, this);
    const char* env = std::getenv("PNANA_PERF_JSON");
    json_perf_enabled_ = (env && std::string(env) == "1");
    // 移除性能日志以提高性能
}

LspAsyncManager::~LspAsyncManager() {
    stop();
}

void LspAsyncManager::requestCompletionAsync(LspClient* client, const std::string& uri,
                                             const LspPosition& position,
                                             CompletionCallback on_success,
                                             ErrorCallback on_error) {
    if (!client || !running_) {
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
        // 移除所有调试日志以提高性能
    }
    queue_cv_.notify_one();
    (void)0;
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
                    if (task.client && task.client->isConnected()) {
                        // 使用带超时的异步调用，避免长时间阻塞
                        auto completion_future = std::async(std::launch::async, [&]() {
                            return task.client->completion(task.uri, task.position);
                        });

                        // 等待最多500ms，避免UI卡顿但允许合理的响应时间
                        if (completion_future.wait_for(std::chrono::milliseconds(500)) ==
                            std::future_status::timeout) {
                            LOG_WARNING("[ASYNC] Completion timeout for " + task.uri +
                                        " after 500ms");
                            if (task.error_callback) {
                                task.error_callback("Completion request timeout");
                            }
                        } else {
                            auto items = completion_future.get();
                            if (task.completion_callback) {
                                task.completion_callback(items);
                            }
                        }
                    } else {
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
