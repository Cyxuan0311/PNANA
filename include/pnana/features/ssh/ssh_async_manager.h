#ifndef PNANA_FEATURES_SSH_SSH_ASYNC_MANAGER_H
#define PNANA_FEATURES_SSH_SSH_ASYNC_MANAGER_H

#include "features/ssh/ssh_client.h"
#include "ui/ssh_dialog.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace pnana {
namespace features {
namespace ssh {

// SSH异步操作类型
enum class SSHTaskType { READ_FILE, WRITE_FILE, UPLOAD_FILE, DOWNLOAD_FILE };

// SSH异步任务状态
enum class SSHTaskStatus { PENDING, RUNNING, COMPLETED, FAILED, CANCELLED };

// SSH异步任务结果
struct SSHTaskResult {
    SSHTaskStatus status;
    std::string content;
    std::string error;

    SSHTaskResult() : status(SSHTaskStatus::PENDING), content(""), error("") {}
};

// SSH异步任务
class SSHTask {
  public:
    SSHTask(SSHTaskType type, const ui::SSHConfig& config, const std::string& param1 = "",
            const std::string& param2 = "");
    ~SSHTask();

    // 执行任务
    void execute();

    // 取消任务
    void cancel();

    // 获取任务状态
    SSHTaskStatus getStatus() const {
        return status_.load();
    }

    // 获取任务结果
    SSHTaskResult getResult() const {
        std::lock_guard<std::mutex> lock(result_mutex_);
        return result_;
    }

    // 获取任务ID
    size_t getId() const {
        return id_;
    }

    // 获取进度信息（可选）
    std::string getProgress() const {
        std::lock_guard<std::mutex> lock(result_mutex_);
        return progress_;
    }

    // 设置进度信息
    void setProgress(const std::string& progress) {
        std::lock_guard<std::mutex> lock(result_mutex_);
        progress_ = progress;
    }

  private:
    size_t id_;
    SSHTaskType type_;
    ui::SSHConfig config_;
    std::string param1_; // 用于writeFile的content，或upload/download的路径
    std::string param2_; // 用于upload/download的第二个路径

    std::atomic<SSHTaskStatus> status_;
    mutable std::mutex result_mutex_;
    SSHTaskResult result_;
    std::string progress_;

    std::atomic<bool> cancelled_;

    static size_t next_id_;
    static std::mutex id_mutex_;
};

// SSH异步操作管理器
class SSHAsyncManager {
  public:
    SSHAsyncManager();
    ~SSHAsyncManager();

    // 提交异步任务
    size_t submitTask(std::shared_ptr<SSHTask> task);

    // 获取任务状态
    SSHTaskStatus getTaskStatus(size_t task_id);

    // 获取任务结果
    SSHTaskResult getTaskResult(size_t task_id);

    // 取消任务
    bool cancelTask(size_t task_id);

    // 等待任务完成
    bool waitForTask(size_t task_id, int timeout_ms = -1);

    // 清理已完成的任务
    void cleanupCompletedTasks();

  private:
    std::map<size_t, std::shared_ptr<SSHTask>> tasks_;
    std::mutex tasks_mutex_;

    // 工作线程
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> should_stop_;

    // 任务队列
    std::queue<std::shared_ptr<SSHTask>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // 工作线程函数
    void workerThread();

    static constexpr size_t MAX_WORKER_THREADS = 2;
};

} // namespace ssh
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SSH_SSH_ASYNC_MANAGER_H
