#ifndef PNANA_FEATURES_AI_CONFIG_AI_CONFIG_H
#define PNANA_FEATURES_AI_CONFIG_AI_CONFIG_H

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace ai_config {

// AI模型信息
struct AIModel {
    std::string id;
    std::string name;
    std::string provider; // openai, anthropic, google, etc.
    int context_window;

    AIModel(const std::string& i, const std::string& n, const std::string& p, int cw)
        : id(i), name(n), provider(p), context_window(cw) {}
};

// AI提供商配置
struct AIProviderConfig {
    std::string name;
    std::string api_key;
    std::string base_url;
    std::string model;
    int max_tokens = 2048;
    float temperature = 0.7f;
    bool enabled = true;

    // 从JSON加载
    static AIProviderConfig fromJson(const nlohmann::json& j);
    // 转换为JSON
    nlohmann::json toJson() const;
};

// AI配置管理器
class AIConfig {
  public:
    AIConfig();
    ~AIConfig() = default;

    // 禁用拷贝
    AIConfig(const AIConfig&) = delete;
    AIConfig& operator=(const AIConfig&) = delete;

    // 获取单例实例
    static AIConfig& getInstance();

    // 配置管理
    void loadConfig();
    void saveConfig() const;
    bool hasConfig() const {
        return !config_file_path_.empty();
    }

    // 提供商管理
    std::vector<std::string> getAvailableProviders() const;
    std::vector<AIModel> getAvailableModels(const std::string& provider = "") const;

    // 当前配置
    AIProviderConfig getCurrentConfig() const {
        return current_config_;
    }
    void setCurrentConfig(const AIProviderConfig& config);

    // 提供商特定配置
    AIProviderConfig getProviderConfig(const std::string& provider) const;
    void setProviderConfig(const std::string& provider, const AIProviderConfig& config);

    // 验证配置
    bool validateConfig(const AIProviderConfig& config) const;
    std::string getValidationError() const {
        return validation_error_;
    }

    // 默认配置
    static AIProviderConfig getDefaultConfig(const std::string& provider = "openai");

  private:
    std::string config_file_path_;
    AIProviderConfig current_config_;
    std::map<std::string, AIProviderConfig> provider_configs_;
    mutable std::string validation_error_;

    // 初始化默认模型
    void initializeDefaultModels();

    // 验证API密钥格式
    bool validateApiKey(const std::string& provider, const std::string& api_key) const;
};

} // namespace ai_config
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_AI_CONFIG_AI_CONFIG_H
