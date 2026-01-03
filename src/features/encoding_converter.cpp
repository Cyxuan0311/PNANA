#include "features/encoding_converter.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

namespace pnana {
namespace features {

std::vector<std::string> EncodingConverter::getSupportedEncodings() {
    return {"UTF-8",  "UTF-16", "UTF-16LE",   "UTF-16BE",    "GBK",
            "GB2312", "ASCII",  "ISO-8859-1", "Windows-1252"};
}

bool EncodingConverter::isEncodingSupported(const std::string& encoding) {
    auto encodings = getSupportedEncodings();
    std::string upper_encoding = encoding;
    std::transform(upper_encoding.begin(), upper_encoding.end(), upper_encoding.begin(), ::toupper);

    for (const auto& enc : encodings) {
        std::string upper_enc = enc;
        std::transform(upper_enc.begin(), upper_enc.end(), upper_enc.begin(), ::toupper);
        if (upper_enc == upper_encoding) {
            return true;
        }
    }
    return false;
}

std::vector<uint8_t> EncodingConverter::readFileAsBytes(const std::string& filepath) {
    std::vector<uint8_t> content;
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return content;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    content.resize(size);
    file.read(reinterpret_cast<char*>(content.data()), size);
    file.close();

    return content;
}

std::string EncodingConverter::detectFileEncoding(const std::string& filepath) {
    auto bytes = readFileAsBytes(filepath);
    if (bytes.empty()) {
        return "UTF-8"; // 默认编码
    }

    // 检测UTF-8 BOM
    if (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return "UTF-8";
    }

    // 检测UTF-16LE BOM
    if (bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return "UTF-16LE";
    }

    // 检测UTF-16BE BOM
    if (bytes.size() >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return "UTF-16BE";
    }

    // 简单检测：检查是否为有效的UTF-8
    bool is_valid_utf8 = true;
    for (size_t i = 0; i < bytes.size() && i < 1024; ++i) { // 只检查前1KB
        uint8_t byte = bytes[i];
        if (byte > 0x7F) { // 非ASCII字符
            // 检查UTF-8序列
            if ((byte & 0xE0) == 0xC0) { // 2字节序列
                if (i + 1 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80) {
                    is_valid_utf8 = false;
                    break;
                }
                i++;
            } else if ((byte & 0xF0) == 0xE0) { // 3字节序列
                if (i + 2 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80 ||
                    (bytes[i + 2] & 0xC0) != 0x80) {
                    is_valid_utf8 = false;
                    break;
                }
                i += 2;
            } else if ((byte & 0xF8) == 0xF0) { // 4字节序列
                if (i + 3 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80 ||
                    (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) {
                    is_valid_utf8 = false;
                    break;
                }
                i += 3;
            } else if ((byte & 0x80) != 0) { // 无效的UTF-8字节
                is_valid_utf8 = false;
                break;
            }
        }
    }

    if (is_valid_utf8) {
        return "UTF-8";
    }

    // 检查是否可能是GBK/GB2312（包含中文字符的常见编码）
    bool has_chinese = false;
    for (size_t i = 0; i < bytes.size() && i < 1024; ++i) {
        if (bytes[i] >= 0x81 && bytes[i] <= 0xFE) {
            if (i + 1 < bytes.size() && bytes[i + 1] >= 0x40 && bytes[i + 1] <= 0xFE) {
                has_chinese = true;
                break;
            }
        }
    }

    if (has_chinese) {
        return "GBK"; // 默认使用GBK
    }

    return "UTF-8"; // 默认编码
}

std::string EncodingConverter::encodingToUtf8(const std::vector<uint8_t>& content,
                                              const std::string& source_encoding) {
    if (content.empty()) {
        return "";
    }

    std::string upper_encoding = source_encoding;
    std::transform(upper_encoding.begin(), upper_encoding.end(), upper_encoding.begin(), ::toupper);

    if (upper_encoding == "UTF-8") {
        // 已经是UTF-8，直接转换
        return std::string(reinterpret_cast<const char*>(content.data()), content.size());
    } else if (upper_encoding == "GBK") {
        return convertGBKToUtf8(content);
    } else if (upper_encoding == "GB2312") {
        return convertGB2312ToUtf8(content);
    } else if (upper_encoding == "ASCII" || upper_encoding == "ISO-8859-1" ||
               upper_encoding == "WINDOWS-1252") {
        return convertLatin1ToUtf8(content);
    } else if (upper_encoding == "UTF-16LE" || upper_encoding == "UTF-16") {
        // UTF-16LE转换（简化实现）
        // 注意：codecvt在C++17中已废弃，这里使用简化实现
        // 实际项目中应该使用更现代的转换方法或第三方库
        if (content.size() < 2) {
            return "";
        }
        // 简化处理：只处理基本ASCII字符
        std::string result;
        for (size_t i = 0; i < content.size() - 1; i += 2) {
            uint16_t code_unit = content[i] | (content[i + 1] << 8);
            if (code_unit < 0x80) {
                result += static_cast<char>(code_unit);
            } else {
                // 非ASCII字符，简化处理
                result += '?';
            }
        }
        return result;
    } else if (upper_encoding == "UTF-16BE") {
        // UTF-16BE转换（需要字节交换）
        if (content.size() < 2) {
            return "";
        }
        std::string result;
        for (size_t i = 0; i < content.size() - 1; i += 2) {
            uint16_t code_unit = (content[i] << 8) | content[i + 1];
            if (code_unit < 0x80) {
                result += static_cast<char>(code_unit);
            } else {
                // 非ASCII字符，简化处理
                result += '?';
            }
        }
        return result;
    } else {
        // 未知编码，尝试作为UTF-8处理
        return std::string(reinterpret_cast<const char*>(content.data()), content.size());
    }
}

std::vector<uint8_t> EncodingConverter::utf8ToEncoding(const std::string& utf8_content,
                                                       const std::string& target_encoding) {
    std::string upper_encoding = target_encoding;
    std::transform(upper_encoding.begin(), upper_encoding.end(), upper_encoding.begin(), ::toupper);

    if (upper_encoding == "UTF-8") {
        std::vector<uint8_t> result(utf8_content.begin(), utf8_content.end());
        return result;
    } else if (upper_encoding == "GBK") {
        std::string gbk = convertUtf8ToGBK(utf8_content);
        return std::vector<uint8_t>(gbk.begin(), gbk.end());
    } else if (upper_encoding == "GB2312") {
        std::string gb2312 = convertUtf8ToGB2312(utf8_content);
        return std::vector<uint8_t>(gb2312.begin(), gb2312.end());
    } else if (upper_encoding == "ASCII" || upper_encoding == "ISO-8859-1" ||
               upper_encoding == "WINDOWS-1252") {
        std::string latin1 = convertUtf8ToLatin1(utf8_content);
        return std::vector<uint8_t>(latin1.begin(), latin1.end());
    } else if (upper_encoding == "UTF-16LE" || upper_encoding == "UTF-16") {
        // UTF-16LE转换（简化实现）
        std::vector<uint8_t> result;
        for (char c : utf8_content) {
            if (static_cast<unsigned char>(c) < 0x80) {
                // ASCII字符，直接转换
                result.push_back(static_cast<uint8_t>(c));
                result.push_back(0);
            } else {
                // 非ASCII字符，简化处理
                result.push_back('?');
                result.push_back(0);
            }
        }
        return result;
    } else {
        // 未知编码，返回UTF-8字节
        return std::vector<uint8_t>(utf8_content.begin(), utf8_content.end());
    }
}

std::string EncodingConverter::convertEncoding(const std::string& from_encoding,
                                               const std::string& to_encoding,
                                               const std::vector<uint8_t>& content) {
    // 先转换为UTF-8，再转换为目标编码
    std::string utf8_content = encodingToUtf8(content, from_encoding);
    std::vector<uint8_t> result = utf8ToEncoding(utf8_content, to_encoding);
    return std::string(reinterpret_cast<const char*>(result.data()), result.size());
}

bool EncodingConverter::writeFileWithEncoding(const std::string& filepath,
                                              const std::string& encoding,
                                              const std::string& content) {
    std::vector<uint8_t> bytes = utf8ToEncoding(content, encoding);

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.close();

    return file.good();
}

// GBK/GB2312转换的简化实现
// 注意：这是一个简化版本，完整的GBK/GB2312转换需要完整的码表
// 这里使用系统locale进行转换（如果可用）

std::string EncodingConverter::convertGBKToUtf8(const std::vector<uint8_t>& gbk_content) {
    // 简化实现：对于GBK/GB2312转换，这是一个占位实现
    // 完整的GBK/GB2312转换需要完整的码表或使用第三方库（如iconv）
    // 这里返回原始内容，实际项目中应该使用iconv或类似的库

    // 注意：这是一个简化版本，实际使用时可能需要：
    // 1. 使用iconv库进行转换
    // 2. 或者使用完整的GBK/GB2312码表
    // 3. 或者依赖系统locale（如果可用）

    // 暂时返回原始内容（可能导致乱码，但不会崩溃）
    return std::string(reinterpret_cast<const char*>(gbk_content.data()), gbk_content.size());
}

std::string EncodingConverter::convertUtf8ToGBK(const std::string& utf8_content) {
    // 简化实现：对于GBK/GB2312转换，这是一个占位实现
    // 完整的GBK/GB2312转换需要完整的码表或使用第三方库（如iconv）
    // 这里返回原始内容，实际项目中应该使用iconv或类似的库

    // 暂时返回原始内容（可能导致乱码，但不会崩溃）
    return utf8_content;
}

std::string EncodingConverter::convertGB2312ToUtf8(const std::vector<uint8_t>& gb2312_content) {
    // GB2312是GBK的子集，可以使用类似的方法
    return convertGBKToUtf8(gb2312_content);
}

std::string EncodingConverter::convertUtf8ToGB2312(const std::string& utf8_content) {
    // GB2312是GBK的子集，可以使用类似的方法
    return convertUtf8ToGBK(utf8_content);
}

std::string EncodingConverter::convertLatin1ToUtf8(const std::vector<uint8_t>& latin1_content) {
    // Latin1/ISO-8859-1: 每个字节直接映射到Unicode
    std::string result;
    result.reserve(latin1_content.size());

    for (uint8_t byte : latin1_content) {
        if (byte < 0x80) {
            // ASCII字符，直接添加
            result += static_cast<char>(byte);
        } else {
            // 扩展字符，转换为UTF-8（2字节）
            result += static_cast<char>(0xC0 | (byte >> 6));
            result += static_cast<char>(0x80 | (byte & 0x3F));
        }
    }

    return result;
}

std::string EncodingConverter::convertUtf8ToLatin1(const std::string& utf8_content) {
    // 将UTF-8转换为Latin1（只保留0-255范围的字符）
    std::string result;
    result.reserve(utf8_content.size());

    for (size_t i = 0; i < utf8_content.size(); ++i) {
        unsigned char byte = static_cast<unsigned char>(utf8_content[i]);

        if (byte < 0x80) {
            // ASCII字符
            result += static_cast<char>(byte);
        } else if ((byte & 0xE0) == 0xC0 && i + 1 < utf8_content.size()) {
            // 2字节UTF-8序列，转换为Latin1
            unsigned char byte2 = static_cast<unsigned char>(utf8_content[i + 1]);
            unsigned char latin1_char = ((byte & 0x1F) << 6) | (byte2 & 0x3F);
            if (latin1_char < 0x80) {
                // 超出Latin1范围，跳过或替换为'?'
                result += '?';
            } else {
                result += static_cast<char>(latin1_char);
            }
            i++; // 跳过第二个字节
        } else {
            // 多字节序列或其他，替换为'?'
            result += '?';
        }
    }

    return result;
}

} // namespace features
} // namespace pnana
