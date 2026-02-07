#include "features/package_manager/npm_manager.h"
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

NpmManager::NpmManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> NpmManager::getInstalledPackages() {
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
            pnana::utils::Logger::getInstance().log("[NPM] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    // 返回当前缓存的数据（如果有）
    return cache_entry_.packages;
}

void NpmManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // 使缓存失效
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
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool NpmManager::isAvailable() const {
    // 检查 npm 命令是否可用
    std::string command = "which npm > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<Package> NpmManager::fetchPackagesFromSystem() {
    std::vector<Package> packages;

    // 使用 npm list 获取已安装的包列表
    // npm list --depth=0 只显示直接依赖，--json=false 输出文本格式
    std::string command = "npm list --depth=0 --json=false 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute npm command");
    }

    char buffer[4096];
    std::string output;
    output.reserve(512 * 1024); // 预分配512KB空间
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int exit_code = pclose(pipe);

    // npm list 在没有 package.json 时会返回非零退出码，这是正常的
    if (exit_code != 0 && output.find("npm ERR!") != std::string::npos) {
        // 检查是否是"没有 package.json"的错误
        if (output.find("ENOENT") != std::string::npos ||
            output.find("No such file") != std::string::npos) {
            // 没有 package.json，返回空列表
            return packages;
        }
    }

    packages = parseNpmListOutput(output);
    return packages;
}

std::vector<Package> NpmManager::parseNpmListOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(100); // 预分配空间

    std::istringstream stream(output);
    std::string line;
    line.reserve(256);

    // npm list 输出格式示例：
    // project-name@1.0.0
    // ├── dependency1@1.0.0
    // ├── dependency2@2.0.0
    // └── dependency3@3.0.0

    bool skip_first = true; // 跳过项目本身

    while (std::getline(stream, line)) {
        // 跳过空行和错误信息
        if (line.empty() || line.find("npm ERR!") != std::string::npos ||
            line.find("extraneous") != std::string::npos ||
            line.find("missing") != std::string::npos) {
            continue;
        }

        // 跳过第一行（项目本身）
        if (skip_first && line.find("@") != std::string::npos &&
            (line.find("├──") == std::string::npos && line.find("└──") == std::string::npos)) {
            skip_first = false;
            continue;
        }

        // 查找包含包名和版本的行
        // 格式：├── package-name@1.2.3 或 └── package-name@1.2.3
        size_t at_pos = line.find("@");
        if (at_pos == std::string::npos) {
            continue;
        }

        // 找到包名的开始位置（跳过树形符号）
        size_t name_start = 0;
        while (name_start < at_pos) {
            if (line[name_start] == ' ') {
                name_start++;
            } else if (line.find("├──", name_start) == name_start ||
                       line.find("└──", name_start) == name_start) {
                name_start += 3; // 跳过 "├──" 或 "└──"
            } else if (line.find("│", name_start) == name_start) {
                name_start += 3; // 跳过 "│" (UTF-8 编码为 3 字节)
            } else {
                break;
            }
        }

        if (name_start >= at_pos) {
            continue;
        }

        std::string name = line.substr(name_start, at_pos - name_start);

        // 提取版本号（@后面的部分，可能包含空格或其他字符）
        size_t version_start = at_pos + 1;
        size_t version_end = version_start;
        while (version_end < line.length() && line[version_end] != ' ' &&
               line[version_end] != '\t' && line[version_end] != '\n' &&
               line[version_end] != '\r') {
            version_end++;
        }

        if (version_end > version_start) {
            std::string version = line.substr(version_start, version_end - version_start);

            Package pkg;
            pkg.name = name;
            pkg.version = version;
            pkg.location = "npm"; // Node.js 包来自 npm

            packages.push_back(pkg);
        }
    }

    // 按包名排序
    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool NpmManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // npm update 更新特定包
    std::string command = "npm update " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute npm command";
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

bool NpmManager::updateAllDependencies(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // npm update 更新包及其依赖
    std::string command = "npm update " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute npm command";
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

bool NpmManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // npm uninstall 删除包
    std::string command = "npm uninstall " + package_name + " 2>&1";

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute npm command";
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

bool NpmManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // npm install 安装包
    std::string command = "npm install " + package_name + " 2>&1";

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute npm command";
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
