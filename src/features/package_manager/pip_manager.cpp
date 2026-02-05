#include "features/package_manager/pip_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace package_manager {

PipManager::PipManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> PipManager::getInstalledPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查缓存是否有效
    if (isCacheValid() && !cache_entry_.packages.empty()) {
        return cache_entry_.packages;
    }

    // 如果正在获取中，返回当前缓存的数据（如果有）
    if (cache_entry_.is_fetching) {
        return cache_entry_.packages;
    }

    // 标记为正在获取
    cache_entry_.is_fetching = true;
    cache_entry_.error_message.clear();

    // 在后台线程异步获取包列表
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
            pnana::utils::Logger::getInstance().log("[PIP] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    // 返回当前缓存的数据（如果有）
    return cache_entry_.packages;
}

void PipManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // 使缓存失效
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

void PipManager::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_entry_.packages.clear();
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
    cache_entry_.error_message.clear();
}

bool PipManager::isCacheValid() const {
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool PipManager::isAvailable() const {
    // 检查 pip3 或 pip 命令是否可用
    const char* pip_commands[] = {"pip3", "pip"};
    for (const char* pip_cmd : pip_commands) {
        std::string command = std::string("which ") + pip_cmd + " > /dev/null 2>&1";
        int result = system(command.c_str());
        if (result == 0) {
            return true;
        }
    }
    return false;
}

std::vector<Package> PipManager::fetchPackagesFromSystem() {
    std::vector<Package> packages;

    // 尝试执行 pip list 命令
    // 使用 pip3 或 pip，取决于系统配置
    const char* pip_commands[] = {"pip3", "pip"};
    std::string output;
    bool success = false;

    for (const char* pip_cmd : pip_commands) {
        std::string command = std::string(pip_cmd) + " list --format=columns 2>&1";

        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            continue;
        }

        char buffer[4096]; // 增大缓冲区，减少系统调用次数
        output.clear();
        output.reserve(1024 * 1024); // 预分配1MB空间，避免多次重新分配
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int exit_code = pclose(pipe);

        if (exit_code == 0) {
            success = true;
            break;
        }
    }

    if (!success) {
        throw std::runtime_error("Failed to execute pip command. Is pip installed?");
    }

    packages = parsePipListOutput(output);
    return packages;
}

std::vector<Package> PipManager::parsePipListOutput(const std::string& output) {
    std::vector<Package> packages;

    std::istringstream stream(output);
    std::string line;
    bool is_first_line = true;

    // pip list 输出格式：
    // Package    Version
    // ---------- -------
    // package1   1.0.0
    // package2   2.0.0

    while (std::getline(stream, line)) {
        // 跳过标题行和分隔行
        if (is_first_line || line.find("---") != std::string::npos || line.empty()) {
            is_first_line = false;
            continue;
        }

        // 解析包名和版本
        std::istringstream line_stream(line);
        std::string name, version;

        if (line_stream >> name >> version) {
            Package pkg;
            pkg.name = name;
            pkg.version = version;

            // 尝试获取剩余信息（位置、描述等）
            std::string rest;
            if (std::getline(line_stream, rest)) {
                // 移除前导空格
                rest.erase(0, rest.find_first_not_of(" \t"));
                if (!rest.empty()) {
                    // 检查是否是位置信息（通常包含路径）
                    if (rest.find("/") != std::string::npos ||
                        rest.find("\\") != std::string::npos) {
                        pkg.location = rest;
                    } else {
                        pkg.description = rest;
                    }
                }
            }

            packages.push_back(pkg);
        }
    }

    // 按名称排序
    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool PipManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // 执行 pip install --upgrade 命令
    const char* pip_commands[] = {"pip3", "pip"};
    std::string command;

    for (const char* pip_cmd : pip_commands) {
        std::string test_cmd = std::string("which ") + pip_cmd + " > /dev/null 2>&1";
        if (system(test_cmd.c_str()) == 0) {
            command = std::string(pip_cmd) + " install --upgrade " + package_name + " 2>&1";
            break;
        }
    }

    if (command.empty()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_entry_.error_message = "pip command not found";
        return false;
    }

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute pip command";
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
                // 只保存前200字符的错误信息
                std::string error_output = output.substr(0, 200);
                this->cache_entry_.error_message += " - " + error_output;
            }
        }
    }).detach();

    return true;
}

bool PipManager::updateAllDependencies(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // 先更新包本身，然后更新其依赖
    const char* pip_commands[] = {"pip3", "pip"};
    std::string command;

    for (const char* pip_cmd : pip_commands) {
        std::string test_cmd = std::string("which ") + pip_cmd + " > /dev/null 2>&1";
        if (system(test_cmd.c_str()) == 0) {
            // 更新包及其所有依赖
            command = std::string(pip_cmd) + " install --upgrade --upgrade-strategy eager " +
                      package_name + " 2>&1";
            break;
        }
    }

    if (command.empty()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_entry_.error_message = "pip command not found";
        return false;
    }

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute pip command";
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
                std::string error_output = output.substr(0, 200);
                this->cache_entry_.error_message += " - " + error_output;
            }
        }
    }).detach();

    return true;
}

bool PipManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // 执行 pip uninstall 命令
    const char* pip_commands[] = {"pip3", "pip"};
    std::string command;

    for (const char* pip_cmd : pip_commands) {
        std::string test_cmd = std::string("which ") + pip_cmd + " > /dev/null 2>&1";
        if (system(test_cmd.c_str()) == 0) {
            command = std::string(pip_cmd) + " uninstall -y " + package_name + " 2>&1";
            break;
        }
    }

    if (command.empty()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_entry_.error_message = "pip command not found";
        return false;
    }

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute pip command";
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
                std::string error_output = output.substr(0, 200);
                this->cache_entry_.error_message += " - " + error_output;
            }
        }
    }).detach();

    return true;
}

bool PipManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // 执行 pip install 命令
    const char* pip_commands[] = {"pip3", "pip"};
    std::string command;

    for (const char* pip_cmd : pip_commands) {
        std::string test_cmd = std::string("which ") + pip_cmd + " > /dev/null 2>&1";
        if (system(test_cmd.c_str()) == 0) {
            command = std::string(pip_cmd) + " install " + package_name + " 2>&1";
            break;
        }
    }

    if (command.empty()) {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        cache_entry_.error_message = "pip command not found";
        return false;
    }

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute pip command";
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
                std::string error_output = output.substr(0, 200);
                this->cache_entry_.error_message += " - " + error_output;
            }
        }
    }).detach();

    return true;
}

} // namespace package_manager
} // namespace features
} // namespace pnana
