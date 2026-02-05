#ifndef PNANA_FEATURES_PACKAGE_MANAGER_CARGO_MANAGER_H
#define PNANA_FEATURES_PACKAGE_MANAGER_CARGO_MANAGER_H

#include "features/package_manager/package_manager_base.h"
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace package_manager {

// Cargo (Rust) 包管理器
class CargoManager : public PackageManagerBase {
  public:
    CargoManager();
    ~CargoManager() = default;

    // 实现基类接口
    std::string getName() const override {
        return "cargo";
    }

    std::string getDisplayName() const override {
        return "Cargo";
    }

    std::vector<Package> getInstalledPackages() override;

    void refreshPackages() override;

    bool hasError() const override {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return !cache_entry_.error_message.empty();
    }

    std::string getError() const override {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cache_entry_.error_message;
    }

    bool isFetching() const override {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return cache_entry_.is_fetching;
    }

    void clearCache() override;

    bool isAvailable() const override;

    bool updatePackage(const std::string& package_name) override;
    bool updateAllDependencies(const std::string& package_name) override;
    bool removePackage(const std::string& package_name) override;
    bool installPackage(const std::string& package_name) override;

  private:
    // 缓存条目
    mutable std::mutex cache_mutex_;
    PackageCacheEntry cache_entry_;

    // 缓存超时时间（30秒）
    static constexpr std::chrono::milliseconds CACHE_TIMEOUT_{30000};

    // 执行 cargo tree 命令并解析结果
    std::vector<Package> fetchPackagesFromSystem();

    // 解析 cargo tree 输出
    std::vector<Package> parseCargoTreeOutput(const std::string& output);

    // 检查缓存是否有效
    bool isCacheValid() const;
};

} // namespace package_manager
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_PACKAGE_MANAGER_CARGO_MANAGER_H
