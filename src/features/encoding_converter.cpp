#include "features/encoding_converter.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

#ifdef BUILD_ICONV_SUPPORT
#include <cerrno>
#include <iconv.h>
#endif

namespace pnana {
namespace features {

#ifdef BUILD_ICONV_SUPPORT
// 将编码名称映射到iconv支持的名称
static std::string mapEncodingToIconv(const std::string& encoding) {
    std::string upper = encoding;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    // iconv编码名称映射
    if (upper == "UTF-8")
        return "UTF-8";
    if (upper == "UTF-16")
        return "UTF-16";
    if (upper == "UTF-16LE")
        return "UTF-16LE";
    if (upper == "UTF-16BE")
        return "UTF-16BE";
    if (upper == "GBK")
        return "GBK";
    if (upper == "GB2312")
        return "GB2312";
    if (upper == "ASCII")
        return "ASCII";
    if (upper == "ISO-8859-1")
        return "ISO-8859-1";
    if (upper == "WINDOWS-1252")
        return "WINDOWS-1252";

    // 如果无法映射，返回原始名称（让iconv尝试）
    return encoding;
}

// 使用iconv进行编码转换的辅助函数
static std::string convertWithIconv(const std::vector<uint8_t>& input,
                                    const std::string& from_encoding,
                                    const std::string& to_encoding) {
    if (input.empty()) {
        return "";
    }

    // 映射编码名称
    std::string iconv_from = mapEncodingToIconv(from_encoding);
    std::string iconv_to = mapEncodingToIconv(to_encoding);

    // 打开转换描述符
    iconv_t cd = iconv_open(iconv_to.c_str(), iconv_from.c_str());
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        // iconv转换失败，返回空字符串（调用者会使用兜底方案）
        return "";
    }

    // 准备输入缓冲区
    size_t inbytesleft = input.size();
    char* inbuf = const_cast<char*>(reinterpret_cast<const char*>(input.data()));

    // 准备输出缓冲区（初始大小为输入的4倍，以应对可能的扩展）
    size_t outbuf_size = input.size() * 4;
    if (outbuf_size == 0) {
        outbuf_size = 256; // 最小缓冲区大小
    }
    std::vector<char> outbuf(outbuf_size);
    char* outptr = outbuf.data();
    size_t outbytesleft = outbuf_size;

    // 执行转换
    size_t result = iconv(cd, &inbuf, &inbytesleft, &outptr, &outbytesleft);

    // 如果输出缓冲区不足（E2BIG），尝试扩大缓冲区并重试
    if (result == static_cast<size_t>(-1) && errno == E2BIG) {
        // 保存已转换的数据
        size_t converted_so_far = outbuf_size - outbytesleft;
        std::vector<char> temp(outbuf.data(), outbuf.data() + converted_so_far);

        // 重新分配更大的缓冲区
        size_t new_size = outbuf_size * 2;
        outbuf.resize(new_size);
        outptr = outbuf.data() + converted_so_far;
        outbytesleft = new_size - converted_so_far;

        // 重试转换（继续从上次停止的地方）
        result = iconv(cd, &inbuf, &inbytesleft, &outptr, &outbytesleft);

        if (result == static_cast<size_t>(-1) && errno == E2BIG) {
            // 仍然不足，返回空字符串，使用兜底方案
            iconv_close(cd);
            return "";
        }
    }

    iconv_close(cd);

    if (result == static_cast<size_t>(-1)) {
        // 转换失败，返回空字符串，让调用者使用兜底方案
        return "";
    }

    // 返回转换后的字符串
    size_t converted_size = outbuf_size - outbytesleft;
    if (converted_size == 0) {
        return "";
    }
    return std::string(outbuf.data(), converted_size);
}

// 使用iconv将UTF-8转换为目标编码
static std::vector<uint8_t> convertFromUtf8WithIconv(const std::string& utf8_input,
                                                     const std::string& to_encoding) {
    if (utf8_input.empty()) {
        return std::vector<uint8_t>();
    }

    // 映射编码名称
    std::string iconv_to = mapEncodingToIconv(to_encoding);

    // 打开转换描述符
    iconv_t cd = iconv_open(iconv_to.c_str(), "UTF-8");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return std::vector<uint8_t>();
    }

    // 准备输入缓冲区
    size_t inbytesleft = utf8_input.size();
    char* inbuf = const_cast<char*>(utf8_input.data());

