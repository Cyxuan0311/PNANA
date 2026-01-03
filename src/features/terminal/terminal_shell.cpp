#include "features/terminal/terminal_shell.h"
#include <cstdlib>
#include <sstream>

namespace pnana {
namespace features {
namespace terminal {

std::string ShellCommandExecutor::escapeCommand(const std::string& command) {
    std::string escaped = command;
    size_t pos = 0;

    // 转义双引号
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }

    // 转义反引号
    pos = 0;
    while ((pos = escaped.find('`', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\`");
        pos += 2;
    }

    return escaped;
}

std::string ShellCommandExecutor::escapeDirectory(const std::string& directory) {
    std::string escaped = directory;
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return escaped;
}

std::string ShellCommandExecutor::executeSystemCommand(const std::string& command,
                                                       const std::vector<std::string>& args,
                                                       const std::string& current_directory) {
    // 构建完整命令字符串（包含所有参数）
    // 这个方法现在主要用于向后兼容，实际应该使用 executeShellCommand
    std::ostringstream cmd_stream;
    cmd_stream << command;
    for (const auto& arg : args) {
        // 如果参数包含空格或特殊字符，需要加引号
        if (arg.find(' ') != std::string::npos || arg.find('|') != std::string::npos ||
            arg.find('>') != std::string::npos || arg.find('<') != std::string::npos ||
            arg.find('&') != std::string::npos || arg.find('$') != std::string::npos) {
            // 转义引号
            std::string escaped = arg;
            size_t pos = 0;
            while ((pos = escaped.find('"', pos)) != std::string::npos) {
                escaped.replace(pos, 1, "\\\"");
                pos += 2;
            }
            cmd_stream << " \"" << escaped << "\"";
        } else {
            cmd_stream << " " << arg;
        }
    }

    std::string cmd_str = cmd_stream.str();

    // 直接使用 executeShellCommand，这样可以统一处理所有命令
    return executeShellCommand(cmd_str, false, current_directory);
}

std::string ShellCommandExecutor::executeShellCommand(const std::string& command, bool background,
                                                      const std::string& current_directory) {
    // 使用 /bin/sh -c 来执行命令，支持所有 shell 特性
    // 这是最通用的方式，可以执行所有 Linux 命令和参数

    // 构建完整的 shell 命令
    // 格式: cd "directory" && command [&]
    std::string full_cmd = "cd \"";
    full_cmd += escapeDirectory(current_directory);
    full_cmd += "\" && ";

    // 对于简单命令，直接执行
    // 对于复杂命令（包含引号、变量等），需要适当转义
    std::string cmd_to_execute = command;

    // 检查命令是否包含需要转义的字符
    bool needs_escaping =
        (command.find('"') != std::string::npos || command.find('$') != std::string::npos ||
         command.find('`') != std::string::npos || command.find('\\') != std::string::npos);

    if (needs_escaping) {
        // 需要转义特殊字符
        cmd_to_execute = escapeCommand(command);
        // 但保留 $ 用于环境变量展开（在 shell 中处理）
        // 重新处理 $，不转义它
        size_t pos = 0;
        while ((pos = cmd_to_execute.find("\\$", pos)) != std::string::npos) {
            cmd_to_execute.replace(pos, 2, "$");
            pos++;
        }
        full_cmd += "\"";
        full_cmd += cmd_to_execute;
        full_cmd += "\"";
    } else {
        // 简单命令，直接使用
        full_cmd += command;
    }

    if (background) {
        full_cmd += " &";
    }

    // 使用 /bin/sh -c 执行
    std::string shell_cmd = "/bin/sh -c \"";
    std::string escaped_full_cmd = escapeCommand(full_cmd);
    // 恢复 $ 用于环境变量
    size_t pos = 0;
    while ((pos = escaped_full_cmd.find("\\$", pos)) != std::string::npos) {
        escaped_full_cmd.replace(pos, 2, "$");
        pos++;
    }
    shell_cmd += escaped_full_cmd;
    shell_cmd += "\"";

    // 执行命令并捕获输出（包括标准错误）
    std::string exec_cmd = shell_cmd + " 2>&1"; // 2>&1 将标准错误重定向到标准输出
    FILE* pipe = popen(exec_cmd.c_str(), "r");
    if (!pipe) {
        return "Failed to execute command: " + command;
    }

    std::ostringstream result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result << buffer;
    }

    int status = pclose(pipe);

    std::string output = result.str();

    // 如果命令失败且没有输出，显示错误信息
    if (status != 0 && output.empty()) {
        output = "Command failed with exit code " + std::to_string(status);
    }

    // 移除末尾的换行符（如果存在）
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }

    return output;
}

} // namespace terminal
} // namespace features
} // namespace pnana
