#ifndef PNANA_FEATURES_SSH_SSH_CONNECTION_H
#define PNANA_FEATURES_SSH_SSH_CONNECTION_H

#include <arpa/inet.h>
#include <atomic>
#include <chrono>
#include <libssh2.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

namespace pnana {
namespace features {
namespace ssh {

// SSH 连接配置
struct SSHConfig {
    std::string host;
    std::string user;
    std::string password;
    std::string key_path;
    std::string remote_path; // 远程路径（用于文件操作）
    int port = 22;

    bool useKeyAuth() const {
        return !key_path.empty();
    }

    bool usePasswordAuth() const {
        return !password.empty();
    }
};

// SSH 连接结果
struct SSHResult {
    bool success = false;
    std::string content;
    std::string error;

    SSHResult() = default;

    static SSHResult ok(const std::string& msg = "") {
        SSHResult r;
        r.success = true;
        r.content = msg;
        return r;
    }

    static SSHResult fail(const std::string& err) {
        SSHResult r;
        r.success = false;
        r.error = err;
        return r;
    }
};

// SSH 连接会话（封装 libssh2 会话）
class SSHConnection {
  public:
    SSHConnection();
    ~SSHConnection();

    // 禁止拷贝，允许移动
    SSHConnection(const SSHConnection&) = delete;
    SSHConnection& operator=(const SSHConnection&) = delete;
    SSHConnection(SSHConnection&& other) noexcept;
    SSHConnection& operator=(SSHConnection&& other) noexcept;

    // 连接管理
    SSHResult connect(const SSHConfig& config);
    void disconnect();
    bool isConnected() const {
        return connected_;
    }
    bool isAlive() const;

    // 认证信息
    const SSHConfig& getConfig() const {
        return config_;
    }
    std::string getConnectionKey() const;

    // 时间戳
    std::chrono::steady_clock::time_point getLastUsedTime() const {
        return last_used_;
    }

    void touch() {
        last_used_ = std::chrono::steady_clock::now();
    }

    // 获取底层会话（用于 SFTP 等操作）
    LIBSSH2_SESSION* getSession() const {
        return session_;
    }
    int getSocket() const {
        return sock_;
    }

  private:
    SSHResult authenticateWithKey();
    SSHResult authenticateWithPassword();
    SSHResult authenticateWithKeyboardInteractive();

    LIBSSH2_SESSION* session_ = nullptr;
    LIBSSH2_CHANNEL* channel_ = nullptr;
    int sock_ = -1;
    SSHConfig config_;
    bool connected_ = false;
    std::chrono::steady_clock::time_point last_used_;
};

// 连接键生成工具
struct ConnectionKey {
    static std::string generate(const SSHConfig& config) {
        return config.user + "@" + config.host + ":" + std::to_string(config.port);
    }
};

} // namespace ssh
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SSH_SSH_CONNECTION_H
