// LSP 集成相关实现
#include "core/editor.h"
#include "features/lsp/lsp_request_manager.h"
#include "features/lsp/lsp_server_manager.h"
#include "features/lsp/lsp_worker_pool.h"
#include "ui/icons.h"
#include "utils/clipboard.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <ftxui/component/event.hpp>
#include <iostream>
#include <mutex>
#include <thread>

using namespace ftxui;

namespace fs = std::filesystem;

namespace pnana {
namespace core {

#ifdef BUILD_LSP_SUPPORT

void Editor::initializeLsp() {
    // 创建 LSP 服务器管理器
    lsp_manager_ = std::make_unique<features::LspServerManager>();

    // 初始化诊断弹窗状态
    show_diagnostics_popup_ = false;

    // 设置诊断回调（应用到所有 LSP 客户端）
    lsp_manager_->setDiagnosticsCallback(
        [this](const std::string& uri, const std::vector<features::Diagnostic>& diagnostics) {
            LOG("Received diagnostics callback: uri=" + uri +
                ", count=" + std::to_string(diagnostics.size()));

            // 更新当前文件的诊断信息（内存更新）
            bool is_current_file = false;
            {
                std::lock_guard<std::mutex> lock(diagnostics_mutex_);
                auto current_doc = getCurrentDocument();
                if (current_doc && uri == filepathToUri(current_doc->getFilePath())) {
                    current_file_diagnostics_ = diagnostics;
                    is_current_file = true;
                    LOG("Updated current file diagnostics: " +
                        std::to_string(current_file_diagnostics_.size()));
                }
            }

            // 对于当前文件，立即更新状态栏以提高实时性
            if (is_current_file) {
                updateDiagnosticsStatus(diagnostics);
            } else {
                // 对于其他文件，使用异步更新
                if (lsp_request_manager_) {
                    auto diags_copy = diagnostics;
                    std::string dedup_key = "diag:" + uri;
                    lsp_request_manager_->postOrReplace(dedup_key,
                                                        features::LspRequestManager::Priority::LOW,
                                                        [this, diags_copy]() mutable {
                                                            updateDiagnosticsStatus(diags_copy);
                                                        });
                } else {
                    // fallback: synchronous update
                    updateDiagnosticsStatus(diagnostics);
                }
            }
        });

    // LSP 管理器使用延迟初始化，只在需要时启动对应的服务器

    // 初始化 LSP 格式化器（稍后根据需要动态获取客户端）
    lsp_formatter_ = std::make_unique<features::LspFormatter>(lsp_manager_.get());

    // 初始化异步请求管理器和线程池
    lsp_request_manager_ = std::make_unique<features::LspRequestManager>();
    lsp_worker_pool_ =
        std::make_unique<features::LspWorkerPool>(std::thread::hardware_concurrency());

    lsp_enabled_ = true;
    setStatusMessage("LSP manager initialized");
}

void Editor::cleanupLocalCacheFiles() {
    // 检查工作目录是否存在 .cache 文件夹
    fs::path current_dir = fs::current_path();
    fs::path local_cache = current_dir / ".cache";

    if (!fs::exists(local_cache)) {
        return; // 没有本地缓存文件，无需处理
    }

    // 获取配置的缓存目录
    std::string config_cache_dir = std::string(getenv("HOME")) + "/.config/pnana/.cache";

    try {
        // 确保配置的缓存目录存在
        fs::create_directories(config_cache_dir);

        // 移动 .cache 文件夹的内容到配置目录
        // 如果目标目录已存在相应文件夹，则合并内容
        for (const auto& entry : fs::directory_iterator(local_cache)) {
            fs::path target_path = fs::path(config_cache_dir) / entry.path().filename();

            if (fs::exists(target_path)) {
                // 如果目标已存在，递归合并内容
                fs::copy(entry.path(), target_path,
                         fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            } else {
                // 直接移动
                fs::rename(entry.path(), target_path);
            }
        }

        // 强制删除本地 .cache 文件夹及其内容
        fs::remove_all(local_cache);

        LOG("Migrated LSP cache files to: " + config_cache_dir);
    } catch (const std::exception& e) {
        LOG_WARNING("Failed to migrate cache files: " + std::string(e.what()));
    }
}

void Editor::shutdownLsp() {
    if (lsp_manager_ && lsp_enabled_) {
        lsp_manager_->shutdownAll();
        lsp_enabled_ = false;
    }
    completion_popup_.hide();
}

std::string Editor::detectLanguageId(const std::string& filepath) {
    // 根据文件扩展名返回语言 ID
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".hpp" || ext == ".hxx" ||
        ext == ".h" || ext == ".c") {
        return "cpp";
    } else if (ext == ".py") {
        return "python";
    } else if (ext == ".go") {
        return "go";
    } else if (ext == ".rs") {
        return "rust";
    } else if (ext == ".java") {
        return "java";
    } else if (ext == ".js" || ext == ".jsx") {
        return "javascript";
    } else if (ext == ".ts" || ext == ".tsx") {
        return "typescript";
    } else if (ext == ".html" || ext == ".htm") {
        return "html";
    } else if (ext == ".css") {
        return "css";
    } else if (ext == ".json") {
        return "json";
    } else if (ext == ".xml") {
        return "xml";
    } else if (ext == ".md") {
        return "markdown";
    } else if (ext == ".sh" || ext == ".bash") {
        return "shellscript";
    } else if (ext == ".yaml" || ext == ".yml") {
        return "yaml";
    } else if (ext == ".toml") {
        return "toml";
    }

    return "plaintext";
}

std::string Editor::filepathToUri(const std::string& filepath) {
#ifdef LSP_DEBUG_LOGGING
    LOG("filepathToUri() called with: " + filepath);
    LOG("Filepath length: " + std::to_string(filepath.length()));
#endif

    // 检查 URI 缓存
    {
        std::lock_guard<std::mutex> lock(uri_cache_mutex_);
        auto it = uri_cache_.find(filepath);
        if (it != uri_cache_.end()) {
#ifdef LSP_DEBUG_LOGGING
            LOG("URI cache hit for: " + filepath);
#endif
            return it->second;
        }
    }

    // 转换为 file:// URI
    // 注意：对于包含非 ASCII 字符（如中文）的路径，需要正确进行 UTF-8 URL 编码
    std::string uri = "file://";

    try {
        // 使用 try-catch 保护，避免路径处理失败导致卡住
        std::string path;
#ifdef LSP_DEBUG_LOGGING
        LOG("Calling fs::absolute()...");
#endif
        try {
            path = fs::absolute(filepath).string();
#ifdef LSP_DEBUG_LOGGING
            LOG("fs::absolute() succeeded, path: " + path);
#endif
        } catch (const std::exception& e) {
            LOG_ERROR("fs::absolute() failed: " + std::string(e.what()));
            // 如果 absolute 失败（可能因为路径包含特殊字符），使用原始路径
            path = filepath;
#ifdef LSP_DEBUG_LOGGING
            LOG("Using original path: " + path);
#endif
        } catch (...) {
            LOG_ERROR("fs::absolute() failed: Unknown exception");
            path = filepath;
#ifdef LSP_DEBUG_LOGGING
            LOG("Using original path: " + path);
#endif
        }

        // 替换反斜杠为正斜杠（Windows）
        std::replace(path.begin(), path.end(), '\\', '/');
#ifdef LSP_DEBUG_LOGGING
        LOG("Path after replacing backslashes: " + path);
#endif

        // URL 编码（正确处理 UTF-8 多字节字符）
        // 对于 ASCII 字符，直接使用；对于多字节 UTF-8 字符，需要按字节编码
#ifdef LSP_DEBUG_LOGGING
        LOG("Starting URL encoding, path length: " + std::to_string(path.length()));
#endif
        for (size_t i = 0; i < path.length();) {
            unsigned char c = static_cast<unsigned char>(path[i]);

            // ASCII 字符（0x00-0x7F）且是安全字符
            if (c < 0x80 &&
                (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == ':')) {
                uri += static_cast<char>(c);
                i++;
            } else {
                // 需要编码的字符（包括 UTF-8 多字节字符）
                // 对于 UTF-8，每个字节都需要编码
                if (c < 0x80) {
                    // 单字节非 ASCII 字符
                    char hex[4];
                    snprintf(hex, sizeof(hex), "%%%02X", c);
                    uri += hex;
                    i++;
                } else {
                    // UTF-8 多字节字符：按字节编码
                    // UTF-8 字符的第一个字节：110xxxxx (2字节), 1110xxxx (3字节), 11110xxx (4字节)
                    int bytes = 0;
                    if ((c & 0xE0) == 0xC0)
                        bytes = 2; // 2字节字符
                    else if ((c & 0xF0) == 0xE0)
                        bytes = 3; // 3字节字符
                    else if ((c & 0xF8) == 0xF0)
                        bytes = 4; // 4字节字符
                    else
                        bytes = 1; // 无效的 UTF-8，按单字节处理

                    // 编码所有字节
                    for (int j = 0; j < bytes && (i + j) < path.length(); j++) {
                        unsigned char byte = static_cast<unsigned char>(path[i + j]);
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%%%02X", byte);
                        uri += hex;
                    }
                    i += bytes;
                }
            }
        }
#ifdef LSP_DEBUG_LOGGING
        LOG("URL encoding completed, URI length: " + std::to_string(uri.length()));
        LOG("Final URI: " + uri);
#endif
    } catch (const std::exception& e) {
        LOG_ERROR("filepathToUri() exception in main try block: " + std::string(e.what()));
        // 如果路径处理失败，使用简单的编码方式
        std::string path = filepath;
        std::replace(path.begin(), path.end(), '\\', '/');
#ifdef LSP_DEBUG_LOGGING
        LOG("Using fallback encoding for path: " + path);
#endif
        for (unsigned char c : path) {
            if (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == ':') {
                uri += static_cast<char>(c);
            } else {
                char hex[4];
                snprintf(hex, sizeof(hex), "%%%02X", c);
                uri += hex;
            }
        }
#ifdef LSP_DEBUG_LOGGING
        LOG("Fallback URI: " + uri);
#endif
    } catch (...) {
        LOG_ERROR("filepathToUri() unknown exception");
    }

    // 缓存 URI（限制缓存大小，使用简单的 LRU 策略）
    {
        std::lock_guard<std::mutex> lock(uri_cache_mutex_);
        if (uri_cache_.size() >= 100) {
            // 简单的 LRU：删除最旧的项（这里简化处理，删除第一个）
            uri_cache_.erase(uri_cache_.begin());
        }
        uri_cache_[filepath] = uri;
    }

#ifdef LSP_DEBUG_LOGGING
    LOG("filepathToUri() returning: " + uri);
#endif
    return uri;
}

void Editor::updateLspDocument() {
    LOG("[LSP_UPDATE] ===== updateLspDocument() START =====");

    if (!lsp_enabled_ || !lsp_manager_) {
        LOG("[LSP_UPDATE] LSP not enabled or manager not available");
        return;
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[LSP_UPDATE] No current document");
        return;
    }

    // 性能优化：检查文档大小，如果文档太大则跳过实时更新
    size_t doc_size = doc->lineCount();
    if (doc_size > 1000) { // 对于超过1000行的文档
        LOG("[LSP_UPDATE] Document too large (" + std::to_string(doc_size) +
            " lines), skipping real-time LSP updates");
        return;
    }

    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        LOG("[LSP_UPDATE] Document has no filepath (unsaved)");
        return;
    }

