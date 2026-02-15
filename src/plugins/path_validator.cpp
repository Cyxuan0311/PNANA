#ifdef BUILD_LUA_SUPPORT

#include "plugins/path_validator.h"
#include "utils/logger.h"
#include <algorithm>

namespace fs = std::filesystem;

namespace pnana {
namespace plugins {

// 系统目录列表（禁止访问）
const std::vector<std::string> PathValidator::system_directories_ = {
    "/bin", "/sbin", "/usr/bin", "/usr/sbin", "/usr/local/bin",
    "/etc", "/sys", "/proc", "/dev", "/boot", "/root",
    "/usr/lib", "/usr/lib64", "/lib", "/lib64",
    "/var/log", "/var/run", "/var/tmp"
};

PathValidator::PathValidator() {}

PathValidator::~PathValidator() {}

void PathValidator::setAllowedPaths(const std::vector<std::string>& paths) {
    allowed_paths_.clear();
    for (const auto& path : paths) {
        addAllowedPath(path);
    }
}

void PathValidator::addAllowedPath(const std::string& path) {
    try {
        if (fs::exists(path)) {
            std::string normalized = normalizePath(path);
            if (!normalized.empty()) {
                // 确保路径以 / 结尾（用于前缀匹配）
                if (normalized.back() != '/') {
                    normalized += '/';
                }
                allowed_paths_.push_back(normalized);
            }
        } else {
            // 即使路径不存在，也添加（可能是相对路径）
            std::string normalized = fs::absolute(path).lexically_normal().string();
            if (!normalized.empty()) {
                if (normalized.back() != '/') {
                    normalized += '/';
                }
                allowed_paths_.push_back(normalized);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to add allowed path: " + path + " - " + e.what());
    }
}

void PathValidator::setWorkingDirectory(const std::string& dir) {
    try {
        if (fs::exists(dir) && fs::is_directory(dir)) {
            working_directory_ = normalizePath(dir);
            if (!working_directory_.empty() && working_directory_.back() != '/') {
                working_directory_ += '/';
            }
        } else {
            working_directory_ = fs::absolute(dir).string();
            if (!working_directory_.empty() && working_directory_.back() != '/') {
                working_directory_ += '/';
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to set working directory: " + dir + " - " + e.what());
        working_directory_.clear();
    }
}

std::string PathValidator::normalizePath(const std::string& path) const {
    try {
        fs::path p(path);
        
        // 如果是相对路径，先转换为绝对路径
        if (!p.is_absolute()) {
            if (!working_directory_.empty()) {
                p = fs::path(working_directory_) / p;
            } else {
                p = fs::absolute(p);
            }
        }
        
        // 规范化路径（解析符号链接、. 和 ..）
        if (fs::exists(p)) {
            return fs::canonical(p).string();
        } else {
            // 如果路径不存在，仍然规范化父目录
            return fs::absolute(p).lexically_normal().string();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to normalize path: " + path + " - " + e.what());
        return "";
    }
}

bool PathValidator::isSystemDirectory(const std::string& path) const {
    std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return true; // 无法规范化的路径视为系统目录
    }
    
    // 确保路径以 / 结尾用于比较
    if (normalized.back() != '/') {
        normalized += '/';
    }
    
    // 检查是否匹配任何系统目录
    for (const auto& sys_dir : system_directories_) {
        if (normalized.find(sys_dir + '/') == 0 || normalized == sys_dir + '/') {
            return true;
        }
    }
    
    return false;
}

bool PathValidator::isPathInAllowedList(const std::string& normalized_path) const {
    // 如果路径为空，拒绝访问
    if (normalized_path.empty()) {
        return false;
    }
    
    // 确保路径以 / 结尾用于前缀匹配
    std::string path_with_slash = normalized_path;
    if (path_with_slash.back() != '/') {
        path_with_slash += '/';
    }
    
    // 检查是否在任何允许的路径前缀下
    for (const auto& allowed : allowed_paths_) {
        if (path_with_slash.find(allowed) == 0) {
            return true;
        }
    }
    
    // 检查工作目录
    if (!working_directory_.empty()) {
        if (path_with_slash.find(working_directory_) == 0) {
            return true;
        }
    }
    
    return false;
}

bool PathValidator::isPathAllowed(const std::string& path) const {
    // 首先检查是否为系统目录
    if (isSystemDirectory(path)) {
        return false;
    }
    
    // 规范化路径
    std::string normalized = normalizePath(path);
    if (normalized.empty()) {
        return false; // 无法规范化的路径拒绝访问
    }
    
    // 检查是否在允许的路径列表中
    return isPathInAllowedList(normalized);
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

