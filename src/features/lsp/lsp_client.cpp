#include "features/lsp/lsp_client.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <future>
#include <iostream>
#include <sstream>
#include <unistd.h>

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

bool LspClient::initialize(const std::string& root_path, const nlohmann::json* init_options) {
    if (!connector_->start()) {
        LOG_WARNING("Failed to start LSP connector (server may not be installed)");
        return false;
    }

    // 等待一小段时间，确保服务器已准备好
    usleep(50000); // 50ms

    try {
        // 发送 initialize 请求
        jsonrpccxx::json params;
        params["processId"] = static_cast<int>(getpid());

        if (root_path.empty()) {
            params["rootUri"] = jsonrpccxx::json(nullptr);
        } else {
            std::string root_uri = filepathToUri(root_path);
            params["rootUri"] = root_uri;
        }

        if (init_options != nullptr) {
            params["initializationOptions"] = *init_options;
        }

        // 简化客户端能力 - 只包含最基本的
        jsonrpccxx::json capabilities;
        capabilities["textDocument"]["formatting"] = jsonrpccxx::json::object();
        capabilities["textDocument"]["foldingRange"] = jsonrpccxx::json::object();

        params["capabilities"] = capabilities;

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }

        // 调用 initialize 方法
        jsonrpccxx::json result =
            rpc_client_->CallMethodNamed<jsonrpccxx::json>(request_id, "initialize", named_params);

        // 保存服务器能力
        if (result.contains("capabilities")) {
            server_capabilities_ = result["capabilities"];
        } else {
            LOG_WARNING("Initialize response missing capabilities");
        }

        // 发送 initialized 通知
        rpc_client_->CallNotificationNamed("initialized", jsonrpccxx::named_parameter());

        // 启动通知监听线程
        connector_->startNotificationListener();

        return true;
    } catch (const jsonrpccxx::JsonRpcException& e) {
        LOG_ERROR("LspClient::initialize() JsonRpcException: " + std::string(e.what()) +
                  " (code: " + std::to_string(e.Code()) + ")");
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
            int request_id = next_request_id_++;
            rpc_client_->CallMethodNamed<jsonrpccxx::json>(request_id, "shutdown",
                                                           jsonrpccxx::named_parameter());

            // 发送 exit 通知（无参数）
            // 注意：发送 exit 后，服务器会立即关闭连接
            rpc_client_->CallNotificationNamed("exit", jsonrpccxx::named_parameter());

            // 给服务器一点时间关闭连接
            usleep(100000); // 100ms
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
        params["contentChanges"] = jsonrpccxx::json::array({change});

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
            if (change.text.empty() ||
                (change.range.start.line == 0 && change.range.start.character == 0 &&
                 change.range.end.line == 0 && change.range.end.character == 0)) {
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
            return; // 或者调用 didChange
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
    if (!isConnected())
        return;

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
    if (!isConnected())
        return;

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

std::vector<CompletionItem> LspClient::completion(const std::string& uri,
                                                  const LspPosition& position,
                                                  const std::string& trigger_character) {
    std::vector<CompletionItem> items;
    if (!isConnected()) {
        return items;
    }

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);

        // CompletionContext：参考 VS Code，区分手动触发 vs 字符触发（. : -> 等）
        jsonrpccxx::json context;
        context["triggerKind"] = trigger_character.empty() ? 1 : 2; // 1=Invoked, 2=TriggerCharacter
        if (!trigger_character.empty()) {
            context["triggerCharacter"] = trigger_character;
        }
        params["context"] = context;

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        // 直接调用RPC方法，超时控制交给上层异步管理器
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/completion", named_params);

        if (!result.is_null() && result.contains("items") && result["items"].is_array()) {
            for (const auto& item : result["items"]) {
                items.push_back(jsonToCompletionItem(item));
            }
        } else if (result.is_array()) {
            for (const auto& item : result) {
                items.push_back(jsonToCompletionItem(item));
            }
        }

        // [clangd debug] 记录解析后的补全项（前 25 个 label，便于排查 vector/string 等）
        // 排序：优先使用 sortText（LSP 服务器推荐），否则按类型+字母
        std::sort(items.begin(), items.end(), [](const CompletionItem& a, const CompletionItem& b) {
            // 1. 有 sortText 时优先按 sortText（服务器通常按 当前文档 > 项目 > 标准库 排序）
            if (!a.sortText.empty() && !b.sortText.empty()) {
                if (a.sortText != b.sortText) {
                    return a.sortText < b.sortText;
                }
            } else if (!a.sortText.empty()) {
                return true; // a 有 sortText 排前面
            } else if (!b.sortText.empty()) {
                return false;
            }

            // 2. 类型优先级：Method/Function > Field/Variable > Class > Constant > Other
            auto getPriority = [](const std::string& kind) -> int {
                if (kind == "2" || kind == "3")
                    return 1; // Method, Function
                if (kind == "5" || kind == "6")
                    return 2; // Field, Variable
                if (kind == "7" || kind == "8" || kind == "22")
                    return 3; // Class, Interface, Struct
                if (kind == "21")
                    return 4; // Constant
                return 5;     // Other
            };
            int prio_a = getPriority(a.kind);
            int prio_b = getPriority(b.kind);
            if (prio_a != prio_b) {
                return prio_a < prio_b;
            }
            return a.label < b.label;
        });

    } catch (const std::exception& e) {
        (void)e;
    }

    return items;
}

