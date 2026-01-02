#ifndef PNANA_FEATURES_ENCODING_CONVERTER_H
#define PNANA_FEATURES_ENCODING_CONVERTER_H

#include <string>
#include <vector>
#include <cstdint>

namespace pnana {
namespace features {

// 编码转换器类
class EncodingConverter {
public:
    // 支持的编码列表
    static std::vector<std::string> getSupportedEncodings();
    
    // 检查编码是否支持
    static bool isEncodingSupported(const std::string& encoding);
    
    // 将字符串从一种编码转换为另一种编码
    // from_encoding: 源编码
    // to_encoding: 目标编码
    // content: 要转换的内容（字节流）
    // 返回: 转换后的内容（UTF-8字符串）
    static std::string convertEncoding(const std::string& from_encoding,
                                      const std::string& to_encoding,
                                      const std::vector<uint8_t>& content);
    
    // 从文件读取内容（按字节读取）
    static std::vector<uint8_t> readFileAsBytes(const std::string& filepath);
    
    // 将内容写入文件（按指定编码）
    static bool writeFileWithEncoding(const std::string& filepath,
                                      const std::string& encoding,
                                      const std::string& content);
    
    // 检测文件编码（简单检测）
    static std::string detectFileEncoding(const std::string& filepath);
    
    // 将UTF-8字符串转换为指定编码的字节流
    static std::vector<uint8_t> utf8ToEncoding(const std::string& utf8_content,
                                                const std::string& target_encoding);
    
    // 将指定编码的字节流转换为UTF-8字符串
    static std::string encodingToUtf8(const std::vector<uint8_t>& content,
                                       const std::string& source_encoding);

private:
    // 内部辅助方法
    static std::string convertUtf8ToGBK(const std::string& utf8_content);
    static std::string convertGBKToUtf8(const std::vector<uint8_t>& gbk_content);
    static std::string convertUtf8ToGB2312(const std::string& utf8_content);
    static std::string convertGB2312ToUtf8(const std::vector<uint8_t>& gb2312_content);
    static std::string convertUtf8ToLatin1(const std::string& utf8_content);
    static std::string convertLatin1ToUtf8(const std::vector<uint8_t>& latin1_content);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_ENCODING_CONVERTER_H

