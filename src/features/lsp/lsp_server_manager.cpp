#include "features/lsp/lsp_server_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <filesystem>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace pnana {
namespace features {

LspServerManager::LspServerManager() {}

LspServerManager::~LspServerManager() {
    shutdownAll();
}

std::string LspServerManager::getExtension(const std::string& filepath) const {
    if (filepath.empty()) {
        return "";
    }

    fs::path path(filepath);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

LspClient* LspServerManager::getClientForFile(const std::string& filepath) {
    LOG("[LSP DEBUG] getClientForFile called for: " + filepath);
    std::string ext = getExtension(filepath);
    LOG("[LSP DEBUG] File extension detected: '" + ext + "'");

    if (ext.empty()) {
        LOG("[LSP DEBUG] Empty extension, returning nullptr");
        return nullptr;
    }

    const LspServerConfig* config = config_manager_.findConfigByExtension(ext);
    if (!config) {
        LOG("[LSP DEBUG] No LSP config found for extension: " + ext);
        return nullptr;
    }

    LOG("[LSP DEBUG] Found LSP config: " + config->name + " for language: " + config->language_id);
    return getClientForLanguage(config->language_id);
}

LspClient* LspServerManager::getClientForLanguage(const std::string& language_id) {
    LOG("[LSP DEBUG] getClientForLanguage called for language: " + language_id);
    std::lock_guard<std::mutex> lock(clients_mutex_);

    // 检查是否已存在客户端
    auto it = clients_.find(language_id);
    if (it != clients_.end()) {
        LOG("[LSP DEBUG] Client already exists for language: " + language_id);
        // 客户端已存在，只返回指针，不在这里初始化
        // 初始化将在实际使用时进行（延迟初始化）
        return it->second.get();
    }

    LOG("[LSP DEBUG] No existing client found, creating new client for: " + language_id);

    // 查找配置
    const LspServerConfig* config = config_manager_.findConfigByLanguageId(language_id);
    if (!config) {
        LOG("[LSP DEBUG] No LSP config found for language: " + language_id);
        return nullptr;
    }

    LOG("[LSP DEBUG] Found config: " + config->name + ", command: " + config->command);

    // 创建新客户端
    auto client = createClient(*config);
    if (!client) {
        LOG("[LSP DEBUG] Failed to create client for language: " + language_id);
        return nullptr;
    }

    LOG("[LSP DEBUG] Successfully created client for language: " + language_id);

    // 如果已设置诊断回调，应用到新客户端
    if (diagnostics_callback_) {
        client->setDiagnosticsCallback(diagnostics_callback_);
        LOG("[LSP DEBUG] Applied diagnostics callback to client");
    }

    LspClient* client_ptr = client.get();
    clients_[language_id] = std::move(client);

    LOG("[LSP DEBUG] Client stored and ready for language: " + language_id);

    // 不在这里初始化，采用延迟初始化策略

    return client_ptr;
}

std::unique_ptr<LspClient> LspServerManager::createClient(const LspServerConfig& config) {
    LOG("[LSP DEBUG] createClient called for server: " + config.name);

    // 确保缓存目录存在
    for (const auto& [key, value] : config.env_vars) {
        if (key == "XDG_CACHE_HOME" || key == "TMPDIR" || key == "GOMODCACHE" ||
            key == "RUSTUP_HOME" || key == "CARGO_HOME") {
            LOG("[LSP DEBUG] Creating directory: " + value);
            // 创建目录
            mkdir(value.c_str(), 0755);
        }
    }

    // 构建完整命令（包括参数）
    std::string full_command = config.command;
    for (const auto& arg : config.args) {
        full_command += " " + arg;
    }

    LOG("[LSP DEBUG] Full command to execute: " + full_command);

    auto client = std::make_unique<LspClient>(full_command, config.env_vars);
    LOG("[LSP DEBUG] LspClient created successfully for: " + config.name);

    return client;
}

bool LspServerManager::initializeClient(const std::string& language_id,
                                        const std::string& root_path) {
    LOG("[LSP DEBUG] initializeClient called for language: " + language_id +
        ", root_path: " + root_path);
    std::lock_guard<std::mutex> lock(clients_mutex_);

    // 检查是否已初始化
    if (initialized_.find(language_id) != initialized_.end() && initialized_[language_id]) {
        LOG("[LSP DEBUG] Client already initialized for language: " + language_id);
        return true;
    }

    auto it = clients_.find(language_id);
    if (it == clients_.end()) {
        LOG("[LSP DEBUG] No client found for language: " + language_id + ", cannot initialize");
        return false;
    }

    LOG("[LSP DEBUG] Starting initialization for language: " + language_id);
    try {
        if (it->second->initialize(root_path)) {
            initialized_[language_id] = true;
            LOG("[LSP DEBUG] Successfully initialized client for language: " + language_id);
            return true;
        } else {
            LOG("[LSP DEBUG] Initialization returned false for language: " + language_id);
        }
    } catch (const std::exception& e) {
        LOG("[LSP ERROR] Exception during initialization for language: " + language_id +
            ", error: " + e.what());
    } catch (...) {
        LOG("[LSP ERROR] Unknown exception during initialization for language: " + language_id);
    }

    initialized_[language_id] = false;
    LOG("[LSP DEBUG] Failed to initialize client for language: " + language_id);
    return false;
}

void LspServerManager::initializeAll(const std::string& /* root_path */) {
    // 延迟初始化：只在需要时初始化
    // 这里可以选择性地预初始化某些服务器
    // root_path 参数保留用于未来扩展
}

void LspServerManager::shutdownAll() {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    for (auto& [language_id, client] : clients_) {
        if (client && initialized_[language_id]) {
            try {
                client->shutdown();
            } catch (...) {
                // 忽略关闭时的错误
            }
        }
    }

    clients_.clear();
    initialized_.clear();
}

bool LspServerManager::hasServerForFile(const std::string& filepath) const {
    LOG("[LSP DEBUG] hasServerForFile called for: " + filepath);
    std::string ext = getExtension(filepath);
    LOG("[LSP DEBUG] File extension for server check: '" + ext + "'");

    if (ext.empty()) {
        LOG("[LSP DEBUG] Empty extension, no server support");
        return false;
    }

    bool has_config = config_manager_.findConfigByExtension(ext) != nullptr;
    LOG("[LSP DEBUG] Server support for extension '" + ext + "': " + (has_config ? "YES" : "NO"));
    return has_config;
}

bool LspServerManager::hasServerForLanguage(const std::string& language_id) const {
    return config_manager_.findConfigByLanguageId(language_id) != nullptr;
}

void LspServerManager::setDiagnosticsCallback(
    std::function<void(const std::string&, const std::vector<Diagnostic>&)> callback) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    // 为所有现有客户端设置回调
    for (auto& [language_id, client] : clients_) {
        if (client) {
            client->setDiagnosticsCallback(callback);
        }
    }

    // 保存回调，以便为新创建的客户端设置
    diagnostics_callback_ = callback;
}

} // namespace features
} // namespace pnana
