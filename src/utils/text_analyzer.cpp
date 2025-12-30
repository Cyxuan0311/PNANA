#include "utils/text_analyzer.h"
#include <algorithm>
#include <cctype>

namespace pnana {
namespace utils {

bool TextAnalyzer::hasChineseContent(
    const std::vector<std::string>& lines,
    const std::string& file_type,
    size_t check_limit,
    int threshold) {
    
    size_t chinese_count = 0;
    size_t total_chars = 0;
    bool in_multiline_comment = false;
    bool in_string = false;
    char string_quote = 0;
    
    for (size_t i = 0; i < lines.size() && total_chars < check_limit; ++i) {
        const std::string& line = lines[i];
        
        for (size_t j = 0; j < line.length() && total_chars < check_limit; ) {
            unsigned char c = static_cast<unsigned char>(line[j]);
            
            // 处理字符串（字符串中的内容不检测中文）
            if (!in_multiline_comment) {
                if (!in_string) {
                    // 检查字符串开始
                    if (c == '"' || c == '\'') {
                        // 检查是否是转义的引号
                        if (j == 0 || line[j - 1] != '\\') {
                            in_string = true;
                            string_quote = c;
                            j++;
                            total_chars++;
                            continue;
                        }
                    }
                } else {
                    // 在字符串中，检查字符串结束
                    if (c == string_quote && (j == 0 || line[j - 1] != '\\')) {
                        in_string = false;
                        string_quote = 0;
                        j++;
                        total_chars++;
                        continue;
                    }
                    // 字符串中的内容，跳过
                    if (c < 0x80) {
                        j++;
                        total_chars++;
                    } else {
                        size_t len = (c & 0xE0) == 0xC0 ? 2 : ((c & 0xF0) == 0xE0 ? 3 : 4);
                        j += len;
                        total_chars += len;
                    }
                    continue;
                }
            }
            
            // 处理注释（注释中的内容不检测中文）
            if (!in_string) {
                if (in_multiline_comment) {
                    // 在多行注释中，查找结束标记
                    if (j + 1 < line.length() && c == '*' && line[j + 1] == '/') {
                        in_multiline_comment = false;
                        j += 2;
                        total_chars += 2;
                        continue;
                    }
                    // 跳过注释内容
                    if (c < 0x80) {
                        j++;
                        total_chars++;
                    } else {
                        size_t len = (c & 0xE0) == 0xC0 ? 2 : ((c & 0xF0) == 0xE0 ? 3 : 4);
                        j += len;
                        total_chars += len;
                    }
                    continue;
                } else {
                    // 检查注释开始
                    if (file_type == "cpp" || file_type == "c" || file_type == "javascript" || 
                        file_type == "typescript" || file_type == "java" || file_type == "go" ||
                        file_type == "rust") {
                        // C风格注释
                        if (j + 1 < line.length() && c == '/' && line[j + 1] == '/') {
                            // 单行注释，跳过到行尾
                            j = line.length();
                            continue;
                        }
                        if (j + 1 < line.length() && c == '/' && line[j + 1] == '*') {
                            // 多行注释开始
                            in_multiline_comment = true;
                            j += 2;
                            total_chars += 2;
                            continue;
                        }
                    } else if (file_type == "python" || file_type == "shell" || file_type == "yaml") {
                        // Python/Shell风格注释
                        if (c == '#') {
                            // 单行注释，跳过到行尾
                            j = line.length();
                            continue;
                        }
                    } else if (file_type == "html" || file_type == "xml") {
                        // HTML/XML注释
                        if (j + 3 < line.length() && 
                            c == '<' && line[j + 1] == '!' && 
                            line[j + 2] == '-' && line[j + 3] == '-') {
                            // 注释开始，跳过到注释结束
                            size_t comment_end = line.find("-->", j + 4);
                            if (comment_end != std::string::npos) {
                                j = comment_end + 3;
                            } else {
                                j = line.length();
                            }
                            continue;
                        }
                    }
                }
            }
            
            // 在非注释、非字符串的代码中检测中文
            if (isChineseChar(line, j)) {
                chinese_count++;
                total_chars += 3;  // 中文字符是3字节
            } else if (c < 0x80) {
                // ASCII字符
                j++;
                total_chars++;
            } else {
                // 其他UTF-8字符，跳过
                size_t len = (c & 0xE0) == 0xC0 ? 2 : ((c & 0xF0) == 0xE0 ? 3 : 4);
                j += len;
                total_chars += len;
            }
        }
        
        // 行尾处理：单行注释不会延续到下一行，但多行注释会
        // 字符串如果未闭合，也应该延续（但为了简化，这里假设字符串在行内闭合）
    }
    
    // 如果中文字符占比超过阈值，认为是中文文件
    if (total_chars > 0 && (chinese_count * 100 / total_chars) > static_cast<size_t>(threshold)) {
        return true;
    }
    
    return false;
}

bool TextAnalyzer::isInComment(
    const std::string& line,
    size_t pos,
    const std::string& file_type,
    bool& in_multiline_comment) {
    
    // 如果已经在多行注释中
    if (in_multiline_comment) {
        // 检查是否结束
        size_t end_pos = line.find("*/", 0);
        if (end_pos != std::string::npos && end_pos < pos) {
            in_multiline_comment = false;
            return false;  // 注释已结束
        }
        return true;  // 仍在多行注释中
    }
    
    // 检查是否在字符串中（字符串中的注释符号不应该被识别为注释）
    bool in_string = false;
    char string_quote = 0;
    for (size_t i = 0; i < pos; ++i) {
        if (!in_string) {
            if (line[i] == '"' || line[i] == '\'') {
                // 检查是否是转义的引号
                if (i == 0 || line[i - 1] != '\\') {
                    in_string = true;
                    string_quote = line[i];
                }
            }
        } else {
            if (line[i] == string_quote && (i == 0 || line[i - 1] != '\\')) {
                in_string = false;
            }
        }
    }
    
    // 如果在字符串中，不在注释中
    if (in_string) {
        return false;
    }
    
    // 检查单行注释
    if (file_type == "cpp" || file_type == "c" || file_type == "javascript" || 
        file_type == "typescript" || file_type == "java" || file_type == "go" ||
        file_type == "rust") {
        // C风格注释：// 和 /* */
        size_t single_comment = line.find("//", 0);
        size_t multi_start = line.find("/*", 0);
        
        // 检查单行注释（//）是否在当前位置之前
        if (single_comment != std::string::npos && single_comment < pos) {
            // 确保不在字符串中
            bool in_str = false;
            for (size_t i = 0; i < single_comment; ++i) {
                if (!in_str && (line[i] == '"' || line[i] == '\'')) {
                    if (i == 0 || line[i - 1] != '\\') {
                        in_str = true;
                    }
                } else if (in_str && line[i] == line[single_comment - 1] && 
                          (i == 0 || line[i - 1] != '\\')) {
                    in_str = false;
                }
            }
            if (!in_str) {
                return true;  // 在单行注释中
            }
        }
        
        // 检查多行注释（/* */）
        if (multi_start != std::string::npos && multi_start < pos) {
            // 确保不在字符串中
            bool in_str = false;
            for (size_t i = 0; i < multi_start; ++i) {
                if (!in_str && (line[i] == '"' || line[i] == '\'')) {
                    if (i == 0 || line[i - 1] != '\\') {
                        in_str = true;
                    }
                } else if (in_str && line[i] == line[multi_start - 1] && 
                          (i == 0 || line[i - 1] != '\\')) {
                    in_str = false;
                }
            }
            if (!in_str) {
                size_t multi_end = line.find("*/", multi_start + 2);
                if (multi_end == std::string::npos || multi_end >= pos) {
                    // 多行注释开始且未结束，或结束位置在当前位置之后
                    if (multi_end == std::string::npos) {
                        in_multiline_comment = true;
                    }
                    return true;
                }
            }
        }
    } else if (file_type == "python" || file_type == "shell" || file_type == "yaml") {
        // Python/Shell风格注释：#
        size_t comment_pos = line.find("#", 0);
        if (comment_pos != std::string::npos && comment_pos < pos) {
            // 确保不在字符串中
            bool in_str = false;
            for (size_t i = 0; i < comment_pos; ++i) {
                if (!in_str && (line[i] == '"' || line[i] == '\'')) {
                    if (i == 0 || line[i - 1] != '\\') {
                        in_str = true;
                    }
                } else if (in_str && line[i] == line[comment_pos - 1] && 
                          (i == 0 || line[i - 1] != '\\')) {
                    in_str = false;
                }
            }
            if (!in_str) {
                return true;  // 在注释中
            }
        }
    } else if (file_type == "html" || file_type == "xml") {
        // HTML/XML注释：<!-- -->
        size_t comment_start = line.find("<!--", 0);
        if (comment_start != std::string::npos && comment_start < pos) {
            size_t comment_end = line.find("-->", comment_start + 4);
            if (comment_end == std::string::npos || comment_end >= pos) {
                return true;  // 在注释中
            }
        }
    }
    
    return false;
}

bool TextAnalyzer::isChineseChar(const std::string& line, size_t& pos) {
    if (pos + 2 >= line.length()) {
        return false;
    }
    
    unsigned char c = static_cast<unsigned char>(line[pos]);
    
    // 检测UTF-8中文字符（3字节，范围：E4-BA-80 到 E9-BF-BF）
    if ((c & 0xE0) == 0xE0) {
        unsigned char c1 = static_cast<unsigned char>(line[pos + 1]);
        unsigned char c2 = static_cast<unsigned char>(line[pos + 2]);
        
        // 中文字符范围：E4-BA-80 到 E9-BF-BF
        if ((c >= 0xE4 && c <= 0xE9) && 
            ((c == 0xE4 && c1 >= 0xBA) || (c > 0xE4 && c < 0xE9) || (c == 0xE9 && c1 <= 0xBF)) &&
            ((c1 >= 0x80 && c1 <= 0xBF) && (c2 >= 0x80 && c2 <= 0xBF))) {
            pos += 3;  // 更新位置到下一个字符
            return true;
        }
    }
    
    return false;
}

} // namespace utils
} // namespace pnana

