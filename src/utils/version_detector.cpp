#include "utils/version_detector.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

// 自定义删除器用于 FILE*，避免函数指针属性警告
struct FileDeleter {
    void operator()(FILE* file) const {
        if (file) {
            pclose(file);
        }
    }
};

namespace pnana {
namespace utils {

VersionDetector::VersionDetector() {}

std::string VersionDetector::getVersionForFileType(const std::string& file_type) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto now = std::chrono::steady_clock::now();
    auto it = version_cache_.find(file_type);

    // 检查缓存是否存在且未过期
    if (it != version_cache_.end()) {
        auto& entry = it->second;
        if (now - entry.timestamp < CACHE_DURATION) {
            return entry.version;
        }
        // 如果正在获取中，返回上一次的结果
        if (entry.is_fetching) {
            return entry.version;
        }
    }

    // 如果没有缓存或已过期，异步获取版本信息
    auto& entry = version_cache_[file_type];
    entry.is_fetching = true;

    // 在后台异步执行版本获取
    std::thread([this, file_type]() {
        std::string version = this->fetchVersionForFileType(file_type);

        std::lock_guard<std::mutex> lock(this->cache_mutex_);
        auto& entry = this->version_cache_[file_type];
        entry.version = version;
        entry.timestamp = std::chrono::steady_clock::now();
        entry.is_fetching = false;
    }).detach();

    // 返回缓存中的版本（如果有的话）或空字符串
    return (it != version_cache_.end()) ? it->second.version : "";
}

void VersionDetector::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    version_cache_.clear();
}

void VersionDetector::clearCacheForType(const std::string& file_type) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    version_cache_.erase(file_type);
}

std::string VersionDetector::fetchVersionForFileType(const std::string& file_type) {
    std::string command;
    std::string version_prefix;

    // 根据文件类型确定对应的版本检查命令
    if (file_type == "python" || file_type == "py") {
        command = "python3 --version 2>&1 || python --version 2>&1";
        version_prefix = "Python ";
    } else if (file_type == "javascript" || file_type == "js") {
        command = "node --version 2>&1";
        version_prefix = "v"; // Node.js 输出格式为 "v18.0.0"
    } else if (file_type == "typescript" || file_type == "ts") {
        command = "tsc --version 2>&1 || node --version 2>&1";
        version_prefix = "Version ";
    } else if (file_type == "java") {
        command = "java -version 2>&1 | head -1";
        version_prefix = "openjdk version \"";
    } else if (file_type == "kotlin" || file_type == "kt") {
        command = "kotlin -version 2>&1 || kotlinc -version 2>&1";
        version_prefix = "kotlin ";
    } else if (file_type == "go" || file_type == "golang") {
        command = "go version 2>&1";
        version_prefix = "go version go";
    } else if (file_type == "rust" || file_type == "rs") {
        command = "rustc --version 2>&1";
        version_prefix = "rustc ";
    } else if (file_type == "ruby" || file_type == "rb") {
        command = "ruby --version 2>&1";
        version_prefix = "ruby ";
    } else if (file_type == "php") {
        command = "php --version 2>&1 | head -1";
        version_prefix = "PHP ";
    } else if (file_type == "perl" || file_type == "pl") {
        command = "perl --version 2>&1 | head -2 | tail -1";
        version_prefix = "This is perl ";
    } else if (file_type == "lua") {
        command = "lua -v 2>&1 || luajit -v 2>&1";
        version_prefix = "Lua ";
    } else if (file_type == "r") {
        command = "R --version 2>&1 | head -1";
        version_prefix = "R version ";
    } else if (file_type == "scala") {
        command = "scala -version 2>&1";
        version_prefix = "Scala ";
    } else if (file_type == "swift") {
        command = "swift --version 2>&1 | head -1";
        version_prefix = "swift version ";
    } else if (file_type == "dart") {
        command = "dart --version 2>&1";
        version_prefix = "Dart SDK version: ";
    } else if (file_type == "haskell" || file_type == "hs") {
        command = "ghc --version 2>&1 || runghc --version 2>&1";
        version_prefix = "The Glorious Glasgow Haskell Compilation System, version ";
    } else if (file_type == "clojure" || file_type == "clj") {
        command = "clojure --version 2>&1 || lein --version 2>&1";
        version_prefix = "Clojure ";
    } else if (file_type == "erlang" || file_type == "erl") {
        command =
            "erl -eval 'erlang:display(erlang:system_info(otp_release)), halt().' -noshell 2>&1";
        version_prefix = "";
    } else if (file_type == "elixir" || file_type == "ex" || file_type == "exs") {
        command = "elixir --version 2>&1";
        version_prefix = "Elixir ";
    } else if (file_type == "shell" || file_type == "bash" || file_type == "sh" ||
               file_type == "zsh") {
        command = "bash --version 2>&1 | head -1";
        version_prefix = "GNU bash, version ";
    } else if (file_type == "fish") {
        command = "fish --version 2>&1";
        version_prefix = "fish, version ";
    } else if (file_type == "powershell" || file_type == "ps1") {
        command = "pwsh --version 2>&1 || powershell --version 2>&1";
        version_prefix = "PowerShell ";
    } else if (file_type == "c") {
        // C 文件：优先检测 gcc，然后是 clang
        command = "gcc --version 2>&1 | head -1 || clang --version 2>&1 | head -1";
        version_prefix = "";
    } else if (file_type == "cpp" || file_type == "cc" || file_type == "cxx" ||
               file_type == "c++") {
        // C++ 文件：优先检测 g++，然后是 clang++，最后是 gcc/clang
        command = "g++ --version 2>&1 | head -1 || clang++ --version 2>&1 | head -1 || "
                  "gcc --version 2>&1 | head -1 || clang --version 2>&1 | head -1";
        version_prefix = "";
    } else if (file_type == "cmake") {
        // CMake 版本检测
        command = "cmake --version 2>&1 | head -1";
        version_prefix = "cmake version ";
    } else {
        return "";
    }

    // 执行命令获取版本信息
    std::string raw_output = executeCommand(command);
    if (raw_output.empty()) {
        return "";
    }

    // 使用优化的解析方法提取版本号
    return parseVersionString(raw_output, file_type);
}

