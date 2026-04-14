#include "features/ssh/ssh_client.h"
#include "ui/ssh_dialog.h"

#ifdef BUILD_CPP_SSH_MODULE
#include "features/ssh/ssh_client_native.h"
#elif defined(BUILD_GO_SSH_MODULE)
// 使用 Go CGO 接口
#include "features/ssh/ssh_client_cgo.h"
#else
// NONE 模式下也需要这些类型定义
#include "features/ssh/ssh_client_cgo.h"
#endif

#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

// 备用实现（当原生 SSH 和 Go 模块都不可用时使用系统命令）
#if !defined(BUILD_CPP_SSH_MODULE) && !defined(BUILD_GO_SSH_MODULE)

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

#endif // !defined(BUILD_CPP_SSH_MODULE) && !defined(BUILD_GO_SSH_MODULE)

// Go CGO 接口函数（仅在没有 Go 模块时使用 C++ 备用实现）
#if !defined(BUILD_CPP_SSH_MODULE) && !defined(BUILD_GO_SSH_MODULE)
extern "C" {
SSHResult_C* ConnectAndReadFile(SSHConfig_C* config) {
    using namespace pnana::features::ssh;
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

    result->success = 1;
    result->content = strdup(output.c_str());
    return result;
}

SSHResult_C* ConnectAndWriteFile(SSHConfig_C* config, const char* content) {
    using namespace pnana::features::ssh;
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

    result->success = 1;
    result->content = strdup("File written successfully");
    return result;
}

SSHResult_C* UploadFile(SSHConfig_C* config, const char* localPath, const char* remotePath) {
    using namespace pnana::features::ssh;
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

    result->success = 1;
    result->content = strdup("File uploaded successfully");
    return result;
}

SSHResult_C* DownloadFile(SSHConfig_C* config, const char* remotePath, const char* localPath) {
    using namespace pnana::features::ssh;
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

    result->success = 1;
    result->content = strdup("File downloaded successfully");
    return result;
}

// 解析 ls -la 输出为 "d\tname\n" 或 "f\tname\n"
static std::string parseLsLaToDf(const std::string& lsOutput,
                                 [[maybe_unused]] const std::string& path) {
    std::istringstream iss(lsOutput);
    std::string line;
    std::ostringstream out;
    bool first = true;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        if (first && line.size() > 5 && line.compare(0, 5, "total") == 0) {
            first = false;
            continue;
        }
        if (line.size() < 2)
            continue;
        char type = (line[0] == 'd' || line[0] == 'l') ? 'd' : 'f';
        // 文件名从第 8 个字段开始（perms links user group size month day time name）
        std::vector<std::string> parts;
        std::string part;
        std::istringstream ls(line);
        while (ls >> part)
            parts.push_back(part);
        if (parts.size() < 9)
            continue;
        std::string name = parts[8];
        for (size_t i = 9; i < parts.size(); ++i)
            name += " " + parts[i];
        if (name == "." || name == "..")
            continue;
        out << type << "\t" << name << "\n";
    }
    return out.str();
}

SSHResult_C* ConnectAndListDir(SSHConfig_C* config) {
    using namespace pnana::features::ssh;
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error = nullptr;
    if (!config || !config->host || !config->remote_path) {
        result->error = strdup("Invalid configuration");
        return result;
    }
    bool use_password = (config->password && strlen(config->password) > 0);
    std::string ssh_base = buildSSHBase(config, use_password);
    if (ssh_base.empty() && use_password) {
        result->error = strdup("Password authentication requires sshpass");
        return result;
    }
    std::string path = config->remote_path ? config->remote_path : "";
    std::string cmd = ssh_base + " " + escapeShellArg("ls -la " + path);
    std::string output = executeCommandWithError(cmd);
    if (!output.empty() && (output.find("Permission denied") != std::string::npos ||
                            output.find("Not a directory") != std::string::npos)) {
        result->error = strdup(output.c_str());
        return result;
    }
    std::string content = parseLsLaToDf(output, path);
    result->success = 1;
    result->content = strdup(content.c_str());
    return result;
}

