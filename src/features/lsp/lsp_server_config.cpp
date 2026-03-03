#include "features/lsp/lsp_server_config.h"
#include <algorithm>
#include <cstdlib>
#include <map>
#include <sys/stat.h>

#include <nlohmann/json.hpp>

namespace {
// 检测 C++ 标准库 include 路径，用于 clangd fallbackFlags
std::vector<std::string> getClangdFallbackFlags() {
    std::vector<std::string> flags = {"-std=c++17", "-xc++"};
    const char* cxx_bases[] = {"/usr/include/c++", "/usr/local/include/c++"};
    const char* versions[] = {"17", "16", "15", "14", "13", "12", "11", "10", "9"};
    for (const char* base : cxx_bases) {
        for (const char* ver : versions) {
            std::string path = std::string(base) + "/" + ver;
            struct stat st;
            if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                flags.push_back("-isystem");
                flags.push_back(path);
                std::string arch_path = path + "/x86_64-linux-gnu";
                if (stat(arch_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                    flags.push_back("-isystem");
                    flags.push_back(arch_path);
                }
                return flags; // 找到就返回
            }
        }
    }
    return flags; // 未找到时仍返回基础 flags
}
} // namespace

namespace pnana {
namespace features {

LspServerConfigManager::LspServerConfigManager() {
    initializeDefaultConfigs();
}

std::vector<LspServerConfig> LspServerConfigManager::getDefaultConfigs() {
    std::vector<LspServerConfig> configs;

    // 获取缓存目录路径
    std::string cache_dir = std::string(getenv("HOME")) + "/.config/pnana/.cache";

    // C++ - clangd（.cpp/.hpp 等使用 C++ 标准库补全：vector、string 等）
    configs.emplace_back(
        "clangd", "clangd", "cpp",
        std::set<std::string>{".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".c++", ".h++"},
        std::vector<std::string>{"--query-driver=/usr/bin/g++", "--query-driver=/usr/bin/gcc"},
        std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                           {"TMPDIR", cache_dir + "/tmp"}});
    auto cxx_fallback = getClangdFallbackFlags();
    configs.back().initialization_options = nlohmann::json{{"fallbackFlags", cxx_fallback}};

    // C - clangd（.c/.h 仅 C 头文件补全：stdio.h、stdlib.h 等，不提示 vector 等）
    configs.emplace_back("clangd", "clangd", "c", std::set<std::string>{".c", ".h"},
                         std::vector<std::string>{"--query-driver=/usr/bin/gcc"},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});
    configs.back().initialization_options = nlohmann::json{{"fallbackFlags", {"-std=c11", "-xc"}}};

    // Python - pylsp (Python Language Server Protocol)
    // pylsp 使用 PYTHONPATH 和缓存目录
    configs.emplace_back("pylsp", "python3", "python", std::set<std::string>{".py", ".pyw", ".pyi"},
                         std::vector<std::string>{"-m", "pylsp"},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // Go - gopls
    // gopls 使用 GOMODCACHE 和缓存目录
    configs.emplace_back("gopls", "gopls", "go", std::set<std::string>{".go"},
                         std::vector<std::string>{},
                         std::map<std::string, std::string>{{"GOMODCACHE", cache_dir + "/go"},
                                                            {"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // Rust - rust-analyzer
    // 不覆盖 RUSTUP_HOME/CARGO_HOME，否则 rust-analyzer 找不到工具链；仅设置缓存目录
    configs.emplace_back("rust-analyzer", "rust-analyzer", "rust", std::set<std::string>{".rs"},
                         std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // Java - jdtls (Eclipse JDT Language Server)
    // jdtls 使用工作目录作为配置目录
    configs.emplace_back("jdtls", "jdtls", "java", std::set<std::string>{".java"},
                         std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // JavaScript/TypeScript - typescript-language-server
    configs.emplace_back("typescript-language-server", "typescript-language-server", "typescript",
                         std::set<std::string>{".ts", ".tsx", ".mts", ".cts"},
                         std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // JavaScript - typescript-language-server (也支持 JS)
    configs.emplace_back("typescript-language-server-js", "typescript-language-server",
                         "javascript", std::set<std::string>{".js", ".jsx", ".mjs", ".cjs"},
                         std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // HTML - html-language-server
    configs.emplace_back("html-language-server", "html-languageserver", "html",
                         std::set<std::string>{".html", ".htm"}, std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // CSS - css-language-server
    configs.emplace_back("css-language-server", "css-languageserver", "css",
                         std::set<std::string>{".css", ".scss", ".less", ".sass"},
                         std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // JSON - json-language-server
    configs.emplace_back("json-language-server", "json-languageserver", "json",
                         std::set<std::string>{".json", ".jsonc"}, std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // YAML - yaml-language-server
    configs.emplace_back("yaml-language-server", "yaml-language-server", "yaml",
                         std::set<std::string>{".yaml", ".yml"}, std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // Markdown - marksman 或 markdown-language-server
    configs.emplace_back("marksman", "marksman", "markdown",
                         std::set<std::string>{".md", ".markdown"}, std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    // Shell - bash-language-server
    configs.emplace_back("bash-language-server", "bash-language-server", "shellscript",
                         std::set<std::string>{".sh", ".bash", ".zsh"}, std::vector<std::string>{},
                         std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                                            {"TMPDIR", cache_dir + "/tmp"}});

    return configs;
}

void LspServerConfigManager::initializeDefaultConfigs() {
    configs_ = getDefaultConfigs();
}

void LspServerConfigManager::loadFromConfig(const pnana::core::LspConfig& lsp_config) {
    if (!lsp_config.enabled) {
        configs_.clear();
        return;
    }
    // 始终以代码内置配置为基准
    configs_ = getDefaultConfigs();

    if (lsp_config.servers.empty()) {
        return;
    }

    // 追加用户配置：仅当 language_id 在内置中不存在时添加；冲突时以代码配置为准
    const char* home = std::getenv("HOME");
    std::string cache_dir =
        (home ? std::string(home) : "/tmp") + (home ? "/.config/pnana/.cache" : "/pnana");

    for (const auto& e : lsp_config.servers) {
        if (findConfigByLanguageId(e.language_id) != nullptr) {
            // 该语言已有内置配置，跳过（代码配置优先）
            continue;
        }
        std::set<std::string> ext_set(e.extensions.begin(), e.extensions.end());
        std::map<std::string, std::string> env = e.env;
        if (env.empty()) {
            env["XDG_CACHE_HOME"] = cache_dir;
            env["TMPDIR"] = cache_dir + "/tmp";
        }
        configs_.emplace_back(e.name, e.command, e.language_id, ext_set, e.args, env);
    }
}

const LspServerConfig* LspServerConfigManager::findConfigByExtension(const std::string& ext) const {
    // 转换为小写
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);

    for (const auto& config : configs_) {
        if (config.matchesExtension(lower_ext)) {
            return &config;
        }
    }

    return nullptr;
}

const LspServerConfig* LspServerConfigManager::findConfigByLanguageId(
    const std::string& language_id) const {
    for (const auto& config : configs_) {
        if (config.language_id == language_id) {
            return &config;
        }
    }

    return nullptr;
}

void LspServerConfigManager::addConfig(const LspServerConfig& config) {
    configs_.push_back(config);
}

} // namespace features
} // namespace pnana
