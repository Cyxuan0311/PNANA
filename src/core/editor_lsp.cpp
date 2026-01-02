// LSP 集成相关实现
#include "core/editor.h"
#include "features/lsp/lsp_server_manager.h"
#include "utils/logger.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <ftxui/component/event.hpp>
#include <cstring>
#include <chrono>
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
    
    // 设置诊断回调（应用到所有 LSP 客户端）
    lsp_manager_->setDiagnosticsCallback([this](
        const std::string& /* uri */,
        const std::vector<features::Diagnostic>& diagnostics) {
        // 可以在这里处理诊断信息，比如在状态栏显示错误数量
        if (!diagnostics.empty()) {
            int error_count = 0;
            int warning_count = 0;
            for (const auto& diag : diagnostics) {
                if (diag.severity == 1) error_count++;
                else if (diag.severity == 2) warning_count++;
            }
            if (error_count > 0 || warning_count > 0) {
                std::string msg = "LSP: ";
                if (error_count > 0) msg += std::to_string(error_count) + " error(s)";
                if (warning_count > 0) {
                    if (error_count > 0) msg += ", ";
                    msg += std::to_string(warning_count) + " warning(s)";
                }
                setStatusMessage(msg);
            }
        }
    });
    
    // LSP 管理器使用延迟初始化，只在需要时启动对应的服务器
        lsp_enabled_ = true;
    setStatusMessage("LSP manager initialized");
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
    
    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".hpp" || 
        ext == ".hxx" || ext == ".h" || ext == ".c") {
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
        for (size_t i = 0; i < path.length(); ) {
            unsigned char c = static_cast<unsigned char>(path[i]);
            
            // ASCII 字符（0x00-0x7F）且是安全字符
            if (c < 0x80 && (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == ':')) {
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
                    if ((c & 0xE0) == 0xC0) bytes = 2;      // 2字节字符
                    else if ((c & 0xF0) == 0xE0) bytes = 3;  // 3字节字符
                    else if ((c & 0xF8) == 0xF0) bytes = 4; // 4字节字符
                    else bytes = 1;  // 无效的 UTF-8，按单字节处理
                    
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
    if (!lsp_enabled_ || !lsp_manager_) {
        return;
    }
    
    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }
    
    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        return;
    }
    
    // 防抖机制：限制文档更新频率
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(document_update_mutex_);
        auto time_since_last_update = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_document_update_time_
        );
        
        if (time_since_last_update < document_update_debounce_interval_) {
            // 更新待处理的文档内容
            pending_document_uri_ = filepathToUri(filepath);
            // 从 Document 的 lines 构建内容
            std::string content;
            const auto& lines = doc->getLines();
            for (size_t i = 0; i < lines.size(); ++i) {
                content += lines[i];
                if (i < lines.size() - 1) {
                    content += "\n";
                }
            }
            pending_document_content_ = content;
            pending_document_version_++;  // 增加版本号
            // 不立即发送，等待防抖间隔
            return;
        }
        
        // 更新最后更新时间
        last_document_update_time_ = now;
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
        features::LspClient* client = lsp_manager_->getClientForFile(filepath);
        if (!client) {
            return;
        }
        
        // 延迟初始化：如果客户端还未初始化，跳过本次更新
        // 初始化应该在后台异步进行，不阻塞文件打开
        if (!client->isConnected()) {
            // 不在这里初始化，避免阻塞文件打开
            // 初始化将在后台异步进行，或者用户首次触发补全时进行
            return;
        }
    
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
            // 首次打开，发送 didOpen
            client->didOpen(uri, language_id, content);
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
                    // 使用增量更新
                    client->didChangeIncremental(uri, changes, version);
                    document_change_tracker_->clear();
                } else {
                    // 回退到全量更新
                    client->didChange(uri, content, version);
                    document_change_tracker_->clear();
                }
            } else {
                // 没有变更记录，使用全量更新
                client->didChange(uri, content, version);
            }
            
            // 注意：不清除补全缓存，因为：
            // 1. 缓存键包含精确的位置信息，位置变化时自然无法命中
            // 2. 缓存有过期时间（5分钟），会自动清理过期项
            // 3. 频繁清空缓存会导致缓存命中率为0，影响性能
            // 只在文档关闭或明确需要时才清空缓存
        }
    } catch (const std::exception& e) {
        LOG_WARNING("updateLspDocument() exception: " + std::string(e.what()) + " (LSP feature disabled for this file)");
        // 任何异常都不应该影响文件打开，静默处理
        return;
    } catch (...) {
        LOG_WARNING("updateLspDocument() unknown exception (LSP feature disabled for this file)");
        // 其他异常，静默处理
        return;
    }
}