bool LspClient::supportsCompletionResolve() const {
    try {
        if (!server_capabilities_.contains("completionProvider")) {
            return false;
        }
        const auto& cp = server_capabilities_["completionProvider"];
        return cp.is_object() && cp.contains("resolveProvider") &&
               cp["resolveProvider"].get<bool>();
    } catch (const std::exception&) {
        return false;
    }
}

CompletionItem LspClient::resolveCompletionItem(const CompletionItem& item) {
    CompletionItem result = item;
    if (!isConnected() || !supportsCompletionResolve()) {
        return result;
    }
    if (item.resolved) {
        return result;
    }

    try {
        jsonrpccxx::json params;
        params["label"] = item.label;
        if (!item.kind.empty()) {
            try {
                params["kind"] = std::stoi(item.kind);
            } catch (...) {
                params["kind"] = item.kind;
            }
        }
        params["detail"] = item.detail;
        params["documentation"] = item.documentation;
        params["insertText"] = item.insertText;
        params["insertTextFormat"] = item.insertTextFormat;
        if (!item.sortText.empty()) {
            params["sortText"] = item.sortText;
        }
        if (!item.filterText.empty()) {
            params["filterText"] = item.filterText;
        }
        if (!item.data.empty()) {
            try {
                params["data"] = jsonrpccxx::json::parse(item.data);
            } catch (...) {
                // 解析失败则跳过 data
            }
        }

        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }

        int request_id = next_request_id_++;
        jsonrpccxx::json resolved = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "completionItem/resolve", named_params);

        CompletionItem resolved_item = jsonToCompletionItem(resolved);
        result.detail = resolved_item.detail;
        result.documentation = resolved_item.documentation;
        result.resolved = true;
    } catch (const std::exception& e) {
        LOG_ERROR("LSP completionItem/resolve failed: " + std::string(e.what()));
    }

    return result;
}

std::vector<Location> LspClient::gotoDefinition(const std::string& uri,
                                                const LspPosition& position) {
    std::vector<Location> locations;
    if (!isConnected())
        return locations;

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/definition", named_params);

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
    if (!isConnected())
        return info;

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/hover", named_params);

        info = jsonToHoverInfo(result);
    } catch (const std::exception& e) {
        std::cerr << "hover failed: " << e.what() << std::endl;
    }

    return info;
}

std::vector<Location> LspClient::findReferences(const std::string& uri, const LspPosition& position,
                                                bool include_declaration) {
    std::vector<Location> locations;
    if (!isConnected())
        return locations;

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);
        params["context"]["includeDeclaration"] = include_declaration;

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/references", named_params);

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

std::string LspClient::applyTextEdits(const std::string& original_content,
                                      const jsonrpccxx::json& edits) {
    if (!edits.is_array() || edits.empty()) {
        return original_content;
    }

    std::string result = original_content;
    std::vector<std::pair<size_t, std::pair<size_t, std::string>>> operations;

    // 收集所有编辑操作，按起始位置排序（从后往前应用）
    for (const auto& edit : edits) {
        if (edit.contains("range") && edit.contains("newText")) {
            LspRange range = jsonToRange(edit["range"]);
            std::string new_text = edit["newText"].get<std::string>();

            // 将行/列位置转换为字符串偏移量
            size_t start_offset = positionToOffset(original_content, range.start);
            size_t end_offset = positionToOffset(original_content, range.end);

            operations.emplace_back(start_offset, std::make_pair(end_offset, new_text));
        }
    }

    // 按起始位置从大到小排序，确保从后往前应用编辑
    std::sort(operations.rbegin(), operations.rend());

    // 应用编辑
    for (const auto& op : operations) {
        size_t start_offset = op.first;
        size_t end_offset = op.second.first;
        const std::string& new_text = op.second.second;

        if (start_offset <= result.length() && end_offset <= result.length() &&
            start_offset <= end_offset) {
            result.replace(start_offset, end_offset - start_offset, new_text);
        }
    }

    return result;
}

