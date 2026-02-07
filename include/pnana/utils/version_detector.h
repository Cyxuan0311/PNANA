#ifndef PNANA_UTILS_VERSION_DETECTOR_H
#define PNANA_UTILS_VERSION_DETECTOR_H

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace pnana {
namespace utils {

// 版本检测器 - 用于检测编程语言和工具的版本号
class VersionDetector {
  public:
    VersionDetector();
    ~VersionDetector() = default;

    // 获取文件类型对应的版本号（带缓存）
    std::string getVersionForFileType(const std::string& file_type);

    // 清除缓存
    void clearCache();

    // 清除特定文件类型的缓存
    void clearCacheForType(const std::string& file_type);

  private:
    // 版本缓存条目
    struct VersionCacheEntry {
        std::string version;
        std::chrono::steady_clock::time_point timestamp;
        bool is_fetching; // 是否正在获取中
    };

    // 实际执行版本获取的方法
    std::string fetchVersionForFileType(const std::string& file_type);

    // 优化后的版本号解析方法
    std::string parseVersionString(const std::string& raw_output, const std::string& file_type);

    // 提取版本号的核心逻辑（使用正则表达式或智能解析）
    std::string extractVersionNumber(const std::string& text);

    // 执行命令并获取输出
    std::string executeCommand(const std::string& command);

    // 缓存相关
    std::unordered_map<std::string, VersionCacheEntry> version_cache_;
    std::mutex cache_mutex_;
    static constexpr auto CACHE_DURATION = std::chrono::minutes(5); // 缓存5分钟
};

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_VERSION_DETECTOR_H