SSHResult_C* ConnectAndGetPathType(SSHConfig_C* config) {
    using namespace pnana::features::ssh;
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error = nullptr;
    if (!config || !config->host || !config->remote_path) {
        result->error = strdup("Invalid configuration");
        return result;
    }
    bool use_password = (config->password && strlen(config->password) > 0);
    std::string ssh_base = buildSSHBase(config, use_password);
    if (ssh_base.empty() && use_password) {
        result->error = strdup("Password authentication requires sshpass");
        return result;
    }
    std::string path = config->remote_path ? config->remote_path : "";
    std::string cmd = ssh_base + " " +
                      escapeShellArg("test -d " + path + " && echo dir || (test -f " + path +
                                     " && echo file || echo error)");
    std::string output = executeCommandWithError(cmd);
    if (output.find("dir") != std::string::npos) {
        result->success = 1;
        result->content = strdup("dir");
    } else if (output.find("file") != std::string::npos) {
        result->success = 1;
        result->content = strdup("file");
    } else {
        result->success = 1;
        result->content = strdup("error");
    }
    return result;
}

SSHResult_C* ConnectAndRunCommand(SSHConfig_C* config, const char* command) {
    using namespace pnana::features::ssh;
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error = nullptr;
    if (!config || !config->host || !command) {
        result->error = strdup("Invalid configuration or command");
        return result;
    }
    bool use_password = (config->password && strlen(config->password) > 0);
    std::string ssh_base = buildSSHBase(config, use_password);
    if (ssh_base.empty() && use_password) {
        result->error = strdup("Password authentication requires sshpass");
        return result;
    }
    std::string cwd = config->remote_path && config->remote_path[0] ? config->remote_path : ".";
    std::string full_cmd = "cd " + cwd + " 2>/dev/null && " + command;
    std::string output = executeCommandWithError(ssh_base + " " + escapeShellArg(full_cmd));
    result->success = 1;
    result->content = strdup(output.c_str());
    return result;
}

// FreeSSHResult 实现（后备版本，当 CPP 和 GO 都不可用时使用）
void FreeSSHResult(SSHResult_C* result) {
    if (!result)
        return;

    if (result->content) {
        free(result->content);
        result->content = nullptr;
    }
    if (result->error) {
        free(result->error);
        result->error = nullptr;
    }
    free(result);
}

} // extern "C"

#endif // !defined(BUILD_CPP_SSH_MODULE) && !defined(BUILD_GO_SSH_MODULE)

