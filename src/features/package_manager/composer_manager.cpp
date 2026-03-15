#include "features/package_manager/composer_manager.h"
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace pnana {
namespace features {
namespace package_manager {

ComposerManager::ComposerManager() {
    cache_entry_.is_fetching = false;
}

std::string ComposerManager::getName() const {
    return "composer";
}

std::string ComposerManager::getDisplayName() const {
    return "Composer";
}

bool ComposerManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return !cache_entry_.packages.empty() && (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

std::vector<Package> ComposerManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (isCacheValid()) {
        return cache_entry_.packages;
    }

    return fetchPackagesFromSystem();
}

void ComposerManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    fetchPackagesFromSystem();
}

bool ComposerManager::hasError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return !cache_entry_.error_message.empty();
}

std::string ComposerManager::getError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.error_message;
}

bool ComposerManager::isFetching() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.is_fetching;
}

void ComposerManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.error_message.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::time_point();
}

bool ComposerManager::isAvailable() const {
    std::string command = "composer --version 2>/dev/null";
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

std::vector<Package> ComposerManager::fetchPackagesFromSystem() {
    cache_entry_.is_fetching = true;
    cache_entry_.error_message.clear();

    std::string command = "composer show --format=json";
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
        cache_entry_.error_message = "Failed to execute composer command";
        cache_entry_.is_fetching = false;
        return {};
    }

    std::vector<Package> packages = parseComposerOutput(output);
    cache_entry_.packages = packages;
    cache_entry_.timestamp = std::chrono::steady_clock::now();
    cache_entry_.is_fetching = false;

    return packages;
}

std::vector<Package> ComposerManager::parseComposerOutput(const std::string& output) {
    std::vector<Package> packages;

    // 简单的JSON解析（实际项目中应该使用JSON库）
    // 这里只做基本的解析，提取包名和版本
    std::string name, version, description;
    size_t start = 0;

    while (true) {
        size_t name_pos = output.find("name:", start);
        if (name_pos == std::string::npos)
            break;

        size_t name_start = output.find('"', name_pos + 6);
        size_t name_end = output.find('"', name_start + 1);
        if (name_start == std::string::npos || name_end == std::string::npos)
            break;
        name = output.substr(name_start + 1, name_end - name_start - 1);

        size_t version_pos = output.find("version:", name_end);
        if (version_pos == std::string::npos)
            break;

        size_t version_start = output.find('"', version_pos + 8);
        size_t version_end = output.find('"', version_start + 1);
        if (version_start == std::string::npos || version_end == std::string::npos)
            break;
        version = output.substr(version_start + 1, version_end - version_start - 1);

        size_t description_pos = output.find("description:", version_end);
        if (description_pos != std::string::npos) {
            size_t description_start = output.find('"', description_pos + 13);
            size_t description_end = output.find('"', description_start + 1);
            if (description_start != std::string::npos && description_end != std::string::npos) {
                description =
                    output.substr(description_start + 1, description_end - description_start - 1);
            }
        }

        Package pkg;
        pkg.name = name;
        pkg.version = version;
        pkg.location = "local";
        pkg.description = description.empty() ? "Composer package" : description;
        pkg.status = "installed";
        packages.push_back(pkg);

        start = version_end;
    }

    return packages;
}

bool ComposerManager::updatePackage(const std::string& package_name) {
    std::string command = "composer update " + package_name;
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

bool ComposerManager::updateAllDependencies(const std::string& package_name) {
    std::string command = "composer update " + package_name;
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

bool ComposerManager::removePackage(const std::string& package_name) {
    std::string command = "composer remove " + package_name;
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

bool ComposerManager::installPackage(const std::string& package_name) {
    std::string command = "composer require " + package_name;
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