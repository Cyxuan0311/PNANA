#include "features/package_manager/package_manager_registry.h"
#include <chrono>

namespace pnana {
namespace features {
namespace package_manager {

PackageManagerRegistry& PackageManagerRegistry::getInstance() {
    static PackageManagerRegistry instance;
    return instance;
}

void PackageManagerRegistry::registerManager(std::shared_ptr<PackageManagerBase> manager) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (manager) {
        managers_[manager->getName()] = manager;
        // 预先检查可用性并缓存结果，避免首次调用 getAvailableManagers() 时的延迟
        bool is_available = manager->isAvailable();
        cached_availability_[manager->getName()] = is_available;
        // 清除可用管理器列表缓存，让下次调用时重新构建（但可用性已缓存）
        cached_available_managers_.clear();
        cache_timestamp_ = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    }
}

std::vector<std::shared_ptr<PackageManagerBase>> PackageManagerRegistry::getAllManagers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<PackageManagerBase>> result;
    for (const auto& [name, manager] : managers_) {
        result.push_back(manager);
    }
    return result;
}

std::shared_ptr<PackageManagerBase> PackageManagerRegistry::getManager(
    const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = managers_.find(name);
    if (it != managers_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<PackageManagerBase>> PackageManagerRegistry::getAvailableManagers()
    const {
    std::lock_guard<std::mutex> lock(mutex_);

    // 检查缓存是否有效
    auto now = std::chrono::steady_clock::now();
    if (!cached_available_managers_.empty() && (now - cache_timestamp_) < CACHE_TIMEOUT_) {
        return cached_available_managers_;
    }

    std::vector<std::shared_ptr<PackageManagerBase>> result;
    for (const auto& [name, manager] : managers_) {
        if (!manager)
            continue;

        bool is_available = false;
        auto cache_it = cached_availability_.find(name);
        if (cache_it != cached_availability_.end()) {
            is_available = cache_it->second;
        } else {
            is_available = manager->isAvailable();
            cached_availability_[name] = is_available;
        }

        if (is_available)
            result.push_back(manager);
    }

    cached_available_managers_ = result;
    cache_timestamp_ = now;

    return result;
}

void PackageManagerRegistry::clearAllCaches() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, manager] : managers_) {
        if (manager) {
            manager->clearCache();
        }
    }
}

void PackageManagerRegistry::setRemoteExecutorForAll(PackageManagerBase::RemoteExecutor executor,
                                                     const std::string& remote_label) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, manager] : managers_) {
        if (manager)
            manager->setRemoteExecutor(executor, remote_label);
    }
    cached_availability_.clear();
    cached_available_managers_.clear();
    cache_timestamp_ = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

void PackageManagerRegistry::clearRemoteContextForAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, manager] : managers_) {
        if (manager)
            manager->clearRemoteContext();
    }
    cached_availability_.clear();
    cached_available_managers_.clear();
    cache_timestamp_ = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

} // namespace package_manager
} // namespace features
} // namespace pnana
