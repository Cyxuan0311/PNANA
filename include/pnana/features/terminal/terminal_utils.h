#ifndef PNANA_FEATURES_TERMINAL_UTILS_H
#define PNANA_FEATURES_TERMINAL_UTILS_H

#include <string>

namespace pnana {
namespace features {
namespace terminal {

// 终端工具函数
class TerminalUtils {
public:
    // 获取用户名
    static std::string getUsername();
    
    // 获取主机名
    static std::string getHostname();
    
    // 获取当前时间（格式: HH:MM:SS）
    static std::string getCurrentTime();
    
    // 获取 Git 分支（从指定目录向上查找）
    static std::string getGitBranch(const std::string& directory);
    
    // 简化路径（将 HOME 替换为 ~）
    static std::string simplifyPath(const std::string& path);
    
    // 截断路径（如果太长，只显示最后一部分）
    static std::string truncatePath(const std::string& path, size_t max_length = 30);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_UTILS_H

