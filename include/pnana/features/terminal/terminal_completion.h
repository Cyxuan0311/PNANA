#ifndef PNANA_FEATURES_TERMINAL_COMPLETION_H
#define PNANA_FEATURES_TERMINAL_COMPLETION_H

#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace terminal {

// Tab 补全功能
class TerminalCompletion {
  public:
    // 执行 Tab 补全
    // 输入: 当前输入字符串和光标位置
    // 输出: 补全后的字符串和新的光标位置
    // 返回: 是否成功补全
    static bool complete(const std::string& input, size_t cursor_pos,
                         const std::string& current_directory, std::string& output,
                         size_t& new_cursor_pos);

  private:
    // 补全命令（从 PATH 中查找）
    static bool completeCommand(const std::string& prefix, std::string& result);

    // 补全文件/目录路径
    static bool completePath(const std::string& prefix, const std::string& current_directory,
                             std::string& result);

    // 获取所有匹配项
    static std::vector<std::string> getMatches(const std::string& prefix,
                                               const std::vector<std::string>& candidates);

    // 获取公共前缀
    static std::string getCommonPrefix(const std::vector<std::string>& matches);

    // 检查是否是路径（以 / 或 ~ 或 ./ 或 ../ 开头）
    static bool isPath(const std::string& token);

    // 展开路径（处理 ~ 和相对路径）
    static std::string expandPath(const std::string& path, const std::string& current_directory);

    // 从 PATH 环境变量获取所有可执行文件
    static std::vector<std::string> getExecutablesFromPath();

    // 列出目录中的文件和文件夹
    static std::vector<std::string> listDirectory(const std::string& dir_path);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_COMPLETION_H
