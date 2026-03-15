#include "features/lsp/folding_manager.h"
#include "features/lsp/lsp_client.h"
#include <algorithm>
#include <limits>
#include <set>
#include <string>

namespace pnana {
namespace features {

FoldingManager::FoldingManager(std::shared_ptr<LspClient> lsp_client) : lsp_client_(lsp_client) {
    (void)0;
}

void FoldingManager::initializeFoldingRanges(const std::string& uri) {
    if (!lsp_client_) {
        return;
    }

    std::vector<FoldingRange> new_ranges;
    try {
        new_ranges = lsp_client_->foldingRange(uri);
    } catch (const std::exception& e) {
        (void)e;
        std::lock_guard<std::mutex> lock(mutex_);
        folding_ranges_.clear();
        folded_lines_.clear();
        return;
    }

    // 按起始行排序
    std::sort(new_ranges.begin(), new_ranges.end(),
              [](const FoldingRange& a, const FoldingRange& b) {
                  return a.startLine < b.startLine;
              });

    std::set<int> valid_lines;
    for (const auto& range : new_ranges) {
        valid_lines.insert(range.startLine);
    }

    bool notify = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        current_uri_ = uri;
        folding_ranges_ = std::move(new_ranges);

        // 移除不在有效范围内的折叠状态
        for (auto it = folded_lines_.begin(); it != folded_lines_.end();) {
            if (valid_lines.find(*it) == valid_lines.end()) {
                it = folded_lines_.erase(it);
                notify = true;
            } else {
                ++it;
            }
        }
        notify = true;
    }
    if (notify) {
        notifyStateChanged();
    }
}

void FoldingManager::toggleFold(int start_line) {
    bool should_notify = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = folded_lines_.find(start_line);
        bool was_folded = (it != folded_lines_.end());

        if (was_folded) {
            folded_lines_.erase(it);
            should_notify = true;
        } else {
            bool has_range = std::any_of(folding_ranges_.begin(), folding_ranges_.end(),
                                         [start_line](const FoldingRange& range) {
                                             return range.startLine == start_line;
                                         });
            if (has_range) {
                folded_lines_.insert(start_line);
                should_notify = true;
            }
        }
    }
    if (should_notify) {
        notifyStateChanged();
    }
}

void FoldingManager::foldAll() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        folded_lines_.clear();
        for (const auto& range : folding_ranges_) {
            folded_lines_.insert(range.startLine);
        }
    }
    notifyStateChanged();
}

void FoldingManager::unfoldAll() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        folded_lines_.clear();
    }
    notifyStateChanged();
}

void FoldingManager::foldAtLine(int line) {
    int start_line = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const FoldingRange* innermost_range = nullptr;
        int min_range_size = std::numeric_limits<int>::max();

        for (const auto& range : folding_ranges_) {
            if (range.containsLine(line)) {
                int range_size = range.endLine - range.startLine;
                if (range_size < min_range_size) {
                    min_range_size = range_size;
                    innermost_range = &range;
                }
            }
        }
        if (innermost_range) {
            start_line = innermost_range->startLine;
        }
    }
    if (start_line >= 0) {
        toggleFold(start_line);
    }
}

bool FoldingManager::isFolded(int start_line) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return folded_lines_.find(start_line) != folded_lines_.end();
}

bool FoldingManager::isLineInFoldedRange(int line) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int folded_line : folded_lines_) {
        auto it = std::find_if(folding_ranges_.begin(), folding_ranges_.end(),
                               [folded_line](const FoldingRange& range) {
                                   return range.startLine == folded_line;
                               });
        if (it != folding_ranges_.end() && it->containsLine(line) && line != folded_line) {
            return true;
        }
    }
    return false;
}

std::vector<FoldingRange> FoldingManager::getFoldingRanges() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return folding_ranges_;
}

std::vector<FoldingRange> FoldingManager::getFoldedRanges() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<FoldingRange> folded_ranges;
    for (int folded_line : folded_lines_) {
        auto it = std::find_if(folding_ranges_.begin(), folding_ranges_.end(),
                               [folded_line](const FoldingRange& range) {
                                   return range.startLine == folded_line;
                               });
        if (it != folding_ranges_.end()) {
            folded_ranges.push_back(*it);
        }
    }
    return folded_ranges;
}

std::vector<int> FoldingManager::getFoldableLines() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> foldable_lines;
    for (const auto& range : folding_ranges_) {
        foldable_lines.push_back(range.startLine);
    }
    return foldable_lines;
}

void FoldingManager::clear() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        folding_ranges_.clear();
        folded_lines_.clear();
    }
    notifyStateChanged();
}

void FoldingManager::setFoldingRangesDirectly(const std::vector<FoldingRange>& ranges) {
    std::lock_guard<std::mutex> lock(mutex_);
    folding_ranges_ = ranges;
}

void FoldingManager::setFoldedLinesDirectly(const std::set<int>& folded_lines) {
    std::lock_guard<std::mutex> lock(mutex_);
    folded_lines_ = folded_lines;
}

bool FoldingManager::isInitialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !folding_ranges_.empty();
}

void FoldingManager::notifyStateChanged() {
    FoldingStateChangedCallback state_cb;
    DocumentSyncCallback doc_cb;
    std::string uri;
    std::vector<FoldingRange> ranges_copy;
    std::set<int> folded_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_cb = state_changed_callback_;
        doc_cb = document_sync_callback_;
        uri = current_uri_;
        ranges_copy = folding_ranges_;
        folded_copy = folded_lines_;
    }
    if (state_cb) {
        state_cb();
    }
    if (doc_cb) {
        doc_cb(uri, ranges_copy, folded_copy);
    }
}

} // namespace features
} // namespace pnana
