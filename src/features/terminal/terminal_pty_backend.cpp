#include "features/terminal/terminal_pty_backend.h"
#include "features/terminal/terminal_pty.h"
#include "utils/logger.h"
#include <poll.h>
#include <signal.h>
#include <unistd.h>

namespace pnana {
namespace features {
namespace terminal {

std::unique_ptr<PTYBackend> PTYBackend::create(const PTYResult& result) {
    if (!result.success || result.master_fd < 0)
        return nullptr;
    return std::unique_ptr<PTYBackend>(
        new PTYBackend(result.pid, result.master_fd, result.slave_fd));
}

PTYBackend::PTYBackend(pid_t pid, int master_fd, int slave_fd)
    : pid_(pid), master_fd_(master_fd), slave_fd_(slave_fd) {
    startReadThread();
}

PTYBackend::~PTYBackend() {
    running_ = false;
    stopReadThread();
    if (master_fd_ >= 0) {
        PTYExecutor::closePTY(master_fd_);
        master_fd_ = -1;
    }
    if (slave_fd_ >= 0) {
        PTYExecutor::closeSlave(slave_fd_);
        slave_fd_ = -1;
    }
}

void PTYBackend::write(std::string_view data) {
    if (master_fd_ < 0)
        return;
    std::lock_guard<std::mutex> lock(write_mutex_);
    PTYExecutor::writeInput(master_fd_, std::string(data));
}

void PTYBackend::resize(int cols, int rows) {
    if (master_fd_ >= 0)
        PTYExecutor::setTerminalSize(master_fd_, rows, cols);
}

void PTYBackend::terminate(int signal) {
    if (pid_ > 0)
        PTYExecutor::sendSignal(pid_, signal);
}

bool PTYBackend::isRunning() const {
    return pid_ > 0 && PTYExecutor::isProcessRunning(pid_);
}

void PTYBackend::startReadThread() {
    read_thread_ = std::thread([this]() {
        readLoop();
    });
}

void PTYBackend::stopReadThread() {
    if (read_thread_.joinable())
        read_thread_.join();
}

void PTYBackend::readLoop() {
    LOG("[PTYBackend] readLoop started");
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];

    while (running_ && master_fd_ >= 0) {
        struct pollfd pfd = {};
        pfd.fd = master_fd_;
        pfd.events = POLLIN;
        int ret = poll(&pfd, 1, 16);

        if (ret > 0 && (pfd.revents & (POLLHUP | POLLERR | POLLNVAL))) {
            LOG("[PTYBackend] poll hangup/error");
            break;
        }

        if (ret > 0 && (pfd.revents & POLLIN)) {
            ssize_t n = PTYExecutor::readOutput(master_fd_, buffer, BUFFER_SIZE);
            if (n > 0 && on_read_) {
                on_read_(buffer, static_cast<size_t>(n));
            } else if (n == 0) {
                // EOF
                break;
            } else {
                LOG("[PTYBackend] readOutput error");
                break;
            }
        } else if (ret < 0) {
            break;
        }

        if (pid_ > 0 && !PTYExecutor::isProcessRunning(pid_)) {
            int exit_code = -1;
            PTYExecutor::tryWaitProcess(pid_, &exit_code);
            if (on_exit_)
                on_exit_(exit_code);
            break;
        }
    }
    running_ = false;
}

} // namespace terminal
} // namespace features
} // namespace pnana
