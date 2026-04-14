#include "features/ssh/ssh_connection.h"
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace pnana {
namespace features {
namespace ssh {

// 全局初始化
static std::atomic<bool> g_libssh2_initialized{false};

void ensureLibSSH2Initialized() {
    if (!g_libssh2_initialized.load()) {
        libssh2_init(0);
        g_libssh2_initialized.store(true);
    }
}

SSHConnection::SSHConnection() {
    ensureLibSSH2Initialized();
    last_used_ = std::chrono::steady_clock::now();
}

SSHConnection::~SSHConnection() {
    disconnect();
}

SSHConnection::SSHConnection(SSHConnection&& other) noexcept
    : session_(other.session_), channel_(other.channel_), sock_(other.sock_),
      config_(std::move(other.config_)), connected_(other.connected_),
      last_used_(other.last_used_) {
    other.session_ = nullptr;
    other.channel_ = nullptr;
    other.sock_ = -1;
    other.connected_ = false;
}

SSHConnection& SSHConnection::operator=(SSHConnection&& other) noexcept {
    if (this != &other) {
        disconnect();
        session_ = other.session_;
        channel_ = other.channel_;
        sock_ = other.sock_;
        config_ = std::move(other.config_);
        connected_ = other.connected_;
        last_used_ = other.last_used_;

        other.session_ = nullptr;
        other.channel_ = nullptr;
        other.sock_ = -1;
        other.connected_ = false;
    }
    return *this;
}

std::string SSHConnection::getConnectionKey() const {
    return ConnectionKey::generate(config_);
}

SSHResult SSHConnection::connect(const SSHConfig& config) {
    if (connected_) {
        return SSHResult::fail("Already connected");
    }

    config_ = config;

    // 解析主机地址
    struct addrinfo hints = {}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string port_str = std::to_string(config.port);
    int ret = getaddrinfo(config.host.c_str(), port_str.c_str(), &hints, &res);
    if (ret != 0) {
        return SSHResult::fail("Failed to resolve host: " + std::string(gai_strerror(ret)));
    }

    // 创建 socket 并连接
    sock_ = -1;
    for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
        sock_ = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock_ == -1)
            continue;

        if (::connect(sock_, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(sock_);
        sock_ = -1;
    }

    freeaddrinfo(res);

    if (sock_ == -1) {
        return SSHResult::fail("Failed to connect to " + config.host + ":" + port_str);
    }

    // 创建 SSH 会话
    session_ = libssh2_session_init();
    if (!session_) {
        close(sock_);
        sock_ = -1;
        return SSHResult::fail("Failed to initialize SSH session");
    }

    // 设置非阻塞模式
    libssh2_session_set_blocking(session_, 1);

    // 执行 SSH 握手
    ret = libssh2_session_handshake(session_, sock_);
    if (ret != 0) {
        char* errmsg;
        libssh2_session_last_error(session_, &errmsg, nullptr, 0);
        std::string error(errmsg);
        libssh2_session_free(session_);
        session_ = nullptr;
        close(sock_);
        sock_ = -1;
        return SSHResult::fail("SSH handshake failed: " + error);
    }

    // 验证主机密钥
    const char* host_key_data = libssh2_session_hostkey(session_, nullptr, nullptr);
    if (!host_key_data) {
        libssh2_session_free(session_);
        session_ = nullptr;
        close(sock_);
        sock_ = -1;
        return SSHResult::fail("Failed to get host key");
    }

    // 进行认证
    SSHResult auth_result;
    if (config.useKeyAuth()) {
        auth_result = authenticateWithKey();
        if (!auth_result.success && config.usePasswordAuth()) {
            auth_result = authenticateWithPassword();
        }
    } else if (config.usePasswordAuth()) {
        auth_result = authenticateWithPassword();
    } else {
        auth_result = authenticateWithKeyboardInteractive();
    }

    if (!auth_result.success) {
        libssh2_session_free(session_);
        session_ = nullptr;
        close(sock_);
        sock_ = -1;
        return auth_result;
    }

    connected_ = true;
    last_used_ = std::chrono::steady_clock::now();

    return SSHResult::ok("Connected to " + config.host + ":" + port_str);
}

void SSHConnection::disconnect() {
    if (channel_) {
        libssh2_channel_close(channel_);
        channel_ = nullptr;
    }

    if (session_) {
        libssh2_session_disconnect(session_, "Client disconnect");
        libssh2_session_free(session_);
        session_ = nullptr;
    }

    if (sock_ != -1) {
        close(sock_);
        sock_ = -1;
    }

    connected_ = false;
}

bool SSHConnection::isAlive() const {
    if (!connected_ || !session_)
        return false;

    // 尝试发送 keepalive 检测连接
    int ret = libssh2_keepalive_send(session_, nullptr);
    return ret == 0;
}

SSHResult SSHConnection::authenticateWithKey() {
    if (!config_.useKeyAuth()) {
        return SSHResult::fail("No key path configured");
    }

    // 检查密钥文件是否存在
    struct stat st;
    if (stat(config_.key_path.c_str(), &st) != 0) {
        return SSHResult::fail("Key file not found: " + config_.key_path);
    }

    // 尝试使用 hostkey 认证（libssh2 自动处理）
    const char* userauth_list =
        libssh2_userauth_list(session_, config_.user.c_str(), strlen(config_.user.c_str()));
    if (!userauth_list) {
        return SSHResult::fail("Failed to get authentication methods");
    }

    // 使用公钥认证
    int ret = libssh2_userauth_publickey_fromfile_ex(
        session_, config_.user.c_str(), strlen(config_.user.c_str()),
        nullptr, // 公钥路径（可选）
        config_.key_path.c_str(),
        config_.password.empty() ? nullptr : config_.password.c_str() //  passphrase
    );

    if (ret == 0) {
        return SSHResult::ok("Authenticated with key");
    }

    char* errmsg;
    libssh2_session_last_error(session_, &errmsg, nullptr, 0);
    return SSHResult::fail("Key authentication failed: " + std::string(errmsg));
}

SSHResult SSHConnection::authenticateWithPassword() {
    if (!config_.usePasswordAuth()) {
        return SSHResult::fail("No password configured");
    }

    int ret =
        libssh2_userauth_password_ex(session_, config_.user.c_str(), strlen(config_.user.c_str()),
                                     config_.password.c_str(), strlen(config_.password.c_str()),
                                     nullptr // password change callback
        );

    if (ret == 0) {
        return SSHResult::ok("Authenticated with password");
    }

    char* errmsg;
    libssh2_session_last_error(session_, &errmsg, nullptr, 0);
    return SSHResult::fail("Password authentication failed: " + std::string(errmsg));
}

SSHResult SSHConnection::authenticateWithKeyboardInteractive() {
    // 简单的 keyboard-interactive 认证（降级到密码认证）
    if (!config_.usePasswordAuth()) {
        return SSHResult::fail("No authentication method available");
    }

    // 直接使用密码认证
    return authenticateWithPassword();
}

} // namespace ssh
} // namespace features
} // namespace pnana
