#include "features/ai_client/ai_client.h"

#ifdef BUILD_AI_CLIENT_SUPPORT
#include <chrono>
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {
namespace ai_client {

// AIClient基类实现
AIClient::AIClient() = default;

void AIClient::setToolCallCallback(ToolCallCallback /*callback*/) {
    // 基类默认实现，子类可以覆盖
}

// HTTP响应回调
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// 流式响应回调
size_t StreamingWriteCallback(void* contents, size_t size, size_t nmemb, void* user_data) {
    size_t total_size = size * nmemb;
    auto callback_data = static_cast<std::pair<StreamingCallback, std::atomic<bool>*>*>(user_data);

    if (callback_data && callback_data->second && !callback_data->second->load()) {
        std::string chunk(static_cast<char*>(contents), total_size);
        callback_data->first(chunk, false);
    }

    return total_size;
}

// AI客户端管理器实现
AIClientManager::AIClientManager() {
    initializeClients();
    current_provider_ = "openai"; // 默认使用OpenAI
}

void AIClientManager::setToolCallCallback(ToolCallCallback callback) {
    tool_call_callback_ = callback;
    // 设置所有客户端的回调
    for (auto& pair : clients_) {
        if (pair.second) {
            pair.second->setToolCallCallback(callback);
        }
    }
}

AIClientManager& AIClientManager::getInstance() {
    static AIClientManager instance;
    return instance;
}

void AIClientManager::initializeClients() {
    auto& ai_config = ai_config::AIConfig::getInstance();

    // 初始化OpenAI客户端
    try {
        auto openai_config = ai_config.getProviderConfig("openai");
        clients_["openai"] = std::make_shared<OpenAIClient>(openai_config);
    } catch (...) {
        // 配置不存在，跳过
    }

    // 初始化Claude客户端
    try {
        auto claude_config = ai_config.getProviderConfig("claude");
        clients_["claude"] = std::make_shared<ClaudeClient>(claude_config);
    } catch (...) {
        // 配置不存在，跳过
    }
}

void AIClientManager::setCurrentProvider(const std::string& provider) {
    if (clients_.find(provider) != clients_.end()) {
        current_provider_ = provider;
    }
}

std::shared_ptr<AIClient> AIClientManager::getCurrentClient() {
    auto it = clients_.find(current_provider_);
    return it != clients_.end() ? it->second : nullptr;
}

std::future<AIResponse> AIClientManager::sendRequest(const AIRequest& request) {
    auto client = getCurrentClient();
    if (client) {
        return client->sendRequest(request);
    }

    // 返回错误响应
    std::promise<AIResponse> promise;
    AIResponse error_response;
    error_response.success = false;
    error_response.error_message = "No AI client available";
    promise.set_value(error_response);
    return promise.get_future();
}

void AIClientManager::sendStreamingRequest(const AIRequest& request, StreamingCallback callback) {
    auto client = getCurrentClient();
    if (client) {
        client->sendStreamingRequest(request, callback);
    } else {
        callback("", true); // 立即结束
    }
}

void AIClientManager::cancelRequest() {
    auto client = getCurrentClient();
    if (client) {
        client->cancelRequest();
    }
}

std::vector<std::string> AIClientManager::getAvailableProviders() const {
    std::vector<std::string> providers;
    for (const auto& pair : clients_) {
        providers.push_back(pair.first);
    }
    return providers;
}

// OpenAI客户端实现
OpenAIClient::OpenAIClient(const ai_config::AIProviderConfig& config)
    : config_(config), cancel_flag_(false) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void OpenAIClient::setToolCallCallback(ToolCallCallback callback) {
    tool_call_callback_ = callback;
}

std::future<AIResponse> OpenAIClient::sendRequest(const AIRequest& request) {
    return std::async(std::launch::async, [this, request]() {
        cancel_flag_ = false;

        try {
            std::string url = config_.base_url + "/chat/completions";
            nlohmann::json request_json = buildRequestJson(request);

            // 如果启用了工具调用，设置tool_choice为auto
            if (request.enable_tool_calling && !request.tools.empty()) {
                request_json["tool_choice"] = "auto";
            }

            std::string body = request_json.dump();
            std::string response = makeHttpRequest(url, "POST", body, "application/json");

            if (response.empty()) {
                AIResponse error_response;
                error_response.success = false;
                error_response.error_message = "Empty response from API";
                return error_response;
            }

            nlohmann::json response_json = nlohmann::json::parse(response);
            AIResponse ai_response = parseResponseJson(response_json);

            // 如果有工具调用且设置了回调，执行工具调用
            if (ai_response.success && !ai_response.tool_calls.empty() && tool_call_callback_) {
                // 这里可以实现工具调用循环，但为了简化，先只处理一次工具调用
                // 在实际应用中，可能需要多次往复直到AI给出最终答案
                for (const auto& tool_call : ai_response.tool_calls) {
                    ToolCallResult result = tool_call_callback_(tool_call);
                    // 将工具调用结果添加到响应中（简化处理）
                    if (!result.success) {
                        ai_response.error_message +=
                            "Tool call failed: " + result.error_message + "\n";
                    }
                }
            }

            return ai_response;

        } catch (const std::exception& e) {
            AIResponse error_response;
            error_response.success = false;
            error_response.error_message = std::string("Request failed: ") + e.what();
            return error_response;
        }
    });
}

void OpenAIClient::sendStreamingRequest(const AIRequest& request, StreamingCallback callback) {
    std::thread([this, request, callback]() {
        cancel_flag_ = false;

        try {
            CURL* curl = curl_easy_init();
            if (!curl) {
                callback("", true);
                return;
            }

            std::string url = config_.base_url + "/chat/completions";
            nlohmann::json request_json = buildRequestJson(request);
            request_json["stream"] = true;
            std::string body = request_json.dump();

            std::string response_buffer;
            auto callback_data = std::make_pair(callback, &cancel_flag_);

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers =
                curl_slist_append(headers, ("Authorization: Bearer " + config_.api_key).c_str());

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamingWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &callback_data);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5分钟超时

            CURLcode res = curl_easy_perform(curl);
            (void)res; // 避免未使用变量警告

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            // 发送结束信号
            callback("", true);

        } catch (const std::exception& e) {
            callback("", true);
        }
    }).detach();
}

void OpenAIClient::cancelRequest() {
    cancel_flag_ = true;
}

nlohmann::json OpenAIClient::buildRequestJson(const AIRequest& request) const {
    nlohmann::json json_request;

    json_request["model"] = config_.model;
    json_request["max_tokens"] = request.max_tokens;
    json_request["temperature"] = request.temperature;
    json_request["stream"] = request.stream;

    // 添加工具定义（如果启用了工具调用）
    if (request.enable_tool_calling && !request.tools.empty()) {
        nlohmann::json tools = nlohmann::json::array();
        for (const auto& tool : request.tools) {
            nlohmann::json tool_json;
            tool_json["type"] = "function";
            tool_json["function"]["name"] = tool.name;
            tool_json["function"]["description"] = tool.description;

            nlohmann::json parameters;
            parameters["type"] = "object";
            nlohmann::json properties = nlohmann::json::object();
            nlohmann::json required = nlohmann::json::array();

            for (const auto& param : tool.parameters) {
                nlohmann::json param_json;
                param_json["type"] = param.type;
                param_json["description"] = param.description;
                if (!param.schema.empty()) {
                    param_json["schema"] = param.schema;
                }
                properties[param.name] = param_json;
                if (param.required) {
                    required.push_back(param.name);
                }
            }

            parameters["properties"] = properties;
            if (!required.empty()) {
                parameters["required"] = required;
            }

            tool_json["function"]["parameters"] = parameters;
            tools.push_back(tool_json);
        }
        json_request["tools"] = tools;
    }

    nlohmann::json messages = nlohmann::json::array();

    // 系统消息
    if (!request.system_message.empty()) {
        messages.push_back({{"role", "system"}, {"content", request.system_message}});
    }

    // 上下文消息（如果有）
    for (const auto& context : request.context) {
        messages.push_back({{"role", "user"}, {"content", "Context: " + context}});
        messages.push_back({{"role", "assistant"}, {"content", "I understand the context."}});
    }

    // 用户消息
    messages.push_back({{"role", "user"}, {"content", request.prompt}});

    json_request["messages"] = messages;
    return json_request;
}

AIResponse OpenAIClient::parseResponseJson(const nlohmann::json& response) const {
    AIResponse ai_response;
    ai_response.model_used = config_.model;

    try {
        if (response.contains("error")) {
            ai_response.success = false;
            ai_response.error_message = response["error"]["message"];
            return ai_response;
        }

        if (response.contains("choices") && !response["choices"].empty()) {
            const auto& choice = response["choices"][0]["message"];
            ai_response.content = choice.value("content", "");
            ai_response.success = true;

            // 解析工具调用
            if (choice.contains("tool_calls") && choice["tool_calls"].is_array()) {
                for (const auto& tool_call_json : choice["tool_calls"]) {
                    ToolCall tool_call;
                    tool_call.id = tool_call_json.value("id", "");
                    tool_call.function_name = tool_call_json["function"]["name"];
                    tool_call.arguments = tool_call_json["function"]["arguments"];
                    ai_response.tool_calls.push_back(tool_call);
                }
            }

            if (response.contains("usage")) {
                ai_response.tokens_used = response["usage"]["total_tokens"];
            }
        } else {
            ai_response.success = false;
            ai_response.error_message = "Invalid response format";
        }
    } catch (const std::exception& e) {
        ai_response.success = false;
        ai_response.error_message = std::string("Failed to parse response: ") + e.what();
    }

    return ai_response;
}

std::string OpenAIClient::makeHttpRequest(const std::string& url, const std::string& method,
                                          const std::string& body,
                                          const std::string& content_type) const {
    (void)method; // 当前实现中未使用method参数
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "";
    }

    std::string response;
    struct curl_slist* headers = nullptr;

    headers = curl_slist_append(headers, ("Content-Type: " + content_type).c_str());
    headers = curl_slist_append(headers, ("Authorization: Bearer " + config_.api_key).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5分钟超时

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return "";
    }

    return response;
}

