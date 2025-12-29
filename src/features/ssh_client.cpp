#include "features/ssh_client.h"
#include "ui/ssh_dialog.h"
#include <cstdlib>
#include <cstring>
#include <sstream>

// 如果 Go 模块可用，包含 CGO 头文件
#ifdef BUILD_GO_SSH_MODULE
#include "features/ssh_client_cgo.h"
#else
// 如果没有 Go 模块，定义备用结构体（使用系统命令实现）
typedef struct {
    char* host;
    char* user;
    char* password;
    char* key_path;
    int port;
    char* remote_path;
} SSHConfig_C;

typedef struct {
    int success;
    char* content;
    char* error;
} SSHResult_C;

// 备用函数声明（使用系统命令）
extern "C" {
    SSHResult_C* ConnectAndReadFile(SSHConfig_C* config);
    SSHResult_C* ConnectAndWriteFile(SSHConfig_C* config, const char* content);
    void FreeSSHResult(SSHResult_C* result);
}
#endif

namespace pnana {
namespace features {

SSHClient::SSHClient() : go_client_(nullptr) {
}

SSHClient::~SSHClient() {
}

SSHResult SSHClient::readFile(const ui::SSHConfig& config) {
    SSHResult result;
    result.success = false;
    result.content = "";
    result.error = "";
    
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

SSHResult SSHClient::writeFile(const ui::SSHConfig& config, const std::string& content) {
    SSHResult result;
    result.success = false;
    result.content = "";
    result.error = "";
    
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
    
    if (!c_result) {
        result.error = "Failed to call Go SSH module";
        // 释放临时分配的字符串
        free(c_config.host);
        free(c_config.user);
        free(c_config.password);
        free(c_config.key_path);
        free(c_config.remote_path);
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
    
    // 释放临时分配的字符串
    free(c_config.host);
    free(c_config.user);
    free(c_config.password);
    free(c_config.key_path);
    free(c_config.remote_path);
    
    return result;
}

} // namespace features
} // namespace pnana