size_t LspClient::positionToOffset(const std::string& content, const LspPosition& position) {
    size_t offset = 0;
    size_t current_line = 0;
    size_t current_char = 0;

    for (size_t i = 0; i < content.length(); ++i) {
        if (static_cast<int>(current_line) == position.line &&
            static_cast<int>(current_char) == position.character) {
            return offset;
        }

        if (content[i] == '\n') {
            current_line++;
            current_char = 0;
        } else {
            current_char++;
        }
        offset++;
    }

    // 如果超出范围，返回字符串末尾
    return content.length();
}

std::string LspClient::formatDocument(const std::string& uri, const std::string& original_content) {
    if (!isConnected())
        return "";

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/formatting", named_params);

        // 格式化结果是 TextEdit 数组，需要应用到原始内容上
        if (result.is_array()) {
            return applyTextEdits(original_content, result);
        }
    } catch (const std::exception& e) {
        std::cerr << "formatDocument failed: " << e.what() << std::endl;
    }

    return "";
}

std::vector<FoldingRange> LspClient::foldingRange(const std::string& uri) {
    std::vector<FoldingRange> ranges;
    if (!isConnected()) {
        return ranges;
    }

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }

        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/foldingRange", named_params);

        if (result.is_array()) {
            for (const auto& item : result) {
                FoldingRange range = jsonToFoldingRange(item);
                if (range.isValid()) {
                    ranges.push_back(range);
                }
            }
        }
    } catch (const std::exception& e) {
        (void)e;
        (void)uri;
        // Silently ignore known non-critical errors (non-added document, -32602, etc.)
    }

    return ranges;
}

std::vector<DocumentSymbol> LspClient::documentSymbol(const std::string& uri) {
    std::vector<DocumentSymbol> symbols;
    if (!isConnected()) {
        return symbols;
    }

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/documentSymbol", named_params);

        if (result.is_array()) {
            for (const auto& item : result) {
                DocumentSymbol symbol = jsonToDocumentSymbol(item, 0);
                if (!symbol.name.empty()) {
                    symbols.push_back(symbol);
                }
            }
        }
    } catch (const std::exception& e) {
        (void)e;
    }

    return symbols;
}

