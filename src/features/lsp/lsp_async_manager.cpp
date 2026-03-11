#include "features/lsp/lsp_async_manager.h"
#include "features/lsp/lsp_client.h"
#include "utils/logger.h"
#include <chrono>
#include <future>
#include <stdexcept>

namespace pnana {
namespace features {

LspAsyncManager::LspAsyncManager() : running_(true) {
    // 创建多个worker线程以支持并发请求处理
    unsigned int num_threads = std::max(2u, std::thread::hardware_concurrency() / 2);

    for (unsigned int i = 0; i < num_threads; ++i) {
        worker_threads_.emplace_back(&LspAsyncManager::workerThread, this);
    }

    const char* env = std::getenv("PNANA_PERF_JSON");
    json_perf_enabled_ = (env && std::string(env) == "1");
}

LspAsyncManager::~LspAsyncManager() {
    stop();
}

void LspAsyncManager::requestCompletionAsync(LspClient* client, const std::string& uri,
                                             const LspPosition& position,
                                             CompletionCallback on_success, ErrorCallback on_error,
                                             const std::string& trigger_character,
                                             int completion_timeout_ms) {
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
    task.trigger_character = trigger_character;
    task.completion_timeout_ms = completion_timeout_ms;
    task.completion_callback = on_success;
    task.error_callback = on_error;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        request_queue_.push(task);
        // 移除所有调试日志以提高性能
    }
    // 通知所有等待的线程，提高并发响应速度
    queue_cv_.notify_all();
}

void LspAsyncManager::requestDocumentOpenAsync(LspClient* client, const std::string& uri,
                                               const std::string& language_id,
                                               const std::string& content) {
    if (!client || !running_)
        return;
    RequestTask task;
    task.type = RequestTask::DOCUMENT_OPEN;
    task.client = client;
    task.uri = uri;
    task.doc_language_id = language_id;
    task.doc_content = content;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        request_queue_.push(task);
    }
    queue_cv_.notify_all();
}

void LspAsyncManager::requestDocumentChangeAsync(LspClient* client, const std::string& uri,
                                                 const std::string& content, int version) {
    if (!client || !running_)
        return;
    RequestTask task;
    task.type = RequestTask::DOCUMENT_CHANGE;
    task.client = client;
    task.uri = uri;
    task.doc_content = content;
    task.doc_version = version;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        request_queue_.push(task);
    }
    queue_cv_.notify_all();
}

void LspAsyncManager::requestResolveAsync(LspClient* client, const CompletionItem& item,
                                          ResolveCallback on_success, ErrorCallback on_error) {
    if (!client || !running_) {
        if (on_error) {
            on_error("Client is null or manager is stopped");
        }
        return;
    }

    RequestTask task;
    task.type = RequestTask::RESOLVE;
    task.client = client;
    task.resolve_item = item;
    task.resolve_callback = on_success;
    task.error_callback = on_error;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        request_queue_.push(task);
    }
    queue_cv_.notify_all();
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
                            return task.client->completion(task.uri, task.position,
                                                           task.trigger_character);
                        });

                        // 超时避免 UI 卡顿，cpp/c 用 500ms，其他语言放宽至 800ms
                        int timeout_ms =
                            task.completion_timeout_ms > 0 ? task.completion_timeout_ms : 500;
                        if (completion_future.wait_for(std::chrono::milliseconds(timeout_ms)) ==
                            std::future_status::timeout) {
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
                } else if (task.type == RequestTask::RESOLVE) {
                    if (task.client && task.client->isConnected()) {
                        // 带超时的异步调用，避免慢 LSP（pylsp/typescript）的 resolve 阻塞 worker
                        auto resolve_future = std::async(std::launch::async, [&]() {
                            return task.client->resolveCompletionItem(task.resolve_item);
                        });
                        constexpr auto resolve_timeout = std::chrono::milliseconds(600);
                        if (resolve_future.wait_for(resolve_timeout) ==
                            std::future_status::timeout) {
                            if (task.resolve_callback) {
                                task.resolve_callback(task.resolve_item);
                            }
                        } else {
                            auto resolved = resolve_future.get();
                            if (task.resolve_callback) {
                                task.resolve_callback(resolved);
                            }
                        }
                    } else if (task.error_callback) {
                        task.error_callback("LSP client is not connected");
                    }
                } else if (task.type == RequestTask::DOCUMENT_OPEN) {
                    if (task.client && task.client->isConnected()) {
                        try {
                            task.client->didOpen(task.uri, task.doc_language_id, task.doc_content);
                        } catch (...) {
                        }
                    }
                } else if (task.type == RequestTask::DOCUMENT_CHANGE) {
                    if (task.client && task.client->isConnected()) {
                        try {
                            task.client->didChange(task.uri, task.doc_content, task.doc_version);
                        } catch (...) {
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

        // 等待所有worker线程结束
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        worker_threads_.clear();
    }
}

} // namespace features
} // namespace pnana
