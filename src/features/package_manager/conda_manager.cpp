#include "features/package_manager/conda_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

CondaManager::CondaManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> CondaManager::getInstalledPackages() {
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
            pnana::utils::Logger::getInstance().log("[CONDA] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    return cache_entry_.packages;
}

void CondaManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void CondaManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool CondaManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool CondaManager::isAvailable() const {
    std::string command = "which conda > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<Package> CondaManager::fetchPackagesFromSystem() {
    std::vector<Package> packages;
    std::string command = "conda list 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute conda command");
    }

    char buffer[4096];
    std::string output;
    output.reserve(1024 * 1024);
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int exit_code = pclose(pipe);

    if (exit_code != 0) {
        throw std::runtime_error("conda command failed with exit code " +
                                 std::to_string(exit_code));
    }

    packages = parseCondaListOutput(output);
    return packages;
}

std::vector<Package> CondaManager::parseCondaListOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(500);

    std::istringstream stream(output);
    std::string line;
    line.reserve(256);

    // conda list 输出格式：
    // # packages in environment at /path/to/env:
    // #
    // # Name                    Version                   Build  Channel
    // package1                  1.0.0                    py39_0  conda-forge
    // package2                  2.0.0                    py39_0  conda-forge

    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#' || line.find("Name") != std::string::npos) {
            continue;
        }

        std::istringstream line_stream(line);
        std::string name, version, build, channel;

        if (line_stream >> name >> version >> build >> channel) {
            Package pkg;
            pkg.name = name;
            pkg.version = version;
            pkg.location = channel.empty() ? "conda" : channel;
            packages.push_back(pkg);
        }
    }

    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool CondaManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // conda update 更新特定包
    std::string command = "conda update -y " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute conda command";
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

bool CondaManager::updateAllDependencies(const std::string& /*package_name*/) {
    // conda update --all 更新所有包
    std::string command = "conda update --all -y 2>&1";

    // 异步执行更新命令
    std::thread([this, command]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute conda command";
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

bool CondaManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // conda remove 删除包
    std::string command = "conda remove -y " + package_name + " 2>&1";

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute conda command";
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

bool CondaManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // conda install 安装包
    std::string command = "conda install -y " + package_name + " 2>&1";

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute conda command";
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
