#include "features/ssh/ssh_async_manager.h"
#include "features/ssh/ssh_client.h"
#include <chrono>
#include <condition_variable>
#include <queue>

namespace pnana {
namespace features {
namespace ssh {

// SSHTask 实现
size_t SSHTask::next_id_ = 1;
std::mutex SSHTask::id_mutex_;

SSHTask::SSHTask(SSHTaskType type, const ui::SSHConfig& config, const std::string& param1,
                 const std::string& param2)
    : type_(type), config_(config), param1_(param1), param2_(param2),
      status_(SSHTaskStatus::PENDING), progress_(""), cancelled_(false) {
    std::lock_guard<std::mutex> lock(id_mutex_);
    id_ = next_id_++;
}

SSHTask::~SSHTask() {}

void SSHTask::execute() {
    status_.store(SSHTaskStatus::RUNNING);

    if (cancelled_.load()) {
        status_.store(SSHTaskStatus::CANCELLED);
        return;
    }

    Client ssh_client;
    Result result;

    switch (type_) {
        case SSHTaskType::READ_FILE:
            setProgress("Reading file from " + config_.host + "...");
            result = ssh_client.readFile(config_);
            break;
        case SSHTaskType::WRITE_FILE:
            setProgress("Writing file to " + config_.host + "...");
            result = ssh_client.writeFile(config_, param1_);
            break;
        case SSHTaskType::UPLOAD_FILE:
            setProgress("Uploading file to " + config_.host + "...");
            result = ssh_client.uploadFile(config_, param1_, param2_);
            break;
        case SSHTaskType::DOWNLOAD_FILE:
            setProgress("Downloading file from " + config_.host + "...");
            result = ssh_client.downloadFile(config_, param1_, param2_);
            break;
    }

    if (cancelled_.load()) {
        status_.store(SSHTaskStatus::CANCELLED);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(result_mutex_);
        if (result.success) {
            result_.status = SSHTaskStatus::COMPLETED;
            result_.content = result.content;
        } else {
            result_.status = SSHTaskStatus::FAILED;
            result_.error = result.error;
        }
    }

    status_.store(result_.status);
}

void SSHTask::cancel() {
    cancelled_.store(true);
    if (status_.load() == SSHTaskStatus::PENDING || status_.load() == SSHTaskStatus::RUNNING) {
        status_.store(SSHTaskStatus::CANCELLED);
    }
}

// SSHAsyncManager 实现
SSHAsyncManager::SSHAsyncManager() : should_stop_(false) {
    // 启动工作线程
    for (size_t i = 0; i < MAX_WORKER_THREADS; ++i) {
        worker_threads_.emplace_back(&SSHAsyncManager::workerThread, this);
    }
}

SSHAsyncManager::~SSHAsyncManager() {
    should_stop_.store(true);
    queue_cv_.notify_all();

    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

size_t SSHAsyncManager::submitTask(std::shared_ptr<SSHTask> task) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    size_t task_id = task->getId();
    tasks_[task_id] = task;

    {
        std::lock_guard<std::mutex> queue_lock(queue_mutex_);
        task_queue_.push(task);
    }
    queue_cv_.notify_one();

    return task_id;
}

SSHTaskStatus SSHAsyncManager::getTaskStatus(size_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        return it->second->getStatus();
    }
    return SSHTaskStatus::FAILED;
}

SSHTaskResult SSHAsyncManager::getTaskResult(size_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        return it->second->getResult();
    }
    SSHTaskResult result;
    result.status = SSHTaskStatus::FAILED;
    result.error = "Task not found";
    return result;
}

bool SSHAsyncManager::cancelTask(size_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second->cancel();
        return true;
    }
    return false;
}

bool SSHAsyncManager::waitForTask(size_t task_id, int timeout_ms) {
    auto start = std::chrono::steady_clock::now();

    while (true) {
        SSHTaskStatus status = getTaskStatus(task_id);
        if (status == SSHTaskStatus::COMPLETED || status == SSHTaskStatus::FAILED ||
            status == SSHTaskStatus::CANCELLED) {
            return true;
        }

        if (timeout_ms > 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed >= timeout_ms) {
                return false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SSHAsyncManager::cleanupCompletedTasks() {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    auto it = tasks_.begin();
    while (it != tasks_.end()) {
        SSHTaskStatus status = it->second->getStatus();
        if (status == SSHTaskStatus::COMPLETED || status == SSHTaskStatus::FAILED ||
            status == SSHTaskStatus::CANCELLED) {
            it = tasks_.erase(it);
        } else {
            ++it;
        }
    }
}

void SSHAsyncManager::workerThread() {
    while (!should_stop_.load()) {
        std::shared_ptr<SSHTask> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return !task_queue_.empty() || should_stop_.load();
            });

            if (should_stop_.load() && task_queue_.empty()) {
                break;
            }

            if (!task_queue_.empty()) {
                task = task_queue_.front();
                task_queue_.pop();
            }
        }

        if (task) {
            task->execute();
        }
    }
}

} // namespace ssh
} // namespace features
} // namespace pnana
