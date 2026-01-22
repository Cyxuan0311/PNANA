#ifndef PNANA_FEATURES_AI_CLIENT_AI_CLIENT_H
#define PNANA_FEATURES_AI_CLIENT_AI_CLIENT_H

#include "features/ai_config/ai_config.h"
#include <functional>
#include <future>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#ifdef BUILD_AI_CLIENT_SUPPORT

namespace pnana {
namespace features {
namespace ai_client {

// 工具参数结构
struct ToolParameter {
    std::string name;
    std::string type;
    std::string description;
    bool required = false;
    nlohmann::json schema;
};

// 工具定义结构
struct ToolDefinition {
    std::string name;
    std::string description;
    std::vector<ToolParameter> parameters;
};

// 工具调用结果
struct ToolCallResult {
    std::string tool_call_id;
    std::string tool_name;
    nlohmann::json result;
    bool success = false;
    std::string error_message;
};

// AI请求结构
struct AIRequest {
    std::string prompt;
    std::string system_message;
    std::vector<std::string> context;  // 代码上下文
    std::vector<ToolDefinition> tools; // 可用工具
    int max_tokens = 2048;
    float temperature = 0.7f;
    bool stream = false;
    bool enable_tool_calling = false; // 启用工具调用
};

// 工具调用结构
struct ToolCall {
    std::string id;
    std::string function_name;
    nlohmann::json arguments;
};

// AI响应结构
struct AIResponse {
    std::string content;
    std::string model_used;
    int tokens_used = 0;
    bool success = false;
    std::string error_message;
    std::vector<ToolCall> tool_calls; // 工具调用列表
};

// 流式响应回调
using StreamingCallback = std::function<void(const std::string& chunk, bool is_finished)>;

// 工具调用回调
using ToolCallCallback = std::function<ToolCallResult(const ToolCall& tool_call)>;

// AI客户端接口
class AIClient {
  public:
    AIClient();
    virtual ~AIClient() = default;

    // 设置工具调用回调
    virtual void setToolCallCallback(ToolCallCallback callback) = 0;

    // 发送请求
    virtual std::future<AIResponse> sendRequest(const AIRequest& request) = 0;

    // 流式请求
    virtual void sendStreamingRequest(const AIRequest& request, StreamingCallback callback) = 0;

    // 取消请求
    virtual void cancelRequest() = 0;

    // 检查是否支持流式
    virtual bool supportsStreaming() const = 0;

    // 检查是否支持工具调用
    virtual bool supportsToolCalling() const = 0;

    // 获取提供商名称
    virtual std::string getProviderName() const = 0;
};

// OpenAI客户端
class OpenAIClient : public AIClient {
  public:
    OpenAIClient(const ai_config::AIProviderConfig& config);
    ~OpenAIClient() override = default;

    void setToolCallCallback(ToolCallCallback callback) override;
    std::future<AIResponse> sendRequest(const AIRequest& request) override;
    void sendStreamingRequest(const AIRequest& request, StreamingCallback callback) override;
    void cancelRequest() override;
    bool supportsStreaming() const override {
        return true;
    }
    bool supportsToolCalling() const override {
        return true;
    }
    std::string getProviderName() const override {
        return "OpenAI";
    }

  private:
    ai_config::AIProviderConfig config_;
    std::atomic<bool> cancel_flag_;
    ToolCallCallback tool_call_callback_;

    nlohmann::json buildRequestJson(const AIRequest& request) const;
    AIResponse parseResponseJson(const nlohmann::json& response) const;
    std::string makeHttpRequest(const std::string& url, const std::string& method,
                                const std::string& body, const std::string& content_type) const;
};

// Claude客户端
class ClaudeClient : public AIClient {
  public:
    ClaudeClient(const ai_config::AIProviderConfig& config);
    ~ClaudeClient() override = default;

    void setToolCallCallback(ToolCallCallback callback) override;
    std::future<AIResponse> sendRequest(const AIRequest& request) override;
    void sendStreamingRequest(const AIRequest& request, StreamingCallback callback) override;
    void cancelRequest() override;
    bool supportsStreaming() const override {
        return true;
    }
    bool supportsToolCalling() const override {
        return true;
    }
    std::string getProviderName() const override {
        return "Claude";
    }

  private:
    ai_config::AIProviderConfig config_;
    std::atomic<bool> cancel_flag_;
    ToolCallCallback tool_call_callback_;

    nlohmann::json buildRequestJson(const AIRequest& request) const;
    AIResponse parseResponseJson(const nlohmann::json& response) const;
    std::string makeHttpRequest(const std::string& url, const std::string& method,
                                const std::string& body, const std::string& content_type) const;
};

// AI客户端管理器
class AIClientManager {
  public:
    AIClientManager();
    ~AIClientManager() = default;

    // 获取单例实例
    static AIClientManager& getInstance();

    // 设置当前提供商
    void setCurrentProvider(const std::string& provider);

    // 获取当前客户端
    std::shared_ptr<AIClient> getCurrentClient();

    // 设置工具调用回调
    void setToolCallCallback(ToolCallCallback callback);

    // 发送请求
    std::future<AIResponse> sendRequest(const AIRequest& request);

    // 流式请求
    void sendStreamingRequest(const AIRequest& request, StreamingCallback callback);

    // 取消请求
    void cancelRequest();

    // 获取可用提供商
    std::vector<std::string> getAvailableProviders() const;

  private:
    std::map<std::string, std::shared_ptr<AIClient>> clients_;
    std::string current_provider_;
    ToolCallCallback tool_call_callback_;

    void initializeClients();
};

} // namespace ai_client
} // namespace features
} // namespace pnana

#endif // BUILD_AI_CLIENT_SUPPORT

#endif // PNANA_FEATURES_AI_CLIENT_AI_CLIENT_H
