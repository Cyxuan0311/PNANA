#ifndef PNANA_FEATURES_EXTRACT_H
#define PNANA_FEATURES_EXTRACT_H

#include <atomic>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace pnana {
namespace features {

// 压缩文件信息
struct ArchiveFile {
    std::string name;
    std::string path;
    std::string type; // zip, tar, gz, rar, 7z, etc.

    ArchiveFile(const std::string& n, const std::string& p, const std::string& t)
        : name(n), path(p), type(t) {}
};

// 解压功能类
class ExtractManager {
  public:
    ExtractManager();
    ~ExtractManager();

    // 扫描目录中的压缩文件
    std::vector<ArchiveFile> scanArchiveFiles(const std::string& directory);

    // 解压文件（同步）
    bool extractArchive(const std::string& archive_path, const std::string& extract_path);

    // 异步解压文件（带进度回调）
    void extractArchiveAsync(const std::string& archive_path, const std::string& extract_path,
                             std::function<void(float)> on_progress,
                             std::function<void(bool, const std::string&)> on_complete);

    // 检查是否正在解压
    bool isExtracting() const {
        return extracting_.load();
    }

    // 取消解压
    void cancelExtraction();

    // 检查文件是否为压缩文件
    static bool isArchiveFile(const std::string& filepath);

    // 获取压缩文件类型
    static std::string getArchiveType(const std::string& filepath);

  private:
    // 使用系统命令解压
    bool extractWithCommand(const std::string& archive_path, const std::string& extract_path,
                            const std::string& type);

    // 检查命令是否存在
    bool commandExists(const std::string& command);

    // 异步解压线程
    std::thread extract_thread_;
    std::atomic<bool> extracting_;
    std::atomic<bool> cancel_requested_;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_EXTRACT_H
