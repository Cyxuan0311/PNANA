#include "core/document_manager.h"
#include <algorithm>

namespace pnana {
namespace core {

DocumentManager::DocumentManager() : current_index_(0), next_untitled_number_(1) {
    // 不自动创建文档，让用户主动创建
}

size_t DocumentManager::openDocument(const std::string& filepath) {
    // 检查是否已经打开
    int existing = findDocumentByPath(filepath);
    if (existing >= 0) {
        switchToDocument(static_cast<size_t>(existing));
        return static_cast<size_t>(existing);
    }

    // 创建新文档
    auto doc = std::make_unique<Document>(filepath);
    documents_.push_back(std::move(doc));
    size_t new_index = documents_.size() - 1;
    switchToDocument(new_index);
    return new_index;
}

size_t DocumentManager::createNewDocument() {
    auto doc = std::make_unique<Document>();
    documents_.push_back(std::move(doc));
    size_t new_index = documents_.size() - 1;
    next_untitled_number_++;
    switchToDocument(new_index);
    return new_index;
}

bool DocumentManager::closeDocument(size_t index) {
    if (index >= documents_.size()) {
        return false;
    }

    // 如果文档有未保存的修改，先检查
    if (documents_[index]->isModified()) {
        // 这里应该提示用户，但现在先简单返回false
        return false;
    }

    documents_.erase(documents_.begin() + index);

    // 调整当前索引（如果文档列表不为空）
    if (!documents_.empty()) {
        if (current_index_ >= documents_.size()) {
            current_index_ = documents_.size() - 1;
        }
    } else {
        // 所有文档已关闭，不自动创建新文档，保持空状态
        current_index_ = 0;
    }

    return true;
}

bool DocumentManager::closeCurrentDocument() {
    return closeDocument(current_index_);
}

bool DocumentManager::closeAllDocuments() {
    // 检查是否有未保存的修改
    for (const auto& doc : documents_) {
        if (doc->isModified()) {
            return false;
        }
    }

    documents_.clear();
    current_index_ = 0;
    // 不自动创建新文档，保持空状态以显示欢迎界面
    return true;
}

void DocumentManager::switchToDocument(size_t index) {
    if (index < documents_.size() && index != current_index_) {
        size_t old_index = current_index_;
        current_index_ = index;

        // 通知文档切换回调
        if (document_switched_callback_) {
            document_switched_callback_(old_index, index);
        }
    }
}

void DocumentManager::switchToNextDocument() {
    if (documents_.empty())
        return;
    current_index_ = (current_index_ + 1) % documents_.size();
}

void DocumentManager::switchToPreviousDocument() {
    if (documents_.empty())
        return;
    if (current_index_ == 0) {
        current_index_ = documents_.size() - 1;
    } else {
        current_index_--;
    }
}

Document* DocumentManager::getCurrentDocument() {
    if (documents_.empty()) {
        return nullptr;
    }
    return documents_[current_index_].get();
}

const Document* DocumentManager::getCurrentDocument() const {
    if (documents_.empty()) {
        return nullptr;
    }
    return documents_[current_index_].get();
}

Document* DocumentManager::getDocument(size_t index) {
    if (index >= documents_.size()) {
        return nullptr;
    }
    return documents_[index].get();
}

const Document* DocumentManager::getDocument(size_t index) const {
    if (index >= documents_.size()) {
        return nullptr;
    }
    return documents_[index].get();
}

int DocumentManager::findDocumentByPath(const std::string& filepath) const {
    for (size_t i = 0; i < documents_.size(); ++i) {
        if (documents_[i]->getFilePath() == filepath) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<DocumentManager::TabInfo> DocumentManager::getAllTabs() const {
    std::vector<TabInfo> tabs;
    for (size_t i = 0; i < documents_.size(); ++i) {
        TabInfo info;
        info.filename = documents_[i]->getFileName();
        if (info.filename.empty()) {
            info.filename = "[Untitled]";
        }
        info.filepath = documents_[i]->getFilePath();
        info.is_modified = documents_[i]->isModified();
        info.is_current = (i == current_index_);
        tabs.push_back(info);
    }
    return tabs;
}

} // namespace core
} // namespace pnana
