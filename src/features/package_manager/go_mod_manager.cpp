#include "features/package_manager/go_mod_manager.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace pnana {
namespace features {
namespace package_manager {

GoModManager::GoModManager() {
    cache_entry_.is_fetching = false;
}

std::string GoModManager::getName() const {
    return "go_mod";
}

std::string GoModManager::getDisplayName() const {
    return "Go Modules";
}

bool GoModManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return !cache_entry_.packages.empty() && (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

std::vector<Package> GoModManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (isCacheValid()) {
        return cache_entry_.packages;
    }

    return fetchPackagesFromSystem();
}

void GoModManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    fetchPackagesFromSystem();
}

bool GoModManager::hasError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return !cache_entry_.error_message.empty();
}

std::string GoModManager::getError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.error_message;
}

bool GoModManager::isFetching() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.is_fetching;
}

void GoModManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.error_message.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::time_point();
}

bool GoModManager::isAvailable() const {
    std::string command = "go version 2>/dev/null";
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

std::vector<Package> GoModManager::fetchPackagesFromSystem() {
    cache_entry_.is_fetching = true;
    cache_entry_.error_message.clear();

    std::string command = "go list -m all";
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
        cache_entry_.error_message = "Failed to execute go command";
        cache_entry_.is_fetching = false;
        return {};
    }

    std::vector<Package> packages = parseGoModOutput(output);
    cache_entry_.packages = packages;
    cache_entry_.timestamp = std::chrono::steady_clock::now();
    cache_entry_.is_fetching = false;

    return packages;
}

std::vector<Package> GoModManager::parseGoModOutput(const std::string& output) {
    std::vector<Package> packages;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        // 解析 go list -m all 输出格式，例如："github.com/gin-gonic/gin v1.9.0"
        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos)
            continue;

        std::string name = line.substr(0, space_pos);
        std::string version = line.substr(space_pos + 1);

        Package pkg;
        pkg.name = name;
        pkg.version = version;
        pkg.location = "local";
        pkg.description = "Go module";
        pkg.status = "installed";
        packages.push_back(pkg);
    }

    return packages;
}

bool GoModManager::updatePackage(const std::string& package_name) {
    std::string command = "go get " + package_name + "@latest";
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

bool GoModManager::updateAllDependencies(const std::string& package_name) {
    std::string command = "go get " + package_name + "@latest";
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

bool GoModManager::removePackage(const std::string& package_name) {
    std::string command = "go get " + package_name + "@none";
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

bool GoModManager::installPackage(const std::string& package_name) {
    std::string command = "go get " + package_name;
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