    LOG("[LSP_UPDATE] Document: " + filepath + " (lines: " + std::to_string(doc->lineCount()) +
        ")");

    // 防抖机制：限制文档更新频率
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(document_update_mutex_);
        auto time_since_last_update =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_document_update_time_);

        LOG("[LSP_UPDATE] Time since last update: " +
            std::to_string(time_since_last_update.count()) + "ms");

        if (time_since_last_update < document_update_debounce_interval_) {
            // 时间间隔太短，跳过更新，避免过于频繁的LSP通信
            // 只记录变更，不构建内容，不发送消息
            LOG("[LSP_UPDATE] SKIPPING document update (debounce: " +
                std::to_string(time_since_last_update.count()) + "ms < " +
                std::to_string(document_update_debounce_interval_.count()) + "ms)");
            return;
        }

        // 更新最后更新时间
        last_document_update_time_ = now;
        LOG("[LSP_UPDATE] Passed debounce check, proceeding with update");
    }

    try {
        std::string uri = filepathToUri(filepath);

        // 初始化变更跟踪器
        if (!document_change_tracker_) {
            document_change_tracker_ = std::make_unique<features::DocumentChangeTracker>();
        }

        // 初始化补全缓存
        if (!completion_cache_) {
            completion_cache_ = std::make_unique<features::LspCompletionCache>();
        }

        std::string language_id = detectLanguageId(filepath);

        // 获取或创建对应的 LSP 客户端
        LOG("LSP: Getting client for file: " + filepath);
        features::LspClient* client = lsp_manager_->getClientForFile(filepath);
        LOG("LSP: Client pointer: " + std::string(client ? "valid" : "null"));

        if (!client) {
            LOG("LSP: No client available for file, skipping LSP document update");
            return;
        }

        // 初始化客户端（如果还未初始化）
        LOG("LSP: Checking if client is connected...");
        bool is_connected = client->isConnected();
        LOG("LSP: Client connected: " + std::string(is_connected ? "YES" : "NO"));

        if (!is_connected) {
            // 客户端未连接：改为异步初始化以避免阻塞 UI 线程
            LOG("LSP: Client not connected, initializing asynchronously...");
            try {
                std::thread([client]() {
                    try {
                        std::string root_path = fs::current_path().string();
                        if (client->initialize(root_path)) {
                            LOG("LSP: Client initialized successfully (background)");
                        } else {
                            LOG_WARNING("LSP: Failed to initialize client (background)");
                        }
                    } catch (...) {
                        LOG_WARNING("LSP: Exception during background client initialization");
                    }
                }).detach();
            } catch (...) {
                LOG_WARNING("LSP: Failed to start background thread for client initialization");
            }

            // 跳过当前文档更新，等待后台初始化完成
            return;
        }

        LOG("LSP: Client is connected, proceeding with document update");

        // 获取文档内容
        std::string content;
        size_t line_count = doc->lineCount();

        // 限制读取的行数，避免大文件卡住（最多读取前1000行）
        size_t max_lines = std::min(line_count, static_cast<size_t>(1000));

        for (size_t i = 0; i < max_lines; ++i) {
            content += doc->getLine(i);
            if (i < max_lines - 1) {
                content += "\n";
            }
        }

        // 检查是否已经打开过
        if (file_language_map_.find(uri) == file_language_map_.end()) {
            // 首次打开，发送 didOpen（异步以避免阻塞 UI）
            if (lsp_request_manager_) {
                std::string uri_copy = uri;
                std::string language_copy = language_id;
                std::string content_copy = content;
                lsp_request_manager_->postOrReplace(
                    "didOpen:" + uri_copy, features::LspRequestManager::Priority::NORMAL,
                    [client, uri_copy, language_copy, content_copy]() mutable {
                        client->didOpen(uri_copy, language_copy, content_copy);
                    });
            } else {
                std::thread([client, uri, language_id, content]() {
                    try {
                        client->didOpen(uri, language_id, content);
                    } catch (...) {
                    }
                }).detach();
            }
            file_language_map_[uri] = language_id;
        } else {
            // 已打开，发送 didChange
            // 使用待处理的版本号，如果没有则从1开始
            int version = pending_document_version_ > 0 ? pending_document_version_ : 1;
            pending_document_version_ = version + 1;
            // 使用增量更新
            if (document_change_tracker_ && document_change_tracker_->hasChanges()) {
                auto changes = document_change_tracker_->getChanges();
                if (!changes.empty() && !changes[0].text.empty()) {
                    // 使用增量更新：异步提交到请求管理器以避免阻塞 UI
                    auto changes_copy = changes; // copy for lambda
                    auto client_ptr = client;
                    auto uri_copy = uri;
                    int ver = version;
                    lsp_request_manager_->postOrReplace(
                        "didChange:" + uri_copy, features::LspRequestManager::Priority::NORMAL,
                        [client_ptr, uri_copy, changes_copy, ver]() mutable {
                            client_ptr->didChangeIncremental(uri_copy, changes_copy, ver);
                        });
                    document_change_tracker_->clear();
                } else {
                    // 回退到全量更新，同样异步
                    auto client_ptr = client;
                    auto uri_copy = uri;
                    int ver = version;
                    auto content_copy = content;
                    lsp_request_manager_->postOrReplace(
                        "didChangeFull:" + uri_copy, features::LspRequestManager::Priority::NORMAL,
                        [client_ptr, uri_copy, content_copy, ver]() mutable {
                            client_ptr->didChange(uri_copy, content_copy, ver);
                        });
                    document_change_tracker_->clear();
                }
            } else {
                // 没有变更记录，使用全量更新（异步以避免阻塞 UI）
                if (lsp_request_manager_) {
                    auto client_ptr = client;
                    auto uri_copy = uri;
                    int ver = version;
                    auto content_copy = content;
                    lsp_request_manager_->postOrReplace(
                        "didChangeFull:" + uri_copy, features::LspRequestManager::Priority::NORMAL,
                        [client_ptr, uri_copy, content_copy, ver]() mutable {
                            client_ptr->didChange(uri_copy, content_copy, ver);
                        });
                } else {
                    std::thread([client, uri, content, version]() {
                        try {
                            client->didChange(uri, content, version);
                        } catch (...) {
                        }
                    }).detach();
                }
            }

            // 注意：不清除补全缓存，因为：
            // 1. 缓存键包含精确的位置信息，位置变化时自然无法命中
            // 2. 缓存有过期时间（5分钟），会自动清理过期项
            // 3. 频繁清空缓存会导致缓存命中率为0，影响性能
            // 只在文档关闭或明确需要时才清空缓存
        }
    } catch (const std::exception& e) {
        LOG_WARNING("updateLspDocument() exception: " + std::string(e.what()) +
                    " (LSP feature disabled for this file)");
        // 任何异常都不应该影响文件打开，静默处理
        return;
    } catch (...) {
        LOG_WARNING("updateLspDocument() unknown exception (LSP feature disabled for this file)");
        // 其他异常，静默处理
        return;
    }
}

void Editor::triggerCompletion() {
    // 补全项评分结构体
    struct ScoredItem {
        features::CompletionItem item;
        int score;

        bool operator<(const ScoredItem& other) const {
            if (score != other.score) {
                return score > other.score; // 分数高的在前
            }
            return item.label < other.item.label; // 相同分数按字母顺序
        }
    };

    LOG("[COMPLETION] ===== triggerCompletion() START =====");
    LOG("[COMPLETION] Current position: line " + std::to_string(cursor_row_) + ", col " +
        std::to_string(cursor_col_));

    if (!lsp_enabled_ || !lsp_manager_) {
        LOG("[COMPLETION] LSP not enabled or manager not available");
        return;
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[COMPLETION] No current document");
        return;
    }

    std::string filepath = doc->getFilePath();
    LOG("[COMPLETION] Document filepath: " + (filepath.empty() ? "<unsaved>" : filepath));

    // 优化的防抖机制（参考VSCode：平衡响应速度和性能）
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(completion_debounce_mutex_);
        auto time_since_last_trigger = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_completion_trigger_time_);

        LOG("[COMPLETION] Time since last trigger: " +
            std::to_string(time_since_last_trigger.count()) + "ms");

        // 使用平衡的debounce时间（100ms），避免过于频繁的请求
        if (time_since_last_trigger < std::chrono::milliseconds(100)) {
            LOG("[COMPLETION] Debounced: too frequent (" +
                std::to_string(time_since_last_trigger.count()) + "ms < 100ms)");
            return;
        }

        last_completion_trigger_time_ = now;
    }

    LOG("[COMPLETION] Passed debounce check, proceeding with completion");

    if (filepath.empty()) {
        // 如果文件未保存，使用临时路径
        filepath = "/tmp/pnana_unsaved_" + std::to_string(reinterpret_cast<uintptr_t>(doc));
    }

    LOG("[COMPLETION] Filepath: " + filepath);
    LOG("[COMPLETION] Cursor position: line " + std::to_string(cursor_row_) + ", col " +
        std::to_string(cursor_col_));

    // 获取或创建对应的 LSP 客户端
    auto client_start = std::chrono::steady_clock::now();
    features::LspClient* client = lsp_manager_->getClientForFile(filepath);
    auto client_end = std::chrono::steady_clock::now();
    auto client_time =
        std::chrono::duration_cast<std::chrono::microseconds>(client_end - client_start);
    LOG("[COMPLETION] Got LSP client (took " + std::to_string(client_time.count() / 1000.0) +
        " ms)");

    if (!client) {
        LOG("[COMPLETION] No LSP client available for this file type");
        completion_popup_.hide();
        return;
    }

    // 如果客户端未连接，尝试初始化（异步，不阻塞）
    if (!client->isConnected()) {
        LOG("[COMPLETION] Client not connected, initializing asynchronously...");
        // 在后台线程中初始化，避免阻塞补全请求
        std::thread([client, filepath]() {
            try {
                std::string root_path = fs::current_path().string();
                client->initialize(root_path);
            } catch (...) {
                // 初始化失败，静默处理
            }
        }).detach();
        completion_popup_.hide();
        return;
    }

    LOG("[COMPLETION] Client is connected");

    auto uri_start = std::chrono::steady_clock::now();
    std::string uri = filepathToUri(filepath);
    auto uri_end = std::chrono::steady_clock::now();
    auto uri_time = std::chrono::duration_cast<std::chrono::microseconds>(uri_end - uri_start);
    LOG("[COMPLETION] URI: " + uri + " (took " + std::to_string(uri_time.count() / 1000.0) +
        " ms)");

    // 获取当前位置
    features::LspPosition pos(static_cast<int>(cursor_row_), static_cast<int>(cursor_col_));
    LOG("[COMPLETION] LSP position: line " + std::to_string(pos.line) + ", character " +
        std::to_string(pos.character));

    // 获取当前行的光标位置之前的文本，用于过滤和排序
    // 参考 VSCode：更智能的前缀提取，支持更多字符类型
    auto prefix_start = std::chrono::steady_clock::now();
    const std::string& line = doc->getLine(cursor_row_);
    std::string prefix = "";
    if (cursor_col_ > 0 && static_cast<size_t>(cursor_col_) <= line.length()) {
        // 从光标位置向前查找单词边界
        // 支持：字母、数字、下划线、点、冒号、减号、箭头、方括号、括号、引用符等
        size_t start = static_cast<size_t>(cursor_col_);
        while (start > 0) {
            char c = line[start - 1];
            if (std::isalnum(c) || c == '_' || c == '.' || c == ':' || c == '-' || c == '>' ||
                c == '<' || c == '[' || c == ']' || c == '(' || c == ')' || c == '&' || c == '*' ||
                c == '#' || c == '@') {
                start--;
            } else {
                break;
            }
        }
        if (start < static_cast<size_t>(cursor_col_)) {
            prefix = line.substr(start, static_cast<size_t>(cursor_col_) - start);
        }
    }
    auto prefix_end = std::chrono::steady_clock::now();
    auto prefix_time =
        std::chrono::duration_cast<std::chrono::microseconds>(prefix_end - prefix_start);
    LOG("[COMPLETION] Extracted prefix: \"" + prefix + "\" (took " +
        std::to_string(prefix_time.count() / 1000.0) + " ms)");

    // 初始化补全缓存
    if (!completion_cache_) {
        completion_cache_ = std::make_unique<features::LspCompletionCache>();
    }

    // 简化的缓存检查 - 只检查当前位置的缓存
    features::LspCompletionCache::CacheKey cache_key;
    cache_key.uri = uri;
    cache_key.line = static_cast<int>(cursor_row_);
    cache_key.character = static_cast<int>(cursor_col_);
    cache_key.prefix = ""; // 简化：不使用前缀匹配

    int screen_width = screen_.dimx();
    int screen_height = screen_.dimy();

    // 计算光标在屏幕上的列位置（近似）：考虑侧边栏和行号宽度
    int editor_left_offset = 0;
    if (file_browser_.isVisible()) {
        editor_left_offset += file_browser_width_ + 1; // file browser + separator
    }
    int line_number_width = show_line_numbers_ ? 6 : 0; // 估算行号宽度（包含空格）
    int relative_col = static_cast<int>(cursor_col_) - static_cast<int>(view_offset_col_);
    if (relative_col < 0)
        relative_col = 0;
    int cursor_screen_col = editor_left_offset + line_number_width + relative_col;
    // 限制列到屏幕宽度范围，避免计算出过大的值导致弹窗遮挡其他UI
    if (cursor_screen_col > screen_width - 10) {
        cursor_screen_col = std::max(0, screen_width - 10);
    }

    // 检查缓存
    auto cached = completion_cache_->get(cache_key);
    if (cached.has_value() && !cached->empty()) {
        LOG("[COMPLETION] Cache HIT: " + std::to_string(cached->size()) + " items");
        // 限制显示数量
        std::vector<features::CompletionItem> limited_items = *cached;
        if (limited_items.size() > 50) {
            limited_items.resize(50);
        }
        showCompletionPopupIfChanged(limited_items, static_cast<int>(cursor_row_),
                                     cursor_screen_col, screen_width, screen_height);
        LOG("[COMPLETION] ===== triggerCompletion() END (from cache) =====");
        return;
    }

    LOG("[COMPLETION] Cache MISS - requesting from LSP server");

    // 使用异步管理器请求补全（参考VSCode：简单的异步处理）
    if (!lsp_async_manager_) {
        LOG("[COMPLETION] Creating new LspAsyncManager");
        lsp_async_manager_ = std::make_unique<features::LspAsyncManager>();
    }

    // 请求补全（非阻塞）
    int req_row = static_cast<int>(cursor_row_);
    int req_col = cursor_screen_col;
    int req_screen_w = screen_.dimx();
    int req_screen_h = screen_.dimy();

    LOG("[COMPLETION] Requesting async completion at pos (" + std::to_string(req_row) + "," +
        std::to_string(req_col) + ")");

    auto request_start = std::chrono::steady_clock::now();
    lsp_async_manager_->requestCompletionAsync(
        client, uri, pos,
        // on_success - 在主线程中更新UI
        [this, cache_key, req_row, req_col, req_screen_w, req_screen_h,
         request_start](const std::vector<features::CompletionItem>& items) {
            auto callback_start = std::chrono::steady_clock::now();
            auto request_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                callback_start - request_start);

            LOG("[COMPLETION] Async completion SUCCESS: received " + std::to_string(items.size()) +
                " items after " + std::to_string(request_duration.count()) + "ms");

            screen_.Post([this, items, cache_key, req_row, req_col, req_screen_w, req_screen_h,
                          callback_start]() {
                auto ui_update_start = std::chrono::steady_clock::now();

                // 缓存结果
                if (completion_cache_ && !items.empty()) {
                    completion_cache_->set(cache_key, items);
                    LOG("[COMPLETION] Cached " + std::to_string(items.size()) +
                        " completion items");
                }

                // 显示补全结果
                if (!items.empty()) {
                    std::vector<features::CompletionItem> limited = items;
                    if (limited.size() > 50) {
                        limited.resize(50);
                        LOG("[COMPLETION] Limited items from " + std::to_string(items.size()) +
                            " to 50");
                    }

                    LOG("[COMPLETION] Showing completion popup with " +
                        std::to_string(limited.size()) + " items");
                    showCompletionPopupIfChanged(limited, req_row, req_col, req_screen_w,
                                                 req_screen_h);
                } else {
                    LOG("[COMPLETION] No completion items, hiding popup");
                    completion_popup_.hide();
                }

                auto ui_update_end = std::chrono::steady_clock::now();
                auto ui_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    ui_update_end - ui_update_start);
                LOG("[COMPLETION] UI update completed in " + std::to_string(ui_duration.count()) +
                    "ms");
            });
        },
        // on_error - 隐藏弹窗
        [this, request_start](const std::string& err) {
            auto error_time = std::chrono::steady_clock::now();
            auto request_duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(error_time - request_start);

            LOG("[COMPLETION] Async completion FAILED after " +
                std::to_string(request_duration.count()) + "ms: " + err);
            screen_.Post([this]() {
                LOG("[COMPLETION] Hiding completion popup due to error");
                completion_popup_.hide();
            });
        });

    LOG("[COMPLETION] ===== triggerCompletion() END =====");
}

