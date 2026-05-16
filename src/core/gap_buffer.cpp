#include "core/gap_buffer.h"
#include <algorithm>
#include <cstring>

namespace pnana {
namespace core {

GapBuffer::GapBuffer(size_t initial_gap_size)
    : gap_start_(0), gap_end_(initial_gap_size), gap_size_(initial_gap_size), line_count_(1),
      lines_dirty_(true) {
    buffer_.resize(initial_gap_size);
}

void GapBuffer::moveGap(size_t pos) {
    if (pos > length()) {
        return;
    }

    if (pos < gap_start_) {
        size_t distance = gap_start_ - pos;
        std::memmove(&buffer_[gap_end_ - distance], &buffer_[pos], distance);
        gap_start_ -= distance;
        gap_end_ -= distance;
    } else if (pos > gap_start_) {
        size_t distance = pos - gap_start_;
        std::memmove(&buffer_[gap_start_], &buffer_[gap_end_], distance);
        gap_start_ += distance;
        gap_end_ += distance;
    }
}

void GapBuffer::grow(size_t min_size) {
    size_t current_size = buffer_.size();
    size_t new_gap_size = std::max(current_size + min_size, gap_size_ * 2);
    if (new_gap_size < 1024) {
        new_gap_size = 1024;
    }

    size_t new_size = length() + new_gap_size;
    std::vector<char> new_buffer(new_size);

    std::memcpy(&new_buffer[0], &buffer_[0], gap_start_);

    size_t after_gap = current_size - gap_end_;
    if (after_gap > 0) {
        std::memcpy(&new_buffer[gap_start_ + new_gap_size], &buffer_[gap_end_], after_gap);
    }

    buffer_ = std::move(new_buffer);
    gap_end_ = gap_start_ + new_gap_size;
    gap_size_ = new_gap_size;
}

void GapBuffer::recomputeLineCount() const {
    line_count_ = 1;
    for (size_t i = 0; i < gap_start_; ++i) {
        if (buffer_[i] == '\n') {
            line_count_++;
        }
    }
    for (size_t i = gap_end_; i < buffer_.size(); ++i) {
        if (buffer_[i] == '\n') {
            line_count_++;
        }
    }
    lines_dirty_ = false;
}

size_t GapBuffer::getLineStart(size_t line_num) const {
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

void GapBuffer::insert(size_t pos, const std::string& text) {
    if (text.empty()) {
        return;
    }

    pos = std::min(pos, length());
    moveGap(pos);

    const size_t text_len = text.length();
    if (gap_size_ < text_len) {
        grow(text_len - gap_size_);
    }

    std::memcpy(&buffer_[gap_start_], text.data(), text_len);
    gap_start_ += text_len;
    gap_size_ -= text_len;
    lines_dirty_ = true;
}

void GapBuffer::remove(size_t pos, size_t len) {
    if (len == 0 || pos >= length()) {
        return;
    }

    len = std::min(len, length() - pos);
    moveGap(pos);

    // 扩展间隙以覆盖要删除的内容
    gap_end_ += len;
    gap_size_ += len;
    lines_dirty_ = true;
}

std::string GapBuffer::getText(size_t pos, size_t len) const {
    const size_t total_len = length();
    if (pos >= total_len || len == 0) {
        return "";
    }

    len = std::min(len, total_len - pos);
    std::string result;
    result.reserve(len);

    if (pos < gap_start_) {
        const size_t left_len = std::min(len, gap_start_ - pos);
        result.append(buffer_.data() + pos, left_len);

        const size_t remaining = len - left_len;
        if (remaining > 0) {
            result.append(buffer_.data() + gap_end_, remaining);
        }
        return result;
    }

    result.append(buffer_.data() + (pos + gap_size_), len);
    return result;
}

std::string GapBuffer::getFullText() const {
    std::string result;
    result.reserve(length());

    result.append(buffer_.data(), gap_start_);
    result.append(buffer_.data() + gap_end_, buffer_.size() - gap_end_);

    return result;
}

void GapBuffer::insertLine(size_t line_num, const std::string& content) {
    size_t pos = getLineStart(line_num);
    std::string text = content + "\n";
    insert(pos, text);
}

void GapBuffer::removeLine(size_t line_num) {
    if (line_num >= lineCount()) {
        return;
    }

    size_t start = getLineStart(line_num);
    size_t end = getLineStart(line_num + 1);

    // 包含换行符
    if (end > start && end <= length()) {
        remove(start, end - start);
    } else if (start < length()) {
        // 最后一行可能没有换行符
        remove(start, length() - start);
    }
}

std::string GapBuffer::getLine(size_t line_num) const {
    if (line_num >= lineCount()) {
        return "";
    }

    size_t start = getLineStart(line_num);
    size_t end = getLineStart(line_num + 1);

    if (end > start && end <= length()) {
        std::string line = getText(start, end - start - 1); // 不包含换行符
        return line;
    } else {
        return getText(start, length() - start);
    }
}

size_t GapBuffer::lineCount() const {
    if (lines_dirty_) {
        recomputeLineCount();
    }
    return line_count_;
}

void GapBuffer::insertChar(size_t pos, char ch) {
    moveGap(pos);

    if (gap_size_ < 1) {
        grow(1);
    }

    buffer_[gap_start_] = ch;
    gap_start_++;
    gap_size_--;

    if (ch == '\n') {
        line_count_++;
    }
}

void GapBuffer::removeChar(size_t pos) {
    if (pos >= length()) {
        return;
    }

    moveGap(pos);

    char ch = buffer_[gap_end_];
    gap_end_++;
    gap_size_++;

    if (ch == '\n') {
        line_count_--;
    }
}

void GapBuffer::replace(size_t pos, size_t length, const std::string& text) {
    remove(pos, length);
    insert(pos, text);
}

void GapBuffer::swapLine(size_t line_a, size_t line_b) {
    if (line_a == line_b)
        return;
    std::string content_a = getLine(line_a);
    std::string content_b = getLine(line_b);
    removeLine(line_a);
    insertLine(line_a, content_b);
    removeLine(line_b);
    insertLine(line_b, content_a);
}

char GapBuffer::getChar(size_t pos) const {
    if (pos >= length()) {
        return '\0';
    }

    if (pos < gap_start_) {
        return buffer_[pos];
    } else {
        return buffer_[pos + gap_size_];
    }
}

size_t GapBuffer::length() const {
    return buffer_.size() - gap_size_;
}

size_t GapBuffer::lineLength(size_t line_num) const {
    if (line_num >= lineCount()) {
        return 0;
    }

    size_t start = getLineStart(line_num);
    size_t end = getLineStart(line_num + 1);

    if (end > start && end <= length()) {
        return end - start - 1; // 不包含换行符
    } else {
        return length() - start;
    }
}

size_t GapBuffer::positionToLineCol(size_t pos) const {
    if (pos > length()) {
        pos = length();
    }

    size_t line = 0;
    size_t col = 0;
    size_t current_pos = 0;

    while (current_pos < pos && current_pos < length()) {
        char ch = getChar(current_pos);
        if (ch == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
        current_pos++;
    }

    return encodeLineCol(line, col);
}

size_t GapBuffer::lineColToPosition(size_t line, size_t col) const {
    size_t current_line = 0;
    size_t current_col = 0;
    size_t pos = 0;

    while (pos < length() && current_line < line) {
        if (getChar(pos) == '\n') {
            current_line++;
            current_col = 0;
        } else {
            current_col++;
        }
        pos++;
    }

    while (pos < length() && current_col < col && getChar(pos) != '\n') {
        pos++;
        current_col++;
    }

    return pos;
}

bool GapBuffer::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0) {
        // 空文件
        return true;
    }

    // 确保有足够的空间
    if (gap_size_ < static_cast<size_t>(size)) {
        grow(size - gap_size_);
    }

    // 读取文件内容到间隙位置
    file.read(&buffer_[gap_start_], size);
    std::streamsize bytes_read = file.gcount();

    gap_start_ += bytes_read;
    gap_size_ -= bytes_read;
    lines_dirty_ = true;

    return true;
}

bool GapBuffer::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(&buffer_[0], gap_start_);

    size_t after_gap = buffer_.size() - gap_end_;
    if (after_gap > 0) {
        file.write(&buffer_[gap_end_], after_gap);
    }

    return file.good();
}

size_t GapBuffer::getMemoryUsage() const {
    return sizeof(GapBuffer) + buffer_.capacity() * sizeof(char);
}

void GapBuffer::optimize() {
    // 如果间隙过大，缩小它
    if (gap_size_ > 4096 && gap_size_ > length() / 2) {
        size_t new_gap_size = std::max(size_t(1024), length() / 4);
        std::vector<char> new_buffer(length() + new_gap_size);

        // 复制数据
        std::memcpy(&new_buffer[0], &buffer_[0], gap_start_);
        size_t after_gap = buffer_.size() - gap_end_;
        if (after_gap > 0) {
            std::memcpy(&new_buffer[length() + new_gap_size - after_gap], &buffer_[gap_end_],
                        after_gap);
        }

        buffer_ = std::move(new_buffer);
        gap_end_ = gap_start_ + new_gap_size;
        gap_size_ = new_gap_size;
    }
}

} // namespace core
} // namespace pnana
