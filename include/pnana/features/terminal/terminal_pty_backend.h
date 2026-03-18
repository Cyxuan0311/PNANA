#ifndef PNANA_FEATURES_TERMINAL_PTY_BACKEND_H
#define PNANA_FEATURES_TERMINAL_PTY_BACKEND_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <thread>

namespace pnana {
namespace features {
namespace terminal {

struct PTYResult;

// 轻量封装：PTY master_fd + pid + 读线程，对上层暴露 write/onRead/resize/terminate
class PTYBackend {
  public:
    using OnReadCallback = std::function<void(const char* data, size_t len)>;
    using OnExitCallback = std::function<void(int exit_code)>;

    // 从 PTYResult 构造（调用方负责 PTYResult 来自 PTYExecutor::createInteractiveShell 等）
    static std::unique_ptr<PTYBackend> create(const PTYResult& result);

    ~PTYBackend();

    PTYBackend(const PTYBackend&) = delete;
    PTYBackend& operator=(const PTYBackend&) = delete;

    // 写入 PTY（线程安全）
    void write(std::string_view data);

    // 设置读取回调（在后台线程中调用）
    void setOnRead(OnReadCallback cb) {
        on_read_ = std::move(cb);
    }

    // 设置进程退出回调
    void setOnExit(OnExitCallback cb) {
        on_exit_ = std::move(cb);
    }

    // 调整终端尺寸
    void resize(int cols, int rows);

    // 终止进程
    void terminate(int signal = 15);

    // 进程是否仍在运行
    bool isRunning() const;

    pid_t getPid() const {
        return pid_;
    }

  private:
    PTYBackend(pid_t pid, int master_fd, int slave_fd);

    void startReadThread();
    void stopReadThread();
    void readLoop();

    pid_t pid_;
    int master_fd_;
    int slave_fd_;
    std::thread read_thread_;
    std::atomic<bool> running_{true};
    mutable std::mutex write_mutex_;
    OnReadCallback on_read_;
    OnExitCallback on_exit_;
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_PTY_BACKEND_H
