#include "core/sqrt_decomposition.h"
#include <algorithm>
#include <cstring>

namespace pnana {
namespace core {

SqrtDecomposition::SqrtDecomposition()
    : total_length_(0), block_size_(32), line_count_(1), lines_dirty_(true) {
    // 初始创建一个空块
    blocks_.emplace_back();
}

void SqrtDecomposition::recalculateBlockSize() {
    // 块大小 = sqrt(总长度)，最小 32，最大 4096
    if (total_length_ == 0) {
        block_size_ = 32;
    } else {
        block_size_ = std::max(
            size_t(32), std::min(size_t(4096), static_cast<size_t>(std::sqrt(total_length_))));
    }
}

size_t SqrtDecomposition::findBlock(size_t pos) const {
    if (blocks_.empty()) {
        return 0;
    }

    size_t cum_len = 0;
    for (size_t i = 0; i < blocks_.size(); ++i) {
        size_t block_len = blocks_[i].data.size();
        if (pos < cum_len + block_len) {
            return i;
        }
        cum_len += block_len;
    }

    return blocks_.size() - 1;
}

void SqrtDecomposition::splitBlock(size_t block_idx) {
    if (block_idx >= blocks_.size()) {
        return;
    }

    Block& block = blocks_[block_idx];
    if (block.data.size() <= block_size_) {
        return; // 不需要分割
    }

    // 创建新块
    Block new_block;
    size_t split_point = block.data.size() / 2;

    new_block.data.insert(new_block.data.end(), block.data.begin() + split_point, block.data.end());

    block.data.resize(split_point);

    // 更新累计长度
    size_t cum_len = 0;
    for (size_t i = 0; i <= block_idx; ++i) {
        cum_len += blocks_[i].data.size();
        blocks_[i].total_length = cum_len;
    }

    new_block.total_length = cum_len + new_block.data.size();
    blocks_.insert(blocks_.begin() + block_idx + 1, std::move(new_block));

    // 更新后续块的累计长度
    for (size_t i = block_idx + 2; i < blocks_.size(); ++i) {
        blocks_[i].total_length += new_block.data.size();
    }
}

void SqrtDecomposition::mergeBlocks() {
    if (blocks_.size() < 2) {
        return;
    }

    bool merged = true;
    while (merged) {
        merged = false;
        for (size_t i = 0; i < blocks_.size() - 1; ++i) {
            size_t combined_size = blocks_[i].data.size() + blocks_[i + 1].data.size();

            if (combined_size <= block_size_) {
                // 合并两个块
                blocks_[i].data.insert(blocks_[i].data.end(), blocks_[i + 1].data.begin(),
                                       blocks_[i + 1].data.end());
                blocks_.erase(blocks_.begin() + i + 1);

                // 更新累计长度
                size_t cum_len = 0;
                for (size_t j = i; j < blocks_.size(); ++j) {
                    cum_len += blocks_[j].data.size();
                    blocks_[j].total_length = cum_len;
                }

                merged = true;
                break;
            }
        }
    }
}

void SqrtDecomposition::recomputeLineCount() const {
    line_count_ = 1;
    for (const auto& block : blocks_) {
        for (char ch : block.data) {
            if (ch == '\n') {
                line_count_++;
            }
        }
    }
    lines_dirty_ = false;
}

size_t SqrtDecomposition::getLineStart(size_t line_num) const {
    if (line_num >= lineCount()) {
        return length();
    }

    size_t current_line = 0;
    size_t pos = 0;

    while (current_line < line_num && pos < length()) {
        char ch = getChar(pos);
        if (ch == '\n') {
            current_line++;
        }
        pos++;
    }

    return pos;
}

void SqrtDecomposition::insert(size_t pos, const std::string& text) {
    if (text.empty()) {
        return;
    }

    recalculateBlockSize();

    if (blocks_.empty()) {
        blocks_.emplace_back(text.size());
        std::memcpy(blocks_[0].data.data(), text.c_str(), text.size());
    } else {
        size_t block_idx = findBlock(pos);
        Block& block = blocks_[block_idx];

        size_t offset_in_block = pos - (block_idx > 0 ? blocks_[block_idx - 1].total_length : 0);

        // 在块内插入
        block.data.insert(block.data.begin() + offset_in_block, text.begin(), text.end());

        // 检查是否需要分割
        if (block.data.size() > block_size_ * 2) {
            splitBlock(block_idx);
        }
    }

    total_length_ += text.size();
    lines_dirty_ = true;

    // 更新累计长度
    size_t cum_len = 0;
    for (auto& block : blocks_) {
        cum_len += block.data.size();
        block.total_length = cum_len;
    }
}

void SqrtDecomposition::remove(size_t pos, size_t length) {
    if (length == 0 || pos >= total_length_) {
        return;
    }

    length = std::min(length, total_length_ - pos);

    size_t start_block = findBlock(pos);
    size_t end_block = findBlock(pos + length - 1);

    size_t offset_in_start = pos - (start_block > 0 ? blocks_[start_block - 1].total_length : 0);

    if (start_block == end_block) {
        // 在同一个块内删除
        blocks_[start_block].data.erase(
            blocks_[start_block].data.begin() + offset_in_start,
            blocks_[start_block].data.begin() + offset_in_start + length);
    } else {
        // 跨块删除
        // 删除起始块中的部分
        blocks_[start_block].data.erase(blocks_[start_block].data.begin() + offset_in_start,
                                        blocks_[start_block].data.end());

        // 删除中间的完整块
        blocks_.erase(blocks_.begin() + start_block + 1, blocks_.begin() + end_block);

        // 删除结束块中的部分
        if (start_block < blocks_.size()) {
            size_t offset_in_end =
                (pos + length) - (start_block > 0 ? blocks_[start_block - 1].total_length : 0);
            blocks_[start_block].data.erase(blocks_[start_block].data.begin(),
                                            blocks_[start_block].data.begin() + offset_in_end);
        }
    }

    total_length_ -= length;
    lines_dirty_ = true;

    // 更新累计长度
    size_t cum_len = 0;
    for (auto& block : blocks_) {
        cum_len += block.data.size();
        block.total_length = cum_len;
    }

    // 检查是否需要合并
    mergeBlocks();
}

std::string SqrtDecomposition::getText(size_t pos, size_t length) const {
    if (pos >= total_length_ || length == 0) {
        return "";
    }

    length = std::min(length, total_length_ - pos);
    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        result += getChar(pos + i);
    }

