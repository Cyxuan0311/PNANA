#ifndef PNANA_FEATURES_LSP_SERVER_CONFIG_H
#define PNANA_FEATURES_LSP_SERVER_CONFIG_H

#include <map>
#include <set>
#include <string>
#include <vector>

namespace pnana {
namespace features {

/**
 * LSP 服务器配置
 * 定义单个语言服务器的配置信息
 */
struct LspServerConfig {
    std::string name;                      // 服务器名称（如 "clangd", "pylsp"）
    std::string command;                   // 启动命令（如 "clangd", "pylsp"）
    std::string language_id;               // LSP 语言 ID（如 "cpp", "python"）
    std::set<std::string> file_extensions; // 支持的文件扩展名（如 {".cpp", ".h", ".c"}）
    std::vector<std::string> args;         // 额外的命令行参数
    std::map<std::string, std::string> env_vars; // 环境变量配置

    LspServerConfig() = default;

    LspServerConfig(const std::string& name, const std::string& command,
                    const std::string& language_id, const std::set<std::string>& extensions)
        : name(name), command(command), language_id(language_id), file_extensions(extensions) {}

    LspServerConfig(const std::string& name, const std::string& command,
                    const std::string& language_id, const std::set<std::string>& extensions,
                    const std::vector<std::string>& args,
                    const std::map<std::string, std::string>& env_vars = {})
        : name(name), command(command), language_id(language_id), file_extensions(extensions),
          args(args), env_vars(env_vars) {}

    // 检查文件扩展名是否匹配
    bool matchesExtension(const std::string& ext) const {
        return file_extensions.find(ext) != file_extensions.end();
    }
};

/**
 * LSP 服务器配置管理器
 * 管理所有可用的 LSP 服务器配置
 */
class LspServerConfigManager {
  public:
    LspServerConfigManager();

    // 获取默认配置（所有常见语言的 LSP 服务器）
    static std::vector<LspServerConfig> getDefaultConfigs();

    // 根据文件扩展名查找匹配的 LSP 服务器配置
    const LspServerConfig* findConfigByExtension(const std::string& ext) const;

    // 根据语言 ID 查找配置
    const LspServerConfig* findConfigByLanguageId(const std::string& language_id) const;

    // 添加自定义配置
    void addConfig(const LspServerConfig& config);

    // 获取所有配置
    const std::vector<LspServerConfig>& getAllConfigs() const {
        return configs_;
    }

  private:
    std::vector<LspServerConfig> configs_;

    void initializeDefaultConfigs();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_SERVER_CONFIG_H
