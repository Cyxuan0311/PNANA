#include "features/lsp/lsp_server_config.h"
#include <algorithm>
#include <map>

namespace pnana {
namespace features {

LspServerConfigManager::LspServerConfigManager() {
    initializeDefaultConfigs();
}

std::vector<LspServerConfig> LspServerConfigManager::getDefaultConfigs() {
    std::vector<LspServerConfig> configs;

    // 获取缓存目录路径
    std::string cache_dir = std::string(getenv("HOME")) + "/.config/pnana/.cache";

    // C/C++ - clangd
    // clangd 使用 XDG_CACHE_HOME 环境变量来指定缓存目录
    configs.emplace_back(
        "clangd", "clangd", "cpp",
        std::set<std::string>{".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".h", ".c", ".c++", ".h++"},
        std::vector<std::string>{}, // 不添加特殊参数，让 clangd 使用默认行为
        std::map<std::string, std::string>{{"XDG_CACHE_HOME", cache_dir},
                                           {"TMPDIR", cache_dir + "/tmp"}});

    // Python - pylsp (Python Language Server Protocol)
    // pylsp 使用 PYTHONPATH 和缓存目录
    configs.emplace_back("pylsp", "pylsp", "python", std::set<std::string>{".py", ".pyw", ".pyi"},
                         std::vector<std::string>{},
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
    // rust-analyzer 使用 RUSTUP_HOME 和缓存目录
    configs.emplace_back("rust-analyzer", "rust-analyzer", "rust", std::set<std::string>{".rs"},
                         std::vector<std::string>{},
                         std::map<std::string, std::string>{{"RUSTUP_HOME", cache_dir + "/rustup"},
                                                            {"CARGO_HOME", cache_dir + "/cargo"},
                                                            {"XDG_CACHE_HOME", cache_dir},
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
