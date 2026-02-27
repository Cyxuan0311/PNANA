#include "features/terminal/terminal_builtin.h"
#include "features/terminal.h"
#include "features/terminal/terminal_job.h"
#include "features/terminal/terminal_utils.h"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <signal.h>
#include <sstream>

namespace fs = std::filesystem;

namespace pnana {
namespace features {
namespace terminal {

bool BuiltinCommandExecutor::isBuiltin(const std::string& command) {
    return (command == "help" || command == "h" || command == "clear" || command == "cls" ||
            command == "pwd" || command == "cd" || command == "ls" || command == "cat" ||
            command == "echo" || command == "whoami" || command == "hostname" ||
            command == "exit" || command == "quit" || command == "jobs" || command == "fg" ||
            command == "bg" || command == "kill");
}

std::string BuiltinCommandExecutor::execute(const std::string& command,
                                            const std::vector<std::string>& args,
                                            std::string& current_directory,
                                            std::vector<TerminalLine>& output_lines) {
    if (command == "help" || command == "h") {
        return executeHelp();
    } else if (command == "clear" || command == "cls") {
        return executeClear(output_lines);
    } else if (command == "pwd") {
        return executePwd(current_directory);
    } else if (command == "cd") {
        return executeCd(args, current_directory);
    } else if (command == "ls") {
        // ls 命令如果有参数（如 -al），应该使用系统命令执行
        // 因为内置的 ls 不支持参数
        if (!args.empty()) {
            return ""; // 返回空，让系统命令处理
        }
        return executeLs(args, current_directory);
    } else if (command == "cat") {
        return executeCat(args, current_directory);
    } else if (command == "echo") {
        return executeEcho(args);
    } else if (command == "whoami") {
        return executeWhoami();
    } else if (command == "hostname") {
        return executeHostname();
    } else if (command == "jobs") {
        return executeJobs();
    } else if (command == "fg") {
        return executeFg(args);
    } else if (command == "bg") {
        return executeBg(args);
    } else if (command == "kill") {
        return executeKill(args);
    } else if (command == "exit" || command == "quit") {
        // 这个命令由 Editor 处理，关闭终端
        return "";
    }

    return ""; // 未知的内置命令
}

std::string BuiltinCommandExecutor::executeHelp() {
    return "Available commands:\n"
           "  help, h          - Show this help message\n"
           "  clear, cls       - Clear terminal output\n"
           "  pwd              - Print current directory\n"
           "  cd <dir>         - Change directory\n"
           "  ls [dir]         - List directory contents\n"
           "  cat <file>       - Display file contents\n"
           "  echo <text>      - Print text\n"
           "  whoami           - Print current user\n"
           "  hostname         - Print hostname\n"
           "  jobs             - List all jobs\n"
           "  fg [%n]          - Bring job to foreground\n"
           "  bg [%n]          - Put job in background\n"
           "  kill [%n|pid]    - Kill a job or process\n"
           "  exit, quit       - Close terminal";
}

std::string BuiltinCommandExecutor::executeClear(std::vector<TerminalLine>& output_lines) {
    output_lines.clear();
    return "";
}

std::string BuiltinCommandExecutor::executePwd(const std::string& current_directory) {
    return current_directory;
}

std::string BuiltinCommandExecutor::executeCd(const std::vector<std::string>& args,
                                              std::string& current_directory) {
    if (args.empty()) {
        // 切换到用户主目录
        const char* home = getenv("HOME");
        if (home) {
            current_directory = home;
        }
    } else {
        std::string target = args[0];
        if (target == "~") {
            const char* home = getenv("HOME");
            if (home) {
                target = home;
            }
        } else if (target[0] == '~') {
            const char* home = getenv("HOME");
            if (home) {
                target = std::string(home) + target.substr(1);
            }
        }

        fs::path new_path;
        if (fs::path(target).is_absolute()) {
            new_path = target;
        } else {
            new_path = fs::path(current_directory) / target;
        }

        try {
            new_path = fs::canonical(new_path);
            if (fs::exists(new_path) && fs::is_directory(new_path)) {
                current_directory = new_path.string();
            } else {
                return "cd: " + target + ": No such file or directory";
            }
        } catch (const std::exception& e) {
            return "cd: " + target + ": " + e.what();
        }
    }
    return "";
}

std::string BuiltinCommandExecutor::executeLs(const std::vector<std::string>& args,
                                              const std::string& current_directory) {
    std::string target_dir = args.empty() ? current_directory : args[0];

    if (target_dir[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            target_dir = std::string(home) + target_dir.substr(1);
        }
    }

    fs::path dir_path;
    if (fs::path(target_dir).is_absolute()) {
        dir_path = target_dir;
    } else {
        dir_path = fs::path(current_directory) / target_dir;
    }

    try {
        dir_path = fs::canonical(dir_path);
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            return "ls: " + target_dir + ": No such file or directory";
        }

        std::vector<std::string> items;
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            std::string name = entry.path().filename().string();
            if (entry.is_directory()) {
                items.push_back(name + "/");
            } else {
                items.push_back(name);
            }
        }

        std::sort(items.begin(), items.end());

        std::ostringstream oss;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0)
                oss << "  ";
            oss << items[i];
        }
        return oss.str();
    } catch (const std::exception& e) {
        return "ls: " + target_dir + ": " + e.what();
    }
}

