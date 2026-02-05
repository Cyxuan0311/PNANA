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
    return {"openai",    "anthropic", "google", "mistral",     "groq",
            "together",  "fireworks", "cohere", "huggingface", "perplexity",
            "deepseek",  "zeroone",   "meta",   "xai",         "openrouter",
            "replicate", "bedrock",   "azure",  "ollama",      "vllm"};
}

std::vector<AIModel> AIConfig::getAvailableModels(const std::string& provider) const {
    static std::map<std::string, std::vector<AIModel>> models = {
        {"openai",
         {AIModel("gpt-4o", "GPT-4o", "openai", 128000),
          AIModel("gpt-4o-mini", "GPT-4o Mini", "openai", 128000),
          AIModel("gpt-4-turbo", "GPT-4 Turbo", "openai", 128000),
          AIModel("gpt-4", "GPT-4", "openai", 8192),
          AIModel("gpt-4-32k", "GPT-4 32K", "openai", 32768),
          AIModel("gpt-3.5-turbo", "GPT-3.5 Turbo", "openai", 16385),
          AIModel("gpt-3.5-turbo-16k", "GPT-3.5 Turbo 16K", "openai", 16385),
          AIModel("o1-preview", "O1 Preview", "openai", 200000),
          AIModel("o1-mini", "O1 Mini", "openai", 128000)}},
        {"anthropic",
         {AIModel("claude-3-5-sonnet", "Claude 3.5 Sonnet", "anthropic", 200000),
          AIModel("claude-3-opus", "Claude 3 Opus", "anthropic", 200000),
          AIModel("claude-3-sonnet", "Claude 3 Sonnet", "anthropic", 200000),
          AIModel("claude-3-haiku", "Claude 3 Haiku", "anthropic", 200000),
          AIModel("claude-2.1", "Claude 2.1", "anthropic", 200000),
          AIModel("claude-2.0", "Claude 2.0", "anthropic", 100000),
          AIModel("claude-instant-1.2", "Claude Instant 1.2", "anthropic", 100000)}},
        {"google",
         {AIModel("gemini-2.0-flash-exp", "Gemini 2.0 Flash", "google", 1048576),
          AIModel("gemini-1.5-pro", "Gemini 1.5 Pro", "google", 1048576),
          AIModel("gemini-1.5-flash", "Gemini 1.5 Flash", "google", 1048576),
          AIModel("gemini-pro", "Gemini Pro", "google", 32768),
          AIModel("gemini-pro-vision", "Gemini Pro Vision", "google", 16384),
          AIModel("gemini-ultra", "Gemini Ultra", "google", 32768)}},
        {"mistral",
         {AIModel("mistral-large-latest", "Mistral Large Latest", "mistral", 32768),
          AIModel("mistral-large", "Mistral Large", "mistral", 32768),
          AIModel("mistral-medium", "Mistral Medium", "mistral", 32768),
          AIModel("mistral-small", "Mistral Small", "mistral", 8192),
          AIModel("mixtral-8x7b", "Mixtral 8x7B", "mistral", 32768),
          AIModel("pixtral-12b", "Pixtral 12B", "mistral", 128000),
          AIModel("codestral-latest", "Codestral Latest", "mistral", 32768)}},
        {"groq",
         {AIModel("llama-3.1-70b-versatile", "Llama 3.1 70B", "groq", 131072),
          AIModel("llama-3.1-8b-instant", "Llama 3.1 8B", "groq", 131072),
          AIModel("llama2-70b-4096", "Llama 2 70B", "groq", 4096),
          AIModel("mixtral-8x7b-32768", "Mixtral 8x7B", "groq", 32768),
          AIModel("gemma-7b-it", "Gemma 7B IT", "groq", 8192),
          AIModel("gemma2-9b-it", "Gemma2 9B IT", "groq", 8192)}},
        {"together",
         {AIModel("meta-llama/Llama-3-70b-chat-hf", "Llama 3 70B Chat", "together", 8192),
          AIModel("meta-llama/Llama-3-8b-chat-hf", "Llama 3 8B Chat", "together", 8192),
          AIModel("llama-2-70b-chat", "Llama 2 70B Chat", "together", 4096),
          AIModel("codellama-34b-instruct", "Code Llama 34B", "together", 16384),
          AIModel("wizardlm-13b-v1.2", "WizardLM 13B", "together", 4096),
          AIModel("mistralai/Mixtral-8x7B-Instruct-v0.1", "Mixtral 8x7B Instruct", "together",
                  32768)}},
        {"fireworks",
         {AIModel("llama-v3-70b-instruct", "Llama V3 70B Instruct", "fireworks", 8192),
          AIModel("llama-v2-70b-chat", "Llama V2 70B Chat", "fireworks", 4096),
          AIModel("mixtral-8x7b-instruct", "Mixtral 8x7B Instruct", "fireworks", 32768),
          AIModel("yi-34b-chat", "Yi 34B Chat", "fireworks", 4096),
          AIModel("qwen-72b-instruct", "Qwen 72B Instruct", "fireworks", 32768)}},
        {"cohere",
         {AIModel("command-r-plus", "Command R+", "cohere", 128000),
          AIModel("command-r", "Command R", "cohere", 128000),
          AIModel("command", "Command", "cohere", 4096),
          AIModel("command-light", "Command Light", "cohere", 4096),
          AIModel("command-nightly", "Command Nightly", "cohere", 4096)}},
        {"huggingface",
         {AIModel("meta-llama/Llama-2-70b-chat-hf", "Llama 2 70B Chat", "huggingface", 4096),
          AIModel("mistralai/Mixtral-8x7B-Instruct-v0.1", "Mixtral 8x7B Instruct", "huggingface",
                  32768),
          AIModel("google/flan-t5-xxl", "FLAN-T5 XXL", "huggingface", 512),
          AIModel("bigcode/starcoder", "StarCoder", "huggingface", 8192),
          AIModel("WizardLM/WizardCoder-15B-V1.0", "WizardCoder 15B", "huggingface", 8192)}},
        {"perplexity",
         {AIModel("llama-3.1-sonar-large-128k-online", "Sonar Large Online", "perplexity", 131072),
          AIModel("llama-3.1-sonar-small-128k-online", "Sonar Small Online", "perplexity", 131072),
          AIModel("llama-3.1-sonar-large-128k-chat", "Sonar Large Chat", "perplexity", 131072),
          AIModel("llama-3.1-sonar-small-128k-chat", "Sonar Small Chat", "perplexity", 131072),
          AIModel("llama-3-sonar-large-32k-online", "Sonar Large 32K Online", "perplexity",
                  32768)}},
        {"deepseek",
         {AIModel("deepseek-chat", "DeepSeek Chat", "deepseek", 32000),
          AIModel("deepseek-coder", "DeepSeek Coder", "deepseek", 16000),
          AIModel("deepseek-reasoner", "DeepSeek Reasoner", "deepseek", 64000)}},
        {"zeroone",
         {AIModel("yi-34b-chat", "Yi 34B Chat", "zeroone", 4096),
          AIModel("yi-6b-chat", "Yi 6B Chat", "zeroone", 4096),
          AIModel("yi-34b-chat-200k", "Yi 34B Chat 200K", "zeroone", 200000),
          AIModel("yi-vl-plus", "Yi VL Plus", "zeroone", 4096)}},
        {"meta",
         {AIModel("llama-3-70b-instruct", "Llama 3 70B Instruct", "meta", 8192),
          AIModel("llama-3-8b-instruct", "Llama 3 8B Instruct", "meta", 8192),
          AIModel("llama-2-70b-chat", "Llama 2 70B Chat", "meta", 4096),
          AIModel("llama-2-13b-chat", "Llama 2 13B Chat", "meta", 4096),
          AIModel("code-llama-34b-instruct", "Code Llama 34B Instruct", "meta", 16384)}},
        {"xai",
         {AIModel("grok-beta", "Grok Beta", "xai", 131072),
          AIModel("grok-2", "Grok 2", "xai", 131072),
          AIModel("grok-2-1212", "Grok 2 1212", "xai", 131072)}},
        {"openrouter",
         {AIModel("openai/gpt-4o", "GPT-4o via OpenRouter", "openrouter", 128000),
          AIModel("anthropic/claude-3.5-sonnet", "Claude 3.5 Sonnet via OpenRouter", "openrouter",
                  200000),
          AIModel("meta-llama/llama-3.1-70b-instruct", "Llama 3.1 70B via OpenRouter", "openrouter",
                  131072),
          AIModel("google/gemini-pro-1.5", "Gemini Pro 1.5 via OpenRouter", "openrouter",
                  1048576)}},
        {"replicate",
         {AIModel("meta/llama-2-70b-chat", "Llama 2 70B Chat", "replicate", 4096),
          AIModel("mistralai/mixtral-8x7b-instruct", "Mixtral 8x7B Instruct", "replicate", 32768),
          AIModel("meta/codellama-34b-instruct", "Code Llama 34B", "replicate", 16384)}},
        {"bedrock",
         {AIModel("anthropic.claude-3-5-sonnet-20241022-v2:0", "Claude 3.5 Sonnet", "bedrock",
                  200000),
          AIModel("anthropic.claude-3-opus-20240229-v1:0", "Claude 3 Opus", "bedrock", 200000),
          AIModel("anthropic.claude-3-sonnet-20240229-v1:0", "Claude 3 Sonnet", "bedrock", 200000),
          AIModel("amazon.titan-text-premier-v1:0", "Titan Text Premier", "bedrock", 8192),
          AIModel("meta.llama3-70b-instruct-v1:0", "Llama 3 70B Instruct", "bedrock", 8192)}},
        {"azure",
         {AIModel("gpt-4o", "GPT-4o Azure", "azure", 128000),
          AIModel("gpt-4-turbo", "GPT-4 Turbo Azure", "azure", 128000),
          AIModel("gpt-35-turbo", "GPT-3.5 Turbo Azure", "azure", 16385)}},
        {"ollama",
         {AIModel("llama3", "Llama 3", "ollama", 8192),
          AIModel("llama3:70b", "Llama 3 70B", "ollama", 8192),
          AIModel("mistral", "Mistral", "ollama", 8192),
          AIModel("mixtral", "Mixtral", "ollama", 32768),
          AIModel("codellama", "Code Llama", "ollama", 16384),
          AIModel("deepseek-coder", "DeepSeek Coder", "ollama", 16000),
          AIModel("qwen", "Qwen", "ollama", 32768)}},
        {"vllm",
         {AIModel("meta-llama/Llama-2-70b-chat-hf", "Llama 2 70B Chat", "vllm", 4096),
          AIModel("mistralai/Mixtral-8x7B-Instruct-v0.1", "Mixtral 8x7B Instruct", "vllm", 32768),
          AIModel("codellama/CodeLlama-34b-Instruct-hf", "Code Llama 34B", "vllm", 16384)}}};

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
    } else if (provider == "groq") {
        config.name = "groq";
        config.base_url = "https://api.groq.com/openai/v1";
        config.model = "llama2-70b-4096";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "together") {
        config.name = "together";
        config.base_url = "https://api.together.xyz/v1";
        config.model = "llama-2-70b-chat";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "fireworks") {
        config.name = "fireworks";
        config.base_url = "https://api.fireworks.ai/inference/v1";
        config.model = "llama-v2-70b-chat";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "cohere") {
        config.name = "cohere";
        config.base_url = "https://api.cohere.ai/v1";
        config.model = "command";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "huggingface") {
        config.name = "huggingface";
        config.base_url = "https://api-inference.huggingface.co";
        config.model = "meta-llama/Llama-2-70b-chat-hf";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "perplexity") {
        config.name = "perplexity";
        config.base_url = "https://api.perplexity.ai";
        config.model = "llama-3.1-sonar-small-128k-chat";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "deepseek") {
        config.name = "deepseek";
        config.base_url = "https://api.deepseek.com";
        config.model = "deepseek-chat";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "zeroone") {
        config.name = "zeroone";
        config.base_url = "https://api.01.ai/v1";
        config.model = "yi-34b-chat";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "meta") {
        config.name = "meta";
        config.base_url = "https://api.llama-api.com";
        config.model = "llama-3-70b-instruct";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "xai") {
        config.name = "xai";
        config.base_url = "https://api.x.ai/v1";
        config.model = "grok-beta";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "openrouter") {
        config.name = "openrouter";
        config.base_url = "https://openrouter.ai/api/v1";
        config.model = "openai/gpt-4o";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "replicate") {
        config.name = "replicate";
        config.base_url = "https://api.replicate.com/v1";
        config.model = "meta/llama-2-70b-chat";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "bedrock") {
        config.name = "bedrock";
        config.base_url = "https://bedrock-runtime.us-east-1.amazonaws.com";
        config.model = "anthropic.claude-3-5-sonnet-20241022-v2:0";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "azure") {
        config.name = "azure";
        config.base_url = "https://your-resource.openai.azure.com";
        config.model = "gpt-35-turbo";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "ollama") {
        config.name = "ollama";
        config.base_url = "http://localhost:11434";
        config.model = "llama3";
        config.max_tokens = 2048;
        config.temperature = 0.7f;
    } else if (provider == "vllm") {
        config.name = "vllm";
        config.base_url = "http://localhost:8000/v1";
        config.model = "meta-llama/Llama-2-70b-chat-hf";
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
    } else if (provider == "mistral") {
        // Mistral API key format: similar to OpenAI
        return api_key.length() > 20 && api_key.substr(0, 3) == "sk-";
    } else if (provider == "groq") {
        // Groq API key format: gsk_...
        return api_key.length() > 20 && api_key.substr(0, 4) == "gsk_";
    } else if (provider == "together") {
        // Together API key format: alphanumeric
        return api_key.length() >= 20 && std::regex_match(api_key, std::regex("^[A-Za-z0-9_-]+$"));
    } else if (provider == "fireworks") {
        // Fireworks API key format: alphanumeric
        return api_key.length() >= 20 && std::regex_match(api_key, std::regex("^[A-Za-z0-9_-]+$"));
    } else if (provider == "cohere") {
        // Cohere API key format: alphanumeric
        return api_key.length() >= 20 && std::regex_match(api_key, std::regex("^[A-Za-z0-9_-]+$"));
    } else if (provider == "huggingface") {
        // Hugging Face API key format: hf_...
        return api_key.length() > 20 && api_key.substr(0, 3) == "hf_";
    } else if (provider == "perplexity") {
        // Perplexity API key format: pplx-...
        return api_key.length() > 20 && api_key.substr(0, 5) == "pplx-";
    } else if (provider == "deepseek") {
        // DeepSeek API key format: sk-...
        return api_key.length() > 20 && api_key.substr(0, 3) == "sk-";
    } else if (provider == "zeroone") {
        // 01.AI API key format: alphanumeric
        return api_key.length() >= 20 && std::regex_match(api_key, std::regex("^[A-Za-z0-9_-]+$"));
    } else if (provider == "meta") {
        // Meta API key format: alphanumeric
        return api_key.length() >= 20 && std::regex_match(api_key, std::regex("^[A-Za-z0-9_-]+$"));
    } else if (provider == "xai") {
        // xAI API key format: xai-...
        return api_key.length() > 20 && api_key.substr(0, 4) == "xai-";
    } else if (provider == "openrouter") {
        // OpenRouter API key format: sk-or-...
        return api_key.length() > 20 && api_key.substr(0, 6) == "sk-or-";
    } else if (provider == "replicate") {
        // Replicate API key format: r8_...
        return api_key.length() > 20 && api_key.substr(0, 3) == "r8_";
    } else if (provider == "bedrock") {
        // AWS Bedrock uses AWS credentials, format varies
        return api_key.length() >= 20;
    } else if (provider == "azure") {
        // Azure OpenAI uses API key format: alphanumeric
        return api_key.length() >= 20 && std::regex_match(api_key, std::regex("^[A-Za-z0-9_-]+$"));
    } else if (provider == "ollama") {
        // Ollama typically doesn't require API key for local use
        return true;
    } else if (provider == "vllm") {
        // vLLM typically doesn't require API key for local use
        return true;
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
