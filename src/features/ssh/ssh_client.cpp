#include "features/ssh/ssh_client.h"
#include "features/ssh/ssh_client_cgo.h"
#include "ui/ssh_dialog.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

// 如果 Go 模块不可用，定义备用结构体和函数
#ifndef BUILD_GO_SSH_MODULE

// 辅助函数：检查命令是否存在
static bool commandExists(const char* cmd) {
    std::string check_cmd = "command -v ";
    check_cmd += cmd;
    check_cmd += " >/dev/null 2>&1";
    return system(check_cmd.c_str()) == 0;
}

// 辅助函数：转义 shell 命令中的特殊字符
static std::string escapeShellArg(const std::string& arg) {
    std::string escaped;
    escaped.reserve(arg.size() + 10);
    escaped += "'";
    for (char c : arg) {
        if (c == '\'') {
            escaped += "'\\''";
        } else {
            escaped += c;
        }
    }
    escaped += "'";
    return escaped;
}

// 辅助函数：构建 SSH 命令基础部分
static std::string buildSSHBase(SSHConfig_C* config, bool use_password) {
    std::ostringstream cmd;

    // 检查是否使用 sshpass（密码认证）
    if (use_password && config->password && strlen(config->password) > 0) {
        if (commandExists("sshpass")) {
            cmd << "sshpass -p " << escapeShellArg(config->password) << " ";
        } else {
            // sshpass 不可用，返回空字符串表示失败
            return "";
        }
    }

    cmd << "ssh";

    // 添加端口
    if (config->port > 0 && config->port != 22) {
        cmd << " -p " << config->port;
    }

    // 添加密钥文件
    if (config->key_path && strlen(config->key_path) > 0) {
        cmd << " -i " << escapeShellArg(config->key_path);
    }

    // 禁用 StrictHostKeyChecking（避免交互式提示）
    cmd << " -o StrictHostKeyChecking=no";
    cmd << " -o UserKnownHostsFile=/dev/null";
    cmd << " -o LogLevel=ERROR";

    // 构建用户@主机
    if (config->user && strlen(config->user) > 0) {
        cmd << " " << escapeShellArg(config->user) << "@";
    }
    cmd << escapeShellArg(config->host);

    return cmd.str();
}

// 辅助函数：构建 SCP 命令基础部分
static std::string buildSCPBase(SSHConfig_C* config, bool use_password) {
    std::ostringstream cmd;

    // 检查是否使用 sshpass（密码认证）
    if (use_password && config->password && strlen(config->password) > 0) {
        if (commandExists("sshpass")) {
            cmd << "sshpass -p " << escapeShellArg(config->password) << " ";
        } else {
            return "";
        }
    }

    cmd << "scp";

    // 添加端口
    if (config->port > 0 && config->port != 22) {
        cmd << " -P " << config->port;
    }

    // 添加密钥文件
    if (config->key_path && strlen(config->key_path) > 0) {
        cmd << " -i " << escapeShellArg(config->key_path);
    }

    // 禁用 StrictHostKeyChecking
    cmd << " -o StrictHostKeyChecking=no";
    cmd << " -o UserKnownHostsFile=/dev/null";
    cmd << " -o LogLevel=ERROR";

    return cmd.str();
}

// 辅助函数：执行命令并获取输出
static std::string executeCommand(const std::string& command) {
    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);
    if (status != 0 && result.empty()) {
        return ""; // 命令失败且无输出
    }

    // 移除末尾的换行符
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

// 辅助函数：执行命令并获取错误信息
static std::string executeCommandWithError(const std::string& command) {
    std::array<char, 4096> buffer;
    std::string result;
    std::string error_output;

    // 执行命令并捕获标准输出和标准错误
    std::string full_cmd = command + " 2>&1";
    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) {
        return "Failed to execute command";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int status = pclose(pipe);

    // 移除末尾的换行符
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    if (status != 0) {
        if (result.empty()) {
            return "Command failed with exit code " + std::to_string(status);
        }
        return result; // 返回错误输出
    }

    return result;
}

