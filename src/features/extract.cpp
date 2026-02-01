#include "features/extract.h"
#include "utils/archive_validator.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

namespace pnana {
namespace features {

ExtractManager::ExtractManager() : extracting_(false), cancel_requested_(false) {}

ExtractManager::~ExtractManager() {
    if (extracting_.load()) {
        cancelExtraction();
        if (extract_thread_.joinable()) {
            extract_thread_.join();
        }
    }
}

std::vector<ArchiveFile> ExtractManager::scanArchiveFiles(const std::string& directory) {
    std::vector<ArchiveFile> archives;

    try {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return archives;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                if (isArchiveFile(filepath)) {
                    std::string name = entry.path().filename().string();
                    std::string type = getArchiveType(filepath);
                    archives.emplace_back(name, filepath, type);
                }
            }
        }

        // 按名称排序
        std::sort(archives.begin(), archives.end(), [](const ArchiveFile& a, const ArchiveFile& b) {
            return a.name < b.name;
        });
    } catch (const std::exception&) {
        // 忽略错误，返回空列表
    }

    return archives;
}

bool ExtractManager::extractArchive(const std::string& archive_path,
                                    const std::string& extract_path) {
    if (!fs::exists(archive_path)) {
        return false;
    }

    // 创建目标目录（如果不存在）
    try {
        if (!fs::exists(extract_path)) {
            fs::create_directories(extract_path);
        }
    } catch (const std::exception&) {
        return false;
    }

    std::string type = getArchiveType(archive_path);
    return extractWithCommand(archive_path, extract_path, type);
}

bool ExtractManager::isArchiveFile(const std::string& filepath) {
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // 支持的压缩文件扩展名
    static const std::vector<std::string> archive_extensions = {
        ".zip",    ".tar", ".gz",      ".bz2",  ".xz",     ".7z", ".rar",
        ".tar.gz", ".tgz", ".tar.bz2", ".tbz2", ".tar.xz", ".txz"};

    // 检查单个扩展名
    for (const auto& archive_ext : archive_extensions) {
        if (ext == archive_ext) {
            return true;
        }
    }

    // 检查组合扩展名（如 .tar.gz）
    std::string lower_path = filepath;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

    size_t len = lower_path.length();
    if ((len >= 7 && lower_path.substr(len - 7) == ".tar.gz") ||
        (len >= 4 && lower_path.substr(len - 4) == ".tgz")) {
        return true;
    }
    if ((len >= 8 && lower_path.substr(len - 8) == ".tar.bz2") ||
        (len >= 5 && lower_path.substr(len - 5) == ".tbz2")) {
        return true;
    }
    if ((len >= 7 && lower_path.substr(len - 7) == ".tar.xz") ||
        (len >= 4 && lower_path.substr(len - 4) == ".txz")) {
        return true;
    }

    return false;
}

std::string ExtractManager::getArchiveType(const std::string& filepath) {
    std::string lower_path = filepath;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);

    size_t len = lower_path.length();
    if (len >= 4 && lower_path.substr(len - 4) == ".zip") {
        return "zip";
    } else if ((len >= 7 && lower_path.substr(len - 7) == ".tar.gz") ||
               (len >= 4 && lower_path.substr(len - 4) == ".tgz")) {
        return "tar.gz";
    } else if ((len >= 8 && lower_path.substr(len - 8) == ".tar.bz2") ||
               (len >= 5 && lower_path.substr(len - 5) == ".tbz2")) {
        return "tar.bz2";
    } else if ((len >= 7 && lower_path.substr(len - 7) == ".tar.xz") ||
               (len >= 4 && lower_path.substr(len - 4) == ".txz")) {
        return "tar.xz";
    } else if (len >= 4 && lower_path.substr(len - 4) == ".tar") {
        return "tar";
    } else if (len >= 3 && lower_path.substr(len - 3) == ".gz") {
        return "gz";
    } else if (len >= 4 && lower_path.substr(len - 4) == ".bz2") {
        return "bz2";
    } else if (len >= 3 && lower_path.substr(len - 3) == ".xz") {
        return "xz";
    } else if (len >= 3 && lower_path.substr(len - 3) == ".7z") {
        return "7z";
    } else if (len >= 4 && lower_path.substr(len - 4) == ".rar") {
        return "rar";
    }

    return "unknown";
}

