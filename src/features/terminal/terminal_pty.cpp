#include "features/terminal/terminal_pty.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#ifdef __linux__
#include <pty.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <util.h>
#else
// 对于其他平台，使用 posix_openpt
#include <stdlib.h>
#endif

namespace pnana {
namespace features {
namespace terminal {

PTYResult PTYExecutor::createPTY(const std::string& command, const std::string& cwd,
                                 const std::vector<std::string>& env) {
    PTYResult result;

#ifdef __linux__
    // Linux: 使用 forkpty
    int master_fd = -1;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);

    if (pid < 0) {
        result.error = "forkpty failed: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        // 子进程
        // 切换到工作目录
        if (chdir(cwd.c_str()) != 0) {
            _exit(1);
        }

        // 设置环境变量（如果有）
        if (!env.empty()) {
            for (const auto& e : env) {
                putenv(const_cast<char*>(e.c_str()));
            }
        }

        // 执行命令
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(1);
    }

    // 父进程：打开 slave 端并保持打开，防止子进程退出时 slave 关闭导致 read 返回 EIO
    result.pid = pid;
    result.master_fd = master_fd;
    result.slave_fd = -1;
    if (master_fd >= 0) {
        const char* slave_name = ptsname(master_fd);
        if (slave_name) {
            result.slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
        }
    }
    result.success = true;

#elif defined(__APPLE__) || defined(__FreeBSD__)
    // macOS/FreeBSD: 使用 forkpty
    int master_fd = -1;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);

    if (pid < 0) {
        result.error = "forkpty failed: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        // 子进程
        if (chdir(cwd.c_str()) != 0) {
            _exit(1);
        }

        if (!env.empty()) {
            for (const auto& e : env) {
                putenv(const_cast<char*>(e.c_str()));
            }
        }

        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(1);
    }

    result.pid = pid;
    result.master_fd = master_fd;
    result.slave_fd = -1;
    if (master_fd >= 0) {
        const char* slave_name = ptsname(master_fd);
        if (slave_name) {
            result.slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
        }
    }
    result.success = true;

#else
    // 其他平台: 使用 posix_openpt
    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0) {
        result.error = "posix_openpt failed: " + std::string(strerror(errno));
        return result;
    }

    if (grantpt(master_fd) != 0) {
        close(master_fd);
        result.error = "grantpt failed: " + std::string(strerror(errno));
        return result;
    }

    if (unlockpt(master_fd) != 0) {
        close(master_fd);
        result.error = "unlockpt failed: " + std::string(strerror(errno));
        return result;
    }

    char* slave_name = ptsname(master_fd);
    if (!slave_name) {
        close(master_fd);
        result.error = "ptsname failed: " + std::string(strerror(errno));
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(master_fd);
        result.error = "fork failed: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        // 子进程
        close(master_fd);

        // 打开 slave PTY
        int slave_fd = open(slave_name, O_RDWR);
        if (slave_fd < 0) {
            _exit(1);
        }

        // 设置终端属性
        setsid();
        ioctl(slave_fd, TIOCSCTTY, nullptr);

        // 重定向标准输入输出错误
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);

        if (slave_fd > 2) {
            close(slave_fd);
        }

        // 切换到工作目录
        if (chdir(cwd.c_str()) != 0) {
            _exit(1);
        }

        // 设置环境变量
        if (!env.empty()) {
            for (const auto& e : env) {
                putenv(const_cast<char*>(e.c_str()));
            }
        }

        // 执行命令
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(1);
    }

    // 父进程：打开 slave 端并保持打开
    result.pid = pid;
    result.master_fd = master_fd;
    result.slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
    result.success = true;
#endif

    // 设置非阻塞模式
    if (result.success) {
        setNonBlocking(result.master_fd);
    }

    return result;
}

static std::string getShellPath() {
    const char* shell = getenv("SHELL");
    if (shell && shell[0] != '\0') {
        return shell;
    }
    return "/bin/bash";
}

