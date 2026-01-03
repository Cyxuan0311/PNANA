#include "features/lsp/lsp_client.h"
#include "utils/logger.h"
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace pnana {
namespace features {

LspClient::LspClient(const std::string& server_command) {
    connector_ = std::make_unique<LspStdioConnector>(server_command);
    rpc_client_ = std::make_unique<jsonrpccxx::JsonRpcClient>(*connector_, jsonrpccxx::version::v2);

    // 设置通知回调
    connector_->setNotificationCallback([this](const std::string& notification) {
        handleNotification(notification);
    });
}

LspClient::LspClient(const std::string& server_command,
                    const std::map<std::string, std::string>& env_vars) {
    connector_ = std::make_unique<LspStdioConnector>(server_command, env_vars);
    rpc_client_ = std::make_unique<jsonrpccxx::JsonRpcClient>(*connector_, jsonrpccxx::version::v2);
    
    // 设置通知回调
    connector_->setNotificationCallback([this](const std::string& notification) {
        handleNotification(notification);
    });
}

LspClient::~LspClient() {
    shutdown();
}

bool LspClient::initialize(const std::string& root_path) {
    LOG("LspClient::initialize() called with root_path: " + root_path);
    
    LOG("Starting LSP connector...");
    if (!connector_->start()) {
        LOG_WARNING("Failed to start LSP connector (server may not be installed)");
        return false;
    }
    LOG("LSP connector started successfully");
    
    // 等待一小段时间，确保服务器已准备好
    LOG("Waiting for server to be ready (50ms)...");
    usleep(50000);  // 50ms
    LOG("Wait completed");
    
    try {
        // 发送 initialize 请求
        LOG("Preparing initialize request...");
        jsonrpccxx::json params;
        params["processId"] = static_cast<int>(getpid());
        params["rootPath"] = root_path;
        if (root_path.empty()) {
            params["rootUri"] = jsonrpccxx::json(nullptr);
            LOG("rootUri set to null (root_path is empty)");
        } else {
            std::string root_uri = filepathToUri(root_path);
            params["rootUri"] = root_uri;
            LOG("rootUri: " + root_uri);
        }
        
        // 客户端能力
        LOG("Setting client capabilities...");
        jsonrpccxx::json capabilities;
        capabilities["textDocument"]["completion"]["completionItem"]["snippetSupport"] = true;
        capabilities["textDocument"]["hover"]["contentFormat"] = jsonrpccxx::json::array({ "markdown", "plaintext" });
        capabilities["textDocument"]["definition"]["linkSupport"] = true;
        capabilities["textDocument"]["references"] = jsonrpccxx::json::object();
        capabilities["textDocument"]["formatting"] = jsonrpccxx::json::object();
        capabilities["textDocument"]["rename"] = jsonrpccxx::json::object();
        
        params["capabilities"] = capabilities;
        LOG("Client capabilities set");
        
        int request_id = 1;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        
        // 调用 initialize 方法
        LOG("Sending initialize request to LSP server...");
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "initialize", named_params
        );
        LOG("Initialize request sent, received response");
        
        // 保存服务器能力
        if (result.contains("capabilities")) {
            server_capabilities_ = result["capabilities"];
            LOG("Server capabilities received and saved");
        } else {
            LOG_WARNING("Initialize response missing capabilities");
        }
        
        // 发送 initialized 通知（无参数）
        // 注意：这应该在收到 initialize 响应后立即发送
        LOG("Sending initialized notification...");
        rpc_client_->CallNotificationNamed("initialized", jsonrpccxx::named_parameter());
        LOG("Initialized notification sent");
        
        // 初始化完成后，启动通知监听线程（在后台线程中运行，不阻塞主线程）
        // 注意：通知监听线程使用非阻塞模式，不会阻塞主线程
        LOG("Starting notification listener...");
        connector_->startNotificationListener();
        LOG("Notification listener started");
        
        LOG("LspClient::initialize() completed successfully");
        return true;
    } catch (const jsonrpccxx::JsonRpcException& e) {
        LOG_ERROR("LspClient::initialize() JsonRpcException: " + std::string(e.what()) + " (code: " + std::to_string(e.Code()) + ")");
        // 不立即关闭连接，让用户知道问题
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("LspClient::initialize() exception: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("LspClient::initialize() unknown exception");
        return false;
    }
}

void LspClient::shutdown() {
    // 先停止通知监听线程，避免在发送 exit 通知后读取 EOF 错误
    connector_->stopNotificationListener();
    
    if (isConnected()) {
        try {
            int request_id = 1;
            rpc_client_->CallMethodNamed<jsonrpccxx::json>(
                request_id, "shutdown", jsonrpccxx::named_parameter()
            );
            
            // 发送 exit 通知（无参数）
            // 注意：发送 exit 后，服务器会立即关闭连接
            rpc_client_->CallNotificationNamed("exit", jsonrpccxx::named_parameter());
            
            // 给服务器一点时间关闭连接
            usleep(100000);  // 100ms
        } catch (const std::exception& e) {
            // 忽略关闭时的错误（服务器可能已经关闭）
        } catch (...) {
            // 忽略所有关闭时的错误
        }
    }
    connector_->stop();
}

void LspClient::didOpen(const std::string& uri, const std::string& language_id,
                       const std::string& content, int version) {
    if (!isConnected()) {
        return;
    }
    
    try {
        document_versions_[uri] = version;
        
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["textDocument"]["languageId"] = language_id;
        params["textDocument"]["version"] = version;
        params["textDocument"]["text"] = content;
        
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        rpc_client_->CallNotificationNamed("textDocument/didOpen", named_params);
    } catch (const std::exception& e) {
        // 静默处理错误
    }
}

void LspClient::didChange(const std::string& uri, const std::string& content, int version) {
    if (!isConnected()) {
        return;
    }
    
    try {
        document_versions_[uri] = version;
        
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["textDocument"]["version"] = version;
        
        jsonrpccxx::json change;
        change["text"] = content;
        params["contentChanges"] = jsonrpccxx::json::array({ change });
        
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        rpc_client_->CallNotificationNamed("textDocument/didChange", named_params);
    } catch (const std::exception& e) {
        // 静默处理错误
    }
}

void LspClient::didChangeIncremental(const std::string& uri, 
                                     const std::vector<TextDocumentContentChangeEvent>& changes,
                                     int version) {
    if (!isConnected() || changes.empty()) {
        return;
    }
    
    try {
        document_versions_[uri] = version;
        
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["textDocument"]["version"] = version;
        
        jsonrpccxx::json content_changes = jsonrpccxx::json::array();
        
        for (const auto& change : changes) {
            jsonrpccxx::json change_obj;
            
            // 如果 range 为空或 text 为空，表示全量更新
            if (change.text.empty() || (change.range.start.line == 0 && 
                                        change.range.start.character == 0 &&
                                        change.range.end.line == 0 &&
                                        change.range.end.character == 0)) {
                // 全量更新（这种情况应该使用 didChange）
                continue;
            }
            
            // 增量更新：包含 range 和 text
            change_obj["range"]["start"]["line"] = change.range.start.line;
            change_obj["range"]["start"]["character"] = change.range.start.character;
            change_obj["range"]["end"]["line"] = change.range.end.line;
            change_obj["range"]["end"]["character"] = change.range.end.character;
            change_obj["rangeLength"] = change.rangeLength;
            change_obj["text"] = change.text;
            
            content_changes.push_back(change_obj);
        }
        
        // 如果没有有效的增量更新，回退到全量更新
        if (content_changes.empty()) {
            return;  // 或者调用 didChange
        }
        
        params["contentChanges"] = content_changes;
        
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        rpc_client_->CallNotificationNamed("textDocument/didChange", named_params);
    } catch (const std::exception& e) {
        // 静默处理错误
    }
}

void LspClient::didClose(const std::string& uri) {
    if (!isConnected()) return;
    
    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        rpc_client_->CallNotificationNamed("textDocument/didClose", named_params);
        
        document_versions_.erase(uri);
    } catch (const std::exception& e) {
        std::cerr << "didClose failed: " << e.what() << std::endl;
    }
}

void LspClient::didSave(const std::string& uri) {
    if (!isConnected()) return;
    
    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        rpc_client_->CallNotificationNamed("textDocument/didSave", named_params);
    } catch (const std::exception& e) {
        std::cerr << "didSave failed: " << e.what() << std::endl;
    }
}