std::map<std::string, std::vector<LspRange>> LspClient::rename(const std::string& uri,
                                                               const LspPosition& position,
                                                               const std::string& new_name) {
    std::map<std::string, std::vector<LspRange>> changes;
    if (!isConnected())
        return changes;

    try {
        jsonrpccxx::json params;
        params["textDocument"]["uri"] = uri;
        params["position"] = positionToJson(position);
        params["newName"] = new_name;

        int request_id = next_request_id_++;
        jsonrpccxx::named_parameter named_params;
        for (auto& [key, value] : params.items()) {
            named_params[key] = value;
        }
        jsonrpccxx::json result = rpc_client_->CallMethodNamed<jsonrpccxx::json>(
            request_id, "textDocument/rename", named_params);

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

int LspClient::getServerPid() const {
    if (!connector_) {
        return -1;
    }
    try {
        return connector_->getServerPid();
    } catch (...) {
        return -1;
    }
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
    return LspPosition(json.value("line", 0), json.value("character", 0));
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

    // insertTextFormat: 1=PlainText, 2=Snippet
    int insert_text_format = 1;
    if (json.contains("insertTextFormat")) {
        try {
            if (json["insertTextFormat"].is_number()) {
                insert_text_format = json["insertTextFormat"].get<int>();
            } else if (json["insertTextFormat"].is_string()) {
                insert_text_format = std::stoi(json["insertTextFormat"].get<std::string>());
            }
        } catch (...) {
            insert_text_format = 1;
        }
    }
    item.insertTextFormat = insert_text_format;

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

    // 处理insertText / textEdit.newText（优先 textEdit.newText）
    std::string new_text;
    if (json.contains("textEdit") && json["textEdit"].is_object()) {
        if (json["textEdit"].contains("newText") && json["textEdit"]["newText"].is_string()) {
            new_text = json["textEdit"]["newText"].get<std::string>();
        }
    }
    if (json.contains("insertText") && json["insertText"].is_string()) {
        // insertText 覆盖 textEdit 的 newText 仅当 new_text 为空
        if (new_text.empty()) {
            new_text = json["insertText"].get<std::string>();
        }
    }
    if (new_text.empty()) {
        new_text = item.label;
    }
    item.insertText = new_text;

    // 如果 insertTextFormat=Snippet，标记为 snippet 以走本地 snippet 展开逻辑
    if (item.insertTextFormat == 2) {
        item.isSnippet = true;
        item.snippet_body = new_text;
    }

    // sortText/filterText：用于排序和过滤，提升补全质量（VS Code 四类提示依赖此）
    if (json.contains("sortText") && json["sortText"].is_string()) {
        item.sortText = json["sortText"].get<std::string>();
    }
    if (json.contains("filterText") && json["filterText"].is_string()) {
        item.filterText = json["filterText"].get<std::string>();
    } else {
        item.filterText = item.label;
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

    // 保存 data 字段，resolve 时需原样回传
    if (json.contains("data")) {
        try {
            item.data = json["data"].dump();
        } catch (...) {
            // 忽略序列化失败
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

FoldingRange LspClient::jsonToFoldingRange(const jsonrpccxx::json& json) {
    FoldingRange range;

    if (json.contains("startLine")) {
        range.startLine = json["startLine"];
    }

    if (json.contains("startCharacter")) {
        range.startCharacter = json["startCharacter"];
    }

    if (json.contains("endLine")) {
        range.endLine = json["endLine"];
    }

    if (json.contains("endCharacter")) {
        range.endCharacter = json["endCharacter"];
    }

    if (json.contains("kind")) {
        std::string kind_str = json["kind"];
        if (kind_str == "comment") {
            range.kind = FoldingRangeKind::Comment;
        } else if (kind_str == "imports") {
            range.kind = FoldingRangeKind::Imports;
        } else {
            range.kind = FoldingRangeKind::Region;
        }
    }

    return range;
}

DocumentSymbol LspClient::jsonToDocumentSymbol(const jsonrpccxx::json& json, int depth) {
    DocumentSymbol symbol;
    symbol.depth = depth;

    // 解析名称
    if (json.contains("name") && json["name"].is_string()) {
        symbol.name = json["name"].get<std::string>();
    }

    // 解析类型（kind）
    if (json.contains("kind")) {
        if (json["kind"].is_number()) {
            // LSP规范中kind是数字，需要转换为字符串
            int kind_num = json["kind"].get<int>();
            // 映射LSP符号类型到字符串
            switch (kind_num) {
                case 1:
                    symbol.kind = "File";
                    break;
                case 2:
                    symbol.kind = "Module";
                    break;
                case 3:
                    symbol.kind = "Namespace";
                    break;
                case 4:
                    symbol.kind = "Package";
                    break;
                case 5:
                    symbol.kind = "Class";
                    break;
                case 6:
                    symbol.kind = "Method";
                    break;
                case 7:
                    symbol.kind = "Property";
                    break;
                case 8:
                    symbol.kind = "Field";
                    break;
                case 9:
                    symbol.kind = "Constructor";
                    break;
                case 10:
                    symbol.kind = "Enum";
                    break;
                case 11:
                    symbol.kind = "Interface";
                    break;
                case 12:
                    symbol.kind = "Function";
                    break;
                case 13:
                    symbol.kind = "Variable";
                    break;
                case 14:
                    symbol.kind = "Constant";
                    break;
                case 15:
                    symbol.kind = "String";
                    break;
                case 16:
                    symbol.kind = "Number";
                    break;
                case 17:
                    symbol.kind = "Boolean";
                    break;
                case 18:
                    symbol.kind = "Array";
                    break;
                case 19:
                    symbol.kind = "Object";
                    break;
                case 20:
                    symbol.kind = "Key";
                    break;
                case 21:
                    symbol.kind = "Null";
                    break;
                case 22:
                    symbol.kind = "EnumMember";
                    break;
                case 23:
                    symbol.kind = "Struct";
                    break;
                case 24:
                    symbol.kind = "Event";
                    break;
                case 25:
                    symbol.kind = "Operator";
                    break;
                case 26:
                    symbol.kind = "TypeParameter";
                    break;
                default:
                    symbol.kind = "Unknown";
                    break;
            }
        } else if (json["kind"].is_string()) {
            symbol.kind = json["kind"].get<std::string>();
        }
    }

    // 解析范围
    if (json.contains("range")) {
        symbol.range = jsonToRange(json["range"]);
    } else if (json.contains("location") && json["location"].is_object() &&
               json["location"].contains("range")) {
        symbol.range = jsonToRange(json["location"]["range"]);
    }

    // 解析详细信息
    if (json.contains("detail") && json["detail"].is_string()) {
        symbol.detail = json["detail"].get<std::string>();
    }

    // 解析子符号（children）
    if (json.contains("children") && json["children"].is_array()) {
        for (const auto& child : json["children"]) {
            DocumentSymbol child_symbol = jsonToDocumentSymbol(child, depth + 1);
            if (!child_symbol.name.empty()) {
                symbol.children.push_back(child_symbol);
            }
        }
    }

    return symbol;
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
    } catch (const std::exception&) {
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

    std::string path = uri.substr(7); // 移除 "file://"

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
