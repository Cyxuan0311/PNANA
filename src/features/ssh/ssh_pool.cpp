#include "features/ssh/ssh_pool.h"
#include <algorithm>
#include <iostream>

namespace pnana {
namespace features {
namespace ssh {

SSHPool& SSHPool::getInstance() {
    static SSHPool instance;
    return instance;
}

SSHPool::SSHPool() {
    // 默认配置
    config_.max_connections_per_host = 3;
    config_.max_idle_connections = 10;
    config_.connection_ttl = std::chrono::minutes(30);
    config_.idle_timeout = std::chrono::minutes(5);
    config_.auto_reconnect = true;
}

SSHPool::~SSHPool() {
    // 关闭所有连接
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [host_key, connections] : pool_) {
        for (auto& entry : connections) {
            entry.connection->disconnect();
        }
    }
    pool_.clear();
}

std::shared_ptr<SSHConnection> SSHPool::acquire(const SSHConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = ConnectionKey::generate(config);

    // 查找池中是否有可用连接
    auto it = pool_.find(key);
    if (it != pool_.end()) {
        for (auto& entry : it->second) {
            if (!entry.in_use && isValidConnection(entry.connection)) {
                entry.in_use = true;
                entry.last_used = std::chrono::steady_clock::now();
                total_acquired_++;
                return entry.connection;
            }
        }
    }

    // 没有可用连接，创建新连接
    auto conn = createConnection(config);
    if (conn) {
        PoolEntry entry{.connection = conn,
                        .created_at = std::chrono::steady_clock::now(),
                        .last_used = std::chrono::steady_clock::now(),
                        .in_use = true};

        pool_[key].push_back(entry);
        total_created_++;
        total_acquired_++;
    }

    return conn;
}

void SSHPool::release(std::shared_ptr<SSHConnection> conn) {
    if (!conn)
        return;

    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = conn->getConnectionKey();
    auto it = pool_.find(key);

    if (it != pool_.end()) {
        for (auto& entry : it->second) {
            if (entry.connection == conn) {
                entry.in_use = false;
                entry.last_used = std::chrono::steady_clock::now();
                conn->touch();
                total_released_++;
                return;
            }
        }
    }

    // 如果找不到连接，可能是外部创建的，直接断开
    conn->disconnect();
}

void SSHPool::closeConnections(const std::string& host_key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pool_.find(host_key);
    if (it != pool_.end()) {
        for (auto& entry : it->second) {
            entry.connection->disconnect();
        }
        pool_.erase(it);
    }
}

void SSHPool::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();

    for (auto it = pool_.begin(); it != pool_.end();) {
        auto& connections = it->second;

        // 移除过期和无效的连接
        connections.erase(
            std::remove_if(connections.begin(), connections.end(),
                           [this, &now](const PoolEntry& entry) {
                               // 检查是否过期
                               auto age = now - entry.created_at;
                               auto idle_time = now - entry.last_used;

                               if (age > config_.connection_ttl) {
                                   entry.connection->disconnect();
                                   return true;
                               }

                               if (!entry.in_use && idle_time > config_.idle_timeout) {
                                   entry.connection->disconnect();
                                   return true;
                               }

                               if (!isValidConnection(entry.connection)) {
                                   return true;
                               }

                               return false;
                           }),
            connections.end());

        // 如果池子为空，删除该主机的条目
        if (connections.empty()) {
            it = pool_.erase(it);
        } else {
            ++it;
        }
    }

    // 检查总连接数是否超过限制
    size_t total_idle = getIdleConnections();
    if (total_idle > config_.max_idle_connections) {
        // 移除最久未使用的连接
        size_t to_remove = total_idle - config_.max_idle_connections;
        size_t removed = 0;

        for (auto& [host_key, connections] : pool_) {
            if (removed >= to_remove)
                break;

            std::sort(connections.begin(), connections.end(),
                      [](const PoolEntry& a, const PoolEntry& b) {
                          return a.last_used < b.last_used;
                      });

            for (auto& entry : connections) {
                if (!entry.in_use && removed < to_remove) {
                    entry.connection->disconnect();
                    entry.connection.reset();
                    removed++;
                }
            }
        }

        // 清理空条目
        for (auto it = pool_.begin(); it != pool_.end();) {
            if (it->second.empty() ||
                std::all_of(it->second.begin(), it->second.end(), [](const PoolEntry& e) {
                    return !e.connection;
                })) {
                it = pool_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

size_t SSHPool::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& [host_key, connections] : pool_) {
        for (const auto& entry : connections) {
            if (entry.in_use) {
                count++;
            }
        }
    }
    return count;
}

size_t SSHPool::getIdleConnections() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& [host_key, connections] : pool_) {
        for (const auto& entry : connections) {
            if (!entry.in_use) {
                count++;
            }
        }
    }
    return count;
}

size_t SSHPool::getTotalConnections() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;
    for (const auto& [host_key, connections] : pool_) {
        count += connections.size();
    }
    return count;
}

std::shared_ptr<SSHConnection> SSHPool::createConnection(const SSHConfig& config) {
    auto conn = std::make_shared<SSHConnection>();

    SSHResult result = conn->connect(config);
    if (!result.success) {
        std::cerr << "Failed to create SSH connection: " << result.error << std::endl;
        return nullptr;
    }

    return conn;
}

bool SSHPool::isValidConnection(const std::shared_ptr<SSHConnection>& conn) {
    if (!conn)
        return false;
    return conn->isAlive();
}

} // namespace ssh
} // namespace features
} // namespace pnana