void Editor::handleCompletionInput(ftxui::Event event) {
    if (!completion_popup_.isVisible()) {
        return;
    }

    if (event == Event::ArrowDown) {
        completion_popup_.selectNext();
    } else if (event == Event::ArrowUp) {
        completion_popup_.selectPrevious();
    } else if (event == Event::Return || event == Event::Tab) {
        applyCompletion();
    } else if (event == Event::Escape) {
        completion_popup_.hide();
    }
}

void Editor::applyCompletion() {
    if (!completion_popup_.isVisible()) {
        return;
    }

    std::string text = completion_popup_.applySelected();
    completion_popup_.hide();

    if (text.empty()) {
        return;
    }

    pnana::core::Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    const std::string& line = doc->getLine(cursor_row_);
    if (cursor_col_ > line.length()) {
        cursor_col_ = line.length();
    }

    // 找到当前单词的开始位置（从光标位置向前查找单词边界）
    size_t word_start = cursor_col_;
    while (word_start > 0) {
        char ch = line[word_start - 1];
        if (!std::isalnum(ch) && ch != '_') {
            break;
        }
        word_start--;
    }

    std::string before_word = line.substr(0, word_start);
    std::string after_cursor = line.substr(cursor_col_);

    // 插入补全文本，替换从word_start到cursor_col_的文本
    std::string new_line = before_word + text + after_cursor;
    doc->replaceLine(cursor_row_, new_line);
    cursor_col_ = word_start + text.length();

    updateLspDocument();
}