static std::string getShellBasename(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos && pos + 1 < path.size()) {
        return path.substr(pos + 1);
    }
    return path.empty() ? "sh" : path;
}

PTYResult PTYExecutor::createInteractiveShell(const std::string& cwd) {
    PTYResult result;
    std::string shell_path = getShellPath();
    std::string shell_name = getShellBasename(shell_path);

#ifdef __linux__
    int master_fd = -1;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);

    if (pid < 0) {
        result.error = "forkpty failed: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        // 在子进程启动 shell 前设置 termios，VERASE=\x08 与发送的 Backspace 字节一致
        struct termios tio;
        if (tcgetattr(STDIN_FILENO, &tio) == 0) {
            tio.c_cc[VERASE] = '\x08';
#ifdef VKILL
            tio.c_cc[VKILL] = '\x15';
#endif
            tcsetattr(STDIN_FILENO, TCSANOW, &tio);
        }
        setenv("TERM", "xterm", 1);
        if (chdir(cwd.c_str()) != 0) {
            _exit(1);
        }
        // --noediting: 禁用 readline，由内核 canonical 模式处理 Backspace（更可靠）
        execl(shell_path.c_str(), shell_name.c_str(), "-i", nullptr);
        _exit(1);
    }

    result.pid = pid;
    result.master_fd = master_fd;
    result.slave_fd = -1;
    if (master_fd >= 0) {
        const char* slave_name = ptsname(master_fd);
        if (slave_name) {
            result.slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
        }
    }
    result.success = true;

#elif defined(__APPLE__) || defined(__FreeBSD__)
    int master_fd = -1;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);

    if (pid < 0) {
        result.error = "forkpty failed: " + std::string(strerror(errno));
        return result;
    }

    if (pid == 0) {
        struct termios tio;
        if (tcgetattr(STDIN_FILENO, &tio) == 0) {
            tio.c_cc[VERASE] = '\x08';
#ifdef VKILL
            tio.c_cc[VKILL] = '\x15';
#endif
            tcsetattr(STDIN_FILENO, TCSANOW, &tio);
        }
        setenv("TERM", "xterm", 1);
        if (chdir(cwd.c_str()) != 0) {
            _exit(1);
        }
        execl(shell_path.c_str(), shell_name.c_str(), "-i", nullptr);
        _exit(1);
    }

    result.pid = pid;
    result.master_fd = master_fd;
    result.slave_fd = -1;
    if (master_fd >= 0) {
        const char* slave_name = ptsname(master_fd);
        if (slave_name) {
            result.slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
        }
    }
    result.success = true;

#else
    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0) {
        result.error = "posix_openpt failed: " + std::string(strerror(errno));
        return result;
    }
    if (grantpt(master_fd) != 0) {
        close(master_fd);
        result.error = "grantpt failed: " + std::string(strerror(errno));
        return result;
    }
    if (unlockpt(master_fd) != 0) {
        close(master_fd);
        result.error = "unlockpt failed: " + std::string(strerror(errno));
        return result;
    }
    char* slave_name = ptsname(master_fd);
    if (!slave_name) {
        close(master_fd);
        result.error = "ptsname failed: " + std::string(strerror(errno));
        return result;
    }
    pid_t pid = fork();
    if (pid < 0) {
        close(master_fd);
        result.error = "fork failed: " + std::string(strerror(errno));
        return result;
    }
    if (pid == 0) {
        close(master_fd);
        int slave_fd = open(slave_name, O_RDWR);
        if (slave_fd < 0) {
            _exit(1);
        }
        setsid();
        ioctl(slave_fd, TIOCSCTTY, nullptr);
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        if (slave_fd > 2) {
            close(slave_fd);
        }
        struct termios tio;
        if (tcgetattr(STDIN_FILENO, &tio) == 0) {
            tio.c_cc[VERASE] = '\x08';
#ifdef VKILL
            tio.c_cc[VKILL] = '\x15';
#endif
            tcsetattr(STDIN_FILENO, TCSANOW, &tio);
        }
        setenv("TERM", "xterm", 1);
        if (chdir(cwd.c_str()) != 0) {
            _exit(1);
        }
        execl(shell_path.c_str(), shell_name.c_str(), "-i", nullptr);
        _exit(1);
    }
    result.pid = pid;
    result.master_fd = master_fd;
    result.slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
    result.success = true;
