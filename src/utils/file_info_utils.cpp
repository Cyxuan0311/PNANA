#include "utils/file_info_utils.h"
#include <iomanip>
#include <sstream>

namespace pnana {
namespace utils {

std::string formatFileSize(uintmax_t size) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);

    if (size >= 1024ULL * 1024 * 1024) {
        // GB
        double gb = static_cast<double>(size) / (1024.0 * 1024.0 * 1024.0);
        // 如果是整数，去掉小数部分
        if (gb == static_cast<double>(static_cast<uintmax_t>(gb))) {
            oss << std::setprecision(0);
        }
        oss << gb << " GB";
    } else if (size >= 1024 * 1024) {
        // MB
        double mb = static_cast<double>(size) / (1024.0 * 1024.0);
        if (mb == static_cast<double>(static_cast<uintmax_t>(mb))) {
            oss << std::setprecision(0);
        }
        oss << mb << " MB";
    } else if (size >= 1024) {
        // KB
        double kb = static_cast<double>(size) / 1024.0;
        if (kb == static_cast<double>(static_cast<uintmax_t>(kb))) {
            oss << std::setprecision(0);
        }
        oss << kb << " KB";
    } else {
        // Bytes
        oss << std::setprecision(0);
        oss << size << " B";
    }

    return oss.str();
}

FileSizeInfo getFileSize(const std::string& filepath) {
    FileSizeInfo info;
    info.raw_size = 0;
    info.is_directory = false;
    info.formatted_size = "Unknown";

    try {
        std::filesystem::file_status status = std::filesystem::status(filepath);
        info.is_directory = std::filesystem::is_directory(status);

        if (info.is_directory) {
            info.formatted_size = "Folder";
            info.raw_size = 0;
        } else {
            info.raw_size = std::filesystem::file_size(filepath);
            info.formatted_size = formatFileSize(info.raw_size);
        }
    } catch (...) {
        // 如果获取失败，保持默认值
        info.formatted_size = "Unknown";
    }

    return info;
}

FilePermissionInfo permissionToString(std::filesystem::perms perm, bool is_directory) {
    FilePermissionInfo info;

    // 文件类型标识
    info.type = is_directory ? 'd' : '-';

    // 辅助 lambda：检查权限位
    auto check = [perm](std::filesystem::perms pm) {
        return (perm & pm) != std::filesystem::perms::none;
    };

    // 所有者权限
    info.owner += check(std::filesystem::perms::owner_read) ? 'r' : '-';
    info.owner += check(std::filesystem::perms::owner_write) ? 'w' : '-';
    info.owner += check(std::filesystem::perms::owner_exec) ? 'x' : '-';

    // 组权限
    info.group += check(std::filesystem::perms::group_read) ? 'r' : '-';
    info.group += check(std::filesystem::perms::group_write) ? 'w' : '-';
    info.group += check(std::filesystem::perms::group_exec) ? 'x' : '-';

    // 其他用户权限
    info.others += check(std::filesystem::perms::others_read) ? 'r' : '-';
    info.others += check(std::filesystem::perms::others_write) ? 'w' : '-';
    info.others += check(std::filesystem::perms::others_exec) ? 'x' : '-';

    // 完整的权限字符串
    info.full_string = info.type + info.owner + info.group + info.others;

    return info;
}

FilePermissionInfo getFilePermission(const std::string& filepath) {
    FilePermissionInfo info;
    info.type = "-";
    info.owner = "---";
    info.group = "---";
    info.others = "---";
    info.full_string = "---------";

    try {
        std::filesystem::file_status status = std::filesystem::status(filepath);
        std::filesystem::perms perm = status.permissions();
        bool is_directory = std::filesystem::is_directory(status);

        info = permissionToString(perm, is_directory);
    } catch (...) {
        // 如果获取失败，保持默认值
    }

    return info;
}

} // namespace utils
} // namespace pnana
