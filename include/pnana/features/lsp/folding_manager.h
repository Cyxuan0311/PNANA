#ifndef PNANA_FEATURES_LSP_FOLDING_MANAGER_H
#define PNANA_FEATURES_LSP_FOLDING_MANAGER_H

#include "features/lsp/lsp_types.h"
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace pnana {
namespace features {

class LspClient;

/**
 * 折叠范围管理器
 * 负责管理代码折叠的逻辑，包括从LSP服务器获取折叠范围和状态管理
 */
class FoldingManager {
  public:
    explicit FoldingManager(std::shared_ptr<LspClient> lsp_client);

    // 初始化折叠范围（从LSP服务器获取）
    void initializeFoldingRanges(const std::string& uri);

    // 获取折叠范围
    const std::vector<FoldingRange>& getFoldingRanges() const {
        return folding_ranges_;
    }

    // 折叠操作
    void toggleFold(int start_line);
    void foldAll();
    void unfoldAll();
    void foldAtLine(int line);

    // 查询折叠状态
    bool isFolded(int start_line) const;
    bool isLineInFoldedRange(int line) const;
    std::vector<FoldingRange> getFoldedRanges() const;

    // 获取可折叠的行（用于UI显示折叠指示器）
    std::vector<int> getFoldableLines() const;

    // 折叠状态变化回调
    using FoldingStateChangedCallback = std::function<void()>;
    void setFoldingStateChangedCallback(FoldingStateChangedCallback callback) {
        state_changed_callback_ = callback;
    }

    // 设置文档同步回调
    using DocumentSyncCallback =
        std::function<void(const std::vector<FoldingRange>&, const std::set<int>&)>;
    void setDocumentSyncCallback(DocumentSyncCallback callback) {
        document_sync_callback_ = callback;
    }

    // 清空折叠状态
    void clear();

    // 检查是否已初始化
    bool isInitialized() const {
        return !folding_ranges_.empty();
    }

  private:
    std::shared_ptr<LspClient> lsp_client_;
    std::vector<FoldingRange> folding_ranges_;
    std::set<int> folded_lines_; // 已折叠的起始行

    FoldingStateChangedCallback state_changed_callback_;
    DocumentSyncCallback document_sync_callback_;

    // 通知状态变化
    void notifyStateChanged();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_FOLDING_MANAGER_H
