#include "features/terminal/terminal_builtin.h"
#include "features/terminal.h"
#include "features/terminal/terminal_utils.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace pnana {
namespace features {
namespace terminal {

bool BuiltinCommandExecutor::isBuiltin(const std::string& command) {
    return (command == "help" || command == "h" || command == "clear" || command == "cls" ||
            command == "pwd" || command == "cd" || command == "ls" || command == "cat" ||
            command == "echo" || command == "whoami" || command == "hostname" ||
            command == "exit" || command == "quit");
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

} // namespace terminal
} // namespace features
} // namespace pnana
