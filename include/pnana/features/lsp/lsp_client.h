#ifndef PNANA_FEATURES_LSP_CLIENT_H
#define PNANA_FEATURES_LSP_CLIENT_H

#include "features/lsp/document_change_tracker.h"
#include "features/lsp/lsp_stdio_connector.h"
#include "features/lsp/lsp_types.h"
#include "jsonrpccxx/client.hpp"
#include "jsonrpccxx/common.hpp"
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace features {

// 代码补全项
struct CompletionItem {
    std::string label;
    std::string kind;
    std::string detail;
    std::string insertText;
    std::string documentation;
};

// 诊断信息
struct Diagnostic {
    LspRange range;
    int severity; // 1=Error, 2=Warning, 3=Info, 4=Hint
    std::string message;
    std::string source;
    std::string code;
};

// 跳转定义结果
struct Location {
    std::string uri;
    LspRange range;
};

// 悬停信息
struct HoverInfo {
    std::vector<std::string> contents;
    LspRange range;
};

/**
 * LSP 客户端封装类
 * 提供高级 API 封装 LSP 协议细节
 */
class LspClient {
  public:
    explicit LspClient(const std::string& server_command);
    LspClient(const std::string& server_command,
              const std::map<std::string, std::string>& env_vars);
    ~LspClient();

    // 初始化和清理
    bool initialize(const std::string& root_path = "");
    void shutdown();

    // 文档生命周期
    void didOpen(const std::string& uri, const std::string& language_id, const std::string& content,
                 int version = 1);
    void didChange(const std::string& uri, const std::string& content, int version = 1);
    void didChangeIncremental(const std::string& uri,
                              const std::vector<TextDocumentContentChangeEvent>& changes,
                              int version = 1);
    void didClose(const std::string& uri);
    void didSave(const std::string& uri);

    // 代码补全
    std::vector<CompletionItem> completion(const std::string& uri, const LspPosition& position);

    // 跳转定义
    std::vector<Location> gotoDefinition(const std::string& uri, const LspPosition& position);

    // 悬停信息
    HoverInfo hover(const std::string& uri, const LspPosition& position);

    // 符号查找
    std::vector<Location> findReferences(const std::string& uri, const LspPosition& position,
                                         bool include_declaration = false);

    // 代码格式化
    std::string formatDocument(const std::string& uri, const std::string& original_content);

    // 重命名符号
    std::map<std::string, std::vector<LspRange>> rename(const std::string& uri,
                                                        const LspPosition& position,
                                                        const std::string& new_name);

    // 设置诊断回调
    using DiagnosticsCallback =
        std::function<void(const std::string& uri, const std::vector<Diagnostic>&)>;
    void setDiagnosticsCallback(DiagnosticsCallback callback);

    // 检查连接状态
    bool isConnected() const;

    // 获取服务器能力
    jsonrpccxx::json getServerCapabilities() const {
        return server_capabilities_;
    }

  private:
    // 辅助函数
    std::string applyTextEdits(const std::string& original_content, const jsonrpccxx::json& edits);
    size_t positionToOffset(const std::string& content, const LspPosition& position);
    std::unique_ptr<LspStdioConnector> connector_;
    std::unique_ptr<jsonrpccxx::JsonRpcClient> rpc_client_;

    jsonrpccxx::json server_capabilities_;

    // 文档版本管理
    std::map<std::string, int> document_versions_;

    // 诊断回调
    DiagnosticsCallback diagnostics_callback_;

    // 将 C++ 对象转换为 JSON
    jsonrpccxx::json positionToJson(const LspPosition& pos);
    jsonrpccxx::json rangeToJson(const LspRange& range);

    // 从 JSON 解析对象
    LspPosition jsonToPosition(const jsonrpccxx::json& json);
    LspRange jsonToRange(const jsonrpccxx::json& json);
    CompletionItem jsonToCompletionItem(const jsonrpccxx::json& json);
    Diagnostic jsonToDiagnostic(const jsonrpccxx::json& json);
    Location jsonToLocation(const jsonrpccxx::json& json);
    HoverInfo jsonToHoverInfo(const jsonrpccxx::json& json);

    // 处理服务器通知
    void handleNotification(const std::string& notification);

    // 解析 JSON 字符串
    jsonrpccxx::json parseJson(const std::string& json_str);

    // URI 转换
    std::string filepathToUri(const std::string& filepath);
    std::string uriToFilepath(const std::string& uri);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_CLIENT_H
