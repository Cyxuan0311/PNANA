#include "features/package_manager/yarn_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>
#include <regex>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

YarnManager::YarnManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> YarnManager::getInstalledPackages() {
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
            pnana::utils::Logger::getInstance().log("[YARN] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    return cache_entry_.packages;
}

void YarnManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void YarnManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool YarnManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool YarnManager::isAvailable() const {
    std::string command = "which yarn > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<Package> YarnManager::fetchPackagesFromSystem() {
    std::vector<Package> packages;
    std::string command = "yarn list --depth=0 --json 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute yarn command");
    }

    char buffer[4096];
    std::string output;
    output.reserve(512 * 1024);
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int exit_code = pclose(pipe);

    if (exit_code != 0 && output.find("error") != std::string::npos) {
        if (output.find("No such file") != std::string::npos ||
            output.find("ENOENT") != std::string::npos) {
            return packages;
        }
    }

    packages = parseYarnListOutput(output);
    return packages;
}

std::vector<Package> YarnManager::parseYarnListOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(100);

    std::istringstream stream(output);
    std::string line;
    line.reserve(256);

    // Yarn list --json 输出格式：
    // {"type":"tree","data":{"type":"list","trees":[...]}}
    // 或者简单的文本格式（如果使用 --depth=0）

    // 尝试解析 JSON 格式
    size_t trees_pos = output.find("\"trees\":");
    if (trees_pos != std::string::npos) {
        // JSON 格式，解析 trees 数组
        size_t start = output.find("[", trees_pos);
        if (start != std::string::npos) {
            // 简化解析：查找 "name":"package@version" 模式
            // 使用原始字符串字面量，但需要转义引号
            std::regex pattern(R"(\"name\":\"([^\"]+)@([^\"]+)\")");
            std::sregex_iterator iter(output.begin() + start, output.end(), pattern);
            std::sregex_iterator end_iter;

            for (; iter != end_iter; ++iter) {
                std::smatch match = *iter;
                if (match.size() == 3) {
                    Package pkg;
                    pkg.name = match[1].str();
                    pkg.version = match[2].str();
                    pkg.location = "yarn";
                    packages.push_back(pkg);
                }
            }
        }
    } else {
        // 文本格式，类似 npm list
        while (std::getline(stream, line)) {
            if (line.empty() || line.find("yarn list") != std::string::npos) {
                continue;
            }

            size_t at_pos = line.find("@");
            if (at_pos == std::string::npos) {
                continue;
            }

            // 解析 package@version
            size_t name_start = 0;
            while (name_start < at_pos && line[name_start] == ' ') {
                name_start++;
            }

            if (name_start >= at_pos) {
                continue;
            }

            std::string name = line.substr(name_start, at_pos - name_start);
            size_t version_start = at_pos + 1;
            size_t version_end = version_start;
            while (version_end < line.length() && line[version_end] != ' ' &&
                   line[version_end] != '\t' && line[version_end] != '\n') {
                version_end++;
            }

            if (version_end > version_start) {
                std::string version = line.substr(version_start, version_end - version_start);
                Package pkg;
                pkg.name = name;
                pkg.version = version;
                pkg.location = "yarn";
                packages.push_back(pkg);
            }
        }
    }

    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool YarnManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // yarn upgrade 更新特定包
    std::string command = "yarn upgrade " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yarn command";
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

bool YarnManager::updateAllDependencies(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // yarn upgrade 更新包及其依赖
    std::string command = "yarn upgrade " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yarn command";
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
            this->cache_entry_.error_message =
                "Failed to update package and dependencies: " + package_name;
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool YarnManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // yarn remove 删除包
    std::string command = "yarn remove " + package_name + " 2>&1";

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yarn command";
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

bool YarnManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // yarn add 安装包
    std::string command = "yarn add " + package_name + " 2>&1";

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute yarn command";
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
