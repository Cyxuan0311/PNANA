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

// 缓存有效期（5分钟）
constexpr std::chrono::minutes VersionDetector::CACHE_DURATION;

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

    // ===================== 原有文件类型 =====================
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
    } else if (file_type == "makefile" || file_type == "Makefile" || file_type == "makefile.in" ||
               file_type == "Makefile.in") {
        command = "make --version 2>&1 | head -1";
        version_prefix = "GNU Make ";
    } else if (file_type == "dockerfile") {
        command = "docker --version 2>&1 | head -1";
        version_prefix = "Docker version ";
    }
    // ===================== 新增文件类型 =====================
    else if (file_type == "vue" || file_type == "vuejs") {
        // Vue 版本检测：优先检测 @vue/cli，然后是 vue --version
        command = "vue --version 2>&1 || npm list vue 2>&1 | grep vue | head -1";
        version_prefix = "@vue/cli ";
    } else if (file_type == "react" || file_type == "jsx" || file_type == "tsx") {
        // React 版本检测：检测 react-scripts 或 npm 中的 react 版本
        command =
            "npm list react 2>&1 | grep react | head -1 || npx create-react-app --version 2>&1";
        version_prefix = "react@";
    } else if (file_type == "yaml" || file_type == "yml") {
        // YAML 检测 yq 工具版本
        command = "yq --version 2>&1 || python3 -c 'import yaml; print(yaml.__version__)' 2>&1";
        version_prefix = "yq version ";
    } else if (file_type == "toml") {
        // TOML 检测 tomlq 或 python toml 库版本
        command = "tomlq --version 2>&1 || python3 -c 'import toml; print(toml.__version__)' 2>&1";
        version_prefix = "tomlq version ";
    } else if (file_type == "protobuf" || file_type == "proto") {
        // Protobuf 检测 protoc 版本
        command = "protoc --version 2>&1";
        version_prefix = "libprotoc ";
    } else if (file_type == "sql") {
        // SQL 检测常见数据库客户端版本
        command = "mysql --version 2>&1 || psql --version 2>&1 || sqlite3 --version 2>&1";
        version_prefix = "";
    } else if (file_type == "html" || file_type == "htm") {
        // HTML 检测浏览器/解析器版本
        command = "curl --version 2>&1 | head -1 || w3m -version 2>&1 | head -1";
        version_prefix = "curl ";
    } else if (file_type == "css" || file_type == "scss" || file_type == "sass") {
        // CSS/SCSS 检测 sass/node-sass 版本
        command = "sass --version 2>&1 || node-sass --version 2>&1";
        version_prefix = "sass ";
    } else if (file_type == "markdown" || file_type == "md") {
        // Markdown 检测 pandoc 版本
        command = "pandoc --version 2>&1 | head -1";
        version_prefix = "pandoc ";
    } else if (file_type == "ini" || file_type == "conf") {
        // 配置文件检测解析工具版本
        command = "crudini --version 2>&1 || cat /etc/os-release | grep VERSION_ID | head -1";
        version_prefix = "crudini ";
    } else if (file_type == "json") {
        // JSON 检测 jq 工具版本
        command = "jq --version 2>&1 || python3 -m json.tool --help 2>&1 | head -1";
        version_prefix = "jq-";
    } else if (file_type == "xml") {
        // XML 检测 xmllint 版本
        command = "xmllint --version 2>&1 | head -1";
        version_prefix = "xmllint: using libxml2 ";
    } else if (file_type == "csv") {
        // CSV 检测 csvkit 工具版本
        command =
            "csvkit --version 2>&1 || python3 -c 'import csv; print(\"Python CSV lib\")' 2>&1";
        version_prefix = "csvkit ";
    } else if (file_type == "rust-script" || file_type == "rs-script") {
        // Rust 脚本检测 cargo-script 版本
        command = "cargo script --version 2>&1 || rustc --version 2>&1";
        version_prefix = "cargo-script ";
    } else if (file_type == "terraform" || file_type == "tf") {
        // Terraform 版本检测
        command = "terraform version 2>&1 | head -1";
        version_prefix = "Terraform v";
    } else if (file_type == "ansible" || file_type == "yml" || file_type == "yaml") {
        // Ansible 版本检测
        command = "ansible --version 2>&1 | head -1";
        version_prefix = "ansible [core ";
    } else if (file_type == "gradle") {
        // Gradle 版本检测
        command = "gradle --version 2>&1 | head -1 || ./gradlew --version 2>&1 | head -1";
        version_prefix = "Gradle ";
    } else if (file_type == "maven" || file_type == "pom") {
        // Maven 版本检测
        command = "mvn --version 2>&1 | head -1";
        version_prefix = "Apache Maven ";
    } else if (file_type == "zig") {
        // Zig 编程语言
        command = "zig version 2>&1 || zig --version 2>&1";
        version_prefix = "zig ";
    } else if (file_type == "odin") {
        // Odin 编程语言
        command = "odin version 2>&1 || odin --version 2>&1";
        version_prefix = "odin ";
    } else if (file_type == "nim" || file_type == "nimble") {
        // Nim 编程语言
        command = "nim --version 2>&1 | head -1";
        version_prefix = "Nim Compiler Version ";
    } else if (file_type == "fortran" || file_type == "f90" || file_type == "f95") {
        // Fortran 编译器版本
        command = "gfortran --version 2>&1 | head -1 || ifort --version 2>&1 | head -1";
        version_prefix = "GNU Fortran (GCC) ";
    } else if (file_type == "matlab" || file_type == "m") {
        // MATLAB/Octave 版本
        command = "octave --version 2>&1 | head -1";
        version_prefix = "octave version ";
    } else if (file_type == "julia") {
        // Julia 编程语言
        command = "julia --version 2>&1";
        version_prefix = "julia version ";
    } else if (file_type == "coffeescript" || file_type == "coffee") {
        // CoffeeScript 版本
        command = "coffee --version 2>&1";
        version_prefix = "CoffeeScript ";
    } else if (file_type == "less") {
        // Less 版本
        command = "lessc --version 2>&1";
        version_prefix = "lessc ";
    } else if (file_type == "stylus") {
        // Stylus 版本
        command = "stylus --version 2>&1";
        version_prefix = "stylus ";
    } else if (file_type == "docker-compose" || file_type == "yml" || file_type == "yaml") {
        // Docker Compose 版本
        command = "docker compose version 2>&1 || docker-compose --version 2>&1";
        version_prefix = "Docker Compose version v";
    } else if (file_type == "kubernetes" || file_type == "k8s" || file_type == "yaml") {
        // Kubernetes kubectl 版本
        command = "kubectl version --client 2>&1 | head -1";
        version_prefix = "Client Version: v";
    } else if (file_type == "helm") {
        // Helm 版本
        command = "helm version 2>&1 | head -1";
        version_prefix = "version.BuildInfo{Version:\"v";
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

    // ===================== 特殊文件类型解析 =====================
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

    // ===================== 新增特殊解析规则 =====================
    // Vue 特殊解析
    if (file_type == "vue" || file_type == "vuejs") {
        std::string version = extractVersionNumber(result);
        if (!version.empty()) {
            return "vue " + version;
        }
        return result;
    }

    // React 特殊解析
    if (file_type == "react" || file_type == "jsx" || file_type == "tsx") {
        std::string version = extractVersionNumber(result);
        if (!version.empty()) {
            return "react " + version;
        }
        return result;
    }

    // Terraform 特殊解析
    if (file_type == "terraform" || file_type == "tf") {
        std::string version = extractVersionNumber(result);
        if (!version.empty()) {
            return "terraform " + version;
        }
        return result;
    }

    // Kubernetes 特殊解析
    if (file_type == "kubernetes" || file_type == "k8s") {
        std::string version = extractVersionNumber(result);
        if (!version.empty()) {
            return "kubectl " + version;
        }
        return result;
    }

    // ===================== 通用解析逻辑 =====================
    // 使用智能版本号提取
    std::string version = extractVersionNumber(result);

    // 如果提取失败，尝试移除已知前缀
    if (version.empty()) {
        // 尝试移除常见的前缀
        std::vector<std::string> prefixes = {
            // 原有前缀
            "Python ", "Node ", "v", "Version ", "java version \"", "openjdk version \"", "kotlin ",
            "go version go", "rustc ", "ruby ", "PHP ", "This is perl ", "Lua ", "R version ",
            "Scala ", "swift version ",
            "Dart SDK version: ", "The Glorious Glasgow Haskell Compilation System, version ",
            "Clojure ", "Elixir ", "GNU bash, version ", "fish, version ", "PowerShell ",
            "gcc (GCC) ", "g++ (GCC) ", "clang version ", "Apple clang version ", "cmake version ",
            "Docker version ", "GNU Make ",
            // 新增前缀
            "@vue/cli ", "react@", "yq version ", "tomlq version ", "libprotoc ", "curl ", "sass ",
            "pandoc ", "crudini ", "jq-", "xmllint: using libxml2 ", "csvkit ", "cargo-script ",
            "Terraform v", "ansible [core ", "Gradle ", "Apache Maven ", "zig ", "odin ",
            "Nim Compiler Version ", "GNU Fortran (GCC) ", "octave version ", "julia version ",
            "CoffeeScript ", "lessc ", "stylus ", "Docker Compose version v", "Client Version: v",
            "version.BuildInfo{Version:\"v"};

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
    version.erase(std::remove(version.begin(), version.end(), '['), version.end());
    version.erase(std::remove(version.begin(), version.end(), ']'), version.end());
    version.erase(std::remove(version.begin(), version.end(), '}'), version.end());
    version.erase(std::remove(version.begin(), version.end(), '{'), version.end());

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