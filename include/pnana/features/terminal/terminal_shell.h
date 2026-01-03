#ifndef PNANA_FEATURES_TERMINAL_SHELL_H
#define PNANA_FEATURES_TERMINAL_SHELL_H

#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace terminal {

// Shell 命令执行器
class ShellCommandExecutor {
  public:
    // 执行系统命令（通过 popen）
    static std::string executeSystemCommand(const std::string& command,
                                            const std::vector<std::string>& args,
                                            const std::string& current_directory);

    // 执行 Shell 命令（支持所有 shell 特性）
    static std::string executeShellCommand(const std::string& command, bool background,
                                           const std::string& current_directory);

  private:
    // 转义命令字符串中的特殊字符
    static std::string escapeCommand(const std::string& command);

    // 转义目录路径
    static std::string escapeDirectory(const std::string& directory);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_SHELL_H
