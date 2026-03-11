#include "features/package_manager/npm_manager.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

NpmManager::NpmManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> NpmManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    if (isCacheValid() && !cache_entry_.packages.empty())
        return cache_entry_.packages;
    if (cache_entry_.is_fetching)
        return cache_entry_.packages;
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

void NpmManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void NpmManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool NpmManager::isCacheValid() const {
    return (std::chrono::steady_clock::now() - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool NpmManager::isAvailable() const {
    if (remote_executor_) {
        auto [ok, out] = remote_executor_("which npm 2>/dev/null");
        return ok && !out.empty();
    }
    return system("which npm > /dev/null 2>&1") == 0;
}

std::vector<Package> NpmManager::fetchPackagesFromSystem() {
    std::string output;
    if (remote_executor_) {
        auto [ok, out] = remote_executor_("npm list --depth=0 --json=false 2>&1");
        if (!ok && (out.find("ENOENT") != std::string::npos ||
                    out.find("No such file") != std::string::npos))
            return {};
        output = out;
    } else {
        FILE* pipe = popen("npm list --depth=0 --json=false 2>&1", "r");
        if (!pipe)
            throw std::runtime_error("Failed to execute npm command");
        char buffer[4096];
        output.reserve(512 * 1024);
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        if (exit_code != 0 && output.find("npm ERR!") != std::string::npos) {
            if (output.find("ENOENT") != std::string::npos ||
                output.find("No such file") != std::string::npos)
                return {};
        }
    }
    return parseNpmListOutput(output);
}

std::vector<Package> NpmManager::parseNpmListOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(100);
    std::istringstream stream(output);
    std::string line;
    line.reserve(256);
    bool skip_first = true;
    while (std::getline(stream, line)) {
        if (line.empty() || line.find("npm ERR!") != std::string::npos ||
            line.find("extraneous") != std::string::npos ||
            line.find("missing") != std::string::npos)
            continue;
        if (skip_first && line.find("@") != std::string::npos &&
            (line.find("├──") == std::string::npos && line.find("└──") == std::string::npos)) {
            skip_first = false;
            continue;
        }
        size_t at_pos = line.find("@");
        if (at_pos == std::string::npos)
            continue;
        size_t name_start = 0;
        while (name_start < at_pos) {
            if (line[name_start] == ' ') {
                name_start++;
            } else if (line.find("├──", name_start) == name_start ||
                       line.find("└──", name_start) == name_start) {
                name_start += 3;
            } else if (line.find("│", name_start) == name_start) {
                name_start += 3;
            } else {
                break;
            }
        }
        if (name_start >= at_pos)
            continue;
        std::string name = line.substr(name_start, at_pos - name_start);
        size_t version_start = at_pos + 1;
        size_t version_end = version_start;
        while (version_end < line.length() && line[version_end] != ' ' &&
               line[version_end] != '\t' && line[version_end] != '\n' && line[version_end] != '\r')
            version_end++;
        if (version_end > version_start) {
            Package pkg;
            pkg.name = name;
            pkg.version = line.substr(version_start, version_end - version_start);
            pkg.location = "npm";
            packages.push_back(pkg);
        }
    }
    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });
    return packages;
}

bool NpmManager::updatePackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "npm update " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute npm command";
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

bool NpmManager::updateAllDependencies(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "npm update " + package_name + " 2>&1";
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
                    "Failed to update package and dependencies: " + package_name + " - " + out;
            }
        }).detach();
        return true;
    }
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute npm command";
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
            this->cache_entry_.error_message =
                "Failed to update package and dependencies: " + package_name;
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

bool NpmManager::removePackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "npm uninstall " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute npm command";
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

bool NpmManager::installPackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "npm install " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute npm command";
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
