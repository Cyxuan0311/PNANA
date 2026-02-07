#ifndef PNANA_FEATURES_PACKAGE_MANAGER_PACKAGE_MANAGER_REGISTRY_H
#define PNANA_FEATURES_PACKAGE_MANAGER_PACKAGE_MANAGER_REGISTRY_H

#include "features/package_manager/package_manager_base.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace package_manager {

// 包管理器注册表
class PackageManagerRegistry {
  public:
    // 获取单例实例
    static PackageManagerRegistry& getInstance();

    // 注册包管理器
    void registerManager(std::shared_ptr<PackageManagerBase> manager);

    // 获取所有已注册的管理器
    std::vector<std::shared_ptr<PackageManagerBase>> getAllManagers() const;

    // 根据名称获取管理器
    std::shared_ptr<PackageManagerBase> getManager(const std::string& name) const;

    // 获取所有可用的管理器（已安装且可用）
    std::vector<std::shared_ptr<PackageManagerBase>> getAvailableManagers() const;

    // 清除所有管理器的缓存
    void clearAllCaches();

  private:
    PackageManagerRegistry() = default;
    ~PackageManagerRegistry() = default;
    PackageManagerRegistry(const PackageManagerRegistry&) = delete;
    PackageManagerRegistry& operator=(const PackageManagerRegistry&) = delete;

    mutable std::mutex mutex_;
    std::map<std::string, std::shared_ptr<PackageManagerBase>> managers_;

    // 性能优化：缓存可用管理器列表和可用性检查结果
    mutable std::vector<std::shared_ptr<PackageManagerBase>> cached_available_managers_;
    mutable std::map<std::string, bool> cached_availability_; // 缓存每个管理器的可用性
    mutable std::chrono::steady_clock::time_point cache_timestamp_;
    static constexpr std::chrono::milliseconds CACHE_TIMEOUT_{30000}; // 30秒缓存
};

} // namespace package_manager
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_PACKAGE_MANAGER_PACKAGE_MANAGER_REGISTRY_H
