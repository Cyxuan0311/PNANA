#ifndef PNANA_FEATURES_TERMINAL_PTY_H
#define PNANA_FEATURES_TERMINAL_PTY_H

#include <string>
#include <sys/types.h>
#include <vector>

namespace pnana {
namespace features {
namespace terminal {

// PTY 执行结果
struct PTYResult {
    pid_t pid;     // 子进程 PID
    int master_fd; // PTY master 文件描述符
    int slave_fd;  // 父进程持有的 slave fd，用于防止子进程退出时 read 返回 EIO
    bool success;  // 是否成功创建
    std::string error; // 错误信息（如果失败）

    PTYResult() : pid(-1), master_fd(-1), slave_fd(-1), success(false) {}
};

// PTY 执行器
class PTYExecutor {
  public:
    // 创建 PTY 并执行命令
    // command: 要执行的命令
    // cwd: 工作目录
    // env: 环境变量（可选，为空则使用当前环境）
    static PTYResult createPTY(const std::string& command, const std::string& cwd,
                               const std::vector<std::string>& env = {});

    // 创建 PTY 并启动交互式 shell ($SHELL -i)，加载用户 .bashrc/.zshrc
    // cwd: 工作目录
    // 返回的 shell 持续运行，提示符由 shell 自身输出
    static PTYResult createInteractiveShell(const std::string& cwd);

    // 从 PTY 读取输出（非阻塞）
    // master_fd: PTY master 文件描述符
    // buffer: 输出缓冲区
    // size: 缓冲区大小
    // 返回：读取的字节数，-1 表示错误，0 表示 EOF
    static ssize_t readOutput(int master_fd, char* buffer, size_t size);

    // 向 PTY 写入输入
    // master_fd: PTY master 文件描述符
    // input: 输入字符串
    // 返回：是否成功
    static bool writeInput(int master_fd, const std::string& input);

    // 向进程发送信号
    // pid: 进程 ID
    // signal: 信号编号（如 SIGINT, SIGTERM）
    // 返回：是否成功
    static bool sendSignal(pid_t pid, int signal);

    // 等待进程结束
    // pid: 进程 ID
    // 返回：进程退出状态码
    static int waitProcess(pid_t pid);

    // 非阻塞检测进程是否已结束，若已结束则回收并返回退出码
    // pid: 进程 ID
    // exit_code: 输出参数，进程结束时的退出码
    // 返回：true 表示进程已结束，false 表示仍在运行
    static bool tryWaitProcess(pid_t pid, int* exit_code);

    // 检查进程是否还在运行
    // pid: 进程 ID
    // 返回：true 如果进程还在运行
    static bool isProcessRunning(pid_t pid);

    // 设置文件描述符为非阻塞模式
    static bool setNonBlocking(int fd);

    // 关闭 PTY
    static void closePTY(int master_fd);

    // 关闭 slave 端（父进程持有的，用于防止子进程退出时 read 返回 EIO）
    static void closeSlave(int slave_fd);

    // 设置 PTY 终端窗口大小（ls 等程序依赖此格式化输出，未设置可能导致无输出）
    static void setTerminalSize(int master_fd, int rows, int cols);

    // 设置 PTY slave 的 termios，确保 Backspace(\x7f) 被正确识别（解决 WSL 等环境继承错误 VERASE
    // 的问题）
    static void setSlaveTermios(int slave_fd);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_PTY_H
