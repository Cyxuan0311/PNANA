#ifndef PNANA_FEATURES_LSP_DOCUMENT_CHANGE_TRACKER_H
#define PNANA_FEATURES_LSP_DOCUMENT_CHANGE_TRACKER_H

#include "features/lsp/lsp_types.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace pnana {
namespace features {

// LSP 文档内容变更事件
struct TextDocumentContentChangeEvent {
    LspRange range;   // 变更范围（可选，全量更新时为空）
    int rangeLength;  // 范围长度（可选）
    std::string text; // 新文本

    TextDocumentContentChangeEvent();
    TextDocumentContentChangeEvent(const std::string& new_text);
    TextDocumentContentChangeEvent(const LspRange& r, int len, const std::string& new_text);
};

// 文档变更记录
struct ChangeRecord {
    int line;
    int col;
    std::string old_text;
    std::string new_text;
    int old_length;
    int new_length;
};

/**
 * 文档变更跟踪器
 * 跟踪文档变更，生成增量更新事件
 */
class DocumentChangeTracker {
  public:
    DocumentChangeTracker();
    ~DocumentChangeTracker();

    // 记录变更
    void recordChange(int line, int col, const std::string& old_text, const std::string& new_text);

    // 记录插入操作
    void recordInsert(int line, int col, const std::string& inserted_text);

    // 记录删除操作
    void recordDelete(int line, int col, int length);

    // 获取变更事件列表（用于 LSP didChange）
    std::vector<TextDocumentContentChangeEvent> getChanges();

    // 清除所有变更记录
    void clear();

    // 检查是否有待处理的变更
    bool hasChanges() const {
        return !changes_.empty();
    }

    // Schedule a delayed flush/callback (debounce). When timer expires, on_flush() will be called.
    void scheduleDelayedSync(std::chrono::milliseconds delay, std::function<void()> on_flush);

  private:
    std::vector<ChangeRecord> changes_;

    // 合并相邻的变更
    void mergeChanges();
    // Debounce thread for delayed sync
    std::thread debounce_thread_;
    std::condition_variable debounce_cv_;
    std::mutex debounce_mutex_;
    std::chrono::milliseconds debounce_delay_;
    std::function<void()> debounce_callback_;
    std::atomic<bool> debounce_running_;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_DOCUMENT_CHANGE_TRACKER_H