std::string VersionDetector::parseVersionString(const std::string& raw_output,
                                                const std::string& file_type) {
    if (raw_output.empty()) {
        return "";
    }

    std::string result = raw_output;

    // 移除首尾空白字符
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
                     return !std::isspace(ch);
                 }));
    result.erase(std::find_if(result.rbegin(), result.rend(),
                              [](unsigned char ch) {
                                  return !std::isspace(ch);
                              })
                     .base(),
                 result.end());

    // 只保留第一行（移除后续行）
    size_t newline_pos = result.find('\n');
    if (newline_pos != std::string::npos) {
        result = result.substr(0, newline_pos);
    }

    // C/C++ 编译器特殊处理
    if (file_type == "c" || file_type == "cpp" || file_type == "cc" || file_type == "cxx" ||
        file_type == "c++") {
        // 检测编译器类型并提取版本
        std::string compiler_name;
        std::string version;

        // 检测 GCC
        if (result.find("gcc") != std::string::npos || result.find("g++") != std::string::npos) {
            if (result.find("g++") != std::string::npos) {
                compiler_name = "g++";
            } else {
                compiler_name = "gcc";
            }
            // GCC 格式: "gcc (GCC) 11.2.0" 或 "g++ (GCC) 11.2.0"
            size_t gcc_pos = result.find("(GCC)");
            if (gcc_pos != std::string::npos) {
                std::string after_gcc = result.substr(gcc_pos + 5);
                version = extractVersionNumber(after_gcc);
                if (!version.empty()) {
                    return compiler_name + " " + version;
                }
            }
        }
        // 检测 Clang
        else if (result.find("clang") != std::string::npos) {
            if (result.find("clang++") != std::string::npos) {
                compiler_name = "clang++";
            } else {
                compiler_name = "clang";
            }
            // Clang 格式: "clang version 14.0.0" 或 "Apple clang version 14.0.0"
            size_t version_pos = result.find("version");
            if (version_pos != std::string::npos) {
                std::string after_version = result.substr(version_pos + 7);
                version = extractVersionNumber(after_version);
                if (!version.empty()) {
                    return compiler_name + " " + version;
                }
            }
        }
        // 检测 MSVC (Windows)
        else if (result.find("Microsoft") != std::string::npos ||
                 result.find("MSVC") != std::string::npos ||
                 result.find("cl.exe") != std::string::npos) {
            compiler_name = "msvc";
            version = extractVersionNumber(result);
            if (!version.empty()) {
                return compiler_name + " " + version;
            }
        }

        // 如果无法识别编译器，尝试通用提取
        version = extractVersionNumber(result);
        if (!version.empty()) {
            return version;
        }

        // 如果都失败，返回原始结果的前20个字符
        if (result.length() > 20) {
            return result.substr(0, 20);
        }
        return result;
    }

    // CMake 特殊处理
    if (file_type == "cmake") {
        // CMake 格式: "cmake version 3.25.0" 或 "cmake version 3.25.0\n..."
        size_t version_pos = result.find("version");
        if (version_pos != std::string::npos) {
            std::string after_version = result.substr(version_pos + 7);
            std::string version = extractVersionNumber(after_version);
            if (!version.empty()) {
                return "cmake " + version;
            }
        }
        // 如果找不到 "version"，尝试直接提取版本号
        std::string version = extractVersionNumber(result);
        if (!version.empty()) {
            return "cmake " + version;
        }
        return result;
    }

    // 使用智能版本号提取
    std::string version = extractVersionNumber(result);

    // 如果提取失败，尝试移除已知前缀
    if (version.empty()) {
        // 尝试移除常见的前缀
        std::vector<std::string> prefixes = {
            "Python ",
            "Node ",
            "v",
            "Version ",
            "java version \"",
            "openjdk version \"",
            "kotlin ",
            "go version go",
            "rustc ",
            "ruby ",
            "PHP ",
            "This is perl ",
            "Lua ",
            "R version ",
            "Scala ",
            "swift version ",
            "Dart SDK version: ",
            "The Glorious Glasgow Haskell Compilation System, version ",
            "Clojure ",
            "Elixir ",
            "GNU bash, version ",
            "fish, version ",
            "PowerShell ",
            "gcc (GCC) ",
            "g++ (GCC) ",
            "clang version ",
            "Apple clang version ",
            "cmake version "};

        for (const auto& prefix : prefixes) {
            if (result.find(prefix) == 0) {
                version = result.substr(prefix.length());
                // 移除可能的引号
                if (!version.empty() && version.back() == '"') {
                    version.pop_back();
                }
                break;
            }
        }
    }

    // 如果还是为空，使用原始结果（去除前后空白）
    if (version.empty()) {
        version = result;
    }

    // 清理版本号：移除引号、括号等
    version.erase(std::remove(version.begin(), version.end(), '"'), version.end());
    version.erase(std::remove(version.begin(), version.end(), '('), version.end());
    version.erase(std::remove(version.begin(), version.end(), ')'), version.end());

    // 移除首尾空白
    version.erase(version.begin(),
                  std::find_if(version.begin(), version.end(), [](unsigned char ch) {
                      return !std::isspace(ch);
                  }));
    version.erase(std::find_if(version.rbegin(), version.rend(),
                               [](unsigned char ch) {
                                   return !std::isspace(ch);
                               })
                      .base(),
                  version.end());

    // 限制长度，避免状态栏过长
    if (version.length() > 15) {
        version = version.substr(0, 15) + "...";
    }

    return version;
}

