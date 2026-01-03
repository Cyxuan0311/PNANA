#ifndef PNANA_CORE_DOCUMENT_MANAGER_H
#define PNANA_CORE_DOCUMENT_MANAGER_H

#include "core/document.h"
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 多文档管理器（支持标签页）
class DocumentManager {
  public:
    DocumentManager();

    // 文档操作
    size_t openDocument(const std::string& filepath); // 返回文档索引
    size_t createNewDocument();                       // 创建新文档，返回索引
    bool closeDocument(size_t index);
    bool closeCurrentDocument();
    bool closeAllDocuments();

    // 文档切换
    void switchToDocument(size_t index);
    void switchToNextDocument();
    void switchToPreviousDocument();

    // 获取文档
    Document* getCurrentDocument();
    const Document* getCurrentDocument() const;
    Document* getDocument(size_t index);
    const Document* getDocument(size_t index) const;

    // 文档信息
    size_t getCurrentIndex() const {
        return current_index_;
    }
    size_t getDocumentCount() const {
        return documents_.size();
    }
    bool hasDocuments() const {
        return !documents_.empty();
    }

    // 查找文档
    int findDocumentByPath(const std::string& filepath) const;

    // 获取所有文档信息（用于标签栏）
    struct TabInfo {
        std::string filename;
        std::string filepath;
        bool is_modified;
        bool is_current;
    };
    std::vector<TabInfo> getAllTabs() const;

  private:
    std::vector<std::unique_ptr<Document>> documents_;
    size_t current_index_;
    size_t next_untitled_number_; // 用于未命名文档编号

    void ensureAtLeastOneDocument();
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_DOCUMENT_MANAGER_H
