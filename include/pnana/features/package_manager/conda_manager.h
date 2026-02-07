#ifndef PNANA_FEATURES_PACKAGE_MANAGER_CONDA_MANAGER_H
#define PNANA_FEATURES_PACKAGE_MANAGER_CONDA_MANAGER_H

#include "features/package_manager/package_manager_base.h"
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace package_manager {

// Conda (Python科学计算) 包管理器
class CondaManager : public PackageManagerBase {
  public:
    CondaManager();
    ~CondaManager() = default;

    std::string getName() const override {
        return "conda";
    }

    std::string getDisplayName() const override {
        return "Conda";
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
    mutable std::mutex cache_mutex_;
    PackageCacheEntry cache_entry_;
    static constexpr std::chrono::milliseconds CACHE_TIMEOUT_{30000};

    std::vector<Package> fetchPackagesFromSystem();
    std::vector<Package> parseCondaListOutput(const std::string& output);
    bool isCacheValid() const;
};

} // namespace package_manager
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_PACKAGE_MANAGER_CONDA_MANAGER_H
