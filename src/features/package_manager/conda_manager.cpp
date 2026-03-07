#include "features/package_manager/conda_manager.h"
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
    return (std::chrono::steady_clock::now() - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool CondaManager::isAvailable() const {
    if (remote_executor_) {
        auto [ok, out] = remote_executor_("which conda 2>/dev/null");
        return ok && !out.empty();
    }
    return system("which conda > /dev/null 2>&1") == 0;
}

std::vector<Package> CondaManager::fetchPackagesFromSystem() {
    std::string output;
    if (remote_executor_) {
        auto [ok, out] = remote_executor_("conda list 2>&1");
        if (!ok)
            throw std::runtime_error("conda command failed on remote: " + out);
        output = out;
    } else {
        FILE* pipe = popen("conda list 2>&1", "r");
        if (!pipe)
            throw std::runtime_error("Failed to execute conda command");
        char buffer[4096];
        output.reserve(1024 * 1024);
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            output += buffer;
        int exit_code = pclose(pipe);
        if (exit_code != 0)
            throw std::runtime_error("conda command failed with exit code " +
                                     std::to_string(exit_code));
    }
    return parseCondaListOutput(output);
}

std::vector<Package> CondaManager::parseCondaListOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(500);
    std::istringstream stream(output);
    std::string line;
    line.reserve(256);
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#' || line.find("Name") != std::string::npos)
            continue;
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
    if (package_name.empty())
        return false;
    std::string command = "conda update -y " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute conda command";
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

bool CondaManager::updateAllDependencies(const std::string& /*package_name*/) {
    std::string command = "conda update --all -y 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute conda command";
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

bool CondaManager::removePackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "conda remove -y " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute conda command";
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

bool CondaManager::installPackage(const std::string& package_name) {
    if (package_name.empty())
        return false;
    std::string command = "conda install -y " + package_name + " 2>&1";
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
            this->cache_entry_.error_message = "Failed to execute conda command";
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
