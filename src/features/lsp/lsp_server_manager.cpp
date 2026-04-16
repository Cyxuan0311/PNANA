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
    std::string ext = getExtension(filepath);
    if (ext.empty()) {
        return nullptr;
    }

    const LspServerConfig* config = config_manager_.findConfigByExtension(ext);
    if (!config) {
        return nullptr;
    }

    return getClientForLanguage(config->language_id);
}

LspClient* LspServerManager::getClientForLanguage(const std::string& language_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = clients_.find(language_id);
    if (it != clients_.end()) {
        return it->second.get();
    }

    const LspServerConfig* config = config_manager_.findConfigByLanguageId(language_id);
    if (!config) {
        return nullptr;
    }

    auto client = createClient(*config);
    if (!client) {
        return nullptr;
    }

    if (diagnostics_callback_) {
        client->setDiagnosticsCallback(diagnostics_callback_);
    }

    LspClient* client_ptr = client.get();
    clients_[language_id] = std::move(client);

    return client_ptr;
}

std::unique_ptr<LspClient> LspServerManager::createClient(const LspServerConfig& config) {
    for (const auto& [key, value] : config.env_vars) {
        if (key == "XDG_CACHE_HOME" || key == "TMPDIR" || key == "GOMODCACHE" ||
            key == "RUSTUP_HOME" || key == "CARGO_HOME") {
            mkdir(value.c_str(), 0755);
        }
    }

    std::string full_command = config.command;
    for (const auto& arg : config.args) {
        full_command += " " + arg;
    }

    return std::make_unique<LspClient>(full_command, config.env_vars);
}

bool LspServerManager::initializeClientForFile(const std::string& filepath,
                                               const std::string& root_path) {
    LspClient* client = getClientForFile(filepath);
    if (!client) {
        return false;
    }
    if (client->isConnected()) {
        return true;
    }
    std::string ext = getExtension(filepath);
    const LspServerConfig* config = config_manager_.findConfigByExtension(ext);
    const nlohmann::json* init_opts =
        (config && config->initialization_options) ? &*config->initialization_options : nullptr;
    return client->initialize(root_path, init_opts);
}

bool LspServerManager::initializeClient(const std::string& language_id,
                                        const std::string& root_path) {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    if (initialized_.find(language_id) != initialized_.end() && initialized_[language_id]) {
        return true;
    }

    auto it = clients_.find(language_id);
    if (it == clients_.end()) {
        return false;
    }

    try {
        const LspServerConfig* config = config_manager_.findConfigByLanguageId(language_id);
        const nlohmann::json* init_opts =
            (config && config->initialization_options) ? &*config->initialization_options : nullptr;
        if (it->second->initialize(root_path, init_opts)) {
            initialized_[language_id] = true;
            return true;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("LSP initialization failed for " + language_id + ": " + e.what());
    } catch (...) {
        LOG_ERROR("LSP initialization failed for " + language_id + ": unknown exception");
    }

    initialized_[language_id] = false;
    return false;
}

void LspServerManager::initializeAll(const std::string& /* root_path */) {
    // 延迟初始化：只在需要时初始化
    // 这里可以选择性地预初始化某些服务器
    // root_path 参数保留用于未来扩展
}

void LspServerManager::shutdownAll() {
    // 注意：此方法通常在析构函数中调用，此时不应有其他线程访问
    // 因此不需要加锁，直接操作即可

    // 先复制一份客户端列表，避免在迭代过程中 map 被修改
    std::vector<std::pair<std::string, std::unique_ptr<LspClient>>> clients_to_shutdown;
    for (auto& [language_id, client] : clients_) {
        if (client && initialized_[language_id]) {
            clients_to_shutdown.emplace_back(language_id, std::move(client));
        }
    }

    // 清空原始 map
    clients_.clear();
    initialized_.clear();

    // 逐个关闭客户端
    for (auto& [language_id, client] : clients_to_shutdown) {
        try {
            if (client && client->isConnected()) {
                client->shutdown();
            }
        } catch (const std::system_error& e) {
            // 忽略 system_error（通常是 mutex 或线程相关错误）
            LOG("LSP shutdown error for " + language_id + ": " + e.what());
        } catch (...) {
            // 忽略其他错误
            LOG("LSP shutdown error for " + language_id);
        }
    }
}

bool LspServerManager::hasServerForFile(const std::string& filepath) const {
    std::string ext = getExtension(filepath);
    if (ext.empty()) {
        return false;
    }
    return config_manager_.findConfigByExtension(ext) != nullptr;
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

std::vector<LspStatusEntry> LspServerManager::getStatusSnapshot() const {
    std::vector<LspStatusEntry> out;
    const std::vector<LspServerConfig>& configs = config_manager_.getAllConfigs();
    out.reserve(configs.size());

    std::lock_guard<std::mutex> lock(clients_mutex_);
    for (const auto& config : configs) {
        LspStatusEntry entry;
        entry.config = config;
        auto it = clients_.find(config.language_id);
        entry.connected = (it != clients_.end() && it->second && it->second->isConnected());
        out.push_back(std::move(entry));
    }
    return out;
}

int LspServerManager::getServerPid(const std::string& language_id) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(language_id);
    if (it == clients_.end() || !it->second) {
        return -1;
    }
    try {
        return it->second->getServerPid();
    } catch (...) {
        return -1;
    }
}

} // namespace features
} // namespace pnana
