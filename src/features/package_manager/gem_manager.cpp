#include "features/package_manager/gem_manager.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace pnana {
namespace features {
namespace package_manager {

GemManager::GemManager() {
    cache_entry_.is_fetching = false;
}

std::string GemManager::getName() const {
    return "gem";
}

std::string GemManager::getDisplayName() const {
    return "RubyGems";
}

bool GemManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return !cache_entry_.packages.empty() && (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

std::vector<Package> GemManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (isCacheValid()) {
        return cache_entry_.packages;
    }

    return fetchPackagesFromSystem();
}

void GemManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    fetchPackagesFromSystem();
}

bool GemManager::hasError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return !cache_entry_.error_message.empty();
}

std::string GemManager::getError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.error_message;
}

bool GemManager::isFetching() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.is_fetching;
}

void GemManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.error_message.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::time_point();
}

bool GemManager::isAvailable() const {
    std::string command = "gem --version 2>/dev/null";
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

std::vector<Package> GemManager::fetchPackagesFromSystem() {
    cache_entry_.is_fetching = true;
    cache_entry_.error_message.clear();

    std::string command = "gem list --local";
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
        cache_entry_.error_message = "Failed to execute gem command";
        cache_entry_.is_fetching = false;
        return {};
    }

    std::vector<Package> packages = parseGemOutput(output);
    cache_entry_.packages = packages;
    cache_entry_.timestamp = std::chrono::steady_clock::now();
    cache_entry_.is_fetching = false;

    return packages;
}

std::vector<Package> GemManager::parseGemOutput(const std::string& output) {
    std::vector<Package> packages;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        // 解析 gem list 输出格式，例如："rails (7.0.4, 6.1.7.3)"
        size_t name_end = line.find(' ');
        if (name_end == std::string::npos)
            continue;

        std::string name = line.substr(0, name_end);
        size_t version_start = line.find('(');
        size_t version_end = line.find(')');
        if (version_start == std::string::npos || version_end == std::string::npos)
            continue;

        std::string version_str = line.substr(version_start + 1, version_end - version_start - 1);
        // 取第一个版本号
        size_t comma_pos = version_str.find(',');
        std::string version =
            comma_pos != std::string::npos ? version_str.substr(0, comma_pos) : version_str;

        Package pkg;
        pkg.name = name;
        pkg.version = version;
        pkg.location = "local";
        pkg.description = "Ruby gem";
        pkg.status = "installed";
        packages.push_back(pkg);
    }

    return packages;
}

bool GemManager::updatePackage(const std::string& package_name) {
    std::string command = "gem update " + package_name;
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

bool GemManager::updateAllDependencies(const std::string& package_name) {
    std::string command = "gem update " + package_name;
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

bool GemManager::removePackage(const std::string& package_name) {
    std::string command = "gem uninstall -a -x " + package_name;
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

bool GemManager::installPackage(const std::string& package_name) {
    std::string command = "gem install " + package_name;
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