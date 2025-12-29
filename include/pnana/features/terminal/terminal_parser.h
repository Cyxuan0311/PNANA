#ifndef PNANA_FEATURES_TERMINAL_PARSER_H
#define PNANA_FEATURES_TERMINAL_PARSER_H

#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace terminal {

// 命令解析器
class CommandParser {
public:
    // 解析命令字符串为参数列表
    // 支持引号、空格分割等
    static std::vector<std::string> parse(const std::string& command);
    
    // 检查命令是否包含 shell 特性（管道、重定向等）
    static bool hasShellFeatures(const std::string& command);
    
    // 检查是否是后台命令（以 & 结尾）
    static bool isBackgroundCommand(const std::string& command, std::string& cmd_without_ampersand);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_PARSER_H

