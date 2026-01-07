#include "features/ssh/ssh_client.h"
#include "features/ssh/ssh_client_cgo.h"
#include "ui/ssh_dialog.h"
#include <cstdlib>
#include <cstring>

// 如果 Go 模块不可用，定义备用结构体和函数
#ifndef BUILD_GO_SSH_MODULE
// 备用函数实现（使用系统命令）
extern "C" {
SSHResult_C* ConnectAndReadFile(SSHConfig_C* config) {
    // 备用实现：使用系统命令
    (void)config; // 抑制未使用参数警告
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error =
        strdup("Go SSH module not available. Please build with BUILD_GO_SSH_MODULE enabled.");
    return result;
}

SSHResult_C* ConnectAndWriteFile(SSHConfig_C* config, const char* content) {
    // 备用实现：使用系统命令
    (void)config;  // 抑制未使用参数警告
    (void)content; // 抑制未使用参数警告
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error =
        strdup("Go SSH module not available. Please build with BUILD_GO_SSH_MODULE enabled.");
    return result;
}

SSHResult_C* UploadFile(SSHConfig_C* config, const char* localPath, const char* remotePath) {
    // 备用实现：使用系统命令
    (void)config;     // 抑制未使用参数警告
    (void)localPath;  // 抑制未使用参数警告
    (void)remotePath; // 抑制未使用参数警告
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error =
        strdup("Go SSH module not available. Please build with BUILD_GO_SSH_MODULE enabled.");
    return result;
}

SSHResult_C* DownloadFile(SSHConfig_C* config, const char* remotePath, const char* localPath) {
    // 备用实现：使用系统命令
    (void)config;     // 抑制未使用参数警告
    (void)remotePath; // 抑制未使用参数警告
    (void)localPath;  // 抑制未使用参数警告
    SSHResult_C* result = static_cast<SSHResult_C*>(malloc(sizeof(SSHResult_C)));
    result->success = 0;
    result->content = nullptr;
    result->error =
        strdup("Go SSH module not available. Please build with BUILD_GO_SSH_MODULE enabled.");
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
