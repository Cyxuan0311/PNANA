#ifndef PNANA_FEATURES_PACKAGE_MANAGER_PACKAGE_MANAGER_BASE_H
#define PNANA_FEATURES_PACKAGE_MANAGER_PACKAGE_MANAGER_BASE_H

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace package_manager {

// 通用包信息结构
struct Package {
    std::string name;
    std::string version;
    std::string location;
    std::string description;
    std::string status; // 安装状态等额外信息
};

// 包列表缓存条目
struct PackageCacheEntry {
    std::vector<Package> packages;
    std::chrono::steady_clock::time_point timestamp;
    bool is_fetching;
    std::string error_message;
};

// 包管理器基类接口
class PackageManagerBase {
  public:
    virtual ~PackageManagerBase() = default;

    // 获取管理器名称
    virtual std::string getName() const = 0;

    // 获取管理器显示名称
    virtual std::string getDisplayName() const = 0;

    // 获取已安装的包列表（异步，带缓存）
    virtual std::vector<Package> getInstalledPackages() = 0;

    // 刷新包列表（强制更新缓存）
    virtual void refreshPackages() = 0;

    // 检查是否有错误
    virtual bool hasError() const = 0;

    // 获取错误信息
    virtual std::string getError() const = 0;

    // 检查是否正在获取数据
    virtual bool isFetching() const = 0;

    // 清除缓存
    virtual void clearCache() = 0;

    // 检查管理器是否可用（例如命令是否存在）
    virtual bool isAvailable() const = 0;

    // 更新单个包
    virtual bool updatePackage(const std::string& package_name) = 0;

    // 链式更新包及其所有依赖
    virtual bool updateAllDependencies(const std::string& package_name) = 0;

    // 删除/卸载包
    virtual bool removePackage(const std::string& package_name) = 0;

    // 安装包
    virtual bool installPackage(const std::string& package_name) = 0;
};

} // namespace package_manager
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_PACKAGE_MANAGER_PACKAGE_MANAGER_BASE_H
