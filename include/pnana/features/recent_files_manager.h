#ifndef PNANA_FEATURES_RECENT_FILES_MANAGER_H
#define PNANA_FEATURES_RECENT_FILES_MANAGER_H

#include <deque>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace features {

// 项目类型枚举
enum class ProjectType { FILE, FOLDER };

// 项目信息结构体
struct ProjectItem {
    ProjectType type;
    std::string path;

    ProjectItem(ProjectType t, const std::string& p) : type(t), path(p) {}
    bool operator==(const ProjectItem& other) const {
        return type == other.type && path == other.path;
    }
};

// 最近文件管理器（现在支持文件和文件夹）
class RecentFilesManager {
  public:
    static constexpr size_t MAX_RECENT_FILES = 8;

    RecentFilesManager();

    // 添加最近打开的文件
    void addFile(const std::string& filepath);

    // 添加最近打开的文件夹
    void addFolder(const std::string& folderpath);

    // 获取最近项目的列表（最新的在前面）
    std::vector<ProjectItem> getRecentProjects() const;

    // 获取最近文件的列表（为了向后兼容）
    std::vector<std::string> getRecentFiles() const;

    // 获取最近文件项目列表（返回 ProjectItem）
    std::vector<ProjectItem> getRecentFileItems() const;

    // 获取最近文件夹项目列表
    std::vector<ProjectItem> getRecentFolderItems() const;

    // 检查文件是否在最近项目列表中
    bool isFileRecent(const std::string& filepath) const;

    // 检查文件夹是否在最近项目列表中
    bool isFolderRecent(const std::string& folderpath) const;

    // 清除所有最近项目记录
    void clearRecentFiles();

    // 获取最近项目数量
    size_t getRecentFilesCount() const;

    // 加载最近文件列表（从配置文件）
    void loadRecentFiles();

    // 保存最近文件列表（到配置文件）
    void saveRecentFiles();

    // 设置文件打开回调
    void setFileOpenCallback(std::function<void(const std::string&)> callback);

    // 设置文件夹打开回调
    void setFolderOpenCallback(std::function<void(const std::string&)> callback);

    // 打开指定索引的项目
    void openProject(size_t index);

    // 打开指定索引的文件（为了向后兼容）
    void openFile(size_t index);

  private:
    // 使用deque来保持插入顺序，最新项目在前
    std::deque<ProjectItem> recent_projects_;

    // 文件打开回调函数
    std::function<void(const std::string&)> file_open_callback_;

    // 文件夹打开回调函数
    std::function<void(const std::string&)> folder_open_callback_;

    // 移除重复项目（保持最新位置）
    void removeDuplicates(const ProjectItem& item);

    // 获取配置目录路径
    std::filesystem::path getConfigDir() const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_RECENT_FILES_MANAGER_H