    return result;
}

std::string SqrtDecomposition::getFullText() const {
    std::string result;
    result.reserve(total_length_);

    for (const auto& block : blocks_) {
        result.insert(result.end(), block.data.begin(), block.data.end());
    }

    return result;
}

void SqrtDecomposition::insertLine(size_t line_num, const std::string& content) {
    size_t pos = getLineStart(line_num);
    std::string text = content + "\n";
    insert(pos, text);
}

void SqrtDecomposition::removeLine(size_t line_num) {
    if (line_num >= lineCount()) {
        return;
    }

    size_t start = getLineStart(line_num);
    size_t end = getLineStart(line_num + 1);

    if (end > start && end <= total_length_) {
        remove(start, end - start);
    } else if (start < total_length_) {
        remove(start, total_length_ - start);
    }
}

std::string SqrtDecomposition::getLine(size_t line_num) const {
    if (line_num >= lineCount()) {
        return "";
    }

    size_t start = getLineStart(line_num);
    size_t end = getLineStart(line_num + 1);

    if (end > start && end <= total_length_) {
        return getText(start, end - start - 1);
    } else {
        return getText(start, total_length_ - start);
    }
}

size_t SqrtDecomposition::lineCount() const {
    if (lines_dirty_) {
        recomputeLineCount();
    }
    return line_count_;
}

void SqrtDecomposition::insertChar(size_t pos, char ch) {
    insert(pos, std::string(1, ch));
}

void SqrtDecomposition::removeChar(size_t pos) {
    remove(pos, 1);
}

char SqrtDecomposition::getChar(size_t pos) const {
    if (pos >= total_length_) {
        return '\0';
    }

    size_t block_idx = findBlock(pos);
    size_t offset_in_block = pos - (block_idx > 0 ? blocks_[block_idx - 1].total_length : 0);

    return blocks_[block_idx].data[offset_in_block];
}

size_t SqrtDecomposition::length() const {
    return total_length_;
}

size_t SqrtDecomposition::lineLength(size_t line_num) const {
    if (line_num >= lineCount()) {
        return 0;
    }

    size_t start = getLineStart(line_num);
    size_t end = getLineStart(line_num + 1);

    if (end > start && end <= total_length_) {
        return end - start - 1;
    } else {
        return total_length_ - start;
    }
}

size_t SqrtDecomposition::positionToLineCol(size_t pos) const {
    if (pos > total_length_) {
        pos = total_length_;
    }

    size_t line = 0;
    size_t col = 0;
    size_t current_pos = 0;

    while (current_pos < pos && current_pos < total_length_) {
        char ch = getChar(current_pos);
        if (ch == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
        current_pos++;
    }

    return line * 1000000 + col;
}

size_t SqrtDecomposition::lineColToPosition(size_t line, size_t col) const {
    size_t current_line = 0;
    size_t current_col = 0;
    size_t pos = 0;

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

bool SqrtDecomposition::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0) {
        return true;
    }

    // 计算需要的块数
    recalculateBlockSize();
    size_t num_blocks = (size + block_size_ - 1) / block_size_;

    blocks_.clear();
    blocks_.reserve(num_blocks);

    size_t total_read = 0;
    size_t cum_len = 0;

    for (size_t i = 0; i < num_blocks; ++i) {
        size_t block_len = std::min(block_size_, static_cast<size_t>(size) - total_read);
        Block block(block_len);

        file.read(block.data.data(), block_len);
        std::streamsize bytes_read = file.gcount();

        if (bytes_read > 0) {
            block.data.resize(bytes_read);
            total_read += bytes_read;
            cum_len += bytes_read;
            block.total_length = cum_len;
            blocks_.push_back(std::move(block));
        }
    }

    total_length_ = total_read;
    lines_dirty_ = true;

    return true;
}

bool SqrtDecomposition::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& block : blocks_) {
        file.write(block.data.data(), block.data.size());
    }

    return file.good();
}

size_t SqrtDecomposition::getMemoryUsage() const {
    size_t usage = sizeof(SqrtDecomposition);
    for (const auto& block : blocks_) {
        usage += sizeof(Block) + block.data.capacity() * sizeof(char);
    }
    return usage;
}

void SqrtDecomposition::optimize() {
    recalculateBlockSize();
    mergeBlocks();

    // 如果有太多小块，重新平衡
    if (blocks_.size() > total_length_ / block_size_ * 2) {
        std::vector<char> all_data;
        all_data.reserve(total_length_);

        for (const auto& block : blocks_) {
            all_data.insert(all_data.end(), block.data.begin(), block.data.end());
        }

        blocks_.clear();
        size_t num_blocks = (total_length_ + block_size_ - 1) / block_size_;

        size_t cum_len = 0;
        for (size_t i = 0; i < num_blocks; ++i) {
            size_t start = i * block_size_;
            size_t len = std::min(block_size_, total_length_ - start);

            Block block(len);
            std::memcpy(block.data.data(), &all_data[start], len);
            cum_len += len;
            block.total_length = cum_len;
            blocks_.push_back(std::move(block));
        }
    }
}

} // namespace core
} // namespace pnana
