#include "features/package_manager/cargo_manager.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

CargoManager::CargoManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> CargoManager::getInstalledPackages() {
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

void CargoManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void CargoManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool CargoManager::isCacheValid() const {
    return (std::chrono::steady_clock::now() - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool CargoManager::isAvailable() const {
    if (remote_executor_) {
        auto [ok, out] = remote_executor_("which cargo 2>/dev/null");
        return ok && !out.empty();
    }
    return system("which cargo > /dev/null 2>&1") == 0;
}

std::vector<Package> CargoManager::fetchPackagesFromSystem() {
    std::string output;
    if (remote_executor_) {
        auto [ok, out] = remote_executor_("cargo tree --depth 0 2>&1");
        if (!ok && out.find("could not find `Cargo.toml`") != std::string::npos)
            return {};
        output = out;
    } else {
        FILE* pipe = popen("cargo tree --depth 0 2>&1", "r");
        if (!pipe)
            throw std::runtime_error("Failed to execute cargo command");
        char buffer[4096];
        output.reserve(512 * 1024);
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        if (exit_code != 0 &&
            output.find("error: could not find `Cargo.toml`") != std::string::npos)
            return {};
        if (exit_code != 0 && output.find("error:") != std::string::npos &&
            output.find("could not find") == std::string::npos)
            throw std::runtime_error("cargo command failed with exit code " +
                                     std::to_string(exit_code));
    }
    return parseCargoTreeOutput(output);
}

std::vector<Package> CargoManager::parseCargoTreeOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(100);
    std::istringstream stream(output);
    std::string line;
    line.reserve(256);
    bool skip_first = true;
    while (std::getline(stream, line)) {
        if (line.empty() || line.find("error:") != std::string::npos)
            continue;
        if (skip_first) {
            if (line.find("├──") == std::string::npos && line.find("└──") == std::string::npos) {
                skip_first = false;
                continue;
            }
        }
        bool is_dependency =
            (line.find("├──") != std::string::npos || line.find("└──") != std::string::npos);
        if (!is_dependency && !skip_first)
            is_dependency = true;
        if (!is_dependency)
            continue;
        size_t start = 0;
        while (start < line.length()) {
            if (line[start] == ' ') {
                start++;
            } else if (line.find("├──", start) == start || line.find("└──", start) == start) {
                start += 3;
            } else if (line.find("│", start) == start) {
                start += 3;
            } else {
                break;
            }
        }
        if (start >= line.length())
            continue;
        std::string cleaned_line = line.substr(start);
        std::istringstream line_stream(cleaned_line);
        std::string name, version;
        if (line_stream >> name >> version) {
            if (!version.empty() && version[0] == 'v')
                version = version.substr(1);
            size_t paren_pos = version.find('(');
            if (paren_pos != std::string::npos)
                version = version.substr(0, paren_pos);
            Package pkg;
            pkg.name = name;
            pkg.version = version;
            pkg.location = "crates.io";
            packages.push_back(pkg);
        }
    }
    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });
    return packages;
}

bool CargoManager::updatePackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "cargo update -p " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
            this->cache_entry_.error_message = "Failed to update package: " + package_name +
                                               " (Note: cargo update requires Cargo.toml)";
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

bool CargoManager::updateAllDependencies(const std::string& /*package_name*/) {
    std::string command = "cargo update 2>&1";
    if (remote_executor_) {
        auto exec = remote_executor_;
        std::thread([this, exec, command]() {
            auto [ok, out] = exec(command);
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            if (ok) {
                this->cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
                this->cache_entry_.error_message.clear();
            } else {
                this->cache_entry_.error_message = "Failed to update all dependencies - " + out;
            }
        }).detach();
        return true;
    }
    std::thread([this, command]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
                "Failed to update all dependencies (Note: cargo update requires Cargo.toml)";
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

bool CargoManager::removePackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "cargo remove " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
            this->cache_entry_.error_message = "Failed to remove package: " + package_name +
                                               " (Note: cargo remove requires Cargo.toml)";
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

bool CargoManager::installPackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "cargo add " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
            this->cache_entry_.error_message = "Failed to install package: " + package_name +
                                               " (Note: cargo add requires Cargo.toml)";
            if (!output.empty())
                this->cache_entry_.error_message += " - " + output;
        }
    }).detach();
    return true;
}

} // namespace package_manager
} // namespace features
} // namespace pnana