ftxui::Element Editor::renderCompletionPopup() {
    if (!completion_popup_.isVisible()) {
        return ftxui::text("");
    }

    completion_popup_.updateCursorPosition(cursor_row_, cursor_col_, screen_.dimx(),
                                           screen_.dimy());

    return completion_popup_.render(theme_);
}

// Helper to avoid showing completion popup repeatedly and causing flicker.
void Editor::showCompletionPopupIfChanged(const std::vector<features::CompletionItem>& items,
                                          int row, int col, int screen_w, int screen_h) {
    auto now = std::chrono::steady_clock::now();
    int count = static_cast<int>(items.size());

    // 如果位置和数量与上次相同且在短时间内重复请求，则跳过显示（防止抖动）
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_popup_shown_time_);
    if (completion_popup_.isVisible() && last_popup_shown_count_ == count &&
        last_popup_row_ == row && last_popup_col_ == col && elapsed.count() < 200) {
        return;
    }

    // 更新状态并显示
    last_popup_shown_time_ = now;
    last_popup_shown_count_ = count;
    last_popup_row_ = row;
    last_popup_col_ = col;

    completion_popup_.show(items, static_cast<size_t>(row), static_cast<size_t>(col), screen_w,
                           screen_h);
}

void Editor::showDiagnosticsPopup() {
    // showDiagnosticsPopup called

    if (!lsp_enabled_) {
        // LSP not enabled
        setStatusMessage("LSP is not enabled. Cannot show diagnostics.");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(diagnostics_mutex_);
        // setting diagnostics to popup
        diagnostics_popup_.setDiagnostics(current_file_diagnostics_);
    }

    if (current_file_diagnostics_.empty()) {
        // no diagnostics found
        setStatusMessage("No diagnostics found for the current file.");
        return;
    }

    diagnostics_popup_.setJumpCallback([this](const pnana::features::Diagnostic& diagnostic) {
        jumpToDiagnostic(diagnostic);
    });

    diagnostics_popup_.setCopyCallback([this](const std::string& /*text*/) {
        copySelectedDiagnostic();
    });

    // 显示弹窗对象并标记为显示（两个状态都需要）
    diagnostics_popup_.show();
    diagnostics_popup_.show();
    show_diagnostics_popup_ = true;
}

