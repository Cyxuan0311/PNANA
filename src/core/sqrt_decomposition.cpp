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

    size_t left = 0;
    size_t right = blocks_.size();
    while (left < right) {
        const size_t mid = left + (right - left) / 2;
        if (pos < blocks_[mid].total_length) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }

    return std::min(left, blocks_.size() - 1);
}

void SqrtDecomposition::refreshTotalsFrom(size_t start_idx) {
    if (blocks_.empty() || start_idx >= blocks_.size()) {
        return;
    }

    size_t cum_len = (start_idx == 0) ? 0 : blocks_[start_idx - 1].total_length;
    for (size_t i = start_idx; i < blocks_.size(); ++i) {
        cum_len += blocks_[i].data.size();
        blocks_[i].total_length = cum_len;
    }
}

size_t SqrtDecomposition::blockStartOffset(size_t block_idx) const {
    return (block_idx == 0) ? 0 : blocks_[block_idx - 1].total_length;
}

void SqrtDecomposition::splitBlock(size_t block_idx) {
    if (block_idx >= blocks_.size()) {
        return;
    }

    Block& block = blocks_[block_idx];
    if (block.data.size() <= block_size_ * 2) {
        return;
    }

    Block new_block;
    const size_t split_point = block.data.size() / 2;
    new_block.data.insert(new_block.data.end(), block.data.begin() + split_point, block.data.end());
    block.data.resize(split_point);

    blocks_.insert(blocks_.begin() + block_idx + 1, std::move(new_block));
    refreshTotalsFrom(block_idx);
}

void SqrtDecomposition::mergeBlocks() {
    if (blocks_.size() < 2) {
        return;
    }

    for (size_t i = 0; i + 1 < blocks_.size();) {
        const size_t combined_size = blocks_[i].data.size() + blocks_[i + 1].data.size();
        if (combined_size <= block_size_) {
            blocks_[i].data.insert(blocks_[i].data.end(), blocks_[i + 1].data.begin(),
                                   blocks_[i + 1].data.end());
            blocks_.erase(blocks_.begin() + i + 1);
        } else {
            ++i;
        }
    }

    refreshTotalsFrom(0);
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

    pos = std::min(pos, total_length_);
    recalculateBlockSize();

    if (blocks_.empty()) {
        blocks_.emplace_back(text.size());
        std::memcpy(blocks_[0].data.data(), text.data(), text.size());
        refreshTotalsFrom(0);
    } else {
        size_t block_idx = findBlock(pos);
        if (pos == total_length_ && !blocks_.empty()) {
            block_idx = blocks_.size() - 1;
        }

        Block& block = blocks_[block_idx];
        const size_t block_start = blockStartOffset(block_idx);
        const size_t offset_in_block = std::min(pos - block_start, block.data.size());

        block.data.insert(block.data.begin() + offset_in_block, text.begin(), text.end());

        if (block.data.size() > block_size_ * 2) {
            splitBlock(block_idx);
        } else {
            refreshTotalsFrom(block_idx);
        }
    }

    total_length_ += text.size();
    lines_dirty_ = true;
}

void SqrtDecomposition::remove(size_t pos, size_t length) {
    if (length == 0 || pos >= total_length_ || blocks_.empty()) {
        return;
    }

    length = std::min(length, total_length_ - pos);
    const size_t end_pos = pos + length;

    const size_t start_block = findBlock(pos);
    const size_t end_block = findBlock(std::min(end_pos, total_length_) > 0 ? end_pos - 1 : 0);

    const size_t start_block_start = blockStartOffset(start_block);
    const size_t offset_in_start = pos - start_block_start;

    if (start_block == end_block) {
        blocks_[start_block].data.erase(
            blocks_[start_block].data.begin() + offset_in_start,
            blocks_[start_block].data.begin() + offset_in_start + length);
    } else {
        const size_t end_block_start = blockStartOffset(end_block);
        const size_t offset_in_end = end_pos - end_block_start;

        blocks_[start_block].data.erase(blocks_[start_block].data.begin() + offset_in_start,
                                        blocks_[start_block].data.end());

        if (end_block < blocks_.size() && offset_in_end < blocks_[end_block].data.size()) {
            blocks_[end_block].data.erase(blocks_[end_block].data.begin(),
                                          blocks_[end_block].data.begin() + offset_in_end);
        } else if (end_block < blocks_.size()) {
            blocks_[end_block].data.clear();
        }

        if (end_block > start_block + 1) {
            blocks_.erase(blocks_.begin() + start_block + 1, blocks_.begin() + end_block);
        }

        if (start_block + 1 < blocks_.size() && !blocks_[start_block + 1].data.empty()) {
            blocks_[start_block].data.insert(blocks_[start_block].data.end(),
                                             blocks_[start_block + 1].data.begin(),
                                             blocks_[start_block + 1].data.end());
            blocks_.erase(blocks_.begin() + start_block + 1);
        }
    }

    total_length_ -= length;
    lines_dirty_ = true;

    if (blocks_.empty()) {
        blocks_.emplace_back();
    }

    mergeBlocks();
    refreshTotalsFrom(0);
}

std::string SqrtDecomposition::getText(size_t pos, size_t length) const {
    if (pos >= total_length_ || length == 0) {
        return "";
    }

    length = std::min(length, total_length_ - pos);
    std::string result;
    result.reserve(length);

    size_t remaining = length;
    size_t current_pos = pos;

    for (size_t i = 0; i < blocks_.size() && remaining > 0; ++i) {
        size_t block_start = blockStartOffset(i);
        size_t block_end = block_start + blocks_[i].data.size();

        if (current_pos < block_end && current_pos >= block_start) {
            size_t offset_in_block = current_pos - block_start;
            size_t available = blocks_[i].data.size() - offset_in_block;
            size_t take = std::min(remaining, available);

            result.append(blocks_[i].data.data() + offset_in_block, take);
            remaining -= take;
            current_pos += take;
        }
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

    for (size_t i = 0; i < blocks_.size() && current_pos < pos; ++i) {
        size_t block_start = blockStartOffset(i);
        size_t block_end = block_start + blocks_[i].data.size();

        size_t start = std::max(current_pos, block_start);
        size_t end = std::min(pos, block_end);

        for (size_t j = start - block_start; j < end - block_start && j < blocks_[i].data.size();
             ++j) {
            if (blocks_[i].data[j] == '\n') {
                line++;
                col = 0;
            } else {
                col++;
            }
        }

        current_pos = block_end;
    }

    return encodeLineCol(line, col);
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
