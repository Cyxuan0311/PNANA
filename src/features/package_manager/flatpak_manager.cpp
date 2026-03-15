#include "features/package_manager/flatpak_manager.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

FlatpakManager::FlatpakManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::string FlatpakManager::getName() const {
    return "flatpak";
}

std::string FlatpakManager::getDisplayName() const {
    return "Flatpak";
}

std::vector<Package> FlatpakManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查缓存是否有效
    if (isCacheValid() && !cache_entry_.packages.empty()) {
        return cache_entry_.packages;
    }

    // 如果正在获取中，返回当前缓存的数据（如果有）
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
        }
    }).detach();

    return cache_entry_.packages;
}

void FlatpakManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // 使缓存失效
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool FlatpakManager::hasError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return !cache_entry_.error_message.empty();
}

std::string FlatpakManager::getError() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.error_message;
}

bool FlatpakManager::isFetching() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_entry_.is_fetching;
}

void FlatpakManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool FlatpakManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool FlatpakManager::isAvailable() const {
    if (remote_executor_) {
        auto [ok, out] = remote_executor_("which flatpak 2>/dev/null");
        return ok && !out.empty();
    }
    return system("which flatpak > /dev/null 2>&1") == 0;
}

std::vector<Package> FlatpakManager::fetchPackagesFromSystem() {
    std::string output;
    if (remote_executor_) {
        auto [ok, out] =
            remote_executor_("flatpak list --columns=name,version,application,description 2>&1");
        if (!ok)
            throw std::runtime_error("flatpak command failed on remote: " + out);
        output = out;
    } else {
        FILE* pipe = popen("flatpak list --columns=name,version,application,description 2>&1", "r");
        if (!pipe)
            throw std::runtime_error("Failed to execute flatpak command");
        char buffer[4096];
        output.reserve(4 * 1024 * 1024);
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        if (exit_code != 0)
            throw std::runtime_error("flatpak command failed with exit code " +
                                     std::to_string(exit_code));
    }
    return parseFlatpakOutput(output);
}

std::vector<Package> FlatpakManager::parseFlatpakOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(1000); // 预分配空间

    std::istringstream stream(output);
    std::string line;
    line.reserve(256); // 预分配行缓冲区
    bool is_first_line = true;

    while (std::getline(stream, line)) {
        // 跳过标题行
        if (is_first_line) {
            is_first_line = false;
            continue;
        }

        if (line.empty()) {
            continue;
        }

        // 解析包信息
        std::istringstream line_stream(line);
        std::string name, version, application, description;

        if (line_stream >> name >> version >> application) {
            Package pkg;
            pkg.name = name;
            pkg.version = version;
            pkg.location = application;

            // 获取描述（剩余部分）
            std::string rest;
            if (std::getline(line_stream, rest)) {
                // 移除前导空格
                rest.erase(0, rest.find_first_not_of(" \t"));
                if (!rest.empty()) {
                    pkg.description = rest;
                }
            }

            pkg.status = "installed";
            packages.push_back(pkg);
        }
    }

    // 按名称排序
    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool FlatpakManager::updatePackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "flatpak update -y " + package_name + " 2>&1";
    if (remote_executor_) {
        auto exec = remote_executor_;
        std::thread([this, exec, command, package_name]() {
            auto [ok, out] = exec(command);
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            if (ok) {
                this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
                this->cache_entry_.error_message.clear();
            } else {
                this->cache_entry_.error_message =
                    "Failed to update package: " + package_name + " - " + out;
            }
        }).detach();
        return true;
    }
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute flatpak command";
            return;
        }
        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message = "Failed to update package: " + package_name;
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

bool FlatpakManager::updateAllDependencies(const std::string& /*package_name*/) {
    std::string command = "flatpak update -y 2>&1";
    if (remote_executor_) {
        auto exec = remote_executor_;
        std::thread([this, exec, command]() {
            auto [ok, out] = exec(command);
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            if (ok) {
                this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
                this->cache_entry_.error_message.clear();
            } else {
                this->cache_entry_.error_message = "Failed to update all packages - " + out;
            }
        }).detach();
        return true;
    }
    std::thread([this, command]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute flatpak command";
            return;
        }
        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message = "Failed to update all packages";
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

bool FlatpakManager::removePackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "flatpak uninstall -y " + package_name + " 2>&1";
    if (remote_executor_) {
        auto exec = remote_executor_;
        std::thread([this, exec, command, package_name]() {
            auto [ok, out] = exec(command);
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            if (ok) {
                this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
                this->cache_entry_.error_message.clear();
            } else {
                this->cache_entry_.error_message =
                    "Failed to remove package: " + package_name + " - " + out;
            }
        }).detach();
        return true;
    }
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute flatpak command";
            return;
        }
        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message = "Failed to remove package: " + package_name;
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

bool FlatpakManager::installPackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "flatpak install -y " + package_name + " 2>&1";
    if (remote_executor_) {
        auto exec = remote_executor_;
        std::thread([this, exec, command, package_name]() {
            auto [ok, out] = exec(command);
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            if (ok) {
                this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
                this->cache_entry_.error_message.clear();
            } else {
                this->cache_entry_.error_message =
                    "Failed to install package: " + package_name + " - " + out;
            }
        }).detach();
        return true;
    }
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute flatpak command";
            return;
        }
        char buffer[4096];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        if (exit_code == 0) {
            this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
            this->cache_entry_.error_message.clear();
        } else {
            this->cache_entry_.error_message = "Failed to install package: " + package_name;
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

} // namespace package_manager
} // namespace features
} // namespace pnana