#ifndef PNANA_FEATURES_SSH_SSH_CLIENT_NATIVE_H
#define PNANA_FEATURES_SSH_SSH_CLIENT_NATIVE_H

#include "features/ssh/ssh_client.h"
#include "features/ssh/ssh_connection.h"
#include "features/ssh/ssh_pool.h"
#include <functional>
#include <vector>

namespace pnana {
namespace features {
namespace ssh {

// 文件传输进度回调
using ProgressCallback = std::function<void(size_t transferred, size_t total)>;

// 原生 C++ SSH 客户端（不依赖 Go）
class SSHClientNative {
  public:
    SSHClientNative();
    ~SSHClientNative();

    // ==================== 文件操作 ====================

    // 读取远程文件内容
    SSHResult readFile(const SSHConfig& config);

    // 写入内容到远程文件
    SSHResult writeFile(const SSHConfig& config, const std::string& content);

    // 上传文件到远程服务器
    SSHResult uploadFile(const SSHConfig& config, const std::string& local_path,
                         const std::string& remote_path, ProgressCallback callback = nullptr);

    // 从远程服务器下载文件
    SSHResult downloadFile(const SSHConfig& config, const std::string& remote_path,
                           const std::string& local_path, ProgressCallback callback = nullptr);

    // ==================== 目录操作 ====================

    // 列出远程目录内容
    SSHResult listDir(const SSHConfig& config);

    // 解析目录列表结果
    static std::vector<RemoteDirEntry> parseListDirContent(const std::string& content);

    // 获取路径类型（dir/file/unknown）
    SSHResult getPathType(const SSHConfig& config);

    // 创建远程目录
    SSHResult mkdir(const SSHConfig& config, const std::string& path);

    // 删除远程文件
    SSHResult removeFile(const SSHConfig& config, const std::string& path);

    // 删除远程目录（递归）
    SSHResult removeDir(const SSHConfig& config, const std::string& path);

    // 重命名/移动远程文件
    SSHResult rename(const SSHConfig& config, const std::string& old_path,
                     const std::string& new_path);

    // 检查远程文件是否存在
    bool exists(const SSHConfig& config, const std::string& path);

    // 获取远程文件大小
    SSHResult getFileSize(const SSHConfig& config, const std::string& path);

    // ==================== 命令执行 ====================

    // 执行远程命令
    SSHResult runCommand(const SSHConfig& config, const std::string& command,
                         const std::string& working_dir = "", bool pty = false);

    // 执行命令并流式输出（适合长时间运行的命令）
    SSHResult runCommandStreaming(const SSHConfig& config, const std::string& command,
                                  std::function<void(const std::string&)> stdout_callback,
                                  std::function<void(const std::string&)> stderr_callback = nullptr,
                                  const std::string& working_dir = "");

  private:
    // SFTP 相关文件操作
    SSHResult sftpReadFile(LIBSSH2_SESSION* session, const std::string& remote_path,
                           std::string& content);
    SSHResult sftpWriteFile(LIBSSH2_SESSION* session, const std::string& remote_path,
                            const std::string& content);
    SSHResult sftpUploadFile(LIBSSH2_SESSION* session, const std::string& local_path,
                             const std::string& remote_path, ProgressCallback callback);
    SSHResult sftpDownloadFile(LIBSSH2_SESSION* session, const std::string& remote_path,
                               const std::string& local_path, ProgressCallback callback);

    // 辅助函数
    std::string escapeShellPath(const std::string& path);
    std::string escapeShellArg(const std::string& arg);
    std::string buildRemoteCommand(const std::string& working_dir, const std::string& command);

    // libssh2 初始化
    static std::once_flag init_flag_;
    static void initLibSSH2();
};

} // namespace ssh
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SSH_SSH_CLIENT_NATIVE_H
