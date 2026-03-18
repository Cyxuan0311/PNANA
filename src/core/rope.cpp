#include "core/rope.h"
#include <algorithm>
#include <sstream>

namespace pnana {
namespace core {

Rope::Rope() : line_count_(1), lines_dirty_(true) {
    root_ = std::make_shared<RopeNode>();
}

std::shared_ptr<Rope::RopeNode> Rope::concatenate(std::shared_ptr<RopeNode> left,
                                                  std::shared_ptr<RopeNode> right) {
    if (!left || left->length == 0)
        return right;
    if (!right || right->length == 0)
        return left;

    auto parent = std::make_shared<RopeNode>();
    parent->left = left;
    parent->right = right;
    parent->updateLength();

    return parent;
}

std::pair<std::shared_ptr<Rope::RopeNode>, std::shared_ptr<Rope::RopeNode>> Rope::split(
    std::shared_ptr<RopeNode> node, size_t pos) {
    if (!node || node->length == 0) {
        return {nullptr, nullptr};
    }

    if (node->isLeaf()) {
        if (pos == 0) {
            return {nullptr, node};
        }
        if (pos >= node->text.size()) {
            return {node, nullptr};
        }

        auto left = std::make_shared<RopeNode>(node->text.substr(0, pos));
        auto right = std::make_shared<RopeNode>(node->text.substr(pos));
        return {left, right};
    }

    size_t left_len = node->left ? node->left->length : 0;

    if (pos <= left_len) {
        auto [ll, lr] = split(node->left, pos);
        return {ll, concatenate(lr, node->right)};
    } else {
        auto [rl, rr] = split(node->right, pos - left_len);
        return {concatenate(node->left, rl), rr};
    }
}

std::shared_ptr<Rope::RopeNode> Rope::insertAt(std::shared_ptr<RopeNode> node, size_t pos,
                                               const std::string& text) {
    if (text.empty())
        return node;

    if (!node || node->length == 0) {
        return std::make_shared<RopeNode>(text);
    }

    auto [left, right] = split(node, pos);
    auto middle = std::make_shared<RopeNode>(text);

    return concatenate(concatenate(left, middle), right);
}

std::shared_ptr<Rope::RopeNode> Rope::removeRange(std::shared_ptr<RopeNode> node, size_t start,
                                                  size_t end) {
    if (!node || node->length == 0 || start >= end)
        return node;

    auto [left, temp] = split(node, start);
    auto [mid, right] = split(temp, end - start);

    return concatenate(left, right);
}

char Rope::charAt(std::shared_ptr<RopeNode> node, size_t pos) {
    if (!node || pos >= node->length)
        return '\0';

    if (node->isLeaf()) {
        return node->text[pos];
    }

    size_t left_len = node->left ? node->left->length : 0;

    if (pos < left_len) {
        return charAt(node->left, pos);
    } else {
        return charAt(node->right, pos - left_len);
    }
}

// const 版本
char Rope::charAt(std::shared_ptr<RopeNode> node, size_t pos) const {
    if (!node || pos >= node->length)
        return '\0';

    if (node->isLeaf()) {
        return node->text[pos];
    }

    size_t left_len = node->left ? node->left->length : 0;

    if (pos < left_len) {
        return charAt(node->left, pos);
    } else {
        return charAt(node->right, pos - left_len);
    }
}

std::string Rope::substring(std::shared_ptr<RopeNode> node, size_t start, size_t len) {
    if (!node || start >= node->length || len == 0)
        return "";

    len = std::min(len, node->length - start);
    std::string result;
    result.reserve(len);

    if (node->isLeaf()) {
        return node->text.substr(start, len);
    }

    size_t left_len = node->left ? node->left->length : 0;

    if (start < left_len) {
        size_t left_part = std::min(len, left_len - start);
        result += substring(node->left, start, left_part);
        if (left_part < len) {
            result += substring(node->right, 0, len - left_part);
        }
    } else {
        result += substring(node->right, start - left_len, len);
    }

    return result;
}

std::string Rope::substringConst(std::shared_ptr<RopeNode> node, size_t start, size_t len) const {
    if (!node || start >= node->length || len == 0)
        return "";

    len = std::min(len, node->length - start);
    std::string result;
    result.reserve(len);

    if (node->isLeaf()) {
        return node->text.substr(start, len);
    }

    size_t left_len = node->left ? node->left->length : 0;

    if (start < left_len) {
        size_t left_part = std::min(len, left_len - start);
        result += substringConst(node->left, start, left_part);
        if (left_part < len) {
            result += substringConst(node->right, 0, len - left_part);
        }
    } else {
        result += substringConst(node->right, start - left_len, len);
    }

    return result;
}

void Rope::collectText(std::shared_ptr<RopeNode> node, std::string& result) const {
    if (!node || node->length == 0)
        return;

    if (node->isLeaf()) {
        result += node->text;
        return;
    }

    collectText(node->left, result);
    collectText(node->right, result);
}

size_t Rope::findLineStart(std::shared_ptr<RopeNode> node, size_t line_num) const {
    if (!node || line_num == 0)
        return 0;

    size_t current_line = 0;
    size_t pos = 0;

    while (current_line < line_num && pos < node->length) {
        if (charAt(node, pos) == '\n') {
            current_line++;
        }
        pos++;
    }

    return pos;
}

void Rope::insert(size_t pos, const std::string& text) {
    if (text.empty())
        return;

    root_ = insertAt(root_, pos, text);
    lines_dirty_ = true;
}

void Rope::remove(size_t pos, size_t length) {
    if (length == 0 || !root_)
        return;

    length = std::min(length, root_->length - pos);
    root_ = removeRange(root_, pos, pos + length);
    lines_dirty_ = true;
}

std::string Rope::getText(size_t pos, size_t length) const {
    if (!root_ || pos >= root_->length || length == 0)
        return "";

    length = std::min(length, root_->length - pos);
    return substringConst(root_, pos, length);
}

std::string Rope::getFullText() const {
    if (!root_ || root_->length == 0)
        return "";

    std::string result;
    result.reserve(root_->length);
    collectText(root_, result);
    return result;
}

void Rope::insertLine(size_t line_num, const std::string& content) {
    size_t pos = findLineStart(root_, line_num);
    insert(pos, content + "\n");
}

void Rope::removeLine(size_t line_num) {
    if (!root_ || line_num >= lineCount())
        return;

    size_t start = findLineStart(root_, line_num);
    size_t end = findLineStart(root_, line_num + 1);

    if (end > start) {
        remove(start, end - start);
    }
}

std::string Rope::getLine(size_t line_num) const {
    if (!root_ || line_num >= lineCount())
        return "";

    size_t start = findLineStart(root_, line_num);
    size_t end = findLineStart(root_, line_num + 1);

    if (end > start && end <= root_->length) {
        std::string line = substringConst(root_, start, end - start - 1);
        return line;
    }

    return substringConst(root_, start, root_->length - start);
}

size_t Rope::lineCount() const {
    if (lines_dirty_) {
        recomputeLineCount();
    }
    return line_count_;
}

void Rope::insertChar(size_t pos, char ch) {
    insert(pos, std::string(1, ch));
}

void Rope::removeChar(size_t pos) {
    remove(pos, 1);
}

char Rope::getChar(size_t pos) const {
    return charAt(root_, pos);
}

size_t Rope::length() const {
    return root_ ? root_->length : 0;
}

size_t Rope::lineLength(size_t line_num) const {
    if (!root_ || line_num >= lineCount())
        return 0;

    size_t start = findLineStart(root_, line_num);
    size_t end = findLineStart(root_, line_num + 1);

    if (end > start && end <= root_->length) {
        return end - start - 1;
    }

    return root_->length - start;
}

size_t Rope::positionToLineCol(size_t pos) const {
    if (!root_ || pos > root_->length)
        pos = root_->length;

    size_t line = 0, col = 0;
    for (size_t i = 0; i < pos && i < root_->length; ++i) {
        if (charAt(root_, i) == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }

    return line * 1000000 + col;
}

size_t Rope::lineColToPosition(size_t line, size_t col) const {
    if (!root_)
        return 0;

    size_t current_line = 0, current_col = 0, pos = 0;

    while (pos < root_->length && current_line < line) {
        if (charAt(root_, pos) == '\n') {
            current_line++;
            current_col = 0;
        } else {
            current_col++;
        }
        pos++;
    }

    while (pos < root_->length && current_col < col && charAt(root_, pos) != '\n') {
        pos++;
        current_col++;
    }

    return pos;
}

bool Rope::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0) {
        root_ = std::make_shared<RopeNode>();
        return true;
    }

