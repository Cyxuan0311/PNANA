// LSP 集成相关实现
#include "core/editor.h"
#include "features/lsp/lsp_request_manager.h"
#include "features/lsp/lsp_server_manager.h"
#include "features/lsp/lsp_types.h"
#include "features/lsp/lsp_worker_pool.h"
#include "ui/icons.h"
#include "utils/clipboard.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <climits>
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

// LSP 补全上下文分析辅助函数
std::string Editor::getSemanticContext(const std::string& line_content, size_t cursor_pos) {
    // 简单的语义上下文分析
    // 检查光标前面的内容，判断是在函数内、类内等

    if (cursor_pos == 0) {
        return "global";
    }

    // 查找最近的函数或类定义
    std::string before_cursor = line_content.substr(0, cursor_pos);
    std::reverse(before_cursor.begin(), before_cursor.end());

    // 检查是否在函数内（查找最近的'('）
    size_t paren_pos = before_cursor.find('(');
    if (paren_pos != std::string::npos && paren_pos < 100) { // 在最近100个字符内
        return "function";
    }

    // 检查是否在类内（查找最近的'class'或'struct'）
    size_t class_pos = before_cursor.find("ssalc");   // "class" reversed
    size_t struct_pos = before_cursor.find("tcurts"); // "struct" reversed
    if (class_pos != std::string::npos || struct_pos != std::string::npos) {
        return "class";
    }

    return "global";
}

std::string Editor::getTriggerCharacter(const std::string& line_content, size_t cursor_pos) {
    // 分析触发字符
    if (cursor_pos == 0) {
        return "";
    }

    // 检查光标前一个字符
    char prev_char = line_content[cursor_pos - 1];

    // LSP 常见的触发字符
    if (prev_char == '.' || prev_char == ':' || prev_char == '>' || prev_char == '/' ||
        prev_char == '\\') {
        return std::string(1, prev_char);
    }

    // 检查双字符触发符
    if (cursor_pos >= 2) {
        std::string prev_two = line_content.substr(cursor_pos - 2, 2);
        if (prev_two == "::" || prev_two == "->" || prev_two == "?.") {
            return prev_two;
        }
    }

    return "";
}