    // 准备输出缓冲区（初始大小为输入的2倍）
    size_t outbuf_size = utf8_input.size() * 2;
    if (outbuf_size == 0) {
        outbuf_size = 256; // 最小缓冲区大小
    }
    std::vector<char> outbuf(outbuf_size);
    char* outptr = outbuf.data();
    size_t outbytesleft = outbuf_size;

    // 执行转换
    size_t result = iconv(cd, &inbuf, &inbytesleft, &outptr, &outbytesleft);

    // 如果输出缓冲区不足（E2BIG），尝试扩大缓冲区并重试
    if (result == static_cast<size_t>(-1) && errno == E2BIG) {
        // 保存已转换的数据
        size_t converted_so_far = outbuf_size - outbytesleft;

        // 重新分配更大的缓冲区
        size_t new_size = outbuf_size * 2;
        outbuf.resize(new_size);
        outptr = outbuf.data() + converted_so_far;
        outbytesleft = new_size - converted_so_far;

        // 重试转换（继续从上次停止的地方）
        result = iconv(cd, &inbuf, &inbytesleft, &outptr, &outbytesleft);

        if (result == static_cast<size_t>(-1) && errno == E2BIG) {
            // 仍然不足，返回空向量，使用兜底方案
            iconv_close(cd);
            return std::vector<uint8_t>();
        }
    }

    iconv_close(cd);

    if (result == static_cast<size_t>(-1)) {
        // 转换失败，返回空向量，让调用者使用兜底方案
        return std::vector<uint8_t>();
    }

    // 返回转换后的字节
    size_t converted_size = outbuf_size - outbytesleft;
    if (converted_size == 0) {
        return std::vector<uint8_t>();
    }
    return std::vector<uint8_t>(outbuf.data(), outbuf.data() + converted_size);
}
#endif // BUILD_ICONV_SUPPORT

