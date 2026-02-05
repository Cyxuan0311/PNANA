#include "features/package_manager/yum_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

YumManager::YumManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> YumManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (isCacheValid() && !cache_entry_.packages.empty()) {
        return cache_entry_.packages;
    }

    if (cache_entry_.is_fetching) {
        return cache_entry_.packages;
    }

    cache_entry_.is_fetching = true;
    cache_entry_.error_message.clear();

    std::thread([this]() {
        try {
            auto packages = fetchPackagesFromSystem();
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.packages = packages;
            this->cache_entry_.timestamp = std::chrono::steady_clock::now();
            this->cache_entry_.is_fetching = false;
            this->cache_entry_.error_message.clear();
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.is_fetching = false;
            this->cache_entry_.error_message = std::string("Error fetching packages: ") + e.what();
            pnana::utils::Logger::getInstance().log("[YUM] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    return cache_entry_.packages;
}

void YumManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void YumManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool YumManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool YumManager::isAvailable() const {
    // 检查 rpm 命令是否可用（yum 的基础）
    std::string command = "which rpm > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<Package> YumManager::fetchPackagesFromSystem() {
    std::vector<Package> packages;
    std::string command = "rpm -qa 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute rpm command");
    }

    char buffer[4096];
    std::string output;
    output.reserve(4 * 1024 * 1024);
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int exit_code = pclose(pipe);

    if (exit_code != 0) {
        throw std::runtime_error("rpm command failed with exit code " + std::to_string(exit_code));
    }

    packages = parseRpmOutput(output);
    return packages;
}

std::vector<Package> YumManager::parseRpmOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(3000);

    std::istringstream stream(output);
    std::string line;
    line.reserve(256);

    // rpm -qa 输出格式：
    // package-name-version-release.architecture

    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        // 解析包名和版本
        // 格式：package-name-version-release.arch
        size_t last_dash = line.find_last_of('-');
        if (last_dash == std::string::npos) {
            continue;
        }

        // 查找版本号开始位置（最后一个数字或点之前）
        size_t version_start = last_dash + 1;
        size_t dot_pos = line.find('.', version_start);
        if (dot_pos == std::string::npos) {
            dot_pos = line.length();
        }

        std::string name = line.substr(0, last_dash);
        std::string version_release = line.substr(version_start, dot_pos - version_start);

        // 尝试分离版本和发布号（如果有）
        size_t release_dash = version_release.find_last_of('-');
        std::string version;
        if (release_dash != std::string::npos) {
            version = version_release.substr(0, release_dash);
        } else {
            version = version_release;
        }

        Package pkg;
        pkg.name = name;
        pkg.version = version;
        pkg.location = "rpm";
        packages.push_back(pkg);
    }

    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool YumManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // yum update 更新特定包（需要 sudo）
    std::string command = "sudo yum update -y " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yum command (may need sudo)";
            return;
        }

        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int exit_code = pclose(pipe);

        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            // 清除缓存，强制刷新
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message =
                "Failed to update package: " + package_name + " (may need sudo)";
            if (!output.empty() && output.find("Permission denied") == std::string::npos) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool YumManager::updateAllDependencies(const std::string& /*package_name*/) {
    // yum update 更新所有包（需要 sudo）
    std::string command = "sudo yum update -y 2>&1";

    // 异步执行更新命令
    std::thread([this, command]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yum command (may need sudo)";
            return;
        }

        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int exit_code = pclose(pipe);

        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            // 清除缓存，强制刷新
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message = "Failed to update all packages (may need sudo)";
            if (!output.empty() && output.find("Permission denied") == std::string::npos) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool YumManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // yum remove 删除包（需要 sudo）
    std::string command = "sudo yum remove -y " + package_name + " 2>&1";

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yum command (may need sudo)";
            return;
        }

        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int exit_code = pclose(pipe);

        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            // 清除缓存，强制刷新
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message =
                "Failed to remove package: " + package_name + " (may need sudo)";
            if (!output.empty() && output.find("Permission denied") == std::string::npos) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool YumManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // yum install 安装包（需要 sudo）
    std::string command = "sudo yum install -y " + package_name + " 2>&1";

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yum command (may need sudo)";
            return;
        }

        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int exit_code = pclose(pipe);

        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            // 清除缓存，强制刷新
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message =
                "Failed to install package: " + package_name + " (may need sudo)";
            if (!output.empty() && output.find("Permission denied") == std::string::npos) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

} // namespace package_manager
} // namespace features
} // namespace pnana
