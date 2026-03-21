#include "core/piece_table.h"
#include <algorithm>
#include <functional>

namespace pnana {
namespace core {

PieceTable::PieceTable() : total_length_(0), line_count_(1), lines_dirty_(true) {
    nil_ = std::make_shared<RBNode>();
    nil_->color = 1; // black
    root_ = nil_;
}

size_t PieceTable::countNewlines(const std::string& str, size_t start, size_t len) const {
    if (start >= str.size())
        return 0;
    len = std::min(len, str.size() - start);

    size_t count = 0;
    for (size_t i = start; i < start + len && i < str.size(); ++i) {
        if (str[i] == '\n')
            count++;
    }
    return count;
}

void PieceTable::updateNodeInfo(std::shared_ptr<RBNode> node) const {
    if (!node || node == nil_)
        return;

    node->subtree_length = node->piece.length;
    node->subtree_newlines = countNewlines(
        node->piece.buffer_type == BufferType::ORIGINAL ? original_buffer_ : append_buffer_,
        node->piece.start, node->piece.length);

    if (node->left && node->left != nil_) {
        node->subtree_length += node->left->subtree_length;
        node->subtree_newlines += node->left->subtree_newlines;
    }
    if (node->right && node->right != nil_) {
        node->subtree_length += node->right->subtree_length;
        node->subtree_newlines += node->right->subtree_newlines;
    }
}

void PieceTable::rotateLeft(std::shared_ptr<RBNode> node) {
    if (!node || node == nil_)
        return;

    std::shared_ptr<RBNode> right = node->right;
    node->right = right->left;

    if (right->left != nil_) {
        right->left->parent = node;
    }

    right->parent = node->parent;

    if (!node->parent) {
        root_ = right;
    } else if (node == node->parent->left) {
        node->parent->left = right;
    } else {
        node->parent->right = right;
    }

    right->left = node;
    node->parent = right;

    updateNodeInfo(node);
    updateNodeInfo(right);
}

void PieceTable::rotateRight(std::shared_ptr<RBNode> node) {
    if (!node || node == nil_)
        return;

    std::shared_ptr<RBNode> left = node->left;
    node->left = left->right;

    if (left->right != nil_) {
        left->right->parent = node;
    }

    left->parent = node->parent;

    if (!node->parent) {
        root_ = left;
    } else if (node == node->parent->right) {
        node->parent->right = left;
    } else {
        node->parent->left = left;
    }

    left->right = node;
    node->parent = left;

    updateNodeInfo(node);
    updateNodeInfo(left);
}

void PieceTable::insertFixup(std::shared_ptr<RBNode> node) {
    while (node && node->parent && node->parent->color == 0) {
        if (node->parent == node->parent->parent->left) {
            std::shared_ptr<RBNode> uncle = node->parent->parent->right;

            if (uncle && uncle->color == 0) {
                node->parent->color = 1;
                uncle->color = 1;
                node->parent->parent->color = 0;
                node = node->parent->parent;
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    rotateLeft(node);
                }
                node->parent->color = 1;
                node->parent->parent->color = 0;
                rotateRight(node->parent->parent);
            }
        } else {
            std::shared_ptr<RBNode> uncle = node->parent->parent->left;

            if (uncle && uncle->color == 0) {
                node->parent->color = 1;
                uncle->color = 1;
                node->parent->parent->color = 0;
                node = node->parent->parent;
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rotateRight(node);
                }
                node->parent->color = 1;
                node->parent->parent->color = 0;
                rotateLeft(node->parent->parent);
            }
        }
    }

    root_->color = 1;
}

