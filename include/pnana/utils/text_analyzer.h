#ifndef PNANA_UTILS_TEXT_ANALYZER_H
#define PNANA_UTILS_TEXT_ANALYZER_H

#include <string>
#include <vector>

namespace pnana {
namespace utils {

/**
 * 文本分析工具类
 * 用于分析文本内容，如检测中文、排除注释等
 */
class TextAnalyzer {
public:
    /**
     * 检测文件是否包含大量中文字符（排除注释）
     * @param lines 文件的所有行
     * @param file_type 文件类型（用于确定注释格式）
     * @param check_limit 检查的字符数限制
     * @param threshold 中文字符占比阈值（百分比）
     * @return 如果中文字符占比超过阈值，返回true
     */
    static bool hasChineseContent(
        const std::vector<std::string>& lines,
        const std::string& file_type = "",
        size_t check_limit = 1000,
        int threshold = 10
    );
    
private:
    /**
     * 检查位置是否在注释中
     * @param line 当前行
     * @param pos 当前位置
     * @param file_type 文件类型
     * @param in_multiline_comment 是否在多行注释中（输入输出参数）
     * @return 如果位置在注释中，返回true
     */
    static bool isInComment(
        const std::string& line,
        size_t pos,
        const std::string& file_type,
        bool& in_multiline_comment
    );
    
    /**
     * 检测UTF-8中文字符
     * @param line 当前行
     * @param pos 位置
     * @return 如果是中文字符，返回true，并更新pos到下一个字符
     */
    static bool isChineseChar(const std::string& line, size_t& pos);
};

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_TEXT_ANALYZER_H