std::string BuiltinCommandExecutor::executeCat(const std::vector<std::string>& args,
                                               const std::string& current_directory) {
    if (args.empty()) {
        return "cat: missing file argument";
    }

    std::string file_path = args[0];
    if (file_path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            file_path = std::string(home) + file_path.substr(1);
        }
    }

    fs::path full_path;
    if (fs::path(file_path).is_absolute()) {
        full_path = file_path;
    } else {
        full_path = fs::path(current_directory) / file_path;
    }

    try {
        full_path = fs::canonical(full_path);
        if (!fs::exists(full_path) || !fs::is_directory(full_path)) {
            std::ifstream file(full_path);
            if (file.is_open()) {
                std::ostringstream oss;
                oss << file.rdbuf();
                return oss.str();
            } else {
                return "cat: " + file_path + ": Cannot open file";
            }
        } else {
            return "cat: " + file_path + ": Is a directory";
        }
    } catch (const std::exception& e) {
        return "cat: " + file_path + ": " + e.what();
    }
}

std::string BuiltinCommandExecutor::executeEcho(const std::vector<std::string>& args) {
    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0)
            oss << " ";
        oss << args[i];
    }
    return oss.str();
}

std::string BuiltinCommandExecutor::executeWhoami() {
    return TerminalUtils::getUsername();
}

std::string BuiltinCommandExecutor::executeHostname() {
    return TerminalUtils::getHostname();
}

std::string BuiltinCommandExecutor::executeJobs() {
    auto jobs = JobManager::listJobs();
    if (jobs.empty()) {
        return "";
    }

    std::ostringstream oss;
    for (const auto& job : jobs) {
        oss << "[" << job.job_id << "]";

        std::string state_str;
        switch (job.state) {
            case JobState::Running:
                state_str = "Running";
                break;
            case JobState::Stopped:
                state_str = "Stopped";
                break;
            case JobState::Done:
                state_str = "Done";
                break;
            case JobState::Terminated:
                state_str = "Terminated";
                break;
        }

        oss << " " << state_str;
        if (job.state == JobState::Done || job.state == JobState::Terminated) {
            oss << " (" << job.exit_code << ")";
        }
        oss << "  " << job.command << "\n";
    }

    std::string result = oss.str();
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

std::string BuiltinCommandExecutor::executeFg(const std::vector<std::string>& args) {
    int job_id = -1;

    if (args.empty()) {
        // 如果没有参数，使用前台作业或最后一个作业
        auto jobs = JobManager::listJobs();
        if (jobs.empty()) {
            return "fg: no current job";
        }
        // 找到最后一个作业
        job_id = jobs.back().job_id;
    } else {
        // 解析作业 ID（支持 %n 格式）
        std::string arg = args[0];
        if (arg[0] == '%') {
            arg = arg.substr(1);
        }
        try {
            job_id = std::stoi(arg);
        } catch (...) {
            return "fg: " + args[0] + ": invalid job specification";
        }
    }

    if (!JobManager::bringToForeground(job_id)) {
        return "fg: " + std::to_string(job_id) + ": no such job";
    }

    return "";
}

std::string BuiltinCommandExecutor::executeBg(const std::vector<std::string>& args) {
    int job_id = -1;

    if (args.empty()) {
        // 如果没有参数，使用最后一个暂停的作业
        auto jobs = JobManager::listJobs();
        for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) {
            if (it->state == JobState::Stopped) {
                job_id = it->job_id;
                break;
            }
        }
        if (job_id < 0) {
            return "bg: no current job";
        }
    } else {
        // 解析作业 ID（支持 %n 格式）
        std::string arg = args[0];
        if (arg[0] == '%') {
            arg = arg.substr(1);
        }
        try {
            job_id = std::stoi(arg);
        } catch (...) {
            return "bg: " + args[0] + ": invalid job specification";
        }
    }

    if (!JobManager::bringToBackground(job_id)) {
        return "bg: " + std::to_string(job_id) + ": no such job";
    }

    return "";
}

std::string BuiltinCommandExecutor::executeKill(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "kill: usage: kill [%n|pid] [signal]";
    }

    int signal = 15; // SIGTERM
    std::string target = args[0];

    // 检查是否有信号参数
    if (args.size() > 1) {
        std::string sig_str = args[1];
        if (sig_str[0] == '-') {
            sig_str = sig_str.substr(1);
        }
        try {
            signal = std::stoi(sig_str);
        } catch (...) {
            return "kill: " + args[1] + ": invalid signal specification";
        }
    }

    // 解析目标（作业 ID 或 PID）
    if (target[0] == '%') {
        // 作业 ID
        target = target.substr(1);
        int job_id;
        try {
            job_id = std::stoi(target);
        } catch (...) {
            return "kill: " + args[0] + ": invalid job specification";
        }

        if (!JobManager::killJob(job_id, signal)) {
            return "kill: " + args[0] + ": no such job";
        }
    } else {
        // PID
        pid_t pid;
        try {
            pid = std::stoi(target);
        } catch (...) {
            return "kill: " + args[0] + ": invalid process ID";
        }

        if (kill(pid, signal) != 0) {
            return "kill: " + args[0] + ": " + std::string(strerror(errno));
        }
    }

    return "";
}

} // namespace terminal
} // namespace features
} // namespace pnana