void Editor::hideDiagnosticsPopup() {
    diagnostics_popup_.hide();
    show_diagnostics_popup_ = false;
}

void Editor::updateDiagnosticsStatus(const std::vector<pnana::features::Diagnostic>& diagnostics) {
    {
        std::lock_guard<std::mutex> lock(diagnostics_mutex_);
        current_file_diagnostics_ = diagnostics;
    }

    size_t error_count = 0;
    size_t warning_count = 0;
    size_t info_count = 0;

    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == 1) { // Error
            error_count++;
        } else if (diagnostic.severity == 2) { // Warning
            warning_count++;
        } else if (diagnostic.severity == 3) { // Information
            info_count++;
        }
    }

    std::string status_msg;
    if (error_count > 0) {
        status_msg = "Errors: " + std::to_string(error_count);
        if (warning_count > 0) {
            status_msg += ", Warnings: " + std::to_string(warning_count);
        }
        if (info_count > 0) {
            status_msg += ", Info: " + std::to_string(info_count);
        }
    } else if (warning_count > 0) {
        status_msg = "Warnings: " + std::to_string(warning_count);
        if (info_count > 0) {
            status_msg += ", Info: " + std::to_string(info_count);
        }
    } else if (!diagnostics.empty()) {
        status_msg = "Diagnostics: " + std::to_string(diagnostics.size());
    }

    if (!status_msg.empty()) {
        setStatusMessage(status_msg);
    }

    // 强制触发UI更新，确保诊断变化立即反映在行号上
    // 通过更新一个内部状态来触发重绘
    force_ui_update_ = true;

    // 如果诊断弹窗当前可见，则同步更新弹窗内容，确保内容实时性
    if (diagnostics_popup_.isVisible()) {
        diagnostics_popup_.setDiagnostics(diagnostics);
    }

    // 主线程重绘触发：向 FTXUI 投递一个空任务以促使 UI 立即重绘（兼容不同 FTXUI 版本）
    try {
        screen_.Post([this]() {
            // no-op: 仅用于唤醒主线程并触发一次重绘循环
            (void)force_ui_update_;
        });
    } catch (...) {
        // 如果投递任务失败，忽略（安全降级）
    }
}

void Editor::copySelectedDiagnostic() {
    if (!diagnostics_popup_.isVisible()) {
        return;
    }

    std::string diagnostic_text = diagnostics_popup_.getSelectedDiagnosticText();

    if (!diagnostic_text.empty()) {
        if (pnana::utils::Clipboard::copyToSystem(diagnostic_text)) {
            setStatusMessage("Diagnostic information copied to clipboard");
        } else {
            setStatusMessage("Failed to copy to clipboard, please check system clipboard tools");
        }
    } else {
        setStatusMessage("No diagnostic information selected");
    }
}

void Editor::jumpToDiagnostic(const pnana::features::Diagnostic& diagnostic) {
    cursor_row_ = diagnostic.range.start.line;
    cursor_col_ = diagnostic.range.start.character;

    if (getCurrentDocument()) {
        adjustViewOffset();
    }

    std::string severity_str = diagnostics_popup_.getSeverityString(diagnostic.severity);
    setStatusMessage("Jumped to " + severity_str + ": " + diagnostic.message.substr(0, 50) + "...");
}

Element Editor::renderDiagnosticsPopup() {
    return diagnostics_popup_.render();
}

} // namespace core
} // namespace pnana

#endif // BUILD_LSP_SUPPORT
