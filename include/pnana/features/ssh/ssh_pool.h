#ifndef PNANA_FEATURES_SSH_SSH_POOL_H
#define PNANA_FEATURES_SSH_SSH_POOL_H

#include "features/ssh/ssh_connection.h"
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

namespace pnana {
namespace features {
namespace ssh {

// 连接池配置
struct PoolConfig {
    size_t max_connections_per_host = 3;                            // 每个主机最大连接数
    size_t max_idle_connections = 10;                               // 最大空闲连接数
    std::chrono::minutes connection_ttl = std::chrono::minutes(30); // 连接存活时间
    std::chrono::minutes idle_timeout = std::chrono::minutes(5);    // 空闲超时
    bool auto_reconnect = true;                                     // 自动重连
};

// 连接池管理器（单例模式）
class SSHPool {
  public:
    static SSHPool& getInstance();

    // 获取连接（从池中获取或创建新连接）
    std::shared_ptr<SSHConnection> acquire(const SSHConfig& config);

    // 释放连接（归还到池中）
    void release(std::shared_ptr<SSHConnection> conn);

    // 关闭指定主机的所有连接
    void closeConnections(const std::string& host_key);

    // 清理过期和无效连接
    void cleanup();

    // 设置池配置
    void setConfig(const PoolConfig& config) {
        config_ = config;
    }

    // 获取统计信息
    size_t getActiveConnections() const;
    size_t getIdleConnections() const;
    size_t getTotalConnections() const;

  private:
    SSHPool();
    ~SSHPool();

    // 禁止拷贝和移动
    SSHPool(const SSHPool&) = delete;
    SSHPool& operator=(const SSHPool&) = delete;

    // 创建新连接
    std::shared_ptr<SSHConnection> createConnection(const SSHConfig& config);

    // 检查连接是否有效
    bool isValidConnection(const std::shared_ptr<SSHConnection>& conn);

    // 连接池条目
    struct PoolEntry {
        std::shared_ptr<SSHConnection> connection;
        std::chrono::steady_clock::time_point created_at;
        std::chrono::steady_clock::time_point last_used;
        bool in_use = false;
    };

    mutable std::mutex mutex_;
    std::map<std::string, std::vector<PoolEntry>> pool_; // host_key -> connections
    PoolConfig config_;
    std::atomic<size_t> total_created_{0};
    std::atomic<size_t> total_acquired_{0};
    std::atomic<size_t> total_released_{0};
};

// RAII 连接守卫（自动归还连接到池中）
class SSHConnectionGuard {
  public:
    SSHConnectionGuard(std::shared_ptr<SSHConnection> conn) : conn_(conn), released_(false) {}

    ~SSHConnectionGuard() {
        if (!released_ && conn_) {
            SSHPool::getInstance().release(conn_);
        }
    }

    // 禁止拷贝
    SSHConnectionGuard(const SSHConnectionGuard&) = delete;
    SSHConnectionGuard& operator=(const SSHConnectionGuard&) = delete;

    // 允许移动
    SSHConnectionGuard(SSHConnectionGuard&& other) noexcept
        : conn_(std::move(other.conn_)), released_(other.released_) {
        other.released_ = true;
    }

    SSHConnectionGuard& operator=(SSHConnectionGuard&& other) noexcept {
        if (this != &other) {
            if (!released_ && conn_) {
                SSHPool::getInstance().release(conn_);
            }
            conn_ = std::move(other.conn_);
            released_ = other.released_;
            other.released_ = true;
        }
        return *this;
    }

    SSHConnection* get() const {
        return conn_.get();
    }
    SSHConnection* operator->() const {
        return conn_.get();
    }

    // 手动释放（不调用此方法会在析构时自动释放）
    void release() {
        if (!released_ && conn_) {
            SSHPool::getInstance().release(conn_);
            released_ = true;
        }
    }

  private:
    std::shared_ptr<SSHConnection> conn_;
    bool released_;
};

} // namespace ssh
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SSH_SSH_POOL_H