std::pair<std::shared_ptr<PieceTable::RBNode>, size_t> PieceTable::findNodeAndOffset(
    size_t pos) const {
    if (!root_ || root_ == nil_ || pos >= total_length_) {
        return {nullptr, 0};
    }

    std::shared_ptr<RBNode> current = root_;
    size_t remaining = pos;

    while (current && current != nil_) {
        size_t left_len =
            (current->left && current->left != nil_) ? current->left->subtree_length : 0;

        if (remaining < left_len) {
            current = current->left;
        } else if (remaining < left_len + current->piece.length) {
            return {current, remaining - left_len};
        } else {
            remaining -= left_len + current->piece.length;
            current = current->right;
        }
    }

    return {nullptr, 0};
}

void PieceTable::traverseInOrder(std::shared_ptr<RBNode> node,
                                 std::function<void(const Piece&)> callback) {
    if (!node || node == nil_)
        return;

    traverseInOrder(node->left, callback);
    callback(node->piece);
    traverseInOrder(node->right, callback);
}

void PieceTable::insertPiece(size_t pos, BufferType type, size_t start, size_t length) {
    if (length == 0)
        return;

    Piece new_piece(type, start, length);
    auto new_node = std::make_shared<RBNode>(new_piece);
    new_node->left = nil_;
    new_node->right = nil_;
    new_node->parent = nullptr;

    if (!root_ || root_ == nil_) {
        root_ = new_node;
        root_->color = 1;
    } else {
        // 找到插入位置
        auto [node, offset] = findNodeAndOffset(pos);

        if (!node) {
            // 插入到末尾
            std::shared_ptr<RBNode> current = root_;
            while (current->right && current->right != nil_) {
                current = current->right;
            }
            current->right = new_node;
            new_node->parent = current;
        } else if (offset == 0 && pos == 0) {
            // 插入到开头
            std::shared_ptr<RBNode> current = root_;
            while (current->left && current->left != nil_) {
                current = current->left;
            }
            current->left = new_node;
            new_node->parent = current;
        } else {
            // 分割现有节点
            Piece& piece = node->piece;

            if (offset > 0 && offset < piece.length) {
                // 需要分割
                Piece right_piece(piece.buffer_type, piece.start + offset, piece.length - offset);
                piece.length = offset;

                auto right_node = std::make_shared<RBNode>(right_piece);
                right_node->left = nil_;
                right_node->right = node->right;
                right_node->parent = node;

                if (node->right != nil_) {
                    node->right->parent = right_node;
                }
                node->right = right_node;
                node = right_node;
            }

            // 插入新节点
            new_node->parent = node;
            new_node->left = node->left;
            node->left = new_node;

            if (new_node->left != nil_) {
                new_node->left->parent = new_node;
            }
        }

        insertFixup(new_node);
    }

    updateNodeInfo(new_node);

    // 更新所有祖先节点的信息
    std::shared_ptr<RBNode> current = new_node;
    while (current && current != nil_) {
        updateNodeInfo(current);
        current = current->parent;
    }

    total_length_ += length;
    lines_dirty_ = true;
}

void PieceTable::insert(size_t pos, const std::string& text) {
    if (text.empty())
        return;

    pos = std::min(pos, total_length_);

    size_t start = append_buffer_.size();
    append_buffer_ += text;

    insertPiece(pos, BufferType::APPEND, start, text.size());
}

void PieceTable::remove(size_t pos, size_t length) {
    if (length == 0 || pos >= total_length_)
        return;

    length = std::min(length, total_length_ - pos);

    // 简化实现：标记要删除的片段
    // 实际实现需要更复杂的树操作
    auto [node, offset] = findNodeAndOffset(pos);

    if (!node)
        return;

    // 调整片段长度
    size_t remaining = length;

    while (remaining > 0 && node) {
        if (offset < node->piece.length) {
            size_t can_remove = std::min(remaining, node->piece.length - offset);
            node->piece.start += offset;
            node->piece.length -= offset + can_remove;
            remaining -= can_remove;
            offset = 0;
        }

        if (remaining > 0) {
            node = node->right != nil_ ? node->right : nullptr;
            offset = 0;
        }
    }

    total_length_ -= length;
    lines_dirty_ = true;

    // 更新树信息
    std::shared_ptr<RBNode> current = root_;
    while (current && current != nil_) {
        updateNodeInfo(current);
        if (current->left != nil_) {
            current = current->left;
        } else if (current->right != nil_) {
            current = current->right;
        } else {
            break;
        }
    }
}