// Claude客户端实现（简化版）
ClaudeClient::ClaudeClient(const ai_config::AIProviderConfig& config)
    : config_(config), cancel_flag_(false) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void ClaudeClient::setToolCallCallback(ToolCallCallback callback) {
    tool_call_callback_ = callback;
}

std::future<AIResponse> ClaudeClient::sendRequest(const AIRequest& request) {
    (void)request; // 暂时未使用，Claude API实现（这里是简化版，实际需要根据Claude API文档实现）
    std::promise<AIResponse> promise;
    AIResponse response;
    response.success = false;
    response.error_message = "Claude client not fully implemented yet";
    promise.set_value(response);
    return promise.get_future();
}

void ClaudeClient::sendStreamingRequest(const AIRequest& request, StreamingCallback callback) {
    (void)request; // 暂时未使用
    // 暂时直接结束
    callback("", true);
}

void ClaudeClient::cancelRequest() {
    cancel_flag_ = true;
}

nlohmann::json ClaudeClient::buildRequestJson(const AIRequest& request) const {
    (void)request; // 暂时未使用
    return nlohmann::json();
}

AIResponse ClaudeClient::parseResponseJson(const nlohmann::json& response) const {
    (void)response; // 暂时未使用
    AIResponse ai_response;
    ai_response.success = false;
    ai_response.error_message = "Not implemented";
    return ai_response;
}

std::string ClaudeClient::makeHttpRequest(const std::string& url, const std::string& method,
                                          const std::string& body,
                                          const std::string& content_type) const {
    (void)url;
    (void)method;
    (void)body;
    (void)content_type; // 暂时未使用
    return "";
}

} // namespace ai_client
} // namespace features
} // namespace pnana

#endif // BUILD_AI_CLIENT_SUPPORT