    // 分块读取文件
    std::vector<std::shared_ptr<RopeNode>> leaves;
    const size_t chunk_size = LEAF_MAX_SIZE;

    std::string buffer(chunk_size, '\0');
    while (file) {
        file.read(&buffer[0], chunk_size);
        std::streamsize bytes_read = file.gcount();

        if (bytes_read > 0) {
            buffer.resize(bytes_read);
            leaves.push_back(std::make_shared<RopeNode>(buffer));
            buffer.resize(chunk_size);
        }
    }

    // 构建平衡树
    if (leaves.empty()) {
        root_ = std::make_shared<RopeNode>();
    } else if (leaves.size() == 1) {
        root_ = leaves[0];
    } else {
        std::vector<std::shared_ptr<RopeNode>> current_level = leaves;

        while (current_level.size() > 1) {
            std::vector<std::shared_ptr<RopeNode>> next_level;

            for (size_t i = 0; i < current_level.size(); i += 2) {
                if (i + 1 < current_level.size()) {
                    next_level.push_back(concatenate(current_level[i], current_level[i + 1]));
                } else {
                    next_level.push_back(current_level[i]);
                }
            }

            current_level = std::move(next_level);
        }

        root_ = current_level[0];
    }

    lines_dirty_ = true;
    return true;
}

bool Rope::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string text = getFullText();
    file.write(text.c_str(), text.size());

    return file.good();
}

size_t Rope::getMemoryUsage() const {
    // 简化计算
    return sizeof(Rope) + (root_ ? root_->length : 0) * 2; // 假设 2 倍开销
}

void Rope::optimize() {
    // Rope 本身是自平衡的，这里可以添加重新平衡逻辑
    // 当前实现已经是相对平衡的
}

} // namespace core
} // namespace pnana
