#ifndef PNANA_FEATURES_HISTORY_FILE_HISTORY_MANAGER_H
#define PNANA_FEATURES_HISTORY_FILE_HISTORY_MANAGER_H

#include "features/diff/myers_diff.h"
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace history {

struct VersionMeta {
    int version = 0;
    long long timestamp = 0;
    bool critical = false;
};

struct HistoryRetentionConfig {
    bool enable = true;
    int max_entries = 50;
    int max_age_days = 30;
    std::string max_total_size = "1GB";
    bool keep_critical_versions = true;
    int critical_change_threshold = 50;
    int critical_time_interval = 86400;
};

class FileHistoryManager {
  public:
    FileHistoryManager();

    // 保存一个新版本（首次保存存 base_v1，后续存增量 diff）
    bool recordVersion(const std::string& file_path, const std::vector<std::string>& lines);

    // 设置历史保留配置
    void setRetentionConfig(const HistoryRetentionConfig& cfg);

    // 获取版本列表（按时间倒序）
    std::vector<VersionMeta> listVersions(const std::string& file_path);

    // 还原指定版本完整内容（base + 链式 diff）
    bool restoreVersion(const std::string& file_path, int version, std::vector<std::string>& out);

    // 读取两个版本之间 diff（用于预览）
    bool diffBetweenVersions(const std::string& file_path, int from_version, int to_version,
                             std::vector<diff::DiffRecord>& out);

  private:
    std::string history_root_;
    mutable std::mutex mutex_;
    HistoryRetentionConfig retention_config_;

    std::string getFileHash(const std::string& file_path) const;
    std::string getFileHistoryDir(const std::string& file_path) const;

    bool ensureHistoryDir(const std::string& dir) const;

    bool loadMeta(const std::string& dir, int& latest_version,
                  std::vector<VersionMeta>& versions) const;
    bool saveMeta(const std::string& dir, int latest_version,
                  const std::vector<VersionMeta>& versions) const;

    bool readLinesFromFile(const std::string& path, std::vector<std::string>& out) const;
    bool writeLinesToFile(const std::string& path, const std::vector<std::string>& lines) const;

    bool readDiff(const std::string& path, std::vector<diff::DiffRecord>& out) const;
    bool writeDiff(const std::string& path,
                   const std::vector<diff::DiffRecord>& diff_records) const;

    std::string basePath(const std::string& dir) const;
    std::string diffPath(const std::string& dir, int from_version, int to_version) const;

    long long parseSizeToBytes(const std::string& text) const;
    long long calculateDirectorySize(const std::filesystem::path& path) const;
    bool cleanupHistory(const std::string& dir, int latest_version,
                        std::vector<VersionMeta>& versions);
};

} // namespace history
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_HISTORY_FILE_HISTORY_MANAGER_H
