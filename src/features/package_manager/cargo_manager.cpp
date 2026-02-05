#include "features/package_manager/cargo_manager.h"
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

CargoManager::CargoManager() {
    cache_entry_.is_fetching = false;
    cache_entry_.timestamp = std::chrono::steady_clock::now() - CACHE_TIMEOUT_;
}

std::vector<Package> CargoManager::getInstalledPackages() {
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
            pnana::utils::Logger::getInstance().log("[CARGO] Error fetching packages: " +
                                                    std::string(e.what()));
        }
    }).detach();

    // 返回当前缓存的数据（如果有）
    return cache_entry_.packages;
}

void CargoManager::refreshPackages() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // 使缓存失效
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
    auto now = std::chrono::steady_clock::now();
    return (now - cache_entry_.timestamp) < CACHE_TIMEOUT_;
}

bool CargoManager::isAvailable() const {
    // 检查 cargo 命令是否可用
    std::string command = "which cargo > /dev/null 2>&1";
    int result = system(command.c_str());
    return result == 0;
}

std::vector<Package> CargoManager::fetchPackagesFromSystem() {
    std::vector<Package> packages;

    // 使用 cargo tree 获取依赖列表（只显示直接依赖）
    // cargo tree --depth 0 显示直接依赖
    // 如果没有 Cargo.toml，会返回错误，这是正常的
    std::string command = "cargo tree --depth 0 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to execute cargo command");
    }

    char buffer[4096];
    std::string output;
    output.reserve(512 * 1024); // 预分配512KB空间
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    int exit_code = pclose(pipe);

    // cargo tree 在没有 Cargo.toml 时会返回非零退出码，这是正常的
    // 我们仍然尝试解析输出，因为可能有一些信息
    if (exit_code != 0 && output.find("error: could not find `Cargo.toml`") != std::string::npos) {
        // 没有 Cargo.toml，返回空列表
        return packages;
    }

    if (exit_code != 0 && output.find("error:") != std::string::npos &&
        output.find("could not find") == std::string::npos) {
        throw std::runtime_error("cargo command failed with exit code " +
                                 std::to_string(exit_code));
    }

    packages = parseCargoTreeOutput(output);
    return packages;
}

std::vector<Package> CargoManager::parseCargoTreeOutput(const std::string& output) {
    std::vector<Package> packages;
    packages.reserve(100); // 预分配空间

    std::istringstream stream(output);
    std::string line;
    line.reserve(256);

    // cargo tree 输出格式示例：
    // project_name v0.1.0
    // ├── dependency1 v1.0.0
    // ├── dependency2 v2.0.0
    // └── dependency3 v3.0.0
    // 或者：
    // project_name v0.1.0
    // dependency1 v1.0.0
    // dependency2 v2.0.0

    bool skip_first = true; // 跳过项目本身

    while (std::getline(stream, line)) {
        // 跳过空行和错误信息
        if (line.empty() || line.find("error:") != std::string::npos) {
            continue;
        }

        // 跳过第一行（项目本身）- 通常不包含树形符号
        if (skip_first) {
            if (line.find("├──") == std::string::npos && line.find("└──") == std::string::npos) {
                skip_first = false;
                continue;
            }
        }

        // 查找包含依赖的行（包含树形符号的行）
        bool is_dependency =
            (line.find("├──") != std::string::npos || line.find("└──") != std::string::npos);

        if (!is_dependency && !skip_first) {
            // 可能是没有树形符号的输出格式
            is_dependency = true;
        }

        if (!is_dependency) {
            continue;
        }

        // 移除前导空格和树形符号
        size_t start = 0;
        while (start < line.length()) {
            if (line[start] == ' ') {
                start++;
            } else if (line.find("├──", start) == start || line.find("└──", start) == start) {
                start += 3; // 跳过 "├──" 或 "└──"
            } else if (line.find("│", start) == start) {
                start += 3; // 跳过 "│" (UTF-8 编码为 3 字节)
            } else {
                break;
            }
        }

        if (start >= line.length()) {
            continue;
        }

        std::string cleaned_line = line.substr(start);

        // 解析包名和版本
        // 格式可能是：package_name v1.2.3 或 package_name v1.2.3 (registry)
        std::istringstream line_stream(cleaned_line);
        std::string name, version;

        if (line_stream >> name >> version) {
            // 移除版本号前的 'v'（如果有）
            if (!version.empty() && version[0] == 'v') {
                version = version.substr(1);
            }

            // 检查版本号是否包含其他字符（如括号）
            size_t paren_pos = version.find('(');
            if (paren_pos != std::string::npos) {
                version = version.substr(0, paren_pos);
            }

            Package pkg;
            pkg.name = name;
            pkg.version = version;
            pkg.location = "crates.io"; // Rust 包通常来自 crates.io

            packages.push_back(pkg);
        }
    }

    // 按包名排序
    std::sort(packages.begin(), packages.end(), [](const Package& a, const Package& b) {
        return a.name < b.name;
    });

    return packages;
}

bool CargoManager::updatePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // cargo update 在项目目录中更新特定包
    // 注意：需要在 Cargo.toml 所在的目录执行
    std::string command = "cargo update -p " + package_name + " 2>&1";

    // 异步执行更新命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
            this->cache_entry_.error_message = "Failed to update package: " + package_name +
                                               " (Note: cargo update requires Cargo.toml)";
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool CargoManager::updateAllDependencies(const std::string& /*package_name*/) {
    // cargo update 更新所有依赖
    std::string command = "cargo update 2>&1";

    // 异步执行更新命令
    std::thread([this, command]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
                "Failed to update all dependencies (Note: cargo update requires Cargo.toml)";
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool CargoManager::removePackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // cargo remove 需要从 Cargo.toml 中移除依赖
    // 注意：需要在 Cargo.toml 所在的目录执行
    std::string command = "cargo remove " + package_name + " 2>&1";

    // 异步执行删除命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
            this->cache_entry_.error_message = "Failed to remove package: " + package_name +
                                               " (Note: cargo remove requires Cargo.toml)";
            if (!output.empty()) {
                this->cache_entry_.error_message += " - " + output;
            }
        }
    }).detach();

    return true;
}

bool CargoManager::installPackage(const std::string& package_name) {
    if (package_name.empty()) {
        return false;
    }

    // cargo add 添加依赖到 Cargo.toml
    std::string command = "cargo add " + package_name + " 2>&1";

    // 异步执行安装命令
    std::thread([this, command, package_name]() {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::lock_guard<std::mutex> lock(this->cache_mutex_);
            this->cache_entry_.error_message = "Failed to execute cargo command";
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
            this->cache_entry_.error_message = "Failed to install package: " + package_name +
                                               " (Note: cargo add requires Cargo.toml)";
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
