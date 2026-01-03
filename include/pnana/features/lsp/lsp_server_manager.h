#ifndef PNANA_FEATURES_LSP_SERVER_MANAGER_H
#define PNANA_FEATURES_LSP_SERVER_MANAGER_H

#include "features/lsp/lsp_client.h"
#include "features/lsp/lsp_server_config.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace pnana {
namespace features {

/**
 * LSP 服务器管理器
 * 管理多个 LSP 服务器实例，根据文件类型动态选择
 */
class LspServerManager {
  public:
    LspServerManager();
    ~LspServerManager();

    // 根据文件路径获取或创建对应的 LSP 客户端
    // 如果该文件类型没有对应的 LSP 服务器，返回 nullptr
    LspClient* getClientForFile(const std::string& filepath);

    // 根据语言 ID 获取或创建对应的 LSP 客户端
    LspClient* getClientForLanguage(const std::string& language_id);

    // 初始化所有已配置的 LSP 服务器（可选，延迟初始化更高效）
    void initializeAll(const std::string& root_path);

    // 关闭所有 LSP 服务器
    void shutdownAll();

    // 检查文件是否有对应的 LSP 服务器
    bool hasServerForFile(const std::string& filepath) const;

    // 检查语言是否有对应的 LSP 服务器
    bool hasServerForLanguage(const std::string& language_id) const;

    // 设置诊断回调（应用到所有客户端）
    void setDiagnosticsCallback(
        std::function<void(const std::string&, const std::vector<Diagnostic>&)> callback);

    // 获取配置管理器
    LspServerConfigManager& getConfigManager() {
        return config_manager_;
    }
    const LspServerConfigManager& getConfigManager() const {
        return config_manager_;
    }

  private:
    LspServerConfigManager config_manager_;

    // 按语言 ID 存储 LSP 客户端
    // 每个语言只有一个客户端实例（可以处理多个文件）
    std::map<std::string, std::unique_ptr<LspClient>> clients_;
    std::mutex clients_mutex_;

    // 已初始化的语言集合
    std::map<std::string, bool> initialized_;

    // 诊断回调（保存以便为新客户端设置）
    std::function<void(const std::string&, const std::vector<Diagnostic>&)> diagnostics_callback_;

    // 创建新的 LSP 客户端
    std::unique_ptr<LspClient> createClient(const LspServerConfig& config);

    // 初始化单个客户端
    bool initializeClient(const std::string& language_id, const std::string& root_path);

    // 从文件路径提取扩展名
    std::string getExtension(const std::string& filepath) const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_SERVER_MANAGER_H
