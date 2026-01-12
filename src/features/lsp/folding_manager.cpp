#include "features/lsp/folding_manager.h"
#include "features/lsp/lsp_client.h"
#include "utils/logger.h"
#include <algorithm>
#include <set>

namespace pnana {
namespace features {

FoldingManager::FoldingManager(std::shared_ptr<LspClient> lsp_client) : lsp_client_(lsp_client) {
    (void)0;
}

void FoldingManager::initializeFoldingRanges(const std::string& uri) {
    if (!lsp_client_) {
        return;
    }

    try {
        folding_ranges_ = lsp_client_->foldingRange(uri);

        // 按起始行排序
        std::sort(folding_ranges_.begin(), folding_ranges_.end(),
                  [](const FoldingRange& a, const FoldingRange& b) {
                      return a.startLine < b.startLine;
                  });

        // 清理无效的折叠状态
        std::set<int> valid_lines;
        for (const auto& range : folding_ranges_) {
            valid_lines.insert(range.startLine);
        }
        // 移除不在有效范围内的折叠状态
        for (auto it = folded_lines_.begin(); it != folded_lines_.end();) {
            if (valid_lines.find(*it) == valid_lines.end()) {
                it = folded_lines_.erase(it);
            } else {
                ++it;
            }
        }

        notifyStateChanged();
    } catch (const std::exception& e) {
        // LSP不支持折叠或出错，清除折叠范围
        folding_ranges_.clear();
        folded_lines_.clear();
    }
}

void FoldingManager::toggleFold(int start_line) {
    auto it = folded_lines_.find(start_line);
    bool was_folded = (it != folded_lines_.end());

    if (was_folded) {
        folded_lines_.erase(it);
    } else {
        // 检查是否有对应的折叠范围
        bool has_range = std::any_of(folding_ranges_.begin(), folding_ranges_.end(),
                                     [start_line](const FoldingRange& range) {
                                         return range.startLine == start_line;
                                     });
        if (has_range) {
            folded_lines_.insert(start_line);
        }
    }

    notifyStateChanged();
}

void FoldingManager::foldAll() {
    folded_lines_.clear();
    for (const auto& range : folding_ranges_) {
        folded_lines_.insert(range.startLine);
    }
    notifyStateChanged();
}

void FoldingManager::unfoldAll() {
    folded_lines_.clear();
    notifyStateChanged();
}

void FoldingManager::foldAtLine(int line) {
    // 查找包含该行的折叠范围
    for (const auto& range : folding_ranges_) {
        if (range.containsLine(line)) {
            toggleFold(range.startLine);
            break;
        }
    }
}

bool FoldingManager::isFolded(int start_line) const {
    bool result =
        std::find(folded_lines_.begin(), folded_lines_.end(), start_line) != folded_lines_.end();
    (void)result;
    return result;
}

bool FoldingManager::isLineInFoldedRange(int line) const {
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

std::vector<FoldingRange> FoldingManager::getFoldedRanges() const {
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
    std::vector<int> foldable_lines;
    for (const auto& range : folding_ranges_) {
        foldable_lines.push_back(range.startLine);
    }
    return foldable_lines;
}

void FoldingManager::clear() {
    folding_ranges_.clear();
    folded_lines_.clear();
    notifyStateChanged();
}

void FoldingManager::notifyStateChanged() {
    if (state_changed_callback_) {
        state_changed_callback_();
    }

    if (document_sync_callback_) {
        document_sync_callback_(folding_ranges_, folded_lines_);
    }
}

} // namespace features
} // namespace pnana