std::vector<CompletionItem> LspClient::completion(
    const std::string& uri, const LspPosition& position) {
    
    auto completion_start = std::chrono::steady_clock::now();
    LOG("[COMPLETION] [LspClient] completion() called for URI: " + uri + 
        " at line " + std::to_string(position.line) + ", char " + std::to_string(position.character));
    
    std::vector<CompletionItem> items;
    if (!isConnected()) {
        LOG("[COMPLETION] [LspClient] Not connected");
        return items;
    }
    
    try {
        auto prepare_start = std::chrono::steady_clock::now();
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);
        
        // 添加上下文信息（可选，但有助于提高补全质量）
        jsonrpccxx::json context;
        context["triggerKind"] = 1;  // Invoked (手动触发) 或 2 (TriggerCharacter)
        params["context"] = context;
        
        int request_id = 1;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        auto prepare_end = std::chrono::steady_clock::now();
        auto prepare_time = std::chrono::duration_cast<std::chrono::microseconds>(
            prepare_end - prepare_start);
        LOG("[COMPLETION] [LspClient] Prepared request params (took " + 
            std::to_string(prepare_time.count() / 1000.0) + " ms)");
        
        auto rpc_start = std::chrono::steady_clock::now();
        LOG("[COMPLETION] [LspClient] Calling RPC method textDocument/completion...");
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/completion", named_params
        );
        auto rpc_end = std::chrono::steady_clock::now();
        auto rpc_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            rpc_end - rpc_start);
        LOG("[COMPLETION] [LspClient] RPC call completed (took " + 
            std::to_string(rpc_time.count()) + " ms)");
        
        auto parse_start = std::chrono::steady_clock::now();
        if (result.contains("items") && result["items"].is_array()) {
            for (const auto& item : result["items"]) {
                items.push_back(jsonToCompletionItem(item));
            }
        } else if (result.is_array()) {
            for (const auto& item : result) {
                items.push_back(jsonToCompletionItem(item));
            }
        }
        auto parse_end = std::chrono::steady_clock::now();
        auto parse_time = std::chrono::duration_cast<std::chrono::microseconds>(
            parse_end - parse_start);
        LOG("[COMPLETION] [LspClient] Parsed " + std::to_string(items.size()) + 
            " items (took " + std::to_string(parse_time.count() / 1000.0) + " ms)");
        
        // 按相关性排序：优先显示更相关的项
        // 1. 函数/方法优先
        // 2. 变量/字段次之
        // 3. 其他类型
        // 4. 相同类型内按字母顺序
        auto sort_start = std::chrono::steady_clock::now();
        std::sort(items.begin(), items.end(), 
                 [](const CompletionItem& a, const CompletionItem& b) {
                     // 获取类型优先级
                     auto getPriority = [](const std::string& kind) -> int {
                         if (kind == "2" || kind == "3") return 1;  // Method, Function
                         if (kind == "5" || kind == "6") return 2;  // Field, Variable
                         if (kind == "7" || kind == "8" || kind == "22") return 3;  // Class, Interface, Struct
                         if (kind == "21") return 4;  // Constant
                         return 5;  // Other
                     };
                     
                     int prio_a = getPriority(a.kind);
                     int prio_b = getPriority(b.kind);
                     
                     if (prio_a != prio_b) {
                         return prio_a < prio_b;
                     }
                     
                     // 相同优先级，按字母顺序
                     return a.label < b.label;
                 });
        auto sort_end = std::chrono::steady_clock::now();
        auto sort_time = std::chrono::duration_cast<std::chrono::microseconds>(
            sort_end - sort_start);
        LOG("[COMPLETION] [LspClient] Sorted " + std::to_string(items.size()) + 
            " items (took " + std::to_string(sort_time.count() / 1000.0) + " ms)");
        
        auto completion_end = std::chrono::steady_clock::now();
        auto completion_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            completion_end - completion_start);
        LOG("[COMPLETION] [LspClient] completion() total time: " + 
            std::to_string(completion_time.count()) + " ms");
        
    } catch (const std::exception& e) {
        auto error_time = std::chrono::steady_clock::now();
        auto completion_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            error_time - completion_start);
        LOG_ERROR("[COMPLETION] [LspClient] completion() failed after " + 
                 std::to_string(completion_time.count()) + " ms: " + e.what());
        std::cerr << "completion failed: " << e.what() << std::endl;
    }
    
    LOG("[COMPLETION] [LspClient] Returning " + std::to_string(items.size()) + " items");
    return items;
}