#endif

    if (result.success) {
        setNonBlocking(result.master_fd);
        setTerminalSize(result.master_fd, 24, 80);
        if (result.slave_fd >= 0) {
            setSlaveTermios(result.slave_fd); // VERASE=\x08 与发送的 Backspace 一致
            tcsetpgrp(result.slave_fd, result.pid);
        }
    }
    return result;
}

void PTYExecutor::setSlaveTermios(int slave_fd) {
    if (slave_fd < 0)
        return;
    struct termios tio;
    if (tcgetattr(slave_fd, &tio) != 0)
        return;
    tio.c_cc[VERASE] = '\x08'; // BS，WSL 下 readline 常用 \x08 作为删除键
#ifdef VKILL
    tio.c_cc[VKILL] = '\x15'; // Ctrl+U
#endif
    tcsetattr(slave_fd, TCSANOW, &tio);
}

void PTYExecutor::setTerminalSize(int master_fd, int rows, int cols) {
    if (master_fd < 0)
        return;
#ifdef TIOCSWINSZ
    struct winsize ws = {};
    ws.ws_row = static_cast<unsigned short>(rows > 0 ? rows : 24);
    ws.ws_col = static_cast<unsigned short>(cols > 0 ? cols : 80);
    ws.ws_xpixel = ws.ws_ypixel = 0;
    ioctl(master_fd, TIOCSWINSZ, &ws);
#endif
}

ssize_t PTYExecutor::readOutput(int master_fd, char* buffer, size_t size) {
    if (master_fd < 0) {
        return -1;
    }

    ssize_t n = read(master_fd, buffer, size);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 非阻塞读取，没有数据可读
            return 0;
        }
        return -1;
    }
    return n;
}

bool PTYExecutor::writeInput(int master_fd, const std::string& input) {
    if (master_fd < 0) {
        return false;
    }

    ssize_t n = write(master_fd, input.c_str(), input.length());
    return n == static_cast<ssize_t>(input.length());
}

bool PTYExecutor::sendSignal(pid_t pid, int signal) {
    if (pid <= 0) {
        return false;
    }

    // 检查进程是否存在
    if (kill(pid, 0) != 0) {
        return false;
    }

    return kill(pid, signal) == 0;
}

int PTYExecutor::waitProcess(pid_t pid) {
    if (pid <= 0) {
        return -1;
    }

    int status = 0;
    pid_t waited = waitpid(pid, &status, 0);
    if (waited < 0) {
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return -1;
}

bool PTYExecutor::tryWaitProcess(pid_t pid, int* exit_code) {
    if (pid <= 0 || exit_code == nullptr) {
        return false;
    }
    int status = 0;
    pid_t waited = waitpid(pid, &status, WNOHANG);
    if (waited == pid) {
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = -1;
        }
        return true;
    }
    return false;
}

bool PTYExecutor::isProcessRunning(pid_t pid) {
    if (pid <= 0) {
        return false;
    }

    // 发送信号 0 来检查进程是否存在
    return kill(pid, 0) == 0;
}

bool PTYExecutor::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }

    flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags) == 0;
}

void PTYExecutor::closePTY(int master_fd) {
    if (master_fd >= 0) {
        close(master_fd);
    }
}

void PTYExecutor::closeSlave(int slave_fd) {
    if (slave_fd >= 0) {
        close(slave_fd);
    }
}

} // namespace terminal
} // namespace features
} // namespace pnana