// 备用函数实现（使用系统命令）
extern "C" {
SSHResult_C* ConnectAndReadFile(SSHConfig_C* config) {
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error = nullptr;

    if (!config || !config->host || !config->remote_path) {
        result->error = strdup("Invalid configuration: host and remote_path are required");
        return result;
    }

    // 检查 ssh 命令是否存在
    if (!commandExists("ssh")) {
        result->error = strdup("ssh command not found. Please install OpenSSH client.");
        return result;
    }

    // 构建 SSH 命令
    bool use_password = (config->password && strlen(config->password) > 0);
    std::string ssh_base = buildSSHBase(config, use_password);

    if (ssh_base.empty() && use_password) {
        result->error = strdup("Password authentication requires 'sshpass' tool. "
                               "Please install sshpass or use SSH key authentication instead.");
        return result;
    }

    // 构建完整命令：ssh user@host "cat /path/to/file"
    std::ostringstream cmd;
    cmd << ssh_base << " " << escapeShellArg("cat " + std::string(config->remote_path));

    // 执行命令
    std::string output = executeCommandWithError(cmd.str());

    // 检查是否成功（简单检查：如果输出包含常见错误信息，认为失败）
    if (output.find("Permission denied") != std::string::npos ||
        output.find("Connection refused") != std::string::npos ||
        output.find("Could not resolve hostname") != std::string::npos ||
        output.find("Host key verification failed") != std::string::npos) {
        result->error = strdup(output.c_str());
        return result;
    }

    // 成功
    result->success = 1;
    result->content = strdup(output.c_str());
    return result;
}

SSHResult_C* ConnectAndWriteFile(SSHConfig_C* config, const char* content) {
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error = nullptr;

    if (!config || !config->host || !config->remote_path) {
        result->error = strdup("Invalid configuration: host and remote_path are required");
        return result;
    }

    if (!content) {
        result->error = strdup("Content is required");
        return result;
    }

    // 检查 ssh 命令是否存在
    if (!commandExists("ssh")) {
        result->error = strdup("ssh command not found. Please install OpenSSH client.");
        return result;
    }

    // 构建 SSH 命令
    bool use_password = (config->password && strlen(config->password) > 0);
    std::string ssh_base = buildSSHBase(config, use_password);

    if (ssh_base.empty() && use_password) {
        result->error = strdup("Password authentication requires 'sshpass' tool. "
                               "Please install sshpass or use SSH key authentication instead.");
        return result;
    }

    // 创建临时文件
    char tmp_template[] = "/tmp/pnana_ssh_XXXXXX";
    int tmp_fd = mkstemp(tmp_template);
    if (tmp_fd == -1) {
        result->error = strdup("Failed to create temporary file");
        return result;
    }

    std::string tmp_file = tmp_template;

    // 写入内容到临时文件
    ssize_t written = write(tmp_fd, content, strlen(content));
    close(tmp_fd);

    if (written != static_cast<ssize_t>(strlen(content))) {
        unlink(tmp_file.c_str());
        result->error = strdup("Failed to write content to temporary file");
        return result;
    }

    // 使用 scp 上传临时文件
    std::string scp_base = buildSCPBase(config, use_password);

    if (scp_base.empty() && use_password) {
        unlink(tmp_file.c_str());
        result->error = strdup("Password authentication requires 'sshpass' tool. "
                               "Please install sshpass or use SSH key authentication instead.");
        return result;
    }

    // 构建远程路径
    std::string remote_full_path;
    if (config->user && strlen(config->user) > 0) {
        remote_full_path = std::string(config->user) + "@";
    }
    remote_full_path += std::string(config->host) + ":" + std::string(config->remote_path);

    // 构建完整命令：scp [options] tmp_file user@host:remote_path
    std::ostringstream cmd;
    cmd << scp_base << " " << escapeShellArg(tmp_file) << " " << escapeShellArg(remote_full_path);

    // 执行命令
    std::string output = executeCommandWithError(cmd.str());

    // 删除临时文件
    unlink(tmp_file.c_str());

    // 检查是否成功
    if (!output.empty() && (output.find("Permission denied") != std::string::npos ||
                            output.find("Connection refused") != std::string::npos ||
                            output.find("Could not resolve hostname") != std::string::npos)) {
        result->error = strdup(output.c_str());
        return result;
    }

    // 成功
    result->success = 1;
    result->content = strdup("File written successfully");
    return result;
}

SSHResult_C* UploadFile(SSHConfig_C* config, const char* localPath, const char* remotePath) {
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error = nullptr;

    if (!config || !config->host || !localPath || !remotePath) {
        result->error =
            strdup("Invalid configuration: host, localPath and remotePath are required");
        return result;
    }

    // 检查本地文件是否存在
    struct stat st;
    if (stat(localPath, &st) != 0) {
        result->error = strdup(("Local file not found: " + std::string(localPath)).c_str());
        return result;
    }

    // 检查 scp 命令是否存在
    if (!commandExists("scp")) {
        result->error = strdup("scp command not found. Please install OpenSSH client.");
        return result;
    }

    // 构建 SCP 命令
    bool use_password = (config->password && strlen(config->password) > 0);
    std::string scp_base = buildSCPBase(config, use_password);

    if (scp_base.empty() && use_password) {
        result->error = strdup("Password authentication requires 'sshpass' tool. "
                               "Please install sshpass or use SSH key authentication instead.");
        return result;
    }

    // 构建远程路径
    std::string remote_full_path;
    if (config->user && strlen(config->user) > 0) {
        remote_full_path = std::string(config->user) + "@";
    }
    remote_full_path += std::string(config->host) + ":" + std::string(remotePath);

    // 构建完整命令：scp [options] local_file user@host:remote_path
    std::ostringstream cmd;
    cmd << scp_base << " " << escapeShellArg(localPath) << " " << escapeShellArg(remote_full_path);

    // 执行命令
    std::string output = executeCommandWithError(cmd.str());

    // 检查是否成功
    if (!output.empty() && (output.find("Permission denied") != std::string::npos ||
                            output.find("Connection refused") != std::string::npos ||
                            output.find("Could not resolve hostname") != std::string::npos ||
                            output.find("No such file or directory") != std::string::npos)) {
        result->error = strdup(output.c_str());
        return result;
    }

    // 成功
    result->success = 1;
    result->content = strdup("File uploaded successfully");
    return result;
}

SSHResult_C* DownloadFile(SSHConfig_C* config, const char* remotePath, const char* localPath) {
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error = nullptr;

    if (!config || !config->host || !remotePath || !localPath) {
        result->error =
            strdup("Invalid configuration: host, remotePath and localPath are required");
        return result;
    }

    // 检查 scp 命令是否存在
    if (!commandExists("scp")) {
        result->error = strdup("scp command not found. Please install OpenSSH client.");
        return result;
    }

    // 构建 SCP 命令
    bool use_password = (config->password && strlen(config->password) > 0);
    std::string scp_base = buildSCPBase(config, use_password);

    if (scp_base.empty() && use_password) {
        result->error = strdup("Password authentication requires 'sshpass' tool. "
                               "Please install sshpass or use SSH key authentication instead.");
        return result;
    }

    // 构建远程路径
    std::string remote_full_path;
    if (config->user && strlen(config->user) > 0) {
        remote_full_path = std::string(config->user) + "@";
    }
    remote_full_path += std::string(config->host) + ":" + std::string(remotePath);

    // 构建完整命令：scp [options] user@host:remote_path local_path
    std::ostringstream cmd;
    cmd << scp_base << " " << escapeShellArg(remote_full_path) << " " << escapeShellArg(localPath);

    // 执行命令
    std::string output = executeCommandWithError(cmd.str());

    // 检查是否成功
    if (!output.empty() && (output.find("Permission denied") != std::string::npos ||
                            output.find("Connection refused") != std::string::npos ||
                            output.find("Could not resolve hostname") != std::string::npos ||
                            output.find("No such file or directory") != std::string::npos)) {
        result->error = strdup(output.c_str());
        return result;
    }

    // 成功
    result->success = 1;
    result->content = strdup("File downloaded successfully");
    return result;
}

void FreeSSHResult(SSHResult_C* result) {
    if (result) {
        if (result->content) {
            free(result->content);
        }
        if (result->error) {
            free(result->error);
        }
        free(result);
    }
}
}
#endif

