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
                                 std::function<void(const Piece&)> callback) const {
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
    new_node->subtree_length = length;
    new_node->subtree_newlines = countNewlines(
        type == BufferType::ORIGINAL ? original_buffer_ : append_buffer_, start, length);
    new_node->color = 0;

    if (!root_ || root_ == nil_) {
        root_ = new_node;
        root_->color = 1;
        root_->parent = nullptr;
    } else {
        auto [node, offset] = findNodeAndOffset(pos);

        if (!node || node == nil_) {
            std::shared_ptr<RBNode> current = root_;
            while (current->right != nil_) {
                current = current->right;
            }
            current->right = new_node;
            new_node->parent = current;
        } else if (offset == 0 && pos == 0) {
            std::shared_ptr<RBNode> current = root_;
            while (current->left != nil_) {
                current = current->left;
            }
            new_node->right = current;
            new_node->left = nil_;
            current->parent = new_node;
            root_ = new_node;
            root_->color = 1;
        } else {
            Piece& piece = node->piece;

            if (offset > 0 && offset < piece.length) {
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
            }

            new_node->parent = node;
            new_node->left = node->right;
            if (node->right != nil_) {
                node->right->parent = new_node;
            }
            node->right = new_node;
        }

        insertFixup(new_node);
    }

    std::shared_ptr<RBNode> current = new_node;
    while (current && current->parent) {
        updateNodeInfo(current->parent);
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

    std::vector<std::shared_ptr<RBNode>> nodes_to_modify;
    std::vector<std::pair<std::shared_ptr<RBNode>, Piece>> new_pieces;

    auto [start_node, start_offset] = findNodeAndOffset(pos);
    if (!start_node)
        return;

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
    size_t current_offset = start_offset;
    std::shared_ptr<RBNode> current = start_node;

    while (current && current != nil_ && remaining > 0) {
        size_t node_remaining = current->piece.length - current_offset;
        size_t to_remove = std::min(remaining, node_remaining);

        if (current_offset == 0 && to_remove == current->piece.length) {
            nodes_to_modify.push_back(current);
        } else if (current_offset == 0) {
            Piece modified = current->piece;
            modified.start += to_remove;
            modified.length -= to_remove;
            new_pieces.push_back({current, modified});
        } else if (to_remove == node_remaining) {
            Piece modified = current->piece;
            modified.length = current_offset;
            new_pieces.push_back({current, modified});
        } else {
            Piece left_part(current->piece.buffer_type, current->piece.start, current_offset);
            Piece right_part(current->piece.buffer_type,
                             current->piece.start + current_offset + to_remove,
                             current->piece.length - current_offset - to_remove);
            new_pieces.push_back({current, left_part});
            if (right_part.length > 0) {
                auto right_node = std::make_shared<RBNode>(right_part);
                right_node->left = nil_;
                right_node->right = current->right;
                right_node->parent = current;
                if (current->right != nil_) {
                    current->right->parent = right_node;
                }
                current->right = right_node;
                current->piece = left_part;
                updateNodeInfo(right_node);
            } else {
                Piece modified = current->piece;
                modified.length = current_offset;
                new_pieces.push_back({current, modified});
            }
        }

        remaining -= to_remove;
        current_offset = 0;
        if (remaining > 0) {
            current = nextInOrder(current);
        }
    }

    for (auto& node : nodes_to_modify) {
        if (node->parent) {
            std::shared_ptr<RBNode> replacement = nil_;
            if (node->left != nil_ && node->right != nil_) {
                std::shared_ptr<RBNode> successor = leftMost(node->right);
                if (successor->parent != node) {
                    successor->parent->left = successor->right;
                    if (successor->right != nil_) {
                        successor->right->parent = successor->parent;
                    }
                    successor->right = node->right;
                    successor->right->parent = successor;
                }
                successor->left = node->left;
                successor->left->parent = successor;
                successor->parent = node->parent;
                successor->color = node->color;
                replacement = successor;
            } else if (node->left != nil_) {
                replacement = node->left;
                replacement->parent = node->parent;
                replacement->color = node->color;
            } else if (node->right != nil_) {
                replacement = node->right;
                replacement->parent = node->parent;
                replacement->color = node->color;
            } else {
                replacement = nil_;
            }

            if (!node->parent) {
                root_ = replacement;
            } else if (node == node->parent->left) {
                node->parent->left = replacement;
            } else {
                node->parent->right = replacement;
            }

            if (replacement != nil_) {
                updateNodeInfo(replacement);
                std::shared_ptr<RBNode> ancestor = replacement->parent;
                while (ancestor) {
                    updateNodeInfo(ancestor);
                    ancestor = ancestor->parent;
                }
            }
        }
    }

    for (auto& [node, piece] : new_pieces) {
        node->piece = piece;
        std::shared_ptr<RBNode> current = node;
        while (current) {
            updateNodeInfo(current);
            current = current->parent;
        }
    }

    total_length_ -= length;
    lines_dirty_ = true;
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
                piece_available, (offset < buffer_available) ? (buffer_available - offset) : 0u);
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

    traverseInOrder(root_, [this, &result](const Piece& piece) {
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

    traverseInOrder(root_, [this](const Piece& piece) {
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
    if (line_num == 0 || !root_ || root_ == nil_)
        return 0;

    size_t current_line = 0;
    size_t pos = 0;
    std::shared_ptr<RBNode> current = root_;

    while (current && current != nil_ && current_line < line_num) {
        size_t left_newlines = (current->left != nil_) ? current->left->subtree_newlines : 0;

        if (current_line + left_newlines >= line_num) {
            current = current->left;
        } else {
            current_line += left_newlines;
            pos += (current->left != nil_) ? current->left->subtree_length : 0;

            const std::string& buffer = (current->piece.buffer_type == BufferType::ORIGINAL)
                                            ? original_buffer_
                                            : append_buffer_;

            size_t piece_pos = 0;
            for (size_t i = 0; i < current->piece.length && current_line < line_num; ++i) {
                if (current->piece.start + i < buffer.size() &&
                    buffer[current->piece.start + i] == '\n') {
                    current_line++;
                    if (current_line == line_num) {
                        return pos + piece_pos + 1;
                    }
                }
                piece_pos++;
            }
            pos += current->piece.length;
            current = current->right;
        }
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
    size_t current_pos = 0;
    std::shared_ptr<RBNode> current = root_;

    while (current && current != nil_ && current_pos < pos) {
        size_t left_len = (current->left != nil_) ? current->left->subtree_length : 0;

        if (current_pos + left_len >= pos) {
            current = current->left;
        } else {
            line += (current->left != nil_) ? current->left->subtree_newlines : 0;
            current_pos += left_len;

            const std::string& buffer = (current->piece.buffer_type == BufferType::ORIGINAL)
                                            ? original_buffer_
                                            : append_buffer_;

            size_t take = std::min(pos - current_pos, current->piece.length);
            for (size_t i = 0; i < take && current->piece.start + i < buffer.size(); ++i) {
                if (buffer[current->piece.start + i] == '\n') {
                    line++;
                    col = 0;
                } else {
                    col++;
                }
            }
            current_pos += take;
            current = current->right;
        }
    }

    return encodeLineCol(line, col);
}

size_t PieceTable::lineColToPosition(size_t line, size_t col) const {
    if (!root_ || root_ == nil_)
        return 0;

    size_t current_line = 0;
    size_t pos = 0;
    std::shared_ptr<RBNode> current = root_;

    while (current && current != nil_ && current_line < line) {
        size_t left_newlines = (current->left != nil_) ? current->left->subtree_newlines : 0;

        if (current_line + left_newlines >= line) {
            current = current->left;
        } else {
            current_line += left_newlines;
            pos += (current->left != nil_) ? current->left->subtree_length : 0;

            const std::string& buffer = (current->piece.buffer_type == BufferType::ORIGINAL)
                                            ? original_buffer_
                                            : append_buffer_;

            size_t piece_pos = 0;
            for (size_t i = 0;
                 i < current->piece.length && current->piece.start + i < buffer.size(); ++i) {
                if (buffer[current->piece.start + i] == '\n') {
                    current_line++;
                    if (current_line >= line) {
                        break;
                    }
                }
                piece_pos++;
            }
            pos += piece_pos;
            current = current->right;
        }
    }

    // 现在 current_line == line，向前移动 col 个字符
    while (current && current != nil_ && col > 0) {
        const std::string& buffer = (current->piece.buffer_type == BufferType::ORIGINAL)
                                        ? original_buffer_
                                        : append_buffer_;

        for (size_t i = 0;
             i < current->piece.length && current->piece.start + i < buffer.size() && col > 0;
             ++i) {
            if (buffer[current->piece.start + i] == '\n') {
                break;
            }
            pos++;
            col--;
        }
        current = current->right;
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
