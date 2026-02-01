#include "features/recent_files_manager.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace pnana {
namespace features {

RecentFilesManager::RecentFilesManager() {
    loadRecentFiles();
}

void RecentFilesManager::addFile(const std::string& filepath) {
    if (filepath.empty()) {
        return;
    }

    ProjectItem item(ProjectType::FILE, filepath);

    // 移除重复项目（如果存在）
    removeDuplicates(item);

    // 添加到队列前端
    recent_projects_.push_front(item);

    // 保持最大数量限制
    if (recent_projects_.size() > MAX_RECENT_FILES) {
        recent_projects_.pop_back();
    }

    // 保存到文件
    saveRecentFiles();
}

void RecentFilesManager::addFolder(const std::string& folderpath) {
    if (folderpath.empty()) {
        return;
    }

    ProjectItem item(ProjectType::FOLDER, folderpath);

    // 移除重复项目（如果存在）
    removeDuplicates(item);

    // 添加到队列前端
    recent_projects_.push_front(item);

    // 保持最大数量限制
    if (recent_projects_.size() > MAX_RECENT_FILES) {
        recent_projects_.pop_back();
    }

    // 保存到文件
    saveRecentFiles();
}

std::vector<ProjectItem> RecentFilesManager::getRecentProjects() const {
    return std::vector<ProjectItem>(recent_projects_.begin(), recent_projects_.end());
}

std::vector<std::string> RecentFilesManager::getRecentFiles() const {
    std::vector<std::string> files;
    for (const auto& item : recent_projects_) {
        if (item.type == ProjectType::FILE) {
            files.push_back(item.path);
        }
    }
    return files;
}

std::vector<ProjectItem> RecentFilesManager::getRecentFileItems() const {
    std::vector<ProjectItem> files;
    for (const auto& item : recent_projects_) {
        if (item.type == ProjectType::FILE) {
            files.push_back(item);
        }
    }
    return files;
}

std::vector<ProjectItem> RecentFilesManager::getRecentFolderItems() const {
    std::vector<ProjectItem> folders;
    for (const auto& item : recent_projects_) {
        if (item.type == ProjectType::FOLDER) {
            folders.push_back(item);
        }
    }
    return folders;
}

bool RecentFilesManager::isFileRecent(const std::string& filepath) const {
    ProjectItem target(ProjectType::FILE, filepath);
    return std::find(recent_projects_.begin(), recent_projects_.end(), target) !=
           recent_projects_.end();
}

bool RecentFilesManager::isFolderRecent(const std::string& folderpath) const {
    ProjectItem target(ProjectType::FOLDER, folderpath);
    return std::find(recent_projects_.begin(), recent_projects_.end(), target) !=
           recent_projects_.end();
}

void RecentFilesManager::clearRecentFiles() {
    recent_projects_.clear();
}

size_t RecentFilesManager::getRecentFilesCount() const {
    return recent_projects_.size();
}

void RecentFilesManager::setFileOpenCallback(std::function<void(const std::string&)> callback) {
    file_open_callback_ = callback;
}

void RecentFilesManager::setFolderOpenCallback(std::function<void(const std::string&)> callback) {
    folder_open_callback_ = callback;
}

void RecentFilesManager::openProject(size_t index) {
    if (index < recent_projects_.size()) {
        const auto& item = recent_projects_[index];
        if (item.type == ProjectType::FILE && file_open_callback_) {
            file_open_callback_(item.path);
        } else if (item.type == ProjectType::FOLDER && folder_open_callback_) {
            folder_open_callback_(item.path);
        }
    }
}

void RecentFilesManager::openFile(size_t index) {
    // 为了向后兼容，尝试找到第index个文件
    size_t file_count = 0;
    for (const auto& item : recent_projects_) {
        if (item.type == ProjectType::FILE) {
            if (file_count == index && file_open_callback_) {
                file_open_callback_(item.path);
                return;
            }
            file_count++;
        }
    }
}

void RecentFilesManager::removeDuplicates(const ProjectItem& item) {
    auto it = std::find(recent_projects_.begin(), recent_projects_.end(), item);
    if (it != recent_projects_.end()) {
        recent_projects_.erase(it);
    }
}

void RecentFilesManager::loadRecentFiles() {
    try {
        std::filesystem::path config_dir = getConfigDir();
        std::filesystem::path recent_file_path = config_dir / "recent_files.txt";

        if (!std::filesystem::exists(recent_file_path)) {
            return;
        }

        std::ifstream file(recent_file_path);
        if (!file.is_open()) {
            return;
        }

        recent_projects_.clear();
        std::string line;
        while (std::getline(file, line)) {
            // 跳过空行和注释行
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // 解析新格式：TYPE|PATH
            std::istringstream iss(line);
            std::string type_str, path;
            if (std::getline(iss, type_str, '|') && std::getline(iss, path)) {
                ProjectType type = (type_str == "FOLDER") ? ProjectType::FOLDER : ProjectType::FILE;

                // 检查项目是否仍然存在
                if (std::filesystem::exists(path)) {
                    recent_projects_.push_back(ProjectItem(type, path));
                }
            } else {
                // 向后兼容：如果没有分隔符，当作文件处理
                if (std::filesystem::exists(line)) {
                    recent_projects_.push_back(ProjectItem(ProjectType::FILE, line));
                }
            }

            // 限制数量
            if (recent_projects_.size() >= MAX_RECENT_FILES) {
                break;
            }
        }

        file.close();
    } catch (...) {
        // 忽略加载错误，使用空的最近文件列表
    }
}

void RecentFilesManager::saveRecentFiles() {
    try {
        std::filesystem::path config_dir = getConfigDir();

        // 确保配置目录存在
        std::filesystem::create_directories(config_dir);

        std::filesystem::path recent_file_path = config_dir / "recent_files.txt";
        std::ofstream file(recent_file_path);

        if (!file.is_open()) {
            return;
        }

        // 写入文件头注释
        file << "# Recent projects list for pnana editor" << std::endl;
        file << "# This file is automatically generated, do not edit manually" << std::endl;
        file << "# Format: TYPE|PATH" << std::endl;
        file << "# TYPE: FILE or FOLDER" << std::endl;
        file << std::endl;

        // 写入项目路径
        for (const auto& item : recent_projects_) {
            std::string type_str = (item.type == ProjectType::FILE) ? "FILE" : "FOLDER";
            file << type_str << "|" << item.path << std::endl;
        }

        file.close();
    } catch (...) {
        // 忽略保存错误
    }
}

std::filesystem::path RecentFilesManager::getConfigDir() const {
    const char* home = std::getenv("HOME");
    if (!home) {
        // 如果无法获取HOME，使用当前目录
        return std::filesystem::current_path() / ".pnana";
    }

    return std::filesystem::path(home) / ".config" / "pnana";
}

} // namespace features
} // namespace pnana