namespace pnana {
namespace features {
namespace ssh {

Client::Client() {}

Client::~Client() {}

Result Client::readFile(const ui::SSHConfig& config) {
#ifdef BUILD_CPP_SSH_MODULE
    // 使用原生 libssh2 实现
    SSHClientNative native_client;

    // 转换配置
    ssh::SSHConfig native_config;
    native_config.host = config.host;
    native_config.user = config.user;
    native_config.password = config.password;
    native_config.key_path = config.key_path;
    native_config.port = config.port;
    native_config.remote_path = config.remote_path;

    SSHResult result = native_client.readFile(native_config);

    Result cpp_result;
    cpp_result.success = result.success;
    cpp_result.content = result.content;
    cpp_result.error = result.error;
    return cpp_result;
#elif defined(BUILD_GO_SSH_MODULE)
    // 使用 Go CGO 接口
    Result result;

    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndReadFile(&c_config);

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#else
    // 使用系统命令备用实现（当 CPP 和 GO 都不可用时）
    // 后备实现在 #if !defined(BUILD_CPP_SSH_MODULE) && !defined(BUILD_GO_SSH_MODULE) 块中定义
    Result result;

    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndReadFile(&c_config);

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#endif
}

Result Client::writeFile(const ui::SSHConfig& config, const std::string& content) {
#ifdef BUILD_CPP_SSH_MODULE
    SSHClientNative native_client;

    ssh::SSHConfig native_config;
    native_config.host = config.host;
    native_config.user = config.user;
    native_config.password = config.password;
    native_config.key_path = config.key_path;
    native_config.port = config.port;
    native_config.remote_path = config.remote_path;

    SSHResult result = native_client.writeFile(native_config, content);

    Result cpp_result;
    cpp_result.success = result.success;
    cpp_result.content = result.content;
    cpp_result.error = result.error;
    return cpp_result;
#elif defined(BUILD_GO_SSH_MODULE)
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndWriteFile(&c_config, content.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#else
    // 使用系统命令备用实现
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndWriteFile(&c_config, content.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#endif
}

Result Client::uploadFile(const ui::SSHConfig& config, const std::string& localPath,
                          const std::string& remotePath) {
#ifdef BUILD_CPP_SSH_MODULE
    SSHClientNative native_client;

    ssh::SSHConfig native_config;
    native_config.host = config.host;
    native_config.user = config.user;
    native_config.password = config.password;
    native_config.key_path = config.key_path;
    native_config.port = config.port;
    native_config.remote_path = remotePath;

    SSHResult result = native_client.uploadFile(native_config, localPath, remotePath);

    Result cpp_result;
    cpp_result.success = result.success;
    cpp_result.content = result.content;
    cpp_result.error = result.error;
    return cpp_result;
#elif defined(BUILD_GO_SSH_MODULE)
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = UploadFile(&c_config, localPath.c_str(), remotePath.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#else
    // 使用系统命令备用实现
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(remotePath.c_str());

    SSHResult_C* c_result = UploadFile(&c_config, localPath.c_str(), remotePath.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#endif
}

Result Client::downloadFile(const ui::SSHConfig& config, const std::string& remotePath,
                            const std::string& localPath) {
#ifdef BUILD_CPP_SSH_MODULE
    SSHClientNative native_client;

    ssh::SSHConfig native_config;
    native_config.host = config.host;
    native_config.user = config.user;
    native_config.password = config.password;
    native_config.key_path = config.key_path;
    native_config.port = config.port;
    native_config.remote_path = remotePath;

    SSHResult result = native_client.downloadFile(native_config, remotePath, localPath);

    Result cpp_result;
    cpp_result.success = result.success;
    cpp_result.content = result.content;
    cpp_result.error = result.error;
    return cpp_result;
#elif defined(BUILD_GO_SSH_MODULE)
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = DownloadFile(&c_config, remotePath.c_str(), localPath.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#else
    // 使用系统命令备用实现
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = DownloadFile(&c_config, remotePath.c_str(), localPath.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }

    result.success = (c_result->success != 0);
    if (c_result->content) {
        result.content = std::string(c_result->content);
    }
    if (c_result->error) {
        result.error = std::string(c_result->error);
    }

    FreeSSHResult(c_result);
    return result;
#endif
}

Result Client::listDir(const ui::SSHConfig& config) {
#ifdef BUILD_CPP_SSH_MODULE
    SSHClientNative native_client;

    ssh::SSHConfig native_config;
    native_config.host = config.host;
    native_config.user = config.user;
    native_config.password = config.password;
    native_config.key_path = config.key_path;
    native_config.port = config.port;
    native_config.remote_path = config.remote_path;

    SSHResult result = native_client.listDir(native_config);

    Result cpp_result;
    cpp_result.success = result.success;
    cpp_result.content = result.content;
    cpp_result.error = result.error;
    return cpp_result;
#elif defined(BUILD_GO_SSH_MODULE)
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndListDir(&c_config);

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }
    result.success = (c_result->success != 0);
    if (c_result->content)
        result.content = std::string(c_result->content);
    if (c_result->error)
        result.error = std::string(c_result->error);
    FreeSSHResult(c_result);
    return result;
#else
    // 使用系统命令备用实现
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndListDir(&c_config);

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }
    result.success = (c_result->success != 0);
    if (c_result->content)
        result.content = std::string(c_result->content);
    if (c_result->error)
        result.error = std::string(c_result->error);
    FreeSSHResult(c_result);
    return result;
#endif
}

Result Client::getPathType(const ui::SSHConfig& config) {
#ifdef BUILD_CPP_SSH_MODULE
    SSHClientNative native_client;

    ssh::SSHConfig native_config;
    native_config.host = config.host;
    native_config.user = config.user;
    native_config.password = config.password;
    native_config.key_path = config.key_path;
    native_config.port = config.port;
    native_config.remote_path = config.remote_path;

    SSHResult result = native_client.getPathType(native_config);

    Result cpp_result;
    cpp_result.success = result.success;
    cpp_result.content = result.content;
    cpp_result.error = result.error;
    return cpp_result;
#elif defined(BUILD_GO_SSH_MODULE)
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndGetPathType(&c_config);

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }
    result.success = (c_result->success != 0);
    if (c_result->content)
        result.content = std::string(c_result->content);
    if (c_result->error)
        result.error = std::string(c_result->error);
    FreeSSHResult(c_result);
    return result;
#else
    // 使用系统命令备用实现
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(config.remote_path.c_str());

    SSHResult_C* c_result = ConnectAndGetPathType(&c_config);

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }
    result.success = (c_result->success != 0);
    if (c_result->content)
        result.content = std::string(c_result->content);
    if (c_result->error)
        result.error = std::string(c_result->error);
    FreeSSHResult(c_result);
    return result;
#endif
}

Result Client::runCommand(const ui::SSHConfig& config, const std::string& working_dir,
                          const std::string& command) {
#ifdef BUILD_CPP_SSH_MODULE
    SSHClientNative native_client;

    ssh::SSHConfig native_config;
    native_config.host = config.host;
    native_config.user = config.user;
    native_config.password = config.password;
    native_config.key_path = config.key_path;
    native_config.port = config.port;
    native_config.remote_path = working_dir.empty() ? "." : working_dir;

    SSHResult result = native_client.runCommand(native_config, command, working_dir);

    Result cpp_result;
    cpp_result.success = result.success;
    cpp_result.content = result.content;
    cpp_result.error = result.error;
    return cpp_result;
#elif defined(BUILD_GO_SSH_MODULE)
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(working_dir.empty() ? "." : working_dir.c_str());

    SSHResult_C* c_result = ConnectAndRunCommand(&c_config, command.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }
    result.success = (c_result->success != 0);
    if (c_result->content)
        result.content = std::string(c_result->content);
    if (c_result->error)
        result.error = std::string(c_result->error);
    FreeSSHResult(c_result);
    return result;
#else
    // 使用系统命令备用实现
    Result result;
    SSHConfig_C c_config;
    c_config.host = strdup(config.host.c_str());
    c_config.user = strdup(config.user.c_str());
    c_config.password = strdup(config.password.c_str());
    c_config.key_path = strdup(config.key_path.c_str());
    c_config.port = config.port;
    c_config.remote_path = strdup(working_dir.empty() ? "." : working_dir.c_str());

    SSHResult_C* c_result = ConnectAndRunCommand(&c_config, command.c_str());

    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);

    if (!c_result) {
        result.error = "Failed to execute SSH command";
        return result;
    }
    result.success = (c_result->success != 0);
    if (c_result->content)
        result.content = std::string(c_result->content);
    if (c_result->error)
        result.error = std::string(c_result->error);
    FreeSSHResult(c_result);
    return result;
#endif
}

std::vector<RemoteDirEntry> parseListDirContent(const std::string& content) {
    std::vector<RemoteDirEntry> entries;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty())
            continue;
        size_t tab = line.find('\t');
        if (tab == std::string::npos)
            continue;
        RemoteDirEntry e;
        e.is_directory = (line[0] == 'd');
        e.name = line.substr(tab + 1);
        entries.push_back(e);
    }
    return entries;
}

} // namespace ssh
} // namespace features
} // namespace pnana