// UTF-16转换辅助函数的前向声明
static std::string convertUtf16LeToUtf8(const std::vector<uint8_t>& content);
static std::string convertUtf16BeToUtf8(const std::vector<uint8_t>& content);
static std::vector<uint8_t> convertUtf8ToUtf16Le(const std::string& utf8_content);
static std::vector<uint8_t> convertUtf8ToUtf16Be(const std::string& utf8_content);

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

    // 检测BOM标记（优先级最高）
    if (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return "UTF-8";
    }

    // 检测UTF-16LE BOM
    if (bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        // 检查是否是UTF-32LE（前4字节是FF FE 00 00）
        if (bytes.size() >= 4 && bytes[2] == 0x00 && bytes[3] == 0x00) {
            return "UTF-32LE";
        }
        return "UTF-16LE";
    }

    // 检测UTF-16BE BOM
    if (bytes.size() >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        // 检查是否是UTF-32BE（前4字节是00 00 FE FF）
        if (bytes.size() >= 4 && bytes[2] == 0x00 && bytes[3] == 0x00) {
            return "UTF-32BE";
        }
        return "UTF-16BE";
    }

    // 检测UTF-32LE BOM (00 00 FE FF)
    if (bytes.size() >= 4 && bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0xFE &&
        bytes[3] == 0xFF) {
        return "UTF-32LE";
    }

    // 检测UTF-32BE BOM (FF FE 00 00)
    if (bytes.size() >= 4 && bytes[0] == 0xFF && bytes[1] == 0xFE && bytes[2] == 0x00 &&
        bytes[3] == 0x00) {
        return "UTF-32BE";
    }

    // 改进的UTF-8验证：检查更多字节以提高准确性
    bool is_valid_utf8 = true;
    size_t check_size = std::min(bytes.size(), size_t(4096)); // 检查前4KB
    size_t utf8_char_count = 0;
    size_t non_ascii_count = 0;

    for (size_t i = 0; i < check_size; ++i) {
        uint8_t byte = bytes[i];
        if (byte > 0x7F) { // 非ASCII字符
            non_ascii_count++;
            // 检查UTF-8序列
            if ((byte & 0xE0) == 0xC0) { // 2字节序列
                if (i + 1 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80) {
                    is_valid_utf8 = false;
                    break;
                }
                utf8_char_count++;
                i++;
            } else if ((byte & 0xF0) == 0xE0) { // 3字节序列
                if (i + 2 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80 ||
                    (bytes[i + 2] & 0xC0) != 0x80) {
                    is_valid_utf8 = false;
                    break;
                }
                utf8_char_count++;
                i += 2;
            } else if ((byte & 0xF8) == 0xF0) { // 4字节序列
                if (i + 3 >= bytes.size() || (bytes[i + 1] & 0xC0) != 0x80 ||
                    (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) {
                    is_valid_utf8 = false;
                    break;
                }
                utf8_char_count++;
                i += 3;
            } else if ((byte & 0x80) != 0) { // 无效的UTF-8字节
                is_valid_utf8 = false;
                break;
            }
        } else {
            utf8_char_count++; // ASCII字符
        }
    }

    // 如果所有检查的字节都是有效的UTF-8，且有一定数量的非ASCII字符，很可能是UTF-8
    if (is_valid_utf8 && (non_ascii_count == 0 || non_ascii_count > 0)) {
        return "UTF-8";
    }

    // 检查是否可能是GBK/GB2312（包含中文字符的常见编码）
    // GBK/GB2312特征：首字节在0x81-0xFE，次字节在0x40-0xFE
    size_t gbk_candidate_count = 0;
    size_t check_gbk_size = std::min(bytes.size(), size_t(2048)); // 检查前2KB

    for (size_t i = 0; i < check_gbk_size && i + 1 < bytes.size(); ++i) {
        if (bytes[i] >= 0x81 && bytes[i] <= 0xFE) {
            if (bytes[i + 1] >= 0x40 && bytes[i + 1] <= 0xFE) {
                gbk_candidate_count++;
                i++; // 跳过第二个字节
            }
        }
    }

    // 如果找到足够多的GBK特征序列，很可能是GBK
    if (gbk_candidate_count > 5) {
        return "GBK";
    }

    // 检查是否可能是Windows-1252或ISO-8859-1（Latin-1）
    // 这些编码的特征是：字节值在0x80-0xFF之间，但不是有效的UTF-8
    bool has_latin1_chars = false;
    for (size_t i = 0; i < check_size; ++i) {
        uint8_t byte = bytes[i];
        if (byte >= 0x80) { // uint8_t 的最大值是 0xFF，所以不需要检查 <= 0xFF
            has_latin1_chars = true;
            break;
        }
    }

    if (has_latin1_chars && !is_valid_utf8) {
        // 检查是否包含Windows-1252特有的字符（0x80-0x9F）
        bool has_win1252 = false;
        for (size_t i = 0; i < check_size; ++i) {
            if (bytes[i] >= 0x80 && bytes[i] <= 0x9F) {
                has_win1252 = true;
                break;
            }
        }
        return has_win1252 ? "Windows-1252" : "ISO-8859-1";
    }

    // 检查是否可能是UTF-16（无BOM）
    // UTF-16特征：文件大小是偶数，且包含很多0x00字节
    if (bytes.size() % 2 == 0 && bytes.size() >= 4) {
        size_t null_count = 0;
        for (size_t i = 0; i < check_size; i += 2) {
            if (bytes[i] == 0x00 || bytes[i + 1] == 0x00) {
                null_count++;
            }
        }
        // 如果超过30%的字节是0x00，可能是UTF-16
        if (null_count * 2 > check_size / 3) {
            // 检查是LE还是BE（通过检查常见ASCII字符）
            bool likely_le = false;
            for (size_t i = 0; i < check_size && i + 1 < bytes.size(); i += 2) {
                if (bytes[i] >= 0x20 && bytes[i] <= 0x7E && bytes[i + 1] == 0x00) {
                    likely_le = true;
                    break;
                }
            }
            return likely_le ? "UTF-16LE" : "UTF-16BE";
        }
    }

    // 默认返回UTF-8（即使可能不准确，也比完全失败好）
    return "UTF-8";
}