bool ExtractManager::extractWithCommand(const std::string& archive_path,
                                        const std::string& extract_path, const std::string& type) {
    // 验证文件是否为有效的压缩文件
    if (!utils::ArchiveValidator::validateArchive(archive_path, type)) {
        return false;
    }

    std::ostringstream cmd;

    if (type == "zip") {
        if (!commandExists("unzip")) {
            return false;
        }
        cmd << "unzip -q -o \"" << archive_path << "\" -d \"" << extract_path << "\"";
    } else if (type == "tar" || type == "tar.gz" || type == "tar.bz2" || type == "tar.xz") {
        if (!commandExists("tar")) {
            return false;
        }
        std::string tar_flags = "";
        if (type == "tar.gz" || type == "tgz") {
            tar_flags = "z";
        } else if (type == "tar.bz2" || type == "tbz2") {
            tar_flags = "j";
        } else if (type == "tar.xz" || type == "txz") {
            tar_flags = "J";
        }
        cmd << "tar -x" << tar_flags << "f \"" << archive_path << "\" -C \"" << extract_path
            << "\"";
    } else if (type == "gz") {
        if (!commandExists("gunzip")) {
            return false;
        }
        cmd << "gunzip -c \"" << archive_path << "\" > \""
            << (fs::path(extract_path) / fs::path(archive_path).stem()).string() << "\"";
    } else if (type == "bz2") {
        if (!commandExists("bunzip2")) {
            return false;
        }
        cmd << "bunzip2 -c \"" << archive_path << "\" > \""
            << (fs::path(extract_path) / fs::path(archive_path).stem()).string() << "\"";
    } else if (type == "xz") {
        if (!commandExists("unxz")) {
            return false;
        }
        cmd << "unxz -c \"" << archive_path << "\" > \""
            << (fs::path(extract_path) / fs::path(archive_path).stem()).string() << "\"";
    } else if (type == "7z") {
        if (!commandExists("7z")) {
            return false;
        }
        cmd << "7z x \"" << archive_path << "\" -o\"" << extract_path << "\" -y";
    } else if (type == "rar") {
        if (!commandExists("unrar")) {
            return false;
        }
        cmd << "unrar x \"" << archive_path << "\" \"" << extract_path << "\" -y";
    } else {
        return false;
    }

    // 执行命令
    int result = std::system(cmd.str().c_str());
    return (result == 0);
}

bool ExtractManager::commandExists(const std::string& command) {
    std::ostringstream cmd;
    cmd << "command -v " << command << " > /dev/null 2>&1";
    int result = std::system(cmd.str().c_str());
    return (result == 0);
}

void ExtractManager::extractArchiveAsync(
    const std::string& archive_path, const std::string& extract_path,
    std::function<void(float)> on_progress,
    std::function<void(bool, const std::string&)> on_complete) {
    // 如果正在解压，先取消
    if (extracting_.load()) {
        cancelExtraction();
        if (extract_thread_.joinable()) {
            extract_thread_.join();
        }
    }

    extracting_.store(true);
    cancel_requested_.store(false);

    // 在后台线程中执行解压
    extract_thread_ = std::thread([this, archive_path, extract_path, on_progress, on_complete]() {
        // 模拟进度更新（因为系统命令不提供实时进度）
        std::vector<float> progress_steps = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f};

        // 创建目标目录
        try {
            if (!std::filesystem::exists(extract_path)) {
                std::filesystem::create_directories(extract_path);
            }
        } catch (const std::exception&) {
            extracting_.store(false);
            if (on_complete) {
                on_complete(false, "Failed to create extract directory");
            }
            return;
        }

        // 更新进度：准备阶段
        if (on_progress && !cancel_requested_.load()) {
            on_progress(0.05f);
        }

        std::string type = getArchiveType(archive_path);

        // 执行解压命令
        bool success = false;
        std::string error_msg;

        if (!cancel_requested_.load()) {
            // 模拟进度更新
            for (float progress : progress_steps) {
                if (cancel_requested_.load()) {
                    break;
                }
                if (on_progress) {
                    on_progress(progress);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // 实际执行解压
            if (!cancel_requested_.load()) {
                success = extractWithCommand(archive_path, extract_path, type);
                if (!success) {
                    error_msg = "Extraction command failed";
                }
            } else {
                error_msg = "Extraction cancelled";
            }
        } else {
            error_msg = "Extraction cancelled";
        }

        // 完成
        if (on_progress && !cancel_requested_.load()) {
            on_progress(1.0f);
        }

        extracting_.store(false);

        if (on_complete) {
            if (cancel_requested_.load()) {
                on_complete(false, "Extraction cancelled by user");
            } else {
                on_complete(success, success ? "" : error_msg);
            }
        }
    });
}

void ExtractManager::cancelExtraction() {
    cancel_requested_.store(true);
}

} // namespace features
} // namespace pnana
