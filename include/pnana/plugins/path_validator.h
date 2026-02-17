#ifndef PNANA_PLUGINS_PATH_VALIDATOR_H
#define PNANA_PLUGINS_PATH_VALIDATOR_H

#ifdef BUILD_LUA_SUPPORT

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace pnana {
namespace plugins {

// 路径验证器 - 用于沙盒环境中的文件系统访问控制
class PathValidator {
  public:
    PathValidator();
    ~PathValidator();

    // 设置允许的路径前缀列表
    void setAllowedPaths(const std::vector<std::string>& paths);

    // 添加允许的路径前缀
    void addAllowedPath(const std::string& path);

    // 设置当前工作目录（允许访问的目录）
    void setWorkingDirectory(const std::string& dir);

    // 验证路径是否允许访问
    // 返回 true 如果路径在允许的目录内
    bool isPathAllowed(const std::string& path) const;

    // 规范化路径（解析符号链接、相对路径等）
    // 返回规范化后的路径，如果失败返回空字符串
    std::string normalizePath(const std::string& path) const;

    // 检查路径是否为系统目录（禁止访问）
    bool isSystemDirectory(const std::string& path) const;

  private:
    std::vector<std::string> allowed_paths_;
    std::string working_directory_;

    // 检查路径是否在允许的前缀列表中
    bool isPathInAllowedList(const std::string& normalized_path) const;

    // 系统目录列表（禁止访问）
    static const std::vector<std::string> system_directories_;
};

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT

#endif // PNANA_PLUGINS_PATH_VALIDATOR_H
