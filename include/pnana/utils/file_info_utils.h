#ifndef PNANA_UTILS_FILE_INFO_UTILS_H
#define PNANA_UTILS_FILE_INFO_UTILS_H

#include <filesystem>
#include <string>

namespace pnana {
namespace utils {

/**
 * @brief 文件大小信息结构体
 */
struct FileSizeInfo {
    std::string formatted_size; // 格式化后的大小（如 "1.5 MB"）
    uintmax_t raw_size;         // 原始字节大小
    bool is_directory;          // 是否为目录
};

/**
 * @brief 文件权限信息结构体
 */
struct FilePermissionInfo {
    std::string type;        // 文件类型标识（'d' 表示目录，'-' 表示文件）
    std::string owner;       // 所有者权限（如 "rwx"）
    std::string group;       // 组权限（如 "r-x"）
    std::string others;      // 其他用户权限（如 "r--"）
    std::string full_string; // 完整的权限字符串（如 "-rwxr-xr--"）
};

/**
 * @brief 获取文件大小信息
 *
 * @param filepath 文件路径
 * @return FileSizeInfo 文件大小信息
 */
FileSizeInfo getFileSize(const std::string& filepath);

/**
 * @brief 获取文件权限信息
 *
 * @param filepath 文件路径
 * @return FilePermissionInfo 文件权限信息
 */
FilePermissionInfo getFilePermission(const std::string& filepath);

/**
 * @brief 格式化文件大小为人类可读的格式
 *
 * @param size 字节大小
 * @return std::string 格式化后的大小字符串
 */
std::string formatFileSize(uintmax_t size);

/**
 * @brief 将文件权限转换为字符串表示
 *
 * @param perm filesystem::perms 权限枚举
 * @param is_directory 是否为目录
 * @return FilePermissionInfo 权限信息结构体
 */
FilePermissionInfo permissionToString(std::filesystem::perms perm, bool is_directory);

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_FILE_INFO_UTILS_H
