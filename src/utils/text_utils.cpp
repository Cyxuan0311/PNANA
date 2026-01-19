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

} // namespace utils
} // namespace pnana
