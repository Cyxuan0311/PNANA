#ifndef PNANA_FEATURES_LSP_LSP_ASYNC_MANAGER_H
#define PNANA_FEATURES_LSP_LSP_ASYNC_MANAGER_H

#include "features/lsp/lsp_client.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace pnana {
namespace features {

class LspAsyncManager {
  public:
    using CompletionCallback = std::function<void(std::vector<CompletionItem>)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    LspAsyncManager();
    ~LspAsyncManager();

    // 禁用拷贝构造和赋值
    LspAsyncManager(const LspAsyncManager&) = delete;
    LspAsyncManager& operator=(const LspAsyncManager&) = delete;

    // 异步补全请求
    void requestCompletionAsync(LspClient* client, const std::string& uri,
                                const LspPosition& position, CompletionCallback on_success,
                                ErrorCallback on_error = nullptr);

    // 取消所有待处理的请求
    void cancelPendingRequests();

    // 停止工作线程
    void stop();

    // 检查是否正在运行
    bool isRunning() const {
        return running_;
    }

  private:
    struct RequestTask {
        enum Type { COMPLETION, HOVER, DEFINITION };
        Type type;
        LspClient* client;
        std::string uri;
        LspPosition position;
        CompletionCallback completion_callback;
        ErrorCallback error_callback;
    };

    void workerThread();

    std::thread worker_thread_;
    std::queue<RequestTask> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> running_;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_LSP_ASYNC_MANAGER_H
