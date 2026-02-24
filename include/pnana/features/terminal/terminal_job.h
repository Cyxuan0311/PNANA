#ifndef PNANA_FEATURES_TERMINAL_JOB_H
#define PNANA_FEATURES_TERMINAL_JOB_H

#include <chrono>
#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

namespace pnana {
namespace features {
namespace terminal {

// 作业状态
enum class JobState {
    Running,   // 正在运行
    Stopped,   // 已暂停
    Done,      // 已完成
    Terminated // 已终止
};

// 作业信息
struct Job {
    int job_id;                                       // 作业 ID
    pid_t pid;                                        // 进程 ID
    pid_t pgid;                                       // 进程组 ID
    std::string command;                              // 命令字符串
    JobState state;                                   // 作业状态
    std::chrono::steady_clock::time_point start_time; // 开始时间
    int exit_code;                                    // 退出码（如果已完成）
    int master_fd; // PTY master 文件描述符（-1 表示没有 PTY）

    Job() : job_id(0), pid(-1), pgid(-1), state(JobState::Running), exit_code(-1), master_fd(-1) {}
};

// 作业管理器
class JobManager {
  public:
    // 添加新作业
    // pid: 进程 ID
    // pgid: 进程组 ID
    // command: 命令字符串
    // master_fd: PTY master 文件描述符（可选）
    // 返回：作业 ID
    static int addJob(pid_t pid, pid_t pgid, const std::string& command, int master_fd = -1);

    // 根据作业 ID 查找作业
    static Job* findJob(int job_id);

    // 根据 PID 查找作业
    static Job* findJobByPid(pid_t pid);

    // 列出所有作业
    static std::vector<Job> listJobs();

    // 将作业带到前台
    // job_id: 作业 ID
    // 返回：是否成功
    static bool bringToForeground(int job_id);

    // 将作业放到后台
    // job_id: 作业 ID
    // 返回：是否成功
    static bool bringToBackground(int job_id);

    // 终止作业
    // job_id: 作业 ID
    // signal: 信号编号（默认 SIGTERM）
    // 返回：是否成功
    static bool killJob(int job_id, int signal = 15);

    // 更新作业状态
    // job_id: 作业 ID
    // state: 新状态
    static void updateJobState(int job_id, JobState state);

    // 更新作业退出码
    // job_id: 作业 ID
    // exit_code: 退出码
    static void updateJobExitCode(int job_id, int exit_code);

    // 移除已完成的作业
    // job_id: 作业 ID
    static void removeJob(int job_id);

    // 清理所有已完成的作业
    static void cleanupFinishedJobs();

    // 获取前台作业
    static Job* getForegroundJob();

    // 设置前台作业
    static void setForegroundJob(int job_id);

    // 获取下一个作业 ID
    static int getNextJobId();

  private:
    static std::vector<Job> jobs_;
    static std::mutex jobs_mutex_;
    static int next_job_id_;
    static int foreground_job_id_;
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_JOB_H
