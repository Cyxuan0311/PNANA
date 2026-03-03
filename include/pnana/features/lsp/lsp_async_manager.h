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
    using ResolveCallback = std::function<void(CompletionItem)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    LspAsyncManager();
    ~LspAsyncManager();

    // 禁用拷贝构造和赋值
    LspAsyncManager(const LspAsyncManager&) = delete;
    LspAsyncManager& operator=(const LspAsyncManager&) = delete;

    // 异步补全请求（trigger_character: . : -> 等，用于成员访问等智能补全）
    // completion_timeout_ms: 超时毫秒数，非 C/C++ LSP 可传 800 以放宽
    void requestCompletionAsync(LspClient* client, const std::string& uri,
                                const LspPosition& position, CompletionCallback on_success,
                                ErrorCallback on_error = nullptr,
                                const std::string& trigger_character = "",
                                int completion_timeout_ms = 500);

    // 异步 resolve 补全项（获取 detail/documentation）
    void requestResolveAsync(LspClient* client, const CompletionItem& item,
                             ResolveCallback on_success, ErrorCallback on_error = nullptr);

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
        enum Type { COMPLETION, RESOLVE, HOVER, DEFINITION };
        Type type;
        LspClient* client;
        std::string uri;
        LspPosition position;
        std::string trigger_character;
        int completion_timeout_ms = 500;
        CompletionItem resolve_item;
        CompletionCallback completion_callback;
        ResolveCallback resolve_callback;
        ErrorCallback error_callback;
    };

    void workerThread();

    std::vector<std::thread> worker_threads_;
    std::queue<RequestTask> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::atomic<bool> running_;
    bool json_perf_enabled_;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_LSP_ASYNC_MANAGER_H