std::string VersionDetector::extractVersionNumber(const std::string& text) {
    if (text.empty()) {
        return "";
    }

    // 使用正则表达式匹配版本号
    // 匹配格式：v?数字.数字.数字 或 数字.数字 或 数字
    // 也支持带后缀的版本号，如 "1.2.3-beta", "1.2.3-rc1" 等
    std::regex version_pattern(
        R"((?:v|V|version\s+)?(\d+(?:\.\d+){0,2}(?:-[a-zA-Z0-9]+)?(?:\+[a-zA-Z0-9]+)?))",
        std::regex_constants::icase);

    std::smatch match;
    if (std::regex_search(text, match, version_pattern)) {
        std::string version = match[1].str();

        // 如果匹配到的是完整版本号（包含多个部分），只取前三个数字部分
        // 例如 "1.2.3.4" -> "1.2.3"
        size_t dot_count = std::count(version.begin(), version.end(), '.');
        if (dot_count > 2) {
            size_t last_dot = version.find_last_of('.');
            size_t second_last_dot = version.find_last_of('.', last_dot - 1);
            if (second_last_dot != std::string::npos) {
                version = version.substr(0, last_dot);
            }
        }

        return version;
    }

    // 如果正则匹配失败，尝试简单的数字提取
    // 查找第一个连续的数字序列（可能包含点）
    std::string version;
    bool found_digit = false;
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (std::isdigit(c) || c == '.' || (found_digit && (c == '-' || c == '+'))) {
            version += c;
            found_digit = true;
        } else if (found_digit) {
            // 遇到非版本字符，停止提取
            break;
        }
    }

    return version;
}

std::string VersionDetector::executeCommand(const std::string& command) {
    std::array<char, 256> buffer;
    std::string result;

    std::unique_ptr<FILE, FileDeleter> pipe(popen(command.c_str(), "r"));
    if (!pipe) {
        return "";
    }

    // 读取命令输出
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

} // namespace utils
} // namespace pnana