std::vector<Location> LspClient::gotoDefinition(
    const std::string& uri, const LspPosition& position) {
    
    std::vector<Location> locations;
    if (!isConnected()) return locations;
    
    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);
        
        int request_id = 1;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/definition", named_params
        );
        
        if (result.is_array()) {
            for (const auto& loc : result) {
                locations.push_back(jsonToLocation(loc));
            }
        } else if (result.is_object()) {
            locations.push_back(jsonToLocation(result));
        }
    } catch (const std::exception& e) {
        std::cerr << "gotoDefinition failed: " << e.what() << std::endl;
    }
    
    return locations;
}

HoverInfo LspClient::hover(const std::string& uri, const LspPosition& position) {
    HoverInfo info;
    if (!isConnected()) return info;
    
    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);
        
        int request_id = 1;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/hover", named_params
        );
        
        info = jsonToHoverInfo(result);
    } catch (const std::exception& e) {
        std::cerr << "hover failed: " << e.what() << std::endl;
    }
    
    return info;
}

std::vector<Location> LspClient::findReferences(
    const std::string& uri, const LspPosition& position, bool include_declaration) {
    
    std::vector<Location> locations;
    if (!isConnected()) return locations;
    
    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);
        params["context"]["includeDeclaration"] = include_declaration;
        
        int request_id = 1;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/references", named_params
        );
        
        if (result.is_array()) {
            for (const auto& loc : result) {
                locations.push_back(jsonToLocation(loc));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "findReferences failed: " << e.what() << std::endl;
    }
    
    return locations;
}

