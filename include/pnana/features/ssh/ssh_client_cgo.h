#ifndef PNANA_FEATURES_SSH_SSH_CLIENT_CGO_H
#define PNANA_FEATURES_SSH_SSH_CLIENT_CGO_H

#ifdef __cplusplus
extern "C" {
#endif

// C 接口结构体（与 Go 中的 C 结构体对应）
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

// C 接口函数（由 Go 模块提供）
SSHResult_C* ConnectAndReadFile(SSHConfig_C* config);
SSHResult_C* ConnectAndWriteFile(SSHConfig_C* config, const char* content);
SSHResult_C* UploadFile(SSHConfig_C* config, const char* localPath, const char* remotePath);
SSHResult_C* DownloadFile(SSHConfig_C* config, const char* remotePath, const char* localPath);
SSHResult_C* ConnectAndListDir(SSHConfig_C* config);
SSHResult_C* ConnectAndGetPathType(SSHConfig_C* config);
SSHResult_C* ConnectAndRunCommand(SSHConfig_C* config, const char* command);
void FreeSSHResult(SSHResult_C* result);

#ifdef __cplusplus
}
#endif

#endif // PNANA_FEATURES_SSH_SSH_CLIENT_CGO_H
