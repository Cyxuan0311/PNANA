#ifndef PNANA_FEATURES_SSH_SSH_CLIENT_H
#define PNANA_FEATURES_SSH_SSH_CLIENT_H

#include <string>
#include <vector>

// 前向声明
namespace pnana {
namespace ui {
struct SSHConfig;
}
} // namespace pnana

namespace pnana {
namespace features {
namespace ssh {

// SSH 结果结构
struct Result {
    bool success;
    std::string content;
    std::string error;

    Result() : success(false), content(""), error("") {}
};

// 远程目录项（listDir 解析结果）
struct RemoteDirEntry {
    std::string name;
    bool is_directory;
};

// SSH 客户端接口（通过 CGO 调用 Go 代码）
class Client {
  public:
    Client();
    ~Client();

    // 连接到 SSH 服务器并读取文件
    Result readFile(const ui::SSHConfig& config);

    // 连接到 SSH 服务器并写入文件
    Result writeFile(const ui::SSHConfig& config, const std::string& content);

    // 上传文件到 SSH 服务器
    Result uploadFile(const ui::SSHConfig& config, const std::string& localPath,
                      const std::string& remotePath);

    // 从 SSH 服务器下载文件
    Result downloadFile(const ui::SSHConfig& config, const std::string& remotePath,
                        const std::string& localPath);

    // 列出远程目录，content 格式为每行 "d\tname" 或 "f\tname"
    Result listDir(const ui::SSHConfig& config);

    // 获取路径类型，content 为 "dir"、"file" 或 "error"
    Result getPathType(const ui::SSHConfig& config);

    // 在远程 working_dir 下执行 shell 命令，stdout 在 content，非零退出时 success=false 且 error 为
    // stderr
    Result runCommand(const ui::SSHConfig& config, const std::string& working_dir,
                      const std::string& command);

  private:
    void* go_client_; // Go 客户端句柄（如果需要）
};

// 解析 listDir 返回的 content 为条目列表
std::vector<RemoteDirEntry> parseListDirContent(const std::string& content);

} // namespace ssh
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SSH_SSH_CLIENT_H