std::string LspClient::formatDocument(const std::string& uri) {
    if (!isConnected()) return "";
    
    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        
        int request_id = 1;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/formatting", named_params
        );
        
        // 格式化结果通常是 TextEdit 数组
        if (result.is_array() && !result.empty()) {
            // 简化处理：返回第一个编辑的文本
            if (result[0].contains("newText") && result[0]["newText"].is_string()) {
                return result[0]["newText"].get<std::string>();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "formatDocument failed: " << e.what() << std::endl;
    }
    
    return "";
}

std::map<std::string, std::vector<LspRange>> LspClient::rename(
    const std::string& uri, const LspPosition& position, const std::string& new_name) {
    
    std::map<std::string, std::vector<LspRange>> changes;
    if (!isConnected()) return changes;
    
    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);
        params["newName"] = new_name;
        
        int request_id = 1;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/rename", named_params
        );
        
        if (result.contains("changes") && result["changes"].is_object()) {
            for (auto& [file_uri, edits] : result["changes"].items()) {
                std::vector<LspRange> ranges;
                if (edits.is_array()) {
                    for (const auto& edit : edits) {
                        if (edit.contains("range")) {
                            ranges.push_back(jsonToRange(edit["range"]));
                        }
                    }
                }
                changes[file_uri] = ranges;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "rename failed: " << e.what() << std::endl;
    }
    
    return changes;
}

void LspClient::setDiagnosticsCallback(DiagnosticsCallback callback) {
    diagnostics_callback_ = callback;
}

bool LspClient::isConnected() const {
    return connector_ && connector_->isRunning();
}

jsonrpccxx::json LspClient::positionToJson(const LspPosition& pos) {
    jsonrpccxx::json json;
    json["line"] = pos.line;
    json["character"] = pos.character;
    return json;
}

jsonrpccxx::json LspClient::rangeToJson(const LspRange& range) {
    jsonrpccxx::json json;
    json["start"] = positionToJson(range.start);
    json["end"] = positionToJson(range.end);
    return json;
}

LspPosition LspClient::jsonToPosition(const jsonrpccxx::json& json) {
    return LspPosition(
        json.value("line", 0),
        json.value("character", 0)
    );
}

LspRange LspClient::jsonToRange(const jsonrpccxx::json& json) {
    LspRange range;
    if (json.contains("start")) {
        range.start = jsonToPosition(json["start"]);
    }
    if (json.contains("end")) {
        range.end = jsonToPosition(json["end"]);
    }
    return range;
}

CompletionItem LspClient::jsonToCompletionItem(const jsonrpccxx::json& json) {
    CompletionItem item;
    item.label = json.value("label", std::string(""));
    
    // 处理kind（可能是数字或字符串）
    if (json.contains("kind")) {
        if (json["kind"].is_number()) {
        item.kind = std::to_string(json["kind"].get<int>());
        } else if (json["kind"].is_string()) {
            item.kind = json["kind"].get<std::string>();
        }
    }
    
    // 处理detail
    if (json.contains("detail")) {
        if (json["detail"].is_string()) {
        item.detail = json["detail"].get<std::string>();
        } else if (json["detail"].is_number()) {
            item.detail = std::to_string(json["detail"].get<int>());
        }
    }
    
    // 处理insertText（优先使用insertText，否则使用label）
    if (json.contains("insertText") && json["insertText"].is_string()) {
        item.insertText = json["insertText"].get<std::string>();
    } else if (json.contains("textEdit") && json["textEdit"].is_object()) {
        // 如果有textEdit，使用newText
        if (json["textEdit"].contains("newText") && 
            json["textEdit"]["newText"].is_string()) {
            item.insertText = json["textEdit"]["newText"].get<std::string>();
        }
    } else {
        item.insertText = item.label;
    }
    
    // 处理documentation（支持多种格式）
    if (json.contains("documentation")) {
        if (json["documentation"].is_string()) {
            item.documentation = json["documentation"].get<std::string>();
        } else if (json["documentation"].is_object()) {
            if (json["documentation"].contains("value") &&
                json["documentation"]["value"].is_string()) {
                item.documentation = json["documentation"]["value"].get<std::string>();
            } else if (json["documentation"].contains("kind") &&
                      json["documentation"]["kind"].is_string() &&
                      json["documentation"]["kind"].get<std::string>() == "markdown" &&
                   json["documentation"].contains("value") &&
                   json["documentation"]["value"].is_string()) {
            item.documentation = json["documentation"]["value"].get<std::string>();
            }
        }
    }
    
    return item;
}

