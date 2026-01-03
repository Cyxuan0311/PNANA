#include "features/lsp/lsp_server_manager.h"
#include <filesystem>
#include <algorithm>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace pnana {
namespace features {

LspServerManager::LspServerManager() {
}

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
    
    // 检查是否已存在客户端
    auto it = clients_.find(language_id);
    if (it != clients_.end()) {
        return it->second.get();
    }
    
    // 查找配置
    const LspServerConfig* config = config_manager_.findConfigByLanguageId(language_id);
    if (!config) {
        return nullptr;
    }
    
    // 创建新客户端
    auto client = createClient(*config);
    if (!client) {
        return nullptr;
    }
    
    // 如果已设置诊断回调，应用到新客户端
    if (diagnostics_callback_) {
        client->setDiagnosticsCallback(diagnostics_callback_);
    }
    
    LspClient* client_ptr = client.get();
    clients_[language_id] = std::move(client);
    
    return client_ptr;
}

std::unique_ptr<LspClient> LspServerManager::createClient(const LspServerConfig& config) {
    // 确保缓存目录存在
    for (const auto& [key, value] : config.env_vars) {
        if (key == "XDG_CACHE_HOME" || key == "TMPDIR" ||
            key == "GOMODCACHE" || key == "RUSTUP_HOME" || key == "CARGO_HOME") {
            // 创建目录
            mkdir(value.c_str(), 0755);
        }
    }

    // 构建完整命令（包括参数）
    std::string full_command = config.command;
    for (const auto& arg : config.args) {
        full_command += " " + arg;
    }
    
    return std::make_unique<LspClient>(full_command, config.env_vars);
}

bool LspServerManager::initializeClient(const std::string& language_id, 
                                         const std::string& root_path) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    // 检查是否已初始化
    if (initialized_.find(language_id) != initialized_.end() && initialized_[language_id]) {
        return true;
    }
    
    auto it = clients_.find(language_id);
    if (it == clients_.end()) {
        return false;
    }
    
    try {
        if (it->second->initialize(root_path)) {
            initialized_[language_id] = true;
            return true;
        }
    } catch (...) {
        // 初始化失败，但不抛出异常
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

} // namespace features
} // namespace pnana

