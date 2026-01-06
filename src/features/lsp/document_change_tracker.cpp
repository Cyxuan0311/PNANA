#include "features/lsp/document_change_tracker.h"
#include "features/lsp/lsp_client.h"
#include <algorithm>

namespace pnana {
namespace features {

TextDocumentContentChangeEvent::TextDocumentContentChangeEvent() : rangeLength(0) {}

TextDocumentContentChangeEvent::TextDocumentContentChangeEvent(const std::string& new_text)
    : rangeLength(0), text(new_text) {}

TextDocumentContentChangeEvent::TextDocumentContentChangeEvent(const LspRange& r, int len,
                                                               const std::string& new_text)
    : range(r), rangeLength(len), text(new_text) {}

DocumentChangeTracker::DocumentChangeTracker() {}
DocumentChangeTracker::~DocumentChangeTracker() {
    // stop debounce thread if running
    debounce_running_.store(false);
    debounce_cv_.notify_all();
    if (debounce_thread_.joinable())
        debounce_thread_.join();
}

void DocumentChangeTracker::recordChange(int line, int col, const std::string& old_text,
                                         const std::string& new_text) {
    ChangeRecord record;
    record.line = line;
    record.col = col;
    record.old_text = old_text;
    record.new_text = new_text;
    record.old_length = static_cast<int>(old_text.length());
    record.new_length = static_cast<int>(new_text.length());

    changes_.push_back(record);
}

void DocumentChangeTracker::recordInsert(int line, int col, const std::string& inserted_text) {
    recordChange(line, col, "", inserted_text);
}

void DocumentChangeTracker::recordDelete(int line, int col, int length) {
    // 对于删除操作，我们需要知道被删除的文本
    // 这里简化处理，实际使用时应该传入被删除的文本
    recordChange(line, col, std::string(length, ' '), "");
}

std::vector<TextDocumentContentChangeEvent> DocumentChangeTracker::getChanges() {
    if (changes_.empty()) {
        return {};
    }

    mergeChanges();

    std::vector<TextDocumentContentChangeEvent> events;

    for (const auto& change : changes_) {
        if (change.old_text.empty() && change.new_text.empty()) {
            continue; // 跳过空变更
        }

        // 如果变更范围很大，使用全量更新
        if (change.old_length > 1000 || change.new_length > 1000) {
            // 返回全量更新标记（由调用者决定是否使用全量更新）
            events.clear();
            events.push_back(TextDocumentContentChangeEvent("")); // 空字符串表示全量更新
            break;
        }

        // 创建范围
        LspRange range;
        range.start = LspPosition(change.line, change.col);
        range.end = LspPosition(change.line, change.col + change.old_length);

        TextDocumentContentChangeEvent event(range, change.old_length, change.new_text);
        events.push_back(event);
    }

    return events;
}

void DocumentChangeTracker::clear() {
    changes_.clear();
}

void DocumentChangeTracker::mergeChanges() {
    if (changes_.size() <= 1) {
        return;
    }

    // 简单的合并策略：合并同一行的连续变更
    std::vector<ChangeRecord> merged;
    merged.push_back(changes_[0]);

    for (size_t i = 1; i < changes_.size(); i++) {
        const auto& current = changes_[i];
        auto& last = merged.back();

        // 如果当前变更与上一个变更在同一行且相邻，合并它们
        if (current.line == last.line && current.col <= last.col + last.new_length) {
            // 合并变更
            int overlap = (last.col + last.new_length) - current.col;
            if (overlap > 0) {
                // 有重叠，需要调整
                last.new_text =
                    last.new_text.substr(0, last.new_text.length() - overlap) + current.new_text;
                last.new_length = static_cast<int>(last.new_text.length());
            } else {
                // 无重叠，直接追加
                last.new_text += current.new_text;
                last.new_length = static_cast<int>(last.new_text.length());
            }
            last.old_length += current.old_length;
        } else {
            merged.push_back(current);
        }
    }

    changes_ = merged;
}

void DocumentChangeTracker::scheduleDelayedSync(std::chrono::milliseconds delay,
                                                std::function<void()> on_flush) {
    {
        std::lock_guard<std::mutex> lock(debounce_mutex_);
        debounce_delay_ = delay;
        debounce_callback_ = std::move(on_flush);
        if (!debounce_running_.load()) {
            debounce_running_.store(true);
            debounce_thread_ = std::thread([this]() {
                std::unique_lock<std::mutex> lock(this->debounce_mutex_);
                while (this->debounce_running_.load()) {
                    if (this->debounce_cv_.wait_for(lock, this->debounce_delay_) ==
                        std::cv_status::timeout) {
                        // timeout => invoke callback outside lock
                        auto cb = this->debounce_callback_;
                        lock.unlock();
                        try {
                            if (cb)
                                cb();
                        } catch (...) {
                            // swallow
                        }
                        lock.lock();
                    } else {
                        // notified (possibly stop or reschedule), loop
                    }
                }
            });
        } else {
            // already running, just notify to reset timer
            debounce_cv_.notify_all();
        }
    }
}

} // namespace features
} // namespace pnana
