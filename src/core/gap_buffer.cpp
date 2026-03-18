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
        // 向左移动间隙
        size_t distance = gap_start_ - pos;
        std::memmove(&buffer_[gap_end_ - distance], &buffer_[pos], distance);
        gap_start_ -= distance;
        gap_end_ -= distance;
    } else if (pos > gap_start_) {
        // 向右移动间隙
        size_t distance = pos - gap_start_;
        std::memmove(&buffer_[gap_start_], &buffer_[gap_end_], distance);
        gap_start_ += distance;
        gap_end_ += distance;
    }
}

void GapBuffer::grow(size_t min_size) {
    size_t current_size = buffer_.size();
    size_t new_size = std::max(current_size * 2, current_size + min_size);
    if (new_size < 1024) {
        new_size = 1024;
    }

    std::vector<char> new_buffer(new_size);

    // 复制间隙前的数据
    std::memcpy(&new_buffer[0], &buffer_[0], gap_start_);

    // 复制间隙后的数据
    size_t after_gap = current_size - gap_end_;
    if (after_gap > 0) {
        std::memcpy(&new_buffer[new_size - after_gap], &buffer_[gap_end_], after_gap);
    }

    buffer_ = std::move(new_buffer);
    gap_end_ = new_size - after_gap;
    gap_size_ = gap_end_ - gap_start_;
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

    moveGap(pos);

    size_t text_len = text.length();
    if (gap_size_ < text_len) {
        grow(text_len - gap_size_);
    }

    std::memcpy(&buffer_[gap_start_], text.c_str(), text_len);
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
    if (pos >= length() || len == 0) {
        return "";
    }

    len = std::min(len, length() - pos);
    std::string result;
    result.reserve(len);

    for (size_t i = 0; i < len; ++i) {
        result += getChar(pos + i);
    }

    return result;
}

std::string GapBuffer::getFullText() const {
    std::string result;
    result.reserve(length());

    for (size_t i = 0; i < gap_start_; ++i) {
        result += buffer_[i];
    }
    for (size_t i = gap_end_; i < buffer_.size(); ++i) {
        if (buffer_[i] == '\0') {
            break;
        }
        result += buffer_[i];
    }

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

    return line * 1000000 + col; // 编码为 line * 1000000 + col
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

    // 现在在正确的行上，移动到正确的列
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

    // 写入间隙前的数据
    file.write(&buffer_[0], gap_start_);

    // 写入间隙后的数据
    for (size_t i = gap_end_; i < buffer_.size(); ++i) {
        if (buffer_[i] == '\0') {
            break;
        }
        file.put(buffer_[i]);
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
