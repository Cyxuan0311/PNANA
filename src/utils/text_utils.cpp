#include "utils/text_utils.h"

namespace pnana {
namespace utils {

// 获取UTF-8字符的辅助函数
std::string getUtf8CharAt(const std::string& str, size_t pos) {
    if (pos >= str.length()) {
        return " ";
    }

    unsigned char first_byte = static_cast<unsigned char>(str[pos]);

    // 单字节ASCII字符
    if ((first_byte & 0x80) == 0) {
        return str.substr(pos, 1);
    }

    // 多字节UTF-8字符
    int bytes_needed;
    if ((first_byte & 0xE0) == 0xC0) {
        bytes_needed = 2;
    } else if ((first_byte & 0xF0) == 0xE0) {
        bytes_needed = 3;
    } else if ((first_byte & 0xF8) == 0xF0) {
        bytes_needed = 4;
    } else {
        // 无效的UTF-8，退回到单字节
        return str.substr(pos, 1);
    }

    // 确保有足够的字节
    if (pos + bytes_needed > str.length()) {
        return str.substr(pos, 1);
    }

    return str.substr(pos, bytes_needed);
}

// 检查字符是否为中文字符
bool isChineseChar(const std::string& ch) {
    if (ch.length() < 3) {
        return false;
    }

    // 中文UTF-8范围：E4-B8-80 到 E9-BF-BF (基本汉字)
    if (ch.length() == 3) {
        unsigned char b1 = static_cast<unsigned char>(ch[0]);
        unsigned char b2 = static_cast<unsigned char>(ch[1]);
        unsigned char b3 = static_cast<unsigned char>(ch[2]);

        // 基本检查：是否为3字节UTF-8且在中文范围内
        return (b1 >= 0xE4 && b1 <= 0xE9) && (b2 >= 0x80 && b2 <= 0xBF) &&
               (b3 >= 0x80 && b3 <= 0xBF);
    }

    return false;
}

// 计算光标位置前一个UTF-8字符的字节数
size_t getUtf8CharBytesBefore(const std::string& str, size_t pos) {
    if (pos == 0 || str.empty()) {
        return 1; // 没有前一个字符，返回1（单字节）
    }

    // 从pos-1开始向前查找UTF-8字符的起始字节
    size_t start = pos - 1;

    // 如果当前位置是ASCII字符，直接返回1
    unsigned char byte = static_cast<unsigned char>(str[start]);

    if ((byte & 0x80) == 0) {
        return 1; // ASCII字符，1字节
    }

    // 多字节UTF-8字符：需要找到字符的起始字节
    // UTF-8字符的起始字节特征：
    // - 110xxxxx (0xC0-0xDF): 2字节字符的起始
    // - 1110xxxx (0xE0-0xEF): 3字节字符的起始
    // - 11110xxx (0xF0-0xF7): 4字节字符的起始
    // 连续字节特征：10xxxxxx (0x80-0xBF)

    // 检查是否为UTF-8连续字节（10xxxxxx）
    if ((byte & 0xC0) == 0x80) {
        // 这是一个连续字节，需要向前找起始字节
        size_t char_start = start;
        while (char_start > 0) {
            unsigned char b = static_cast<unsigned char>(str[char_start]);

            // 检查是否是UTF-8字符的起始字节（不是连续字节）
            if ((b & 0xC0) != 0x80) {
                // 找到了起始字节
                break;
            }
            char_start--;
        }

        return pos - char_start;
    } else {
        // 这是一个起始字节（110xxxxx, 1110xxxx, 11110xxx）
        // 说明光标位置在字符的第一个字节之后
        unsigned char first_byte = byte;
        size_t expected_bytes = 1;
        if ((first_byte & 0xE0) == 0xC0) {
            expected_bytes = 2; // 2字节UTF-8
        } else if ((first_byte & 0xF0) == 0xE0) {
            expected_bytes = 3; // 3字节UTF-8
        } else if ((first_byte & 0xF8) == 0xF0) {
            expected_bytes = 4; // 4字节UTF-8
        }

        // 检查光标是否在完整字符之后
        if (pos >= start + expected_bytes) {
            return expected_bytes;
        } else {
            // 光标在字符中间（不应该发生）
            return 1;
        }
    }
}

// 计算光标位置后一个UTF-8字符的字节数
size_t getUtf8CharBytesAfter(const std::string& str, size_t pos) {
    if (pos >= str.length()) {
        return 1; // 没有下一个字符，返回1（单字节）
    }

    unsigned char first_byte = static_cast<unsigned char>(str[pos]);

    // ASCII字符
    if ((first_byte & 0x80) == 0) {
        return 1;
    }

    // 多字节UTF-8字符
    if ((first_byte & 0xE0) == 0xC0) {
        return 2; // 2字节UTF-8
    } else if ((first_byte & 0xF0) == 0xE0) {
        return 3; // 3字节UTF-8
    } else if ((first_byte & 0xF8) == 0xF0) {
        return 4; // 4字节UTF-8
    }

    // 无效的UTF-8，返回1
    return 1;
}

} // namespace utils
} // namespace pnana
