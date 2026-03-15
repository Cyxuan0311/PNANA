#ifndef PNANA_FEATURES_PACKAGE_MANAGER_SNAP_MANAGER_H
#define PNANA_FEATURES_PACKAGE_MANAGER_SNAP_MANAGER_H

#include "features/package_manager/package_manager_base.h"
#include <mutex>

namespace pnana {
namespace features {
namespace package_manager {

class SnapManager : public PackageManagerBase {
  public:
    SnapManager();

    // PackageManagerBase 接口实现
    std::string getName() const override;
    std::string getDisplayName() const override;
    std::vector<Package> getInstalledPackages() override;
    void refreshPackages() override;
    bool hasError() const override;
    std::string getError() const override;
    bool isFetching() const override;
    void clearCache() override;
    bool isAvailable() const override;
    bool updatePackage(const std::string& package_name) override;
    bool updateAllDependencies(const std::string& package_name) override;
    bool removePackage(const std::string& package_name) override;
    bool installPackage(const std::string& package_name) override;

  private:
    // 缓存管理
    bool isCacheValid() const;
    std::vector<Package> fetchPackagesFromSystem();
    std::vector<Package> parseSnapOutput(const std::string& output);

    // 缓存数据
    PackageCacheEntry cache_entry_;
    mutable std::mutex cache_mutex_;
    static constexpr std::chrono::milliseconds CACHE_TIMEOUT_{60000}; // 1分钟缓存
};

} // namespace package_manager
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_PACKAGE_MANAGER_SNAP_MANAGER_H