std::string EncodingConverter::encodingToUtf8(const std::vector<uint8_t>& content,
                                              const std::string& source_encoding) {
    if (content.empty()) {
        return "";
    }

    std::string upper_encoding = source_encoding;
    std::transform(upper_encoding.begin(), upper_encoding.end(), upper_encoding.begin(), ::toupper);

    // 如果已经是UTF-8，直接返回
    if (upper_encoding == "UTF-8") {
        return std::string(reinterpret_cast<const char*>(content.data()), content.size());
    }

#ifdef BUILD_ICONV_SUPPORT
    // 尝试使用iconv进行转换
    std::string iconv_result = convertWithIconv(content, source_encoding, "UTF-8");
    if (!iconv_result.empty()) {
        return iconv_result;
    }
    // 如果iconv失败，继续使用内置实现作为兜底
#endif

    // 内置转换实现（兜底方案）
    if (upper_encoding == "GBK") {
        return convertGBKToUtf8(content);
    } else if (upper_encoding == "GB2312") {
        return convertGB2312ToUtf8(content);
    } else if (upper_encoding == "ASCII" || upper_encoding == "ISO-8859-1" ||
               upper_encoding == "WINDOWS-1252") {
        return convertLatin1ToUtf8(content);
    } else if (upper_encoding == "UTF-16LE" || upper_encoding == "UTF-16") {
        return convertUtf16LeToUtf8(content);
    } else if (upper_encoding == "UTF-16BE") {
        return convertUtf16BeToUtf8(content);
    } else {
        // 未知编码，尝试作为UTF-8处理（可能产生乱码，但不会崩溃）
        return std::string(reinterpret_cast<const char*>(content.data()), content.size());
    }
}