namespace pnana {
namespace features {
namespace ssh {

Client::Client() : go_client_(nullptr) {}

Client::~Client() {}

Result Client::readFile(const ui::SSHConfig& config) {
    Result result;

    // 将 C++ 配置转换为 C 配置（需要分配内存，因为 Go 会释放）
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    // 调用 Go 模块
    SSHResult_C* c_result = ConnectAndReadFile(&c_config);

    // 释放临时分配的字符串
    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to call Go SSH module";
        return result;
    }

    // 转换结果
    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    // 释放 Go 模块分配的内存
    FreeSSHResult(c_result);

    return result;
}

Result Client::writeFile(const ui::SSHConfig& config, const std::string& content) {
    Result result;

    // 将 C++ 配置转换为 C 配置（需要分配内存，因为 Go 会释放）
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    // 调用 Go 模块
    SSHResult_C* c_result = ConnectAndWriteFile(&c_config, content.c_str());

    // 释放临时分配的字符串
    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to call Go SSH module";
        return result;
    }

    // 转换结果
    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    // 释放 Go 模块分配的内存
    FreeSSHResult(c_result);

    return result;
}

Result Client::uploadFile(const ui::SSHConfig& config, const std::string& localPath,
                          const std::string& remotePath) {
    Result result;

    // 将 C++ 配置转换为 C 配置（需要分配内存，因为 Go 会释放）
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    // 调用 Go 模块
    SSHResult_C* c_result = UploadFile(&c_config, localPath.c_str(), remotePath.c_str());

    // 释放临时分配的字符串
    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to call Go SSH module";
        return result;
    }

    // 转换结果
    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    // 释放 Go 模块分配的内存
    FreeSSHResult(c_result);

    return result;
}

Result Client::downloadFile(const ui::SSHConfig& config, const std::string& remotePath,
                            const std::string& localPath) {
    Result result;

    // 将 C++ 配置转换为 C 配置（需要分配内存，因为 Go 会释放）
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    // 调用 Go 模块
    SSHResult_C* c_result = DownloadFile(&c_config, remotePath.c_str(), localPath.c_str());

    // 释放临时分配的字符串
    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to call Go SSH module";
        return result;
    }

    // 转换结果
    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    // 释放 Go 模块分配的内存
    FreeSSHResult(c_result);

    return result;
}

} // namespace ssh
} // namespace features
} // namespace pnana
