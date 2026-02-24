#include "features/terminal/terminal_job.h"
#include "features/terminal/terminal_pty.h"
#include <algorithm>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

namespace pnana {
namespace features {
namespace terminal {

// 静态成员初始化
std::vector<Job> JobManager::jobs_;
std::mutex JobManager::jobs_mutex_;
int JobManager::next_job_id_ = 1;
int JobManager::foreground_job_id_ = -1;

int JobManager::addJob(pid_t pid, pid_t pgid, const std::string& command, int master_fd) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    Job job;
    job.job_id = next_job_id_++;
    job.pid = pid;
    job.pgid = pgid;
    job.command = command;
    job.state = JobState::Running;
    job.start_time = std::chrono::steady_clock::now();
    job.master_fd = master_fd;

    jobs_.push_back(job);

    // 如果是前台作业，设置为前台
    foreground_job_id_ = job.job_id;

    return job.job_id;
}

Job* JobManager::findJob(int job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    for (auto& job : jobs_) {
        if (job.job_id == job_id) {
            return &job;
        }
    }

    return nullptr;
}

Job* JobManager::findJobByPid(pid_t pid) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    for (auto& job : jobs_) {
        if (job.pid == pid) {
            return &job;
        }
    }

    return nullptr;
}

std::vector<Job> JobManager::listJobs() {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    return jobs_;
}

bool JobManager::bringToForeground(int job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = std::find_if(jobs_.begin(), jobs_.end(), [job_id](const Job& j) {
        return j.job_id == job_id;
    });

    if (it == jobs_.end()) {
        return false;
    }

    Job& job = *it;

    if (job.state == JobState::Stopped) {
        // 恢复暂停的进程
        if (kill(-job.pgid, SIGCONT) != 0) {
            return false;
        }
        job.state = JobState::Running;
    }

    // 将进程组带到前台
    if (tcsetpgrp(STDIN_FILENO, job.pgid) != 0) {
        return false;
    }

    foreground_job_id_ = job_id;
    return true;
}

bool JobManager::bringToBackground(int job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = std::find_if(jobs_.begin(), jobs_.end(), [job_id](const Job& j) {
        return j.job_id == job_id;
    });

    if (it == jobs_.end()) {
        return false;
    }

    Job& job = *it;

    if (job.state == JobState::Running) {
        // 暂停进程
        if (kill(-job.pgid, SIGTSTP) != 0) {
            return false;
        }
        job.state = JobState::Stopped;
    }

    foreground_job_id_ = -1;
    return true;
}

bool JobManager::killJob(int job_id, int signal) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = std::find_if(jobs_.begin(), jobs_.end(), [job_id](const Job& j) {
        return j.job_id == job_id;
    });

    if (it == jobs_.end()) {
        return false;
    }

    Job& job = *it;

    // 发送信号
    if (kill(-job.pgid, signal) != 0) {
        return false;
    }

    if (signal == SIGKILL || signal == SIGTERM) {
        job.state = JobState::Terminated;
    }

    // 关闭 PTY
    if (job.master_fd >= 0) {
        PTYExecutor::closePTY(job.master_fd);
        job.master_fd = -1;
    }

    return true;
}

void JobManager::updateJobState(int job_id, JobState state) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = std::find_if(jobs_.begin(), jobs_.end(), [job_id](const Job& j) {
        return j.job_id == job_id;
    });

    if (it != jobs_.end()) {
        it->state = state;
    }
}

void JobManager::updateJobExitCode(int job_id, int exit_code) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = std::find_if(jobs_.begin(), jobs_.end(), [job_id](const Job& j) {
        return j.job_id == job_id;
    });

    if (it != jobs_.end()) {
        it->exit_code = exit_code;
        it->state = JobState::Done;
    }
}

void JobManager::removeJob(int job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = std::remove_if(jobs_.begin(), jobs_.end(), [job_id](const Job& j) {
        return j.job_id == job_id;
    });

    if (it != jobs_.end()) {
        // 关闭 PTY
        if (it->master_fd >= 0) {
            PTYExecutor::closePTY(it->master_fd);
        }
        jobs_.erase(it, jobs_.end());
    }

    if (foreground_job_id_ == job_id) {
        foreground_job_id_ = -1;
    }
}

void JobManager::cleanupFinishedJobs() {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    auto it = std::remove_if(jobs_.begin(), jobs_.end(), [](const Job& job) {
        if (job.state == JobState::Done || job.state == JobState::Terminated) {
            // 关闭 PTY
            if (job.master_fd >= 0) {
                PTYExecutor::closePTY(job.master_fd);
            }
            return true;
        }
        return false;
    });

    jobs_.erase(it, jobs_.end());
}

Job* JobManager::getForegroundJob() {
    std::lock_guard<std::mutex> lock(jobs_mutex_);

    if (foreground_job_id_ < 0) {
        return nullptr;
    }

    return findJob(foreground_job_id_);
}

void JobManager::setForegroundJob(int job_id) {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    foreground_job_id_ = job_id;
}

int JobManager::getNextJobId() {
    std::lock_guard<std::mutex> lock(jobs_mutex_);
    return next_job_id_;
}

} // namespace terminal
} // namespace features
} // namespace pnana