std::vector<uint8_t> EncodingConverter::utf8ToEncoding(const std::string& utf8_content,
                                                       const std::string& target_encoding) {
    if (utf8_content.empty()) {
        return std::vector<uint8_t>();
    }

    std::string upper_encoding = target_encoding;
    std::transform(upper_encoding.begin(), upper_encoding.end(), upper_encoding.begin(), ::toupper);

    // 如果目标编码是UTF-8，直接返回
    if (upper_encoding == "UTF-8") {
        return std::vector<uint8_t>(utf8_content.begin(), utf8_content.end());
    }

#ifdef BUILD_ICONV_SUPPORT
    // 尝试使用iconv进行转换
    std::vector<uint8_t> iconv_result = convertFromUtf8WithIconv(utf8_content, target_encoding);
    if (!iconv_result.empty()) {
        return iconv_result;
    }
    // 如果iconv失败，继续使用内置实现作为兜底
#endif

    // 内置转换实现（兜底方案）
    if (upper_encoding == "GBK") {
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
        return convertUtf8ToUtf16Le(utf8_content);
    } else if (upper_encoding == "UTF-16BE") {
        return convertUtf8ToUtf16Be(utf8_content);
    } else {
        // 未知编码，返回UTF-8字节（可能产生乱码，但不会崩溃）
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

// UTF-16转换辅助函数
static std::string convertUtf16LeToUtf8(const std::vector<uint8_t>& content) {
    if (content.size() < 2) {
        return "";
    }

    std::string result;
    result.reserve(content.size()); // 预分配空间

    // 跳过BOM（如果存在）
    size_t start = 0;
    if (content.size() >= 2 && content[0] == 0xFF && content[1] == 0xFE) {
        start = 2;
    }

    for (size_t i = start; i + 1 < content.size(); i += 2) {
        uint16_t code_unit = content[i] | (content[i + 1] << 8);

        if (code_unit < 0x80) {
            // ASCII字符
            result += static_cast<char>(code_unit);
        } else if (code_unit < 0x800) {
            // 2字节UTF-8
            result += static_cast<char>(0xC0 | (code_unit >> 6));
            result += static_cast<char>(0x80 | (code_unit & 0x3F));
        } else if (code_unit >= 0xD800 && code_unit <= 0xDBFF && i + 3 < content.size()) {
            // 代理对（surrogate pair）- UTF-16编码大于0xFFFF的字符
            uint16_t high_surrogate = code_unit;
            uint16_t low_surrogate = content[i + 2] | (content[i + 3] << 8);

            if (low_surrogate >= 0xDC00 && low_surrogate <= 0xDFFF) {
                uint32_t code_point =
                    0x10000 + ((high_surrogate & 0x3FF) << 10) + (low_surrogate & 0x3FF);
                // 4字节UTF-8
                result += static_cast<char>(0xF0 | (code_point >> 18));
                result += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (code_point & 0x3F));
                i += 2; // 额外跳过低代理
            } else {
                // 无效的代理对，使用替换字符
                result += "\xEF\xBF\xBD"; // U+FFFD
            }
        } else {
            // 3字节UTF-8
            result += static_cast<char>(0xE0 | (code_unit >> 12));
            result += static_cast<char>(0x80 | ((code_unit >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (code_unit & 0x3F));
        }
    }

    return result;
}

static std::string convertUtf16BeToUtf8(const std::vector<uint8_t>& content) {
    if (content.size() < 2) {
        return "";
    }

    std::string result;
    result.reserve(content.size());

    // 跳过BOM（如果存在）
    size_t start = 0;
    if (content.size() >= 2 && content[0] == 0xFE && content[1] == 0xFF) {
        start = 2;
    }

    for (size_t i = start; i + 1 < content.size(); i += 2) {
        uint16_t code_unit = (content[i] << 8) | content[i + 1];

        if (code_unit < 0x80) {
            result += static_cast<char>(code_unit);
        } else if (code_unit < 0x800) {
            result += static_cast<char>(0xC0 | (code_unit >> 6));
            result += static_cast<char>(0x80 | (code_unit & 0x3F));
        } else if (code_unit >= 0xD800 && code_unit <= 0xDBFF && i + 3 < content.size()) {
            uint16_t high_surrogate = code_unit;
            uint16_t low_surrogate = (content[i + 2] << 8) | content[i + 3];

            if (low_surrogate >= 0xDC00 && low_surrogate <= 0xDFFF) {
                uint32_t code_point =
                    0x10000 + ((high_surrogate & 0x3FF) << 10) + (low_surrogate & 0x3FF);
                result += static_cast<char>(0xF0 | (code_point >> 18));
                result += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (code_point & 0x3F));
                i += 2;
            } else {
                result += "\xEF\xBF\xBD";
            }
        } else {
            result += static_cast<char>(0xE0 | (code_unit >> 12));
            result += static_cast<char>(0x80 | ((code_unit >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (code_unit & 0x3F));
        }
    }

    return result;
}

static std::vector<uint8_t> convertUtf8ToUtf16Le(const std::string& utf8_content) {
    std::vector<uint8_t> result;
    result.reserve(utf8_content.size() * 2);

    for (size_t i = 0; i < utf8_content.size();) {
        unsigned char byte = static_cast<unsigned char>(utf8_content[i]);
        uint32_t code_point = 0;

        if (byte < 0x80) {
            // ASCII字符
            code_point = byte;
            i++;
        } else if ((byte & 0xE0) == 0xC0 && i + 1 < utf8_content.size()) {
            // 2字节UTF-8
            code_point =
                ((byte & 0x1F) << 6) | (static_cast<unsigned char>(utf8_content[i + 1]) & 0x3F);
            i += 2;
        } else if ((byte & 0xF0) == 0xE0 && i + 2 < utf8_content.size()) {
            // 3字节UTF-8
            code_point = ((byte & 0x0F) << 12) |
                         ((static_cast<unsigned char>(utf8_content[i + 1]) & 0x3F) << 6) |
                         (static_cast<unsigned char>(utf8_content[i + 2]) & 0x3F);
            i += 3;
        } else if ((byte & 0xF8) == 0xF0 && i + 3 < utf8_content.size()) {
            // 4字节UTF-8（需要代理对）
            code_point = ((byte & 0x07) << 18) |
                         ((static_cast<unsigned char>(utf8_content[i + 1]) & 0x3F) << 12) |
                         ((static_cast<unsigned char>(utf8_content[i + 2]) & 0x3F) << 6) |
                         (static_cast<unsigned char>(utf8_content[i + 3]) & 0x3F);
            i += 4;
        } else {
            // 无效的UTF-8序列，跳过
            i++;
            continue;
        }

        // 转换为UTF-16
        if (code_point < 0x10000) {
            // 基本多文种平面（BMP）
            result.push_back(static_cast<uint8_t>(code_point & 0xFF));
            result.push_back(static_cast<uint8_t>((code_point >> 8) & 0xFF));
        } else {
            // 需要代理对
            code_point -= 0x10000;
            uint16_t high_surrogate = 0xD800 | ((code_point >> 10) & 0x3FF);
            uint16_t low_surrogate = 0xDC00 | (code_point & 0x3FF);
            result.push_back(static_cast<uint8_t>(high_surrogate & 0xFF));
            result.push_back(static_cast<uint8_t>((high_surrogate >> 8) & 0xFF));
            result.push_back(static_cast<uint8_t>(low_surrogate & 0xFF));
            result.push_back(static_cast<uint8_t>((low_surrogate >> 8) & 0xFF));
        }
    }

    return result;
}

static std::vector<uint8_t> convertUtf8ToUtf16Be(const std::string& utf8_content) {
    std::vector<uint8_t> result;
    result.reserve(utf8_content.size() * 2);

    for (size_t i = 0; i < utf8_content.size();) {
        unsigned char byte = static_cast<unsigned char>(utf8_content[i]);
        uint32_t code_point = 0;

        if (byte < 0x80) {
            code_point = byte;
            i++;
        } else if ((byte & 0xE0) == 0xC0 && i + 1 < utf8_content.size()) {
            code_point =
                ((byte & 0x1F) << 6) | (static_cast<unsigned char>(utf8_content[i + 1]) & 0x3F);
            i += 2;
        } else if ((byte & 0xF0) == 0xE0 && i + 2 < utf8_content.size()) {
            code_point = ((byte & 0x0F) << 12) |
                         ((static_cast<unsigned char>(utf8_content[i + 1]) & 0x3F) << 6) |
                         (static_cast<unsigned char>(utf8_content[i + 2]) & 0x3F);
            i += 3;
        } else if ((byte & 0xF8) == 0xF0 && i + 3 < utf8_content.size()) {
            code_point = ((byte & 0x07) << 18) |
                         ((static_cast<unsigned char>(utf8_content[i + 1]) & 0x3F) << 12) |
                         ((static_cast<unsigned char>(utf8_content[i + 2]) & 0x3F) << 6) |
                         (static_cast<unsigned char>(utf8_content[i + 3]) & 0x3F);
            i += 4;
        } else {
            i++;
            continue;
        }

        if (code_point < 0x10000) {
            // 大端序
            result.push_back(static_cast<uint8_t>((code_point >> 8) & 0xFF));
            result.push_back(static_cast<uint8_t>(code_point & 0xFF));
        } else {
            code_point -= 0x10000;
            uint16_t high_surrogate = 0xD800 | ((code_point >> 10) & 0x3FF);
            uint16_t low_surrogate = 0xDC00 | (code_point & 0x3FF);
            result.push_back(static_cast<uint8_t>((high_surrogate >> 8) & 0xFF));
            result.push_back(static_cast<uint8_t>(high_surrogate & 0xFF));
            result.push_back(static_cast<uint8_t>((low_surrogate >> 8) & 0xFF));
            result.push_back(static_cast<uint8_t>(low_surrogate & 0xFF));
        }
    }

    return result;
}

// GBK/GB2312转换的改进实现
// 注意：这是一个改进版本，但完整的GBK/GB2312转换仍需要完整的码表
// 如果iconv可用，会优先使用iconv；否则使用这个简化实现

std::string EncodingConverter::convertGBKToUtf8(const std::vector<uint8_t>& gbk_content) {
    if (gbk_content.empty()) {
        return "";
    }

    // 如果没有iconv，使用简化实现
    // 这个实现只能处理ASCII字符，其他字符会被保留为原始字节
    // 这可能导致乱码，但不会崩溃
    std::string result;
    result.reserve(gbk_content.size());

    for (size_t i = 0; i < gbk_content.size(); ++i) {
        uint8_t byte = gbk_content[i];
        if (byte < 0x80) {
            // ASCII字符，直接添加
            result += static_cast<char>(byte);
        } else if (byte >= 0x81 && byte <= 0xFE && i + 1 < gbk_content.size()) {
            // GBK双字节字符，无法直接转换，使用替换字符
            result += "\xEF\xBF\xBD"; // U+FFFD (替换字符)
            i++;                      // 跳过第二个字节
        } else {
            // 其他情况，使用替换字符
            result += "\xEF\xBF\xBD";
        }
    }

    return result;
}

std::string EncodingConverter::convertUtf8ToGBK(const std::string& utf8_content) {
    if (utf8_content.empty()) {
        return "";
    }

    // 如果没有iconv，使用简化实现
    // 这个实现只能处理ASCII字符，其他字符会被替换为'?'
    std::string result;
    result.reserve(utf8_content.size());

    for (size_t i = 0; i < utf8_content.size();) {
        unsigned char byte = static_cast<unsigned char>(utf8_content[i]);
        if (byte < 0x80) {
            // ASCII字符，直接添加
            result += static_cast<char>(byte);
            i++;
        } else {
            // 非ASCII字符，无法直接转换，使用替换字符
            result += '?';
            // 跳过UTF-8序列的剩余字节
            if ((byte & 0xE0) == 0xC0 && i + 1 < utf8_content.size()) {
                i += 2;
            } else if ((byte & 0xF0) == 0xE0 && i + 2 < utf8_content.size()) {
                i += 3;
            } else if ((byte & 0xF8) == 0xF0 && i + 3 < utf8_content.size()) {
                i += 4;
            } else {
                i++;
            }
        }
    }

    return result;
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