void Editor::initializeLsp() {
    // 创建 LSP 服务器管理器
    lsp_manager_ = std::make_unique<features::LspServerManager>();

    // 初始化诊断弹窗状态
    show_diagnostics_popup_ = false;
#ifdef BUILD_LSP_SUPPORT
    show_symbol_navigation_popup_ = false;
#endif

    // 设置诊断回调（应用到所有 LSP 客户端）
    lsp_manager_->setDiagnosticsCallback(
        [this](const std::string& uri, const std::vector<features::Diagnostic>& diagnostics) {
            // LOG("[LSP_DIAGNOSTICS_CALLBACK] ===== RECEIVED DIAGNOSTICS =====");
            // LOG("[LSP_DIAGNOSTICS_CALLBACK] URI: " + uri +
            //     ", count=" + std::to_string(diagnostics.size()));

            // 打印前几个诊断的详细信息（用于调试）
            for (size_t i = 0; i < std::min(diagnostics.size(), size_t(3)); ++i) {
                // Diagnostic variable intentionally unused when logging is disabled
                // LOG("[LSP_DIAGNOSTICS_CALLBACK] Diagnostic " + std::to_string(i) +
                //    ": line=" + std::to_string(diagnostics[i].range.start.line) +
                //    ", severity=" + std::to_string(diagnostics[i].severity) + ", message='" +
                //    diagnostics[i].message.substr(0, 50) + "'");
            }

            // 更新当前文件的诊断信息（内存更新 + 缓存）
            bool is_current_file = false;
            {
                std::lock_guard<std::mutex> lock(diagnostics_mutex_);
                std::lock_guard<std::mutex> cache_lock(diagnostics_cache_mutex_);

                // 更新缓存
                diagnostics_cache_[uri] = diagnostics;
                // LOG("[LSP_DIAGNOSTICS_CALLBACK] Updated cache for " + uri + " with " +
                //     std::to_string(diagnostics.size()) + " diagnostics");

                auto current_doc = getCurrentDocument();
                std::string current_uri =
                    current_doc ? filepathToUri(current_doc->getFilePath()) : "";
                // LOG("[LSP_DIAGNOSTICS_CALLBACK] Current document URI: " + current_uri);

                if (current_doc && uri == current_uri) {
                    current_file_diagnostics_ = diagnostics;
                    is_current_file = true;
                    // LOG("[LSP_DIAGNOSTICS_CALLBACK] Updated current file diagnostics: " +
                    //     std::to_string(current_file_diagnostics_.size()));

                    // 对于当前文件，立即触发UI重新渲染以显示诊断信息
                    force_ui_update_ = true;
                    last_render_source_ = "lsp_diagnostics_callback";
                    // LOG("[LSP_DIAGNOSTICS_CALLBACK] Set force_ui_update for current file "
                    //     "diagnostics display");
                } else {
                    // LOG("[LSP_DIAGNOSTICS_CALLBACK] Not current file, only updated cache");
                }
            }

            // 对于当前文件，立即更新状态栏并强制UI重新渲染
            if (is_current_file) {
                updateDiagnosticsStatus(diagnostics);
                // 立即触发UI重新渲染以显示诊断信息
                force_ui_update_ = true;
                last_render_source_ = "lsp_diagnostics_callback";
                // 注意：不在回调线程中直接调用screen_.PostEvent，这可能导致线程安全问题
                // 改为在下一个UI渲染周期中处理
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

    // 初始化代码片段管理器
    snippet_manager_ = std::make_unique<features::SnippetManager>();

    // 初始化折叠管理器（暂时为空的shared_ptr，后续在文件打开时设置）
    folding_manager_ = std::make_unique<features::FoldingManager>(nullptr);

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

        // LOG("Migrated LSP cache files to: " + config_cache_dir);
    } catch (const std::exception& e) {
        // LOG_WARNING("Failed to migrate cache files: " + std::string(e.what()));
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

    // LOG("[LSP DEBUG] Detecting language for file: " + filepath + ", extension: '" + ext + "'");

    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".hpp" || ext == ".hxx" ||
        ext == ".h" || ext == ".c") {
        return "cpp";
    } else if (ext == ".py") {
        // LOG("[LSP DEBUG] Detected Python file, returning language_id: python");
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
    // 检查 URI 缓存
    {
        std::lock_guard<std::mutex> lock(uri_cache_mutex_);
        auto it = uri_cache_.find(filepath);
        if (it != uri_cache_.end()) {
            return it->second;
        }
    }

    // 转换为 file:// URI
    // 注意：对于包含非 ASCII 字符（如中文）的路径，需要正确进行 UTF-8 URL 编码
    std::string uri = "file://";

    try {
        // 使用 try-catch 保护，避免路径处理失败导致卡住
        std::string path;
        try {
            path = fs::absolute(filepath).string();
        } catch (const std::exception& e) {
            path = filepath;
        } catch (...) {
            path = filepath;
        }

        std::replace(path.begin(), path.end(), '\\', '/');

        // URL 编码（正确处理 UTF-8 多字节字符）
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
    } catch (const std::exception& e) {
        std::string path = filepath;
        std::replace(path.begin(), path.end(), '\\', '/');
        for (unsigned char c : path) {
            if (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == ':') {
                uri += static_cast<char>(c);
            } else {
                char hex[4];
                snprintf(hex, sizeof(hex), "%%%02X", c);
                uri += hex;
            }
        }
    } catch (...) {
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
    return uri;
}

void Editor::updateCurrentFileDiagnostics() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        std::lock_guard<std::mutex> lock(diagnostics_mutex_);
        current_file_diagnostics_.clear();
        // 在文档切换期间不强制UI更新，使用needs_render_避免抖动
        needs_render_ = true;
        last_render_source_ = "diagnostic_clear";
        return;
    }

    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        std::lock_guard<std::mutex> lock(diagnostics_mutex_);
        current_file_diagnostics_.clear();
        // 在文档切换期间不强制UI更新，使用needs_render_避免抖动
        needs_render_ = true;
        last_render_source_ = "diagnostic_clear";
        return;
    }

    std::string uri = filepathToUri(filepath);

    std::lock_guard<std::mutex> cache_lock(diagnostics_cache_mutex_);
    auto it = diagnostics_cache_.find(uri);
    if (it != diagnostics_cache_.end()) {
        std::lock_guard<std::mutex> lock(diagnostics_mutex_);
        current_file_diagnostics_ = it->second;
    } else {
        // 不清空 current_file_diagnostics_，让它保持之前的状态
        // LSP 回调到来时会更新为正确的诊断信息
    }

    needs_render_ = true;
    last_render_source_ = "diagnostic_update";
}

void Editor::preloadAdjacentDocuments(size_t current_index) {
    // 预加载相邻文档的诊断和折叠数据，提升后续切换响应速度
    auto tabs = document_manager_.getAllTabs();
    if (tabs.empty()) {
        return;
    }

    // 只预加载最近的2个文档，避免过度消耗资源
    const size_t PRELOAD_COUNT = 2;
    std::vector<size_t> indices_to_preload;

    // 收集需要预加载的文档索引（当前文档的前后各1个）
    if (current_index > 0) {
        indices_to_preload.push_back(current_index - 1);
    }
    if (current_index + 1 < tabs.size()) {
        indices_to_preload.push_back(current_index + 1);
    }

    // 限制预加载数量
    if (indices_to_preload.size() > PRELOAD_COUNT) {
        indices_to_preload.resize(PRELOAD_COUNT);
    }

    for (size_t idx : indices_to_preload) {
        if (idx >= tabs.size())
            continue;

        const auto& tab = tabs[idx];
        if (tab.filepath.empty())
            continue; // 跳过未保存的文件

        std::string uri = filepathToUri(tab.filepath);

        // 检查诊断缓存，如果没有则异步请求
        {
            std::lock_guard<std::mutex> cache_lock(diagnostics_cache_mutex_);
            if (diagnostics_cache_.find(uri) == diagnostics_cache_.end()) {
                // 诊断数据只能通过LSP回调获得，这里不主动触发
            }
        }

        // 检查折叠缓存，如果没有则异步初始化
        {
            std::lock_guard<std::mutex> cache_lock(folding_cache_mutex_);
            if (folding_cache_.find(uri) == folding_cache_.end()) {
                if (lsp_request_manager_) {
                    std::string fold_key = std::string("preload:fold:") + uri;
                    lsp_request_manager_->postOrReplace(
                        fold_key, features::LspRequestManager::Priority::LOW, [this, uri]() {
                            try {
                                if (folding_manager_) {
                                    folding_manager_->initializeFoldingRanges(uri);
                                }
                            } catch (...) {
                            }
                        });
                }
            }
        }
    }
}

void Editor::cleanupExpiredCaches() {
    // 定期清理过期的缓存，避免内存泄漏
    auto now = std::chrono::steady_clock::now();

    // 清理诊断缓存
    {
        std::lock_guard<std::mutex> lock(diagnostics_cache_mutex_);
        for (auto it = diagnostics_cache_.begin(); it != diagnostics_cache_.end();) {
            // 诊断缓存不过期，因为它们相对稳定且有用
            // 但如果缓存过多，可以清理最老的条目
            if (diagnostics_cache_.size() > 100) { // 最多缓存100个文件的诊断信息
                it = diagnostics_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // 清理折叠缓存
    {
        std::lock_guard<std::mutex> lock(folding_cache_mutex_);
        for (auto it = folding_cache_.begin(); it != folding_cache_.end();) {
            auto age = now - it->second.timestamp;
            if (age > FOLDING_CACHE_DURATION ||
                folding_cache_.size() > 50) { // 最多缓存50个文件的折叠状态
                it = folding_cache_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void Editor::updateCurrentFileFolding() {
    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        return;
    }

    std::string uri = filepathToUri(filepath);

    // 首先尝试从缓存恢复折叠状态（更积极的策略）
    bool cache_restored = false;
    {
        std::lock_guard<std::mutex> cache_lock(folding_cache_mutex_);

        auto cache_it = folding_cache_.find(uri);
        if (cache_it != folding_cache_.end()) {
            auto now = std::chrono::steady_clock::now();
            auto age = now - cache_it->second.timestamp;

            auto extended_duration = std::chrono::minutes(60);

            if (age <= extended_duration) {
                doc->setFoldingRanges(cache_it->second.ranges);
                doc->unfoldAll();
                for (int line : cache_it->second.folded_lines) {
                    doc->setFolded(line, true);
                }

                cache_restored = true;

                if (folding_manager_) {
                    // 直接设置状态而不调用clear()，避免触发同步回调
                    folding_manager_->setFoldingRangesDirectly(cache_it->second.ranges);
                    folding_manager_->setFoldedLinesDirectly(cache_it->second.folded_lines);
                }
            } else {
                folding_cache_.erase(cache_it);
            }
        }
    }

    if (!folding_manager_) {
        if (cache_restored) {
            needs_render_ = true;
            last_render_source_ = "folding_cache_restored";
        }
        return;
    }

    if (!folding_manager_->isInitialized()) {
        if (lsp_request_manager_) {
            std::string fold_key = std::string("fold:switch:") + uri;
            lsp_request_manager_->postOrReplace(
                fold_key, features::LspRequestManager::Priority::LOW, [this, uri]() {
                    try {
                        if (folding_manager_) {
                            folding_manager_->initializeFoldingRanges(uri);
                            needs_render_ = true;
                            last_render_source_ = "async_folding_init";
                        }
                    } catch (...) {
                    }
                });
        } else {
            std::thread([this, uri]() {
                try {
                    if (folding_manager_) {
                        folding_manager_->initializeFoldingRanges(uri);
                        needs_render_ = true;
                        last_render_source_ = "async_folding_init_fallback";
                    }
                } catch (...) {
                }
            }).detach();
        }
    }

    needs_render_ = true;
    last_render_source_ = "folding_update";
}

void Editor::updateLspDocument() {
    if (!lsp_enabled_ || !lsp_manager_) {
        return;
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    size_t doc_size = doc->lineCount();
    if (doc_size > 1000) {
        return;
    }

    std::string filepath = doc->getFilePath();
    if (filepath.empty()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(document_update_mutex_);
        auto time_since_last_update =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_document_update_time_);

        if (time_since_last_update < document_update_debounce_interval_) {
            return;
        }

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

        features::LspClient* client = lsp_manager_->getClientForFile(filepath);

        if (!client) {
            return;
        }

        bool is_connected = client->isConnected();

        if (!is_connected) {
            try {
                std::thread([client]() {
                    try {
                        std::string root_path = fs::current_path().string();
                        client->initialize(root_path);
                    } catch (...) {
                    }
                }).detach();
            } catch (...) {
            }

            // 跳过当前文档更新，等待后台初始化完成
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
            // 首次打开，发送 didOpen（同步发送以确保文档被正确添加）
            try {
                client->didOpen(uri, language_id, content);

                // 初始化折叠管理器
                folding_manager_ = std::make_unique<features::FoldingManager>(
                    std::shared_ptr<features::LspClient>(client, [](features::LspClient*) {}));

                // 设置折叠状态变化回调
                folding_manager_->setFoldingStateChangedCallback([this]() {
                    needs_render_ = true;
                    last_render_source_ = "folding_state_changed";
                });

                folding_manager_->setDocumentSyncCallback(
                    [this, uri](const auto& ranges, const auto& folded) {
                        if (auto doc = getCurrentDocument()) {
                            // Update folding ranges
                            doc->setFoldingRanges(ranges);

                            // Reset folded state and then apply new folded set so that
                            // previously folded lines that are no longer folded get cleared.
                            doc->unfoldAll();

                            for (int line : folded) {
                                doc->setFolded(line, true);
                            }

                            {
                                std::unique_lock<std::mutex> cache_lock(folding_cache_mutex_,
                                                                        std::try_to_lock);
                                if (cache_lock.owns_lock()) {
                                    folding_cache_[uri] = {ranges, folded,
                                                           std::chrono::steady_clock::now()};
                                }
                            }

                            needs_render_ = true;
                            last_render_source_ = "folding_sync";
                        }
                    });

                // 异步初始化折叠范围，不阻塞文件打开
                // 使用更高的优先级，确保快速响应
                if (lsp_request_manager_) {
                    std::string fold_key = std::string("fold:init:") + uri;
                    lsp_request_manager_->postOrReplace(
                        fold_key, features::LspRequestManager::Priority::HIGH, [this, uri]() {
                            try {
                                if (folding_manager_) {
                                    folding_manager_->initializeFoldingRanges(uri);
                                    needs_render_ = true;
                                    last_render_source_ = "folding_async_init";
                                }
                            } catch (...) {
                            }
                        });
                } else {
                    // Fallback: spawn background thread
                    std::thread([this, uri]() {
                        try {
                            if (folding_manager_) {
                                folding_manager_->initializeFoldingRanges(uri);
                                needs_render_ = true;
                                last_render_source_ = "folding_async_init_fallback";
                            }
                        } catch (...) {
                        }
                    }).detach();
                }

            } catch (...) {
            }
            file_language_map_[uri] = language_id;
        } else {
            int version = pending_document_version_ > 0 ? pending_document_version_ : 2;
            pending_document_version_ = version + 1;

            try {
                client->didChange(uri, content, version);
                // Schedule folding ranges refresh for this document (debounced by request manager).
                if (lsp_request_manager_) {
                    std::string fold_key = std::string("fold:") + uri;
                    lsp_request_manager_->postOrReplace(
                        fold_key, features::LspRequestManager::Priority::LOW, [this, uri]() {
                            try {
                                if (folding_manager_) {
                                    folding_manager_->initializeFoldingRanges(uri);
                                }
                            } catch (...) {
                            }
                        });
                } else {
                    // Fallback: spawn background thread to refresh folding ranges (no debouncing)
                    try {
                        std::thread([this, uri]() {
                            try {
                                if (folding_manager_) {
                                    folding_manager_->initializeFoldingRanges(uri);
                                }
                            } catch (...) {
                            }
                        }).detach();
                    } catch (...) {
                    }
                }
            } catch (...) {
            }
        }

        // 注意：不清除补全缓存，因为：
        // 1. 缓存键包含精确的位置信息，位置变化时自然无法命中
        // 2. 缓存有过期时间（5分钟），会自动清理过期项
        // 3. 频繁清空缓存会导致缓存命中率为0，影响性能
        // 只在文档关闭或明确需要时才清空缓存
    } catch (...) {
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

    if (!lsp_enabled_ || !lsp_manager_) {
        return;
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        return;
    }

    std::string filepath = doc->getFilePath();

    // 优化的防抖机制（参考VSCode：平衡响应速度和性能）
    auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(completion_debounce_mutex_);
        auto time_since_last_trigger = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_completion_trigger_time_);

        if (time_since_last_trigger < std::chrono::milliseconds(50)) {
            return;
        }

        last_completion_trigger_time_ = now;
    }

    if (filepath.empty()) {
        // 如果文件未保存，使用临时路径
        filepath = "/tmp/pnana_unsaved_" + std::to_string(reinterpret_cast<uintptr_t>(doc));
    }

    features::LspClient* client = lsp_manager_->getClientForFile(filepath);
    if (!client) {
        completion_popup_.hide();
        return;
    }

    if (!client->isConnected()) {
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

    std::string uri = filepathToUri(filepath);

    features::LspPosition pos(static_cast<int>(cursor_row_), static_cast<int>(cursor_col_));

    // 获取当前行的光标位置之前的文本，用于过滤和排序
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
    // 初始化补全缓存
    if (!completion_cache_) {
        completion_cache_ = std::make_unique<features::LspCompletionCache>();
    }

    // 改进的缓存策略 - 基于前缀和语言ID，提高命中率
    features::LspCompletionCache::CacheKey cache_key;
    cache_key.uri = uri;

    // 获取当前行内容和前缀
    std::string line_content = doc->getLine(cursor_row_);
    std::string current_prefix = line_content.substr(0, cursor_col_);

    // 找到最近的单词边界作为缓存key，避免位置依赖
    size_t last_space = current_prefix.find_last_of(" \t.()[]{};:,");
    if (last_space != std::string::npos && last_space < current_prefix.size() - 1) {
        cache_key.context_prefix = current_prefix.substr(last_space + 1);
    } else {
        cache_key.context_prefix = current_prefix;
    }

    // 获取语言ID用于缓存区分
    std::string language_id = detectLanguageId(filepath);
    cache_key.semantic_context = language_id;

    // 简化其他字段，减少缓存粒度
    cache_key.line = 0;      // 不基于行号
    cache_key.character = 0; // 不基于列号
    cache_key.trigger_character = "";
    cache_key.prefix = "";

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

    auto cached = completion_cache_->get(cache_key);

    if (cached.has_value() && !cached->empty()) {
        // 限制显示数量
        std::vector<features::CompletionItem> limited_items = *cached;
        if (limited_items.size() > 50) {
            limited_items.resize(50);
        }
        showCompletionPopupIfChanged(limited_items, static_cast<int>(cursor_row_),
                                     cursor_screen_col, screen_width, screen_height, prefix);
        return;
    }

    if (!lsp_async_manager_) {
        lsp_async_manager_ = std::make_unique<features::LspAsyncManager>();
    }

    int req_row = static_cast<int>(cursor_row_);
    int req_col = cursor_screen_col;
    int req_screen_w = screen_.dimx();
    int req_screen_h = screen_.dimy();

    lsp_async_manager_->requestCompletionAsync(
        client, uri, pos,
        // on_success - 在主线程中更新UI
        [this, cache_key, req_row, req_col, req_screen_w, req_screen_h, prefix,
         filepath](const std::vector<features::CompletionItem>& items) {
            screen_.Post([this, items, cache_key, req_row, req_col, req_screen_w, req_screen_h,
                          prefix, filepath]() {
                if (completion_cache_ && !items.empty()) {
                    completion_cache_->set(cache_key, items);
                }

                if (!items.empty()) {
                    std::vector<features::CompletionItem> all_items = items;

                    // 添加代码片段到补全列表
                    if (snippet_manager_) {
                        std::string language_id = detectLanguageId(filepath);
                        auto snippets = snippet_manager_->findMatchingSnippets(prefix, language_id);

                        for (const auto& snippet : snippets) {
                            features::CompletionItem snippet_item;
                            snippet_item.label = snippet.prefix;
                            snippet_item.kind = "snippet";
                            snippet_item.detail = snippet.description;
                            snippet_item.documentation = "Code snippet: " + snippet.description;
                            snippet_item.isSnippet = true;
                            snippet_item.snippet_body = snippet.body;
                            snippet_item.snippet_placeholders = snippet.placeholders;

                            all_items.push_back(snippet_item);
                        }
                    }

                    std::vector<features::CompletionItem> sorted_items = all_items;

                    std::sort(
                        sorted_items.begin(), sorted_items.end(),
                        [this, prefix = prefix](const features::CompletionItem& a,
                                                const features::CompletionItem& b) {
                            // 计算评分：相关性、使用频率、上下文匹配、类型优先级、位置接近度
                            auto calculate_score =
                                [prefix](const features::CompletionItem& item) -> int {
                                int score = 0;

                                // 1. 前缀匹配评分 (最高权重)
                                if (!prefix.empty()) {
                                    if (item.label.find(prefix) == 0) {
                                        score += 100; // 完全匹配前缀
                                    } else if (item.label.find(prefix) != std::string::npos) {
                                        score += 50; // 包含前缀
                                    }
                                }

                                // 2. 类型优先级评分
                                if (item.kind == "method" || item.kind == "function") {
                                    score += 30;
                                } else if (item.kind == "variable" || item.kind == "property") {
                                    score += 20;
                                } else if (item.kind == "class" || item.kind == "interface") {
                                    score += 40;
                                }

                                // 3. 长度评分（较短的通常更常用）
                                if (item.label.length() <= 10) {
                                    score += 10;
                                } else if (item.label.length() <= 20) {
                                    score += 5;
                                }

                                return score;
                            };

                            int score_a = calculate_score(a);
                            int score_b = calculate_score(b);

                            if (score_a != score_b) {
                                return score_a > score_b; // 分数高的在前
                            }

                            return a.label < b.label; // 相同分数按字母顺序
                        });
                    std::vector<features::CompletionItem> limited = sorted_items;
                    if (limited.size() > 50) {
                        limited.resize(50);
                    }

                    showCompletionPopupIfChanged(limited, req_row, req_col, req_screen_w,
                                                 req_screen_h, prefix);
                } else {
                    completion_popup_.hide();
                }
            });
        },
        // on_error - 隐藏弹窗
        [this](const std::string&) {
            screen_.Post([this]() {
                completion_popup_.hide();
            });
        });
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

    // 检查是否是代码片段
    const auto* selected_item = completion_popup_.getSelectedItem();
    if (selected_item && selected_item->isSnippet && snippet_manager_) {
        pnana::core::Document* doc = getCurrentDocument();
        if (!doc) {
            completion_popup_.hide();
            return;
        }

        // 像普通补全一样：先替换/删除触发代码片段的输入（通常是当前单词前缀）
        const std::string& line = doc->getLine(cursor_row_);
        if (cursor_col_ > line.length()) {
            cursor_col_ = line.length();
        }
        size_t word_start = cursor_col_;
        while (word_start > 0) {
            char ch = line[word_start - 1];
            if (!std::isalnum(ch) && ch != '_') {
                break;
            }
            word_start--;
        }
        if (word_start < cursor_col_) {
            // 删除 [word_start, cursor_col_) 这段触发文本，然后把光标移到 word_start
            doc->deleteRange(cursor_row_, word_start, cursor_row_, cursor_col_);
            cursor_col_ = word_start;
        }

        // 展开代码片段
        features::Snippet snippet;
        snippet.prefix = selected_item->label;
        snippet.body = selected_item->snippet_body;
        snippet.description = selected_item->detail;
        snippet.placeholders = selected_item->snippet_placeholders;

        snippet_manager_->expandSnippet(snippet, *this);
        completion_popup_.hide();

        // 同步 LSP 文档状态
        updateLspDocument();

        // 立刻刷新“格式/高亮”效果：
        // SyntaxHighlighter 有多行状态，只在 openFile/setFileType 时重置；
        // snippet 一次性插入多行会让状态滞后，导致需要重开文件才恢复。
        syntax_highlighter_.resetMultiLineState();
        needs_render_ = true;
        last_render_source_ = "snippet_insert";
        // 触发一次 UI render（避免等下一次输入事件）
        screen_.PostEvent(ftxui::Event::Custom);
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

void Editor::startSnippetSession(std::vector<SnippetPlaceholderRange> ranges) {
    snippet_placeholder_ranges_ = std::move(ranges);
    snippet_placeholder_index_ = 0;
    snippet_session_active_ = !snippet_placeholder_ranges_.empty();
    if (snippet_session_active_) {
        handleSnippetTabJump(); // jump to first placeholder
    }
}

void Editor::endSnippetSession() {
    snippet_session_active_ = false;
    snippet_placeholder_ranges_.clear();
    snippet_placeholder_index_ = 0;
    // keep user's selection state clean
    if (selection_active_) {
        endSelection();
    }
}

bool Editor::handleSnippetTabJump() {
    if (!snippet_session_active_ || snippet_placeholder_ranges_.empty()) {
        return false;
    }

    // If cursor moved outside current placeholder and user edits freely, stop session.
    if (snippet_placeholder_index_ >= snippet_placeholder_ranges_.size()) {
        endSnippetSession();
        return false;
    }

    const auto& r = snippet_placeholder_ranges_[snippet_placeholder_index_];
    snippet_placeholder_index_++;

    // If placeholder range is not valid anymore (user edited with newlines etc.), stop.
    Document* doc = getCurrentDocument();
    if (!doc || r.row >= doc->lineCount()) {
        endSnippetSession();
        return false;
    }
    const std::string& line = doc->getLine(r.row);
    if (r.col > line.size()) {
        endSnippetSession();
        return false;
    }

    // Select placeholder text if len>0, otherwise just move cursor.
    cursor_row_ = r.row;
    cursor_col_ = r.col;
    if (r.len > 0 && r.col + r.len <= line.size()) {
        selection_active_ = true;
        selection_start_row_ = r.row;
        selection_start_col_ = r.col;
        cursor_col_ = r.col + r.len;
    } else {
        if (selection_active_) {
            endSelection();
        }
    }

    adjustCursor();
    adjustViewOffset();
    return true;
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
                                          int row, int col, int screen_w, int screen_h,
                                          const std::string& query) {
    auto now = std::chrono::steady_clock::now();
    int count = static_cast<int>(items.size());

    // 如果位置和数量与上次相同且在短时间内重复请求，则跳过显示（防止抖动）
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_popup_shown_time_);
    if (completion_popup_.isVisible() && last_popup_shown_count_ == count &&
        last_popup_row_ == row && last_popup_col_ == col && elapsed.count() < 50) {
        return;
    }

    // 更新状态并显示
    last_popup_shown_time_ = now;
    last_popup_shown_count_ = count;
    last_popup_row_ = row;
    last_popup_col_ = col;

    completion_popup_.show(items, static_cast<size_t>(row), static_cast<size_t>(col), screen_w,
                           screen_h, query);
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

#ifdef BUILD_LSP_SUPPORT
void Editor::showSymbolNavigation() {
    if (!lsp_enabled_) {
        setStatusMessage("LSP is not enabled. Cannot show symbol navigation.");
        return;
    }

    const Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No document open. Cannot show symbol navigation.");
        return;
    }

    std::string file_path = doc->getFilePath();
    if (file_path.empty()) {
        setStatusMessage("File not saved. Cannot show symbol navigation.");
        return;
    }

    // 获取当前文件的LSP客户端
    auto* lsp_client = lsp_manager_->getClientForFile(file_path);
    if (!lsp_client) {
        setStatusMessage("LSP server not available for this file.");
        return;
    }
    if (!lsp_client->isConnected()) {
        setStatusMessage("LSP server not connected for this file.");
        return;
    }

    // 确保文档已经通过 didOpen 添加到 LSP 服务器
    std::string uri = lsp_client->filepathToUri(file_path);
    std::string language_id = detectLanguageId(file_path);

    // 检查文档是否已经打开
    bool needs_did_open = (file_language_map_.find(uri) == file_language_map_.end());
    if (needs_did_open) {
        try {
            // 获取文档内容
            std::string content;
            size_t line_count = doc->lineCount();
            size_t max_lines = std::min(line_count, static_cast<size_t>(1000));
            for (size_t i = 0; i < max_lines; ++i) {
                content += doc->getLine(i);
                if (i < max_lines - 1) {
                    content += "\n";
                }
            }
            lsp_client->didOpen(uri, language_id, content);
            file_language_map_[uri] = language_id;
        } catch (const std::exception& e) {
            setStatusMessage("Failed to prepare document for symbol navigation.");
            return;
        }
    }

    // 异步获取符号列表
    std::thread([this, file_path, lsp_client, uri]() {
        std::vector<pnana::features::DocumentSymbol> symbols = lsp_client->documentSymbol(uri);

        // 在主线程更新UI
        screen_.Post([this, symbols]() {
            symbol_navigation_popup_.setSymbols(symbols);
            if (symbols.empty()) {
                setStatusMessage("No symbols found in this file.");
                return;
            }

            // 设置跳转回调（用于预览跳转）
            symbol_navigation_popup_.setJumpCallback(
                [this](const pnana::features::DocumentSymbol& symbol) {
                    jumpToSymbol(symbol);
                });

            symbol_navigation_popup_.show();
            show_symbol_navigation_popup_ = true;
        });
    }).detach();
}

void Editor::hideSymbolNavigation() {
    symbol_navigation_popup_.hide();
    show_symbol_navigation_popup_ = false;
}

void Editor::jumpToSymbol(const pnana::features::DocumentSymbol& symbol) {
    // 跳转到符号位置
    setCursorPosForLua(symbol.range.start.line, symbol.range.start.character);
    adjustCursor();
    adjustViewOffset();
    force_ui_update_ = true;
}
#endif

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

#ifdef BUILD_LSP_SUPPORT
Element Editor::renderSymbolNavigationPopup() {
    if (!show_symbol_navigation_popup_ || !symbol_navigation_popup_.isVisible()) {
        return text("");
    }
    return symbol_navigation_popup_.render();
}
#endif

// 代码折叠方法实现（Neovim-like 行为）
void Editor::toggleFold() {
    // (Debounce removed) Allow each toggle request to be handled. Key-repeat should
    // be handled at input layer; excessive suppression here caused fold to not trigger.

    if (!folding_manager_) {
        setStatusMessage("Folding manager not initialized");
        return;
    }

    if (!folding_manager_->isInitialized()) {
        setStatusMessage("Folding ranges not ready yet, please wait...");
        return;
    }

    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No document open");
        return;
    }

    bool was_modified = doc->isModified();
    int cursor_line = static_cast<int>(cursor_row_);

    const auto& ranges = folding_manager_->getFoldingRanges();

    // 1) If cursor is inside a folding range, toggle the innermost range.
    const pnana::features::FoldingRange* innermost_range = nullptr;
    int min_range_size = INT_MAX;
    for (const auto& range : ranges) {
        if (range.containsLine(cursor_line)) {
            int range_size = range.endLine - range.startLine;
            if (range_size < min_range_size) {
                min_range_size = range_size;
                innermost_range = &range;
            }
        }
    }
    if (innermost_range) {
        int start = innermost_range->startLine;

        (void)folding_manager_->isFolded(start);
        folding_manager_->toggleFold(start);
        bool now_folded = folding_manager_->isFolded(start);

        // Neovim behavior:
        // - If we just folded, move cursor to the fold start.
        // - If we unfolded, keep the cursor on the same logical line but ensure it's visible.
        if (now_folded) {
            cursor_row_ = static_cast<size_t>(start);
            adjustCursor();
            adjustViewOffset();
        } else {
            adjustCursor();
            adjustViewOffset();
        }

        setStatusMessage(now_folded ? "Folded" : "Unfolded");
        doc->setModified(was_modified);
        force_ui_update_ = true;
        last_render_time_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(200);
        return;
    }

    // 2) If cursor is exactly on a fold start, toggle it.
    for (const auto& range : ranges) {
        if (range.startLine == cursor_line) {
            int start = range.startLine;
            (void)folding_manager_->isFolded(start);
            folding_manager_->toggleFold(start);
            bool now_folded = folding_manager_->isFolded(start);

            if (now_folded) {
                // Move cursor to start to keep it visible
                cursor_row_ = static_cast<size_t>(start);
                adjustCursor();
                adjustViewOffset();
            } else {
                adjustCursor();
                adjustViewOffset();
            }

            setStatusMessage(now_folded ? "Folded" : "Unfolded");
            doc->setModified(was_modified);
            force_ui_update_ = true;
            last_render_time_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(200);
            return;
        }
    }

    // 3) Fallback: find nearest foldable start line above the cursor and toggle it.
    int nearest_start = -1;
    int best_dist = INT_MAX;
    for (const auto& range : ranges) {
        if (range.startLine <= cursor_line) {
            int dist = cursor_line - range.startLine;
            if (dist < best_dist) {
                best_dist = dist;
                nearest_start = range.startLine;
            }
        }
    }
    if (nearest_start >= 0) {
        (void)folding_manager_->isFolded(nearest_start);
        folding_manager_->toggleFold(nearest_start);
        bool now_folded = folding_manager_->isFolded(nearest_start);

        if (now_folded) {
            cursor_row_ = static_cast<size_t>(nearest_start);
            adjustCursor();
            adjustViewOffset();
        } else {
            adjustCursor();
            adjustViewOffset();
        }

        setStatusMessage(now_folded ? "Folded" : "Unfolded");
        doc->setModified(was_modified);
        force_ui_update_ = true;
        last_render_time_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(200);
        return;
    }

    // Nothing to fold/unfold
    setStatusMessage("No foldable region at cursor");
}

void Editor::toggleFoldAtCursor() {
    if (!folding_manager_)
        return;

    auto doc = getCurrentDocument();
    if (!doc)
        return;

    int cursor_line = static_cast<int>(cursor_row_);
    folding_manager_->foldAtLine(cursor_line);
}

void Editor::foldAll() {
    if (!folding_manager_)
        return;

    auto doc = getCurrentDocument();
    if (!doc)
        return;

    // 保存当前的修改状态
    bool was_modified = doc->isModified();

    folding_manager_->foldAll();
    setStatusMessage("Folded all regions");

    // 恢复修改状态（折叠不应该改变文件的修改状态）
    doc->setModified(was_modified);

    // 强制UI更新
    force_ui_update_ = true;
    last_render_time_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(200);
}

void Editor::unfoldAll() {
    if (!folding_manager_)
        return;

    auto doc = getCurrentDocument();
    if (!doc)
        return;

    // 保存当前的修改状态
    bool was_modified = doc->isModified();

    folding_manager_->unfoldAll();
    setStatusMessage("Unfolded all regions");

    // 恢复修改状态（折叠不应该改变文件的修改状态）
    doc->setModified(was_modified);

    // 强制UI更新
    force_ui_update_ = true;
    last_render_time_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(200);
}

} // namespace core
} // namespace pnana

#endif // BUILD_LSP_SUPPORT