std::string PieceTable::getText(size_t pos, size_t length) const {
    if (pos >= total_length_ || length == 0)
        return "";

    length = std::min(length, total_length_ - pos);
    std::string result;
    result.reserve(length);

    auto [node, offset] = findNodeAndOffset(pos);
    if (!node || node == nil_) {
        return "";
    }

    auto leftMost = [this](std::shared_ptr<RBNode> current) {
        while (current && current != nil_ && current->left && current->left != nil_) {
            current = current->left;
        }
        return current;
    };

    auto nextInOrder = [this, &leftMost](std::shared_ptr<RBNode> current) {
        if (!current || current == nil_) {
            return std::shared_ptr<RBNode>(nullptr);
        }

        if (current->right && current->right != nil_) {
            return leftMost(current->right);
        }

        auto child = current;
        auto parent = current->parent;
        while (parent && parent != nil_ && child == parent->right) {
            child = parent;
            parent = parent->parent;
        }

        if (!parent || parent == nil_) {
            return std::shared_ptr<RBNode>(nullptr);
        }
        return parent;
    };

    size_t remaining = length;
    while (node && node != nil_ && remaining > 0) {
        const std::string& buffer =
            (node->piece.buffer_type == BufferType::ORIGINAL) ? original_buffer_ : append_buffer_;

        if (offset < node->piece.length && node->piece.start < buffer.size()) {
            const size_t piece_available = node->piece.length - offset;
            const size_t buffer_available = buffer.size() - node->piece.start;
            const size_t safe_available = std::min(
                piece_available, (offset < buffer_available) ? (buffer_available - offset) : 0UL);
            const size_t take = std::min(remaining, safe_available);

            if (take > 0) {
                result.append(buffer, node->piece.start + offset, take);
                remaining -= take;
            }
        }

        offset = 0;
        if (remaining == 0) {
            break;
        }
        node = nextInOrder(node);
    }

    return result;
}

std::string PieceTable::getFullText() const {
    std::string result;
    result.reserve(total_length_);

    // 使用 const_cast 来调用非 const 版本的 traverseInOrder
    auto* non_const_this = const_cast<PieceTable*>(this);
    non_const_this->traverseInOrder(root_, [this, &result](const Piece& piece) {
        const std::string& buffer =
            piece.buffer_type == BufferType::ORIGINAL ? original_buffer_ : append_buffer_;
        if (piece.start < buffer.size()) {
            size_t len = std::min(piece.length, buffer.size() - piece.start);
            result += buffer.substr(piece.start, len);
        }
    });

    return result;
}

void PieceTable::insertLine(size_t line_num, const std::string& content) {
    size_t pos = findLineStart(line_num);
    insert(pos, content + "\n");
}

void PieceTable::removeLine(size_t line_num) {
    if (line_num >= lineCount())
        return;

    size_t start = findLineStart(line_num);
    size_t end = findLineStart(line_num + 1);

    if (end > start) {
        remove(start, end - start);
    }
}

std::string PieceTable::getLine(size_t line_num) const {
    if (line_num >= lineCount())
        return "";

    size_t start = findLineStart(line_num);
    size_t end = findLineStart(line_num + 1);

    if (end > start && end <= total_length_) {
        return getText(start, end - start - 1);
    }

    return getText(start, total_length_ - start);
}

size_t PieceTable::lineCount() const {
    if (lines_dirty_) {
        recomputeLineCount();
    }
    return line_count_;
}

