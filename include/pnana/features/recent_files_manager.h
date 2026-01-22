#ifndef PNANA_FEATURES_RECENT_FILES_MANAGER_H
#define PNANA_FEATURES_RECENT_FILES_MANAGER_H

#include <deque>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace features {

// 最近文件管理器
class RecentFilesManager {
  public:
    static constexpr size_t MAX_RECENT_FILES = 8;

    RecentFilesManager();

    // 添加最近打开的文件
    void addFile(const std::string& filepath);

    // 获取最近文件的列表（最新的在前面）
    std::vector<std::string> getRecentFiles() const;

    // 检查文件是否在最近文件列表中
    bool isFileRecent(const std::string& filepath) const;

    // 清除所有最近文件记录
    void clearRecentFiles();

    // 获取最近文件数量
    size_t getRecentFilesCount() const;

    // 加载最近文件列表（从配置文件）
    void loadRecentFiles();

    // 保存最近文件列表（到配置文件）
    void saveRecentFiles();

    // 设置文件打开回调
    void setFileOpenCallback(std::function<void(const std::string&)> callback);

    // 打开指定索引的文件
    void openFile(size_t index);

  private:
    // 使用deque来保持插入顺序，最新文件在前
    std::deque<std::string> recent_files_;

    // 文件打开回调函数
    std::function<void(const std::string&)> file_open_callback_;

    // 移除重复文件（保持最新位置）
    void removeDuplicates(const std::string& filepath);

    // 获取配置目录路径
    std::filesystem::path getConfigDir() const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_RECENT_FILES_MANAGER_H
