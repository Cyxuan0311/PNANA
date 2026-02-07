#include "features/package_manager/brew_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

BrewManager::BrewManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> BrewManager::getInstalledPackages() {
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
            pnana::utils::Logger::getInstance().log("[BREW] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    return cache_entry_.packages;
}

void BrewManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void BrewManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool BrewManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool BrewManager::isAvailable() const {
    std::string command = "which brew > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<Package> BrewManager::fetchPackagesFromSystem() {
    std::vector<Package> packages;
    std::string command = "brew list --versions 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute brew command");
    }

    char buffer[4096];
    std::string output;
    output.reserve(512 * 1024);
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int exit_code = pclose(pipe);

    if (exit_code != 0) {
        throw std::runtime_error("brew command failed with exit code " + std::to_string(exit_code));
    }

    packages = parseBrewListOutput(output);
    return packages;
}

std::vector<Package> BrewManager::parseBrewListOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(500);

    std::istringstream stream(output);
    std::string line;
    line.reserve(256);

    // brew list --versions 输出格式：
    // package-name version
    // 或者多个版本：
    // package-name version1 version2

    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream line_stream(line);
        std::string name, version;

        if (line_stream >> name >> version) {
            Package pkg;
            pkg.name = name;
            pkg.version = version;
            pkg.location = "homebrew";
            packages.push_back(pkg);
        }
    }

    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool BrewManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // brew upgrade 更新特定包
    std::string command = "brew upgrade " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute brew command";
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
            this->cache_entry_.error_message = "Failed to update package: " + package_name;
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool BrewManager::updateAllDependencies(const std::string& /*package_name*/) {
    // brew upgrade 更新所有包
    std::string command = "brew upgrade 2>&1";

    // 异步执行更新命令
    std::thread([this, command]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute brew command";
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
            this->cache_entry_.error_message = "Failed to update all packages";
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool BrewManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // brew uninstall 删除包
    std::string command = "brew uninstall " + package_name + " 2>&1";

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute brew command";
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
            this->cache_entry_.error_message = "Failed to remove package: " + package_name;
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool BrewManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // brew install 安装包
    std::string command = "brew install " + package_name + " 2>&1";

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute brew command";
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
            this->cache_entry_.error_message = "Failed to install package: " + package_name;
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

} // namespace package_manager
} // namespace features
} // namespace pnana
