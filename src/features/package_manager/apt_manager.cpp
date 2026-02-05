#include "features/package_manager/apt_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

AptManager::AptManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> AptManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查缓存是否有效
    if (isCacheValid() && !cache_entry_.packages.empty()) {
        return cache_entry_.packages;
    }

    // 如果正在获取中，返回当前缓存的数据（如果有）
    if (cache_entry_.is_fetching) {
        return cache_entry_.packages;
    }

    // 标记为正在获取
    cache_entry_.is_fetching = true;
    cache_entry_.error_message.clear();

    // 在后台线程异步获取包列表
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
            pnana::utils::Logger::getInstance().log("[APT] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    // 返回当前缓存的数据（如果有）
    return cache_entry_.packages;
}

void AptManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // 使缓存失效
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void AptManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool AptManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool AptManager::isAvailable() const {
    // 检查 dpkg 命令是否可用（apt 系统的基础）
    std::string command = "which dpkg > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<Package> AptManager::fetchPackagesFromSystem() {
    // 使用 dpkg -l 获取已安装的包列表
    // dpkg -l 输出格式：
    // ii  package-name  version  architecture  description
    std::string command = "dpkg -l 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute dpkg command");
    }

    char buffer[4096]; // 增大缓冲区，减少系统调用次数
    std::string output;
    output.reserve(4 * 1024 * 1024); // 预分配4MB空间（apt包列表通常很大）
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int exit_code = pclose(pipe);

    if (exit_code != 0) {
        throw std::runtime_error("dpkg command failed with exit code " + std::to_string(exit_code));
    }

    auto packages = parseDpkgOutput(output);
    return packages;
}

std::vector<Package> AptManager::parseDpkgOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(5000); // 预分配空间，apt系统通常有数千个包

    std::istringstream stream(output);
    std::string line;
    line.reserve(256); // 预分配行缓冲区
    bool is_first_line = true;

    // dpkg -l 输出格式：
    // Desired=Unknown/Install/Remove/Purge/Hold
    // | Status=Not/Inst/Conf-files/Unpacked/halF-conf/Half-inst/trig-aWait/Trig-pend
    // |/ Err?=(none)/Reinst-required (Status,Err: uppercase=bad)
    // ||/ Name           Version      Architecture Description
    // +++-==============-============-============-=================================
    // ii  package-name   1.0.0-1     amd64        Description text

    while (std::getline(stream, line)) {
        // 跳过标题行和分隔行
        if (is_first_line || line.find("+++") != std::string::npos || line.empty() ||
            line.find("Desired=") != std::string::npos ||
            line.find("| Status=") != std::string::npos ||
            line.find("||/ Name") != std::string::npos) {
            is_first_line = false;
            continue;
        }

        // 解析包信息
        // 格式：ii  package-name  version  architecture  description
        // 状态码：ii = 已安装且配置完成
        if (line.length() < 5) {
            continue;
        }

        std::string status = line.substr(0, 2);
        // 只处理已安装的包（ii, iU, iH, iF 等）
        if (status[0] != 'i' && status[0] != 'r') {
            continue;
        }

        // 跳过状态码和空格
        size_t start = 4;
        while (start < line.length() && line[start] == ' ') {
            start++;
        }

        if (start >= line.length()) {
            continue;
        }

        // 解析包名、版本、架构、描述
        std::istringstream line_stream(line.substr(start));
        std::string name, version, architecture;

        if (line_stream >> name >> version >> architecture) {
            Package pkg;
            pkg.name = name;
            pkg.version = version;
            pkg.status = status;

            // 获取描述（剩余部分）
            std::string rest;
            if (std::getline(line_stream, rest)) {
                // 移除前导空格
                rest.erase(0, rest.find_first_not_of(" \t"));
                if (!rest.empty()) {
                    pkg.description = rest;
                }
            }

            // 架构信息可以放在 location 字段
            if (!architecture.empty()) {
                pkg.location = architecture;
            }

            packages.push_back(pkg);
        }
    }

    // 按名称排序
    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool AptManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // apt-get install --only-upgrade 需要 sudo 权限
    std::string command = "sudo apt-get install --only-upgrade -y " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute apt-get command (may need sudo)";
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

bool AptManager::updateAllDependencies(const std::string& /*package_name*/) {
    // apt-get upgrade 更新所有包
    std::string command = "sudo apt-get upgrade -y 2>&1";

    // 异步执行更新命令
    std::thread([this, command]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute apt-get command (may need sudo)";
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

bool AptManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // apt-get remove 需要 sudo 权限
    std::string command = "sudo apt-get remove -y " + package_name + " 2>&1";

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute apt-get command (may need sudo)";
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

bool AptManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // apt-get install 需要 sudo 权限
    std::string command = "sudo apt-get install -y " + package_name + " 2>&1";

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute apt-get command (may need sudo)";
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