void PieceTable::recomputeLineCount() const {
    line_count_ = 1;

    // 使用 const_cast 来调用非 const 版本的 traverseInOrder
    auto* non_const_this = const_cast<PieceTable*>(this);
    non_const_this->traverseInOrder(root_, [this](const Piece& piece) {
        const std::string& buffer =
            piece.buffer_type == BufferType::ORIGINAL ? original_buffer_ : append_buffer_;
        for (size_t i = 0; i < piece.length && piece.start + i < buffer.size(); ++i) {
            if (buffer[piece.start + i] == '\n') {
                line_count_++;
            }
        }
    });

    lines_dirty_ = false;
}

size_t PieceTable::findLineStart(size_t line_num) const {
    if (line_num == 0)
        return 0;

    size_t current_line = 0;
    size_t pos = 0;

    while (current_line < line_num && pos < total_length_) {
        if (getChar(pos) == '\n') {
            current_line++;
        }
        pos++;
    }

    return pos;
}

void PieceTable::insertChar(size_t pos, char ch) {
    insert(pos, std::string(1, ch));
}

void PieceTable::removeChar(size_t pos) {
    remove(pos, 1);
}

char PieceTable::getChar(size_t pos) const {
    if (pos >= total_length_)
        return '\0';

    auto [node, offset] = findNodeAndOffset(pos);

    if (!node)
        return '\0';

    const std::string& buffer =
        node->piece.buffer_type == BufferType::ORIGINAL ? original_buffer_ : append_buffer_;

    if (node->piece.start + offset < buffer.size()) {
        return buffer[node->piece.start + offset];
    }

    return '\0';
}

size_t PieceTable::length() const {
    return total_length_;
}

size_t PieceTable::lineLength(size_t line_num) const {
    if (line_num >= lineCount())
        return 0;

    size_t start = findLineStart(line_num);
    size_t end = findLineStart(line_num + 1);

    if (end > start && end <= total_length_) {
        return end - start - 1;
    }

    return total_length_ - start;
}

size_t PieceTable::positionToLineCol(size_t pos) const {
    if (pos > total_length_)
        pos = total_length_;

    size_t line = 0, col = 0;
    for (size_t i = 0; i < pos && i < total_length_; ++i) {
        if (getChar(i) == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }

    return line * 1000000 + col;
}

size_t PieceTable::lineColToPosition(size_t line, size_t col) const {
    size_t current_line = 0, current_col = 0, pos = 0;

    while (pos < total_length_ && current_line < line) {
        if (getChar(pos) == '\n') {
            current_line++;
            current_col = 0;
        } else {
            current_col++;
        }
        pos++;
    }

    while (pos < total_length_ && current_col < col && getChar(pos) != '\n') {
        pos++;
        current_col++;
    }

    return pos;
}

bool PieceTable::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0) {
        original_buffer_.clear();
        append_buffer_.clear();
        root_ = nil_;
        total_length_ = 0;
        return true;
    }

    original_buffer_.resize(size);
    file.read(&original_buffer_[0], size);

    // 创建初始片段
    Piece initial_piece(BufferType::ORIGINAL, 0, size);
    root_ = std::make_shared<RBNode>(initial_piece);
    root_->left = nil_;
    root_->right = nil_;
    root_->color = 1;

    total_length_ = size;
    lines_dirty_ = true;
    updateNodeInfo(root_);

    return true;
}

bool PieceTable::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string text = getFullText();
    file.write(text.c_str(), text.size());

    return file.good();
}

size_t PieceTable::getMemoryUsage() const {
    size_t usage = sizeof(PieceTable);
    usage += original_buffer_.capacity() + append_buffer_.capacity();
    // 加上树的节点开销（简化估计）
    return usage * 2; // 假设 2 倍开销
}

void PieceTable::optimize() {
    // 合并相邻的片段（如果它们来自同一个缓冲区且连续）
    // 这是一个简化的实现
}

} // namespace core
} // namespace pnana
