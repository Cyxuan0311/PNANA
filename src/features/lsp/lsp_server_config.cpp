#include "features/lsp/lsp_server_config.h"
#include "utils/clangd_flags.h"
#include <algorithm>
#include <cstdlib>
#include <map>

#include <nlohmann/json.hpp>

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
    auto cxx_fallback = utils::getClangdFallbackFlags();
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

namespace {

// 将用户配置项与内置配置合并：用户提供的非空字段优先，空字段用内置兜底。
// 这样“以配置文件为主”，若配置中某项未填或不可用，仍可回退到内置值。
LspServerConfig mergeUserWithBuiltin(const pnana::core::LspServerConfigEntry& user,
                                     const LspServerConfig& builtin, const std::string& cache_dir) {
    LspServerConfig out;
    out.name = user.name.empty() ? builtin.name : user.name;
    out.command = user.command.empty() ? builtin.command : user.command;
    out.language_id = user.language_id.empty() ? builtin.language_id : user.language_id;
    if (user.extensions.empty()) {
        out.file_extensions = builtin.file_extensions;
    } else {
        out.file_extensions = std::set<std::string>(user.extensions.begin(), user.extensions.end());
    }
    out.args = user.args.empty() ? builtin.args : user.args;
    if (user.env.empty()) {
        out.env_vars = builtin.env_vars;
    } else {
        out.env_vars = user.env;
        // 若用户未设置缓存相关变量，用默认值补全
        if (out.env_vars.find("XDG_CACHE_HOME") == out.env_vars.end())
            out.env_vars["XDG_CACHE_HOME"] = cache_dir;
        if (out.env_vars.find("TMPDIR") == out.env_vars.end())
            out.env_vars["TMPDIR"] = cache_dir + "/tmp";
    }
    out.initialization_options = builtin.initialization_options;
    return out;
}

// 仅用用户配置项生成 LspServerConfig（用于新增语言，无内置）
LspServerConfig entryToConfig(const pnana::core::LspServerConfigEntry& e,
                              const std::string& cache_dir) {
    std::set<std::string> ext_set(e.extensions.begin(), e.extensions.end());
    std::map<std::string, std::string> env = e.env;
    if (env.empty()) {
        env["XDG_CACHE_HOME"] = cache_dir;
        env["TMPDIR"] = cache_dir + "/tmp";
    }
    LspServerConfig c(e.name, e.command, e.language_id, ext_set, e.args, env);
    return c;
}

} // namespace

void LspServerConfigManager::loadFromConfig(const pnana::core::LspConfig& lsp_config) {
    if (!lsp_config.enabled) {
        configs_.clear();
        return;
    }

    std::vector<LspServerConfig> builtin = getDefaultConfigs();
    const char* home = std::getenv("HOME");
    std::string cache_dir =
        (home ? std::string(home) : "/tmp") + (home ? "/.config/pnana/.cache" : "/pnana");

    // 按 language_id 建立内置配置索引，便于用用户配置覆盖
    std::map<std::string, size_t> builtin_index;
    for (size_t i = 0; i < builtin.size(); ++i)
        builtin_index[builtin[i].language_id] = i;

    configs_.clear();

    // 1) 先处理内置列表：若用户对同一 language_id
    // 有配置则用合并结果（配置优先，空项用内置），否则用内置
    for (size_t i = 0; i < builtin.size(); ++i) {
        const std::string& lang = builtin[i].language_id;
        auto it = std::find_if(lsp_config.servers.begin(), lsp_config.servers.end(),
                               [&lang](const pnana::core::LspServerConfigEntry& e) {
                                   return e.language_id == lang;
                               });
        if (it != lsp_config.servers.end()) {
            configs_.push_back(mergeUserWithBuiltin(*it, builtin[i], cache_dir));
        } else {
            configs_.push_back(builtin[i]);
        }
    }

    // 2) 再追加用户中“仅新增”的语言（language_id 不在内置里）
    for (const auto& e : lsp_config.servers) {
        if (builtin_index.find(e.language_id) != builtin_index.end())
            continue;
        if (e.name.empty() || e.command.empty() || e.language_id.empty())
            continue;
        configs_.push_back(entryToConfig(e, cache_dir));
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
