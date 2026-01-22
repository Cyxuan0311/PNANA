#include "features/ai_config/ai_config.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

namespace pnana {
namespace features {
namespace ai_config {

AIConfig::AIConfig() {
    // 获取用户配置目录
    const char* home = getenv("HOME");
    if (home) {
        config_file_path_ = std::string(home) + "/.config/pnana/ai_config.json";
    }

    // 初始化默认模型
    initializeDefaultModels();

    // 加载配置
    loadConfig();
}

AIConfig& AIConfig::getInstance() {
    static AIConfig instance;
    return instance;
}

void AIConfig::loadConfig() {
    if (!fs::exists(config_file_path_)) {
        // 如果配置文件不存在，使用默认配置
        current_config_ = getDefaultConfig();
        return;
    }

    try {
        std::ifstream file(config_file_path_);
        nlohmann::json j;
        file >> j;

        // 加载当前配置
        if (j.contains("current")) {
            current_config_ = AIProviderConfig::fromJson(j["current"]);
        } else {
            current_config_ = getDefaultConfig();
        }

        // 加载提供商配置
        if (j.contains("providers")) {
            for (const auto& [provider, config_json] : j["providers"].items()) {
                provider_configs_[provider] = AIProviderConfig::fromJson(config_json);
            }
        }

    } catch (const std::exception& e) {
        // 配置加载失败，使用默认配置
        current_config_ = getDefaultConfig();
    }
}

void AIConfig::saveConfig() const {
    // 确保配置目录存在
    fs::path config_path(config_file_path_);
    fs::create_directories(config_path.parent_path());

    try {
        nlohmann::json j;

        // 保存当前配置
        j["current"] = current_config_.toJson();

        // 保存所有提供商配置
        for (const auto& [provider, config] : provider_configs_) {
            j["providers"][provider] = config.toJson();
        }

        std::ofstream file(config_file_path_);
        file << j.dump(2);

    } catch (const std::exception& e) {
        // 保存失败，暂时忽略
    }
}

std::vector<std::string> AIConfig::getAvailableProviders() const {
    return {"openai", "anthropic", "google", "mistral", "groq", "together", "fireworks"};
}

std::vector<AIModel> AIConfig::getAvailableModels(const std::string& provider) const {
    static std::map<std::string, std::vector<AIModel>> models = {
        {"openai",
         {AIModel("gpt-4o", "GPT-4o", "openai", 128000),
          AIModel("gpt-4o-mini", "GPT-4o Mini", "openai", 128000),
          AIModel("gpt-4-turbo", "GPT-4 Turbo", "openai", 128000),
          AIModel("gpt-4", "GPT-4", "openai", 8192),
          AIModel("gpt-3.5-turbo", "GPT-3.5 Turbo", "openai", 16385),
          AIModel("gpt-3.5-turbo-16k", "GPT-3.5 Turbo 16K", "openai", 16385)}},
        {"anthropic",
         {AIModel("claude-3-opus", "Claude 3 Opus", "anthropic", 200000),
          AIModel("claude-3-sonnet", "Claude 3 Sonnet", "anthropic", 200000),
          AIModel("claude-3-haiku", "Claude 3 Haiku", "anthropic", 200000),
          AIModel("claude-2.1", "Claude 2.1", "anthropic", 200000)}},
        {"google",
         {AIModel("gemini-pro", "Gemini Pro", "google", 32768),
          AIModel("gemini-pro-vision", "Gemini Pro Vision", "google", 16384),
          AIModel("gemini-1.5-pro", "Gemini 1.5 Pro", "google", 1048576),
          AIModel("gemini-1.5-flash", "Gemini 1.5 Flash", "google", 1048576)}},
        {"mistral",
         {AIModel("mistral-large", "Mistral Large", "mistral", 32768),
          AIModel("mistral-medium", "Mistral Medium", "mistral", 32768),
          AIModel("mistral-small", "Mistral Small", "mistral", 8192),
          AIModel("mixtral-8x7b", "Mixtral 8x7B", "mistral", 32768)}},
        {"groq",
         {AIModel("llama2-70b-4096", "Llama 2 70B", "groq", 4096),
          AIModel("mixtral-8x7b-32768", "Mixtral 8x7B", "groq", 32768),
          AIModel("gemma-7b-it", "Gemma 7B IT", "groq", 8192)}},
        {"together",
         {AIModel("llama-2-70b-chat", "Llama 2 70B Chat", "together", 4096),
          AIModel("codellama-34b-instruct", "Code Llama 34B", "together", 16384),
          AIModel("wizardlm-13b-v1.2", "WizardLM 13B", "together", 4096)}},
        {"fireworks",
         {AIModel("llama-v2-70b-chat", "Llama V2 70B Chat", "fireworks", 4096),
          AIModel("mixtral-8x7b-instruct", "Mixtral 8x7B Instruct", "fireworks", 32768),
          AIModel("yi-34b-chat", "Yi 34B Chat", "fireworks", 4096)}}};

    if (provider.empty()) {
        std::vector<AIModel> all_models;
        for (const auto& [prov, model_list] : models) {
            all_models.insert(all_models.end(), model_list.begin(), model_list.end());
        }
        return all_models;
    }

    auto it = models.find(provider);
    return it != models.end() ? it->second : std::vector<AIModel>{};
}

void AIConfig::setCurrentConfig(const AIProviderConfig& config) {
    if (validateConfig(config)) {
        current_config_ = config;
        provider_configs_[config.name] = config;
        saveConfig();
    }
}

AIProviderConfig AIConfig::getProviderConfig(const std::string& provider) const {
    auto it = provider_configs_.find(provider);
    if (it != provider_configs_.end()) {
        return it->second;
    }
    return getDefaultConfig(provider);
}

void AIConfig::setProviderConfig(const std::string& provider, const AIProviderConfig& config) {
    provider_configs_[provider] = config;
    if (current_config_.name == provider) {
        current_config_ = config;
    }
    saveConfig();
}

bool AIConfig::validateConfig(const AIProviderConfig& config) const {
    validation_error_.clear();

    if (config.name.empty()) {
        validation_error_ = "Provider name cannot be empty";
        return false;
    }

    if (config.api_key.empty()) {
        validation_error_ = "API key cannot be empty";
        return false;
    }

    if (!validateApiKey(config.name, config.api_key)) {
        validation_error_ = "Invalid API key format for provider: " + config.name;
        return false;
    }

    if (config.base_url.empty()) {
        validation_error_ = "Base URL cannot be empty";
        return false;
    }

    if (config.model.empty()) {
        validation_error_ = "Model cannot be empty";
        return false;
    }

    if (config.max_tokens <= 0 || config.max_tokens > 32768) {
        validation_error_ = "Max tokens must be between 1 and 32768";
        return false;
    }

    if (config.temperature < 0.0f || config.temperature > 2.0f) {
        validation_error_ = "Temperature must be between 0.0 and 2.0";
        return false;
    }

    return true;
}

AIProviderConfig AIConfig::getDefaultConfig(const std::string& provider) {
    AIProviderConfig config;

    if (provider == "openai" || provider.empty()) {
        config.name = "openai";
        config.base_url = "https://api.openai.com/v1";
        config.model = "gpt-3.5-turbo";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "anthropic") {
        config.name = "anthropic";
        config.base_url = "https://api.anthropic.com";
        config.model = "claude-3-haiku";
        config.max_tokens = 4096;
        config.temperature = 0.7f;
    } else if (provider == "google") {
        config.name = "google";
        config.base_url = "https://generativelanguage.googleapis.com";
        config.model = "gemini-pro";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "mistral") {
        config.name = "mistral";
        config.base_url = "https://api.mistral.ai";
        config.model = "mistral-small";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else {
        // 默认回退到OpenAI
        config.name = provider.empty() ? "openai" : provider;
        config.base_url = "https://api.openai.com/v1";
        config.model = "gpt-3.5-turbo";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    }

    return config;
}

void AIConfig::initializeDefaultModels() {
    // 默认模型已在getAvailableModels中定义
}

bool AIConfig::validateApiKey(const std::string& provider, const std::string& api_key) const {
    if (api_key.empty())
        return false;

    if (provider == "openai") {
        // OpenAI API key format: sk-...
        return api_key.length() > 20 && api_key.substr(0, 3) == "sk-";
    } else if (provider == "anthropic") {
        // Anthropic API key format: sk-ant-...
        return api_key.length() > 30 && api_key.substr(0, 7) == "sk-ant-";
    } else if (provider == "google") {
        // Google API key is typically a long alphanumeric string
        return api_key.length() >= 20 && std::regex_match(api_key, std::regex("^[A-Za-z0-9_-]+$"));
    }

    // 对于其他提供商，基本验证长度
    return api_key.length() >= 10;
}

// AIProviderConfig JSON序列化
AIProviderConfig AIProviderConfig::fromJson(const nlohmann::json& j) {
    AIProviderConfig config;
    config.name = j.value("name", "");
    config.api_key = j.value("api_key", "");
    config.base_url = j.value("base_url", "");
    config.model = j.value("model", "");
    config.max_tokens = j.value("max_tokens", 2048);
    config.temperature = j.value("temperature", 0.7f);
    config.enabled = j.value("enabled", true);
    return config;
}

nlohmann::json AIProviderConfig::toJson() const {
    return {{"name", name},      {"api_key", api_key},       {"base_url", base_url},
            {"model", model},    {"max_tokens", max_tokens}, {"temperature", temperature},
            {"enabled", enabled}};
}

} // namespace ai_config
} // namespace features
} // namespace pnana
