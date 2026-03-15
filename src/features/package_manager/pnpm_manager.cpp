#include "features/package_manager/pnpm_manager.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace pnana {
namespace features {
namespace package_manager {

PnpmManager::PnpmManager() {
    cache_entry_.is_fetching = false;
}

std::string PnpmManager::getName() const {
    return "pnpm";
}

std::string PnpmManager::getDisplayName() const {
    return "pnpm";
}

bool PnpmManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return !cache_entry_.packages.empty() && (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

std::vector<Package> PnpmManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (isCacheValid()) {
        return cache_entry_.packages;
    }

    return fetchPackagesFromSystem();
}

void PnpmManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    fetchPackagesFromSystem();
}

bool PnpmManager::hasError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return !cache_entry_.error_message.empty();
}

std::string PnpmManager::getError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.error_message;
}

bool PnpmManager::isFetching() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.is_fetching;
}

void PnpmManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.error_message.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::time_point();
}

bool PnpmManager::isAvailable() const {
    std::string command = "pnpm --version 2>/dev/null";
    std::string output;
    bool success = false;

    if (remote_executor_) {
        auto result = remote_executor_(command);
        success = result.first;
        output = result.second;
    } else {
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                output += buffer;
            }
            pclose(pipe);
            success = true;
        }
    }

    return success && !output.empty();
}

std::vector<Package> PnpmManager::fetchPackagesFromSystem() {
    cache_entry_.is_fetching = true;
    cache_entry_.error_message.clear();

    std::string command = "pnpm list --depth=0 --json";
    std::string output;
    bool success = false;

    if (remote_executor_) {
        auto result = remote_executor_(command);
        success = result.first;
        output = result.second;
    } else {
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                output += buffer;
            }
            pclose(pipe);
            success = true;
        }
    }

    if (!success || output.empty()) {
        cache_entry_.error_message = "Failed to execute pnpm command";
        cache_entry_.is_fetching = false;
        return {};
    }

    std::vector<Package> packages = parsePnpmOutput(output);
    cache_entry_.packages = packages;
    cache_entry_.timestamp = std::chrono::steady_clock::now();
    cache_entry_.is_fetching = false;

    return packages;
}

std::vector<Package> PnpmManager::parsePnpmOutput(const std::string& output) {
    std::vector<Package> packages;

    // 简单的JSON解析（实际项目中应该使用JSON库）
    // 这里只做基本的解析，提取包名和版本
    std::string name, version;
    size_t start = 0;

    while (true) {
        size_t name_pos = output.find("name:", start);
        if (name_pos == std::string::npos)
            break;

        size_t name_start = output.find('"', name_pos + 7);
        size_t name_end = output.find('"', name_start + 1);
        if (name_start == std::string::npos || name_end == std::string::npos)
            break;
        name = output.substr(name_start + 1, name_end - name_start - 1);

        size_t version_pos = output.find("version:", name_end);
        if (version_pos == std::string::npos)
            break;

        size_t version_start = output.find('"', version_pos + 9);
        size_t version_end = output.find('"', version_start + 1);
        if (version_start == std::string::npos || version_end == std::string::npos)
            break;
        version = output.substr(version_start + 1, version_end - version_start - 1);

        Package pkg;
        pkg.name = name;
        pkg.version = version;
        pkg.location = "local";
        pkg.description = "pnpm package";
        pkg.status = "installed";
        packages.push_back(pkg);

        start = version_end;
    }

    return packages;
}

bool PnpmManager::updatePackage(const std::string& package_name) {
    std::string command = "pnpm update " + package_name;
    std::string output;
    bool success = false;

    if (remote_executor_) {
        auto result = remote_executor_(command);
        success = result.first;
        output = result.second;
    } else {
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                output += buffer;
            }
            int status = pclose(pipe);
            success = (status == 0);
        }
    }

    if (success) {
        clearCache();
    }

    return success;
}

bool PnpmManager::updateAllDependencies(const std::string& package_name) {
    std::string command = "pnpm update " + package_name + " --depth=9999";
    std::string output;
    bool success = false;

    if (remote_executor_) {
        auto result = remote_executor_(command);
        success = result.first;
        output = result.second;
    } else {
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                output += buffer;
            }
            int status = pclose(pipe);
            success = (status == 0);
        }
    }

    if (success) {
        clearCache();
    }

    return success;
}

bool PnpmManager::removePackage(const std::string& package_name) {
    std::string command = "pnpm remove " + package_name;
    std::string output;
    bool success = false;

    if (remote_executor_) {
        auto result = remote_executor_(command);
        success = result.first;
        output = result.second;
    } else {
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                output += buffer;
            }
            int status = pclose(pipe);
            success = (status == 0);
        }
    }

    if (success) {
        clearCache();
    }

    return success;
}

bool PnpmManager::installPackage(const std::string& package_name) {
    std::string command = "pnpm add " + package_name;
    std::string output;
    bool success = false;

    if (remote_executor_) {
        auto result = remote_executor_(command);
        success = result.first;
        output = result.second;
    } else {
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                output += buffer;
            }
            int status = pclose(pipe);
            success = (status == 0);
        }
    }

    if (success) {
        clearCache();
    }

    return success;
}

} // namespace package_manager
} // namespace features
} // namespace pnana