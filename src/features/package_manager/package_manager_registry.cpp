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

    // 缓存失效，重新计算可用管理器
    std::vector<std::shared_ptr<PackageManagerBase>> result;
    for (const auto& [name, manager] : managers_) {
        if (!manager) {
            continue;
        }

        // 检查缓存中是否有该管理器的可用性结果（独立于列表缓存）
        bool is_available = false;
        auto cache_it = cached_availability_.find(name);
        if (cache_it != cached_availability_.end()) {
            // 使用缓存的结果（可用性缓存永久有效，直到管理器重新注册）
            is_available = cache_it->second;
        } else {
            // 调用 isAvailable() 并缓存结果
            is_available = manager->isAvailable();
            cached_availability_[name] = is_available;
        }

        if (is_available) {
            result.push_back(manager);
        }
    }

    // 更新缓存
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

} // namespace package_manager
} // namespace features
} // namespace pnana