void Editor::triggerCompletion() {
    auto trigger_start = std::chrono::steady_clock::now();
    LOG("[COMPLETION] ===== triggerCompletion() START =====");
    
    if (!lsp_enabled_ || !lsp_manager_) {
        LOG("[COMPLETION] LSP not enabled or manager not available");
        return;
    }
    
    Document* doc = getCurrentDocument();
    if (!doc) {
        LOG("[COMPLETION] No current document");
        return;
    }
    
    // 补全防抖机制（优化：参考 VSCode，快速响应）
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(completion_debounce_mutex_);
        auto time_since_last_trigger = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_completion_trigger_time_
        );
        
        // 参考 VSCode：防抖时间已优化为 100ms，足够快速响应
        // 如果时间间隔太短，跳过此次请求（避免过于频繁）
        if (time_since_last_trigger < completion_debounce_interval_) {
            LOG("[COMPLETION] Debounced: " + std::to_string(time_since_last_trigger.count()) + 
                " ms < " + std::to_string(completion_debounce_interval_.count()) + " ms");
            return;
        }
        
        last_completion_trigger_time_ = now;
    }
    
    LOG("[COMPLETION] Passed debounce check");
    
    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        // 如果文件未保存，使用临时路径
        filepath = "/tmp/pnana_unsaved_" + std::to_string(reinterpret_cast<uintptr_t>(doc));
    }
    
    LOG("[COMPLETION] Filepath: " + filepath);
    LOG("[COMPLETION] Cursor position: line " + std::to_string(cursor_row_) + 
        ", col " + std::to_string(cursor_col_));
    
    // 获取或创建对应的 LSP 客户端
    auto client_start = std::chrono::steady_clock::now();
    features::LspClient* client = lsp_manager_->getClientForFile(filepath);
    auto client_end = std::chrono::steady_clock::now();
    auto client_time = std::chrono::duration_cast<std::chrono::microseconds>(client_end - client_start);
    LOG("[COMPLETION] Got LSP client (took " + std::to_string(client_time.count() / 1000.0) + " ms)");
    
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
    LOG("[COMPLETION] URI: " + uri + " (took " + std::to_string(uri_time.count() / 1000.0) + " ms)");
    
    // 获取当前位置
    features::LspPosition pos(static_cast<int>(cursor_row_), static_cast<int>(cursor_col_));
    LOG("[COMPLETION] LSP position: line " + std::to_string(pos.line) + 
        ", character " + std::to_string(pos.character));
    
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
            if (std::isalnum(c) || 
                c == '_' || 
                c == '.' ||
                c == ':' ||
                c == '-' ||
                c == '>' ||
                c == '<' ||
                c == '[' ||
                c == ']' ||
                c == '(' ||
                c == ')' ||
                c == '&' ||
                c == '*' ||
                c == '#' ||
                c == '@') {
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
    auto prefix_time = std::chrono::duration_cast<std::chrono::microseconds>(prefix_end - prefix_start);
    LOG("[COMPLETION] Extracted prefix: \"" + prefix + "\" (took " + 
        std::to_string(prefix_time.count() / 1000.0) + " ms)");
    
    // 初始化补全缓存（阶段2优化）
    if (!completion_cache_) {
        completion_cache_ = std::make_unique<features::LspCompletionCache>();
        LOG("[COMPLETION] Created completion cache");
    }
    
    // 检查缓存（优化：支持智能匹配）
    auto cache_check_start = std::chrono::steady_clock::now();
    features::LspCompletionCache::CacheKey cache_key;
    cache_key.uri = uri;
    cache_key.line = static_cast<int>(cursor_row_);
    cache_key.character = static_cast<int>(cursor_col_);
    cache_key.prefix = prefix;
    
    LOG("[COMPLETION] [CACHE] Checking cache: uri=" + uri + 
        ", line=" + std::to_string(cache_key.line) + 
        ", char=" + std::to_string(cache_key.character) + 
        ", prefix=\"" + prefix + "\"");
    
    int screen_width = screen_.dimx();
    int screen_height = screen_.dimy();
    
    // 首先尝试精确匹配
    auto cached = completion_cache_->get(cache_key);
    auto cache_check_end = std::chrono::steady_clock::now();
    auto cache_check_time = std::chrono::duration_cast<std::chrono::microseconds>(
        cache_check_end - cache_check_start);
    LOG("[COMPLETION] Cache check (took " + std::to_string(cache_check_time.count() / 1000.0) + " ms)");
    
    if (cached.has_value()) {
        LOG("[COMPLETION] Cache HIT: found " + std::to_string(cached->size()) + " items");
        // 使用缓存结果（已经过智能排序）
        if (!cached->empty()) {
            // 限制显示数量，提高响应速度
            std::vector<features::CompletionItem> limited_items = *cached;
            if (limited_items.size() > 30) {
                limited_items.resize(30);
                LOG("[COMPLETION] Limited items from " + std::to_string(cached->size()) + " to 30");
            }
            auto show_start = std::chrono::steady_clock::now();
            completion_popup_.show(limited_items, cursor_row_, cursor_col_, screen_width, screen_height);
            auto show_end = std::chrono::steady_clock::now();
            auto show_time = std::chrono::duration_cast<std::chrono::microseconds>(show_end - show_start);
            LOG("[COMPLETION] Showed popup from cache (took " + 
                std::to_string(show_time.count() / 1000.0) + " ms)");
            
            auto trigger_end = std::chrono::steady_clock::now();
            auto trigger_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                trigger_end - trigger_start);
            LOG("[COMPLETION] ===== triggerCompletion() END (from cache, took " + 
                std::to_string(trigger_time.count()) + " ms) =====");
        } else {
            LOG("[COMPLETION] Cache hit but empty, hiding popup");
            completion_popup_.hide();
        }
        return;
    }
    
    LOG("[COMPLETION] [CACHE] Cache MISS for exact key");
    LOG("[COMPLETION] [CACHE] Cache size: " + std::to_string(completion_cache_->size()));
    
    // 优化：收集缓存结果作为异步请求失败时的回退，但不立即显示（避免屏幕抖动）
    std::vector<features::CompletionItem> cached_fallback_results;

    // 1. 如果有前缀，首先尝试使用前缀过滤（从任何相同位置的缓存项中过滤）
    if (!prefix.empty()) {
        LOG("[COMPLETION] [CACHE] Trying prefix filter with prefix=\"" + prefix + "\"");
        auto filter_start = std::chrono::steady_clock::now();
        auto filtered = completion_cache_->filterByPrefix(cache_key, prefix);
        auto filter_end = std::chrono::steady_clock::now();
        auto filter_time = std::chrono::duration_cast<std::chrono::microseconds>(
            filter_end - filter_start);
        LOG("[COMPLETION] [CACHE] Prefix filter (took " + std::to_string(filter_time.count() / 1000.0) +
            " ms, found " + std::to_string(filtered.size()) + " items)");

        if (!filtered.empty()) {
            cached_fallback_results = std::move(filtered);
            LOG("[COMPLETION] [CACHE] Found " + std::to_string(cached_fallback_results.size()) +
                " cached items, showing immediately for fast response");

            // 立即显示缓存结果，提供快速响应，避免屏幕空白
            auto show_start = std::chrono::steady_clock::now();
            int screen_width = screen_.dimx();
            int screen_height = screen_.dimy();
            completion_popup_.show(cached_fallback_results, cursor_row_, cursor_col_, screen_width, screen_height);
            auto show_end = std::chrono::steady_clock::now();
            auto show_time = std::chrono::duration_cast<std::chrono::microseconds>(show_end - show_start);
            LOG("[COMPLETION] [CACHE] Immediate cache display (took " +
                std::to_string(show_time.count() / 1000.0) + " ms)");
        }
    }

    // 2. 如果没有缓存过滤结果，尝试查找空前缀的缓存项
    if (cached_fallback_results.empty()) {
        LOG("[COMPLETION] [CACHE] Trying cache without prefix...");
        features::LspCompletionCache::CacheKey no_prefix_key = cache_key;
        no_prefix_key.prefix = "";
        auto cached_no_prefix = completion_cache_->get(no_prefix_key);
        if (cached_no_prefix.has_value() && !cached_no_prefix->empty()) {
            cached_fallback_results = *cached_no_prefix;
            // 限制显示数量，避免显示过多项
            if (cached_fallback_results.size() > 30) {
                cached_fallback_results.resize(30);
            }
            LOG("[COMPLETION] [CACHE] Found " + std::to_string(cached_fallback_results.size()) +
                " no-prefix cached items, showing immediately for fast response");

            // 立即显示缓存结果，提供快速响应
            auto show_start = std::chrono::steady_clock::now();
            int screen_width = screen_.dimx();
            int screen_height = screen_.dimy();
            completion_popup_.show(cached_fallback_results, cursor_row_, cursor_col_, screen_width, screen_height);
            auto show_end = std::chrono::steady_clock::now();
            auto show_time = std::chrono::duration_cast<std::chrono::microseconds>(show_end - show_start);
            LOG("[COMPLETION] [CACHE] Immediate no-prefix cache display (took " +
                std::to_string(show_time.count() / 1000.0) + " ms)");
        }
    }
    
    // 使用异步管理器请求补全
    if (!lsp_async_manager_) {
        LOG("[COMPLETION] Creating LspAsyncManager...");
        lsp_async_manager_ = std::make_unique<features::LspAsyncManager>();
    }
    
    LOG("[COMPLETION] Queuing async completion request...");
    
    // 异步请求补全
    bool has_shown_cache = !cached_fallback_results.empty();
    lsp_async_manager_->requestCompletionAsync(
        client, uri, pos,
        [this, uri, cache_key, trigger_start, cached_fallback_results = std::move(cached_fallback_results), has_shown_cache](std::vector<features::CompletionItem> items) {
            auto callback_start = std::chrono::steady_clock::now();
            auto request_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                callback_start - trigger_start);
            LOG("[COMPLETION] ===== Async callback START (request took " + 
                std::to_string(request_time.count()) + " ms) =====");
            LOG("[COMPLETION] Received " + std::to_string(items.size()) + " items from LSP");
            
            // 缓存结果（优化：同时保存完整前缀和空前缀的缓存，提高命中率）
            auto cache_set_start = std::chrono::steady_clock::now();
            if (completion_cache_ && !items.empty()) {
                LOG("[COMPLETION] [CACHE] Saving cache: prefix=\"" + cache_key.prefix + 
                    "\", items=" + std::to_string(items.size()));
                
                // 保存完整前缀的缓存
                completion_cache_->set(cache_key, items, true);
                LOG("[COMPLETION] [CACHE] Saved cache with prefix=\"" + cache_key.prefix + "\"");
                
                // 如果前缀不为空，也保存空前缀的缓存（用于后续前缀过滤）
                if (!cache_key.prefix.empty()) {
                    features::LspCompletionCache::CacheKey no_prefix_key = cache_key;
                    no_prefix_key.prefix = "";
                    // 检查是否已存在空前缀缓存，避免覆盖更完整的缓存
                    auto existing = completion_cache_->get(no_prefix_key);
                    if (!existing.has_value() || existing->size() < items.size()) {
                        completion_cache_->set(no_prefix_key, items, true);
                        LOG("[COMPLETION] [CACHE] Also saved cache without prefix (items=" + 
                            std::to_string(items.size()) + ")");
                    } else {
                        LOG("[COMPLETION] [CACHE] Skipped saving no-prefix cache (existing has " + 
                            std::to_string(existing->size()) + " items)");
                    }
                }
                
                auto cache_set_end = std::chrono::steady_clock::now();
                auto cache_set_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    cache_set_end - cache_set_start);
                LOG("[COMPLETION] [CACHE] Cache save completed (took " + 
                    std::to_string(cache_set_time.count() / 1000.0) + " ms, cache size now: " + 
                    std::to_string(completion_cache_->size()) + ")");
            }
            
            // 在回调中更新补全列表
            if (!items.empty()) {
                auto filter_start = std::chrono::steady_clock::now();
                Document* doc = getCurrentDocument();
                if (!doc) {
                    LOG("[COMPLETION] No document in callback, hiding popup");
                    // 只在 popup 可见时才 hide，避免不必要的操作
                    if (completion_popup_.isVisible()) {
                        completion_popup_.hide();
                    }
                    return;
                }
                
                const std::string& line = doc->getLine(cursor_row_);
                std::string prefix = "";
                if (cursor_col_ > 0 && static_cast<size_t>(cursor_col_) <= line.length()) {
                    // 参考 VSCode：更智能的前缀提取
                    size_t start = static_cast<size_t>(cursor_col_);
                    while (start > 0) {
                        char c = line[start - 1];
                        if (std::isalnum(c) || 
                            c == '_' || 
                            c == '.' ||
                            c == ':' ||
                            c == '-' ||
                            c == '>' ||
                            c == '<' ||
                            c == '[' ||
                            c == ']' ||
                            c == '(' ||
                            c == ')' ||
                            c == '&' ||
                            c == '*' ||
                            c == '#' ||
                            c == '@') {
                            start--;
                        } else {
                            break;
                        }
                    }
                    if (start < static_cast<size_t>(cursor_col_)) {
                        prefix = line.substr(start, static_cast<size_t>(cursor_col_) - start);
                    }
                }
                LOG("[COMPLETION] Prefix in callback: \"" + prefix + "\"");
                LOG("[COMPLETION] Total items from LSP: " + std::to_string(items.size()));
                
                // 辅助函数：规范化字符串（去除特殊字符，用于匹配）
                // 需要先定义，因为后面会用到
                auto normalizeForMatching = [](const std::string& str) -> std::string {
                    std::string normalized;
                    normalized.reserve(str.length());
                    for (char c : str) {
                        // 保留字母、数字、下划线、点、冒号、减号
                        // 去除 #、<、>、[、]、(、)、&、*、@ 等特殊字符
                        if (std::isalnum(c) || c == '_' || c == '.' || c == ':' || c == '-') {
                            normalized += c;
                        }
                    }
                    return normalized;
                };
                
                std::string normalized_prefix_for_log = normalizeForMatching(prefix);
                bool has_special_chars = (prefix != normalized_prefix_for_log);
                LOG("[COMPLETION] Prefix length: " + std::to_string(prefix.length()) + 
                    ", has special chars: " + (has_special_chars ? "yes" : "no"));
                
                // 调试：输出前几个 items 的详细信息
                if (items.size() <= 10) {
                    for (size_t i = 0; i < items.size(); i++) {
                        LOG("[COMPLETION] Item[" + std::to_string(i) + "]: label=\"" + items[i].label + 
                            "\", kind=" + items[i].kind);
                    }
                } else {
                    for (size_t i = 0; i < 5; i++) {
                        LOG("[COMPLETION] Item[" + std::to_string(i) + "]: label=\"" + items[i].label + 
                            "\", kind=" + items[i].kind);
                    }
                    LOG("[COMPLETION] ... (and " + std::to_string(items.size() - 5) + " more items)");
                }
                
                // 参考 VSCode 的智能排序和过滤算法
                // 使用评分系统：精确匹配 > 前缀匹配 > 包含匹配，同时考虑类型优先级
                struct ScoredItem {
                    features::CompletionItem item;
                    int score;
                    
                    bool operator<(const ScoredItem& other) const {
                        if (score != other.score) {
                            return score > other.score;  // 分数高的在前
                        }
                        return item.label < other.item.label;  // 相同分数按字母顺序
                    }
                };
                
                std::vector<ScoredItem> scored_items;
                std::string lower_prefix = prefix;
                std::transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(), ::tolower);
                std::string normalized_prefix = normalizeForMatching(prefix);
                std::string lower_normalized_prefix = normalizeForMatching(lower_prefix);
                
                // 获取类型优先级（函数/方法 > 变量/字段 > 类/接口 > 其他）
                auto getTypePriority = [](const std::string& kind) -> int {
                    if (kind == "2" || kind == "3" || kind == "4") return 10;  // Method, Function, Constructor
                    if (kind == "5" || kind == "6") return 8;  // Field, Variable
                    if (kind == "7" || kind == "8" || kind == "22") return 6;  // Class, Interface, Struct
                    if (kind == "21") return 5;  // Constant
                    if (kind == "14") return 4;  // Keyword
                    return 3;  // Other
                };
                
                int exact_count = 0, prefix_count = 0, contain_count = 0, other_count = 0;
                
                // 预计算前缀长度，避免重复计算
                size_t prefix_len = prefix.length();
                size_t lower_prefix_len = lower_prefix.length();
                size_t normalized_prefix_len = normalized_prefix.length();
                size_t lower_normalized_prefix_len = lower_normalized_prefix.length();
                
                // 预计算前缀是否包含特殊字符（用于优化匹配顺序）
                bool prefix_has_special_chars = (prefix != normalized_prefix);
                std::string prefix_has_special_chars_str = prefix_has_special_chars ? "true" : "false";
                LOG("[COMPLETION] Matching strategy: prefix_has_special_chars=" + prefix_has_special_chars_str + 
                    ", prefix_len=" + std::to_string(prefix_len) + 
                    ", normalized_prefix_len=" + std::to_string(normalized_prefix_len));
                
                int processed_count = 0;
                int matched_count = 0;
                for (const auto& item : items) {
                    processed_count++;
                    // 处理label前面的空格：trim前面的空格
                    std::string label = item.label;
                    size_t label_start = 0;
                    while (label_start < label.length() && std::isspace(label[label_start])) {
                        label_start++;
                    }
                    if (label_start > 0) {
                        label = label.substr(label_start);
                    }
                    
                    int score = 0;
                    
                    // 延迟计算 lower_label 和 normalized_label（只在需要时计算）
                    std::string lower_label;
                    std::string normalized_label;
                    std::string lower_normalized_label;
                    bool lower_label_computed = false;
                    bool normalized_label_computed = false;
                    
                    // 辅助函数：延迟计算 lower_label
                    auto getLowerLabel = [&]() -> const std::string& {
                        if (!lower_label_computed) {
                            lower_label = label;
                            std::transform(lower_label.begin(), lower_label.end(), lower_label.begin(), ::tolower);
                            lower_label_computed = true;
                        }
                        return lower_label;
                    };
                    
                    // 辅助函数：延迟计算 normalized_label
                    auto getNormalizedLabel = [&]() -> const std::string& {
                        if (!normalized_label_computed) {
                            normalized_label = normalizeForMatching(label);
                            lower_normalized_label = normalizeForMatching(getLowerLabel());
                            normalized_label_computed = true;
                        }
                        return normalized_label;
                    };
                    
                    if (prefix.empty()) {
                        // 没有前缀，只按类型优先级排序
                        score = getTypePriority(item.kind) * 100;
                        other_count++;
                    } else {
                        // 优化：根据前缀是否包含特殊字符，调整匹配顺序
                        // 如果前缀包含特殊字符，优先尝试规范化匹配
                        // 否则优先尝试普通匹配
                        
                        // 1. 精确匹配（大小写敏感）- 最高优先级 (10000分)
                        if (label.length() == prefix_len && label == prefix) {
                            score = 10000 + getTypePriority(item.kind) * 100;
                            exact_count++;
                        }
                        // 2. 前缀匹配（大小写敏感）- 高优先级 (8000分) - 最常见的匹配类型
                        else if (label.length() >= prefix_len && 
                                 label.compare(0, prefix_len, prefix) == 0) {
                            score = 8000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(label.length()));
                            prefix_count++;
                        }
                        // 3. 精确匹配（大小写不敏感）- 次高优先级 (9000分)
                        else if (label.length() == prefix_len) {
                            const std::string& lower = getLowerLabel();
                            if (lower == lower_prefix) {
                                score = 9000 + getTypePriority(item.kind) * 100;
                                exact_count++;
                            }
                        }
                        // 4. 前缀匹配（大小写不敏感）- 中高优先级 (7000分)
                        else if (label.length() >= lower_prefix_len) {
                            const std::string& lower = getLowerLabel();
                            if (lower.compare(0, lower_prefix_len, lower_prefix) == 0) {
                            score = 7000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(label.length()));
                            prefix_count++;
                        }
                        }
                        
                        // 如果还没有匹配，尝试规范化匹配（在包含匹配之前）
                        if (score == 0 && prefix_has_special_chars) {
                            const std::string& normalized = getNormalizedLabel();
                            // 5. 规范化精确匹配（去除特殊字符后）- 高优先级 (8500分)
                            if (normalized.length() == normalized_prefix_len && 
                                normalized == normalized_prefix) {
                                score = 8500 + getTypePriority(item.kind) * 100;
                                exact_count++;
                            }
                            // 6. 规范化精确匹配（大小写不敏感）- 高优先级 (8200分)
                            else if (normalized.length() == normalized_prefix_len && 
                                     lower_normalized_label == lower_normalized_prefix) {
                                score = 8200 + getTypePriority(item.kind) * 100;
                                exact_count++;
                            }
                            // 7. 规范化前缀匹配（去除特殊字符后）- 中高优先级 (6500分)
                            else if (normalized.length() >= normalized_prefix_len && 
                                     normalized.compare(0, normalized_prefix_len, normalized_prefix) == 0) {
                                score = 6500 + getTypePriority(item.kind) * 100;
                                score += (100 - static_cast<int>(label.length()));
                                prefix_count++;
                            }
                            // 8. 规范化前缀匹配（大小写不敏感）- 中优先级 (6000分)
                            else if (normalized.length() >= lower_normalized_prefix_len && 
                                     lower_normalized_label.compare(0, lower_normalized_prefix_len, lower_normalized_prefix) == 0) {
                                score = 6000 + getTypePriority(item.kind) * 100;
                                score += (100 - static_cast<int>(label.length()));
                                prefix_count++;
                            }
                        }
                        
                        // 如果还没有匹配，尝试包含匹配
                        if (score == 0) {
                            // 9. 包含匹配（大小写敏感）- 中优先级 (5000分)
                            size_t pos = label.find(prefix);
                            if (pos != std::string::npos) {
                            score = 5000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(pos));
                            contain_count++;
                        }
                            // 10. 包含匹配（大小写不敏感）- 低优先级 (3000分)
                            else {
                                const std::string& lower = getLowerLabel();
                                pos = lower.find(lower_prefix);
                                if (pos != std::string::npos) {
                            score = 3000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(pos));
                            contain_count++;
                        }
                                // 11-12. 规范化包含匹配（仅在包含特殊字符时尝试）
                                else if (prefix_has_special_chars) {
                                    const std::string& normalized = getNormalizedLabel();
                                    pos = normalized.find(normalized_prefix);
                                    if (pos != std::string::npos) {
                                        score = 2500 + getTypePriority(item.kind) * 100;
                                        score += (100 - static_cast<int>(pos));
                                        contain_count++;
                                        // 详细日志：记录规范化包含匹配成功
                                        if (processed_count <= 3) {
                                            LOG("[COMPLETION] [MATCH] Normalized contain match: normalized_prefix=\"" + 
                                                normalized_prefix + "\" found at pos=" + std::to_string(pos) + 
                                                " in normalized=\"" + normalized + "\"");
                                        }
                                    } else {
                                        pos = lower_normalized_label.find(lower_normalized_prefix);
                                        if (pos != std::string::npos) {
                                            score = 2000 + getTypePriority(item.kind) * 100;
                                            score += (100 - static_cast<int>(pos));
                                            contain_count++;
                                            // 详细日志：记录规范化包含匹配成功（大小写不敏感）
                                            if (processed_count <= 3) {
                                                LOG("[COMPLETION] [MATCH] Normalized contain match (case-insensitive): lower_normalized_prefix=\"" + 
                                                    lower_normalized_prefix + "\" found at pos=" + std::to_string(pos) + 
                                                    " in lower_normalized=\"" + lower_normalized_label + "\"");
                                            }
                                        } else {
                                            // 详细日志：记录规范化包含匹配失败
                                            if (processed_count <= 3) {
                                                LOG("[COMPLETION] [MATCH] Normalized contain match failed: normalized_prefix=\"" + 
                                                    normalized_prefix + "\" not found in normalized=\"" + normalized + 
                                                    "\", lower_normalized_prefix=\"" + lower_normalized_prefix + 
                                                    "\" not found in lower_normalized=\"" + lower_normalized_label + "\"");
                                            }
                                        }
                                    }
                                }
                                // 13. 模糊匹配（字符序列匹配，如 "usi" 匹配 "using"）- 最低优先级 (1000分)
                                if (score == 0 && prefix_len >= 3) {
                                    const std::string& lower = getLowerLabel();
                                    size_t prefix_idx = 0;
                                    for (size_t i = 0; i < lower.length() && prefix_idx < lower_prefix_len; i++) {
                                        if (lower[i] == lower_prefix[prefix_idx]) {
                                            prefix_idx++;
                                        }
                                    }
                                    if (prefix_idx == lower_prefix_len) {
                                        // 计算匹配质量：连续字符越多，分数越高
                                        int consecutive = 0;
                                        int max_consecutive = 0;
                                        prefix_idx = 0;
                                        for (size_t i = 0; i < lower.length() && prefix_idx < lower_prefix_len; i++) {
                                            if (lower[i] == lower_prefix[prefix_idx]) {
                                                consecutive++;
                                                max_consecutive = std::max(max_consecutive, consecutive);
                                                prefix_idx++;
                                            } else {
                                                consecutive = 0;
                                            }
                                        }
                                        score = 1000 + getTypePriority(item.kind) * 100;
                                        score += max_consecutive * 10;
                                        size_t first_match = lower.find(lower_prefix[0]);
                                        if (first_match != std::string::npos) {
                                            score += (100 - static_cast<int>(first_match));
                                        }
                                        contain_count++;
                                    } else {
                                        // 详细日志：记录模糊匹配失败的原因
                                        if (processed_count <= 3) {
                                            LOG("[COMPLETION] [MATCH] Fuzzy match failed: prefix_idx=" + 
                                                std::to_string(prefix_idx) + "/" + std::to_string(lower_prefix_len) + 
                                                ", label=\"" + label + "\", lower=\"" + lower + "\"");
                                        }
                            continue;  // 跳过不匹配的项
                                    }
                                } else if (score == 0) {
                                    // 详细日志：记录为什么没有匹配（仅前几个项）
                                    if (processed_count <= 3 && prefix_len < 3) {
                                        LOG("[COMPLETION] [MATCH] No match: prefix too short (" + 
                                            std::to_string(prefix_len) + " < 3), label=\"" + label + "\"");
                                    }
                                    continue;  // 跳过不匹配的项
                                }
                            }
                        }
                    }
                    
                    if (score > 0) {
                        matched_count++;
                        // 详细日志：只记录前几个匹配的项
                        if (matched_count <= 5) {
                            LOG("[COMPLETION] [MATCH] Matched item[" + std::to_string(processed_count - 1) + 
                                "]: original_label=\"" + item.label + "\", trimmed_label=\"" + label + 
                                "\", score=" + std::to_string(score));
                        }
                    } else {
                        // 详细日志：记录前几个未匹配的项（用于调试）
                        if (processed_count <= 5) {
                            std::string normalized = normalizeForMatching(label);
                            std::string lower = label;
                            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                            LOG("[COMPLETION] [MATCH] Unmatched item[" + std::to_string(processed_count - 1) + 
                                "]: original_label=\"" + item.label + "\", trimmed_label=\"" + label + 
                                "\", normalized=\"" + normalized + "\", prefix=\"" + prefix + 
                                "\", normalized_prefix=\"" + normalized_prefix + "\"");
                        }
                    }
                    
                    scored_items.push_back({item, score});
                }
                
                LOG("[COMPLETION] Processed " + std::to_string(processed_count) + " items, matched " + 
                    std::to_string(matched_count) + " items");
                LOG("[COMPLETION] Scoring: exact=" + std::to_string(exact_count) + 
                    ", prefix=" + std::to_string(prefix_count) + 
                    ", contain=" + std::to_string(contain_count) + 
                    ", other=" + std::to_string(other_count) + 
                    ", total=" + std::to_string(scored_items.size()));
                
                // 调试：输出匹配失败的详细信息
                if (scored_items.empty() && !items.empty()) {
                    LOG("[COMPLETION] [DEBUG] No items matched prefix \"" + prefix + "\"");
                    LOG("[COMPLETION] [DEBUG] Normalized prefix: \"" + normalized_prefix + "\"");
                    if (items.size() <= 10) {
                        std::string debug_info = "[DEBUG] LSP items details: ";
                        for (size_t i = 0; i < items.size(); i++) {
                            if (i > 0) debug_info += "; ";
                            std::string norm_label = normalizeForMatching(items[i].label);
                            debug_info += "label=\"" + items[i].label + "\", normalized=\"" + 
                                         norm_label + "\", kind=" + items[i].kind;
                        }
                        LOG("[COMPLETION] " + debug_info);
                    } else {
                        LOG("[COMPLETION] [DEBUG] First 5 items: ");
                        for (size_t i = 0; i < 5; i++) {
                            std::string norm_label = normalizeForMatching(items[i].label);
                            LOG("[COMPLETION] [DEBUG]   [" + std::to_string(i) + "] label=\"" + 
                                items[i].label + "\", normalized=\"" + norm_label + "\"");
                        }
                    }
                }
                
                // 按分数排序
                auto sort_start = std::chrono::steady_clock::now();
                std::sort(scored_items.begin(), scored_items.end());
                auto sort_end = std::chrono::steady_clock::now();
                auto sort_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    sort_end - sort_start);
                LOG("[COMPLETION] Sorted " + std::to_string(scored_items.size()) + 
                    " items (took " + std::to_string(sort_time.count() / 1000.0) + " ms)");
                
                // 提取排序后的 items
                std::vector<features::CompletionItem> filtered_items;
                filtered_items.reserve(scored_items.size());
                for (const auto& scored : scored_items) {
                    filtered_items.push_back(scored.item);
                }
                
                // 限制显示数量（VSCode 默认显示 12-15 个，我们显示最多 30 个）
                size_t original_size = filtered_items.size();
                if (filtered_items.size() > 30) {
                    filtered_items.resize(30);
                    LOG("[COMPLETION] Limited items from " + std::to_string(original_size) + " to 30");
                }
                
                auto filter_end = std::chrono::steady_clock::now();
                auto filter_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    filter_end - filter_start);
                LOG("[COMPLETION] Filtering and sorting (took " + 
                    std::to_string(filter_time.count() / 1000.0) + " ms)");
                
                // 只有在以下情况才更新显示：
                // 1. 之前没有显示缓存结果（需要显示LSP结果）
                // 2. 或者LSP结果与缓存结果数量差异较大（>50%差异）
                bool should_update_display = !has_shown_cache;
                if (has_shown_cache && !cached_fallback_results.empty()) {
                    size_t cache_count = cached_fallback_results.size();
                    size_t lsp_count = filtered_items.size();
                    double ratio = cache_count > 0 ? static_cast<double>(lsp_count) / cache_count : 0.0;
                    // 如果LSP结果比缓存结果多50%以上或者少30%以上，则更新显示
                    if (ratio > 1.5 || ratio < 0.7) {
                        should_update_display = true;
                        LOG("[COMPLETION] LSP results significantly different from cache (" +
                            std::to_string(lsp_count) + " vs " + std::to_string(cache_count) +
                            "), updating display");
                    } else {
                        LOG("[COMPLETION] LSP results similar to cache (" +
                            std::to_string(lsp_count) + " vs " + std::to_string(cache_count) +
                            "), keeping cache display to avoid flicker");
                    }
                }

                if (should_update_display) {
                    auto show_start = std::chrono::steady_clock::now();
                    int screen_width = screen_.dimx();
                    int screen_height = screen_.dimy();
                    completion_popup_.show(filtered_items, cursor_row_, cursor_col_, screen_width, screen_height);
                    auto show_end = std::chrono::steady_clock::now();
                    auto show_time = std::chrono::duration_cast<std::chrono::microseconds>(
                        show_end - show_start);
                    LOG("[COMPLETION] Showed popup with " + std::to_string(filtered_items.size()) +
                        " items (took " + std::to_string(show_time.count() / 1000.0) + " ms)");
                } else {
                    LOG("[COMPLETION] Keeping existing cache display to avoid flicker");
                }
                
                auto callback_end = std::chrono::steady_clock::now();
                auto callback_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    callback_end - callback_start);
                auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    callback_end - trigger_start);
                LOG("[COMPLETION] ===== Async callback END (callback took " + 
                    std::to_string(callback_time.count()) + " ms, total " + 
                    std::to_string(total_time.count()) + " ms) =====");
            } else {
                LOG("[COMPLETION] No items received, hiding popup");
                // 只在 popup 可见时才 hide，避免不必要的操作
                if (completion_popup_.isVisible()) {
                    completion_popup_.hide();
                }
            }
        },
        [this, trigger_start, cached_fallback_results](const std::string& error) {
            auto error_time = std::chrono::steady_clock::now();
            auto request_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                error_time - trigger_start);
            LOG_ERROR("[COMPLETION] Request failed after " + std::to_string(request_time.count()) +
                     " ms: " + error);

            // 如果有缓存结果作为回退，显示缓存结果
            if (!cached_fallback_results.empty()) {
                LOG("[COMPLETION] [CACHE] Using cached fallback results: " +
                    std::to_string(cached_fallback_results.size()) + " items");
                auto show_start = std::chrono::steady_clock::now();
                int screen_width = screen_.dimx();
                int screen_height = screen_.dimy();
                completion_popup_.show(cached_fallback_results, cursor_row_, cursor_col_, screen_width, screen_height);
                auto show_end = std::chrono::steady_clock::now();
                auto show_time = std::chrono::duration_cast<std::chrono::microseconds>(show_end - show_start);
                LOG("[COMPLETION] Showed cached fallback popup (took " +
                    std::to_string(show_time.count() / 1000.0) + " ms)");
            } else {
                LOG("[COMPLETION] No cached fallback available, hiding popup");
                completion_popup_.hide();
            }
        }
    );
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
    if (text.empty()) {
        return;
    }
    
    // 获取当前行的光标位置之前的文本
    Document* doc = getCurrentDocument();
    if (!doc) return;
    
    const std::string& line = doc->getLine(cursor_row_);
    if (cursor_col_ > line.length()) {
        cursor_col_ = line.length();
    }
    
    std::string before_cursor = line.substr(0, cursor_col_);
    
    // 找到要替换的前缀（从最后一个单词开始）
    size_t prefix_start = before_cursor.length();
    while (prefix_start > 0 && 
           (std::isalnum(before_cursor[prefix_start - 1]) || 
            before_cursor[prefix_start - 1] == '_' ||
            before_cursor[prefix_start - 1] == '.')) {
        prefix_start--;
    }
    
    std::string prefix = before_cursor.substr(prefix_start);
    
    // 删除前缀
    size_t prefix_len = prefix.length();
    for (size_t i = 0; i < prefix_len; ++i) {
        if (cursor_col_ > 0) {
            cursor_col_--;
            doc->deleteChar(cursor_row_, cursor_col_);
        }
    }
    
    // 插入补全文本
    doc->insertText(cursor_row_, cursor_col_, text);
    cursor_col_ += text.length();
    
    // 隐藏补全弹窗
    completion_popup_.hide();
    
    // 更新 LSP 文档
    updateLspDocument();
}

ftxui::Element Editor::renderCompletionPopup() {
    if (!completion_popup_.isVisible()) {
        return ftxui::text("");
    }
    
    // 更新光标位置（用于重新计算弹窗位置）
    completion_popup_.updateCursorPosition(
        cursor_row_, cursor_col_,
        screen_.dimx(), screen_.dimy()
    );
    
    // 渲染补全弹窗（固定尺寸以避免抖动）
    return completion_popup_.render(theme_);
}

#endif // BUILD_LSP_SUPPORT

} // namespace core
} // namespace pnana