Diagnostic LspClient::jsonToDiagnostic(const jsonrpccxx::json& json) {
    Diagnostic diag;
    
    if (json.contains("range")) {
        diag.range = jsonToRange(json["range"]);
    }
    
    diag.severity = json.value("severity", 1);
    diag.message = json.value("message", std::string(""));
    diag.source = json.value("source", std::string(""));
    
    if (json.contains("code")) {
        if (json["code"].is_string()) {
            diag.code = json["code"].get<std::string>();
        } else if (json["code"].is_number()) {
            diag.code = std::to_string(json["code"].get<int>());
        }
    }
    
    return diag;
}

Location LspClient::jsonToLocation(const jsonrpccxx::json& json) {
    Location loc;
    loc.uri = json.value("uri", std::string(""));
    
    if (json.contains("range")) {
        loc.range = jsonToRange(json["range"]);
    }
    
    return loc;
}

HoverInfo LspClient::jsonToHoverInfo(const jsonrpccxx::json& json) {
    HoverInfo info;
    
    if (json.contains("range")) {
        info.range = jsonToRange(json["range"]);
    }
    
    if (json.contains("contents")) {
        if (json["contents"].is_string()) {
            info.contents.push_back(json["contents"].get<std::string>());
        } else if (json["contents"].is_array()) {
            for (const auto& content : json["contents"]) {
                if (content.is_string()) {
                    info.contents.push_back(content.get<std::string>());
                } else if (content.is_object() && content.contains("value") &&
                           content["value"].is_string()) {
                    info.contents.push_back(content["value"].get<std::string>());
                }
            }
        } else if (json["contents"].is_object() && json["contents"].contains("value") &&
                   json["contents"]["value"].is_string()) {
            info.contents.push_back(json["contents"]["value"].get<std::string>());
        }
    }
    
    return info;
}

void LspClient::handleNotification(const std::string& notification) {
    try {
        jsonrpccxx::json json = parseJson(notification);
        
        if (json.contains("method") && json["method"].is_string()) {
            std::string method = json["method"].get<std::string>();
            
            if (method == "textDocument/publishDiagnostics") {
                if (json.contains("params")) {
                    jsonrpccxx::json params = json["params"];
                    std::string uri = params.value("uri", std::string(""));
                    
                    std::vector<Diagnostic> diagnostics;
                    if (params.contains("diagnostics") && params["diagnostics"].is_array()) {
                        for (const auto& diag : params["diagnostics"]) {
                            diagnostics.push_back(jsonToDiagnostic(diag));
                        }
                    }
                    
                    if (diagnostics_callback_) {
                        diagnostics_callback_(uri, diagnostics);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        // 静默处理通知错误，避免影响界面
    }
}

jsonrpccxx::json LspClient::parseJson(const std::string& json_str) {
    try {
        return jsonrpccxx::json::parse(json_str);
    } catch (const jsonrpccxx::json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
}

std::string LspClient::filepathToUri(const std::string& filepath) {
    // 简单的 URI 转换，实际应该处理特殊字符
    std::string uri = "file://";
    
    // 处理 Windows 路径
    std::string path = filepath;
    std::replace(path.begin(), path.end(), '\\', '/');
    
    // URL 编码（简化版）
    for (char c : path) {
        if (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == ':') {
            uri += c;
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
            uri += hex;
        }
    }
    
    return uri;
}

std::string LspClient::uriToFilepath(const std::string& uri) {
    if (uri.find("file://") != 0) {
        return uri;
    }
    
    std::string path = uri.substr(7);  // 移除 "file://"
    
    // URL 解码（简化版）
    std::string decoded;
    for (size_t i = 0; i < path.length(); ++i) {
        if (path[i] == '%' && i + 2 < path.length()) {
            int value;
            if (sscanf(path.substr(i + 1, 2).c_str(), "%x", &value) == 1) {
                decoded += static_cast<char>(value);
                i += 2;
            } else {
                decoded += path[i];
            }
        } else {
            decoded += path[i];
        }
    }
    
    return decoded;
}

} // namespace features
} // namespace pnana
