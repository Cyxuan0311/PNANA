#ifndef PNANA_FEATURES_TERMINAL_BUILTIN_H
#define PNANA_FEATURES_TERMINAL_BUILTIN_H

#include <string>
#include <vector>

namespace pnana {
namespace features {

// 前向声明
struct TerminalLine;

namespace terminal {

// 内置命令执行器
class BuiltinCommandExecutor {
  public:
    // 执行内置命令
    // 返回执行结果，空字符串表示成功但无输出
    static std::string execute(const std::string& command, const std::vector<std::string>& args,
                               std::string& current_directory,
                               std::vector<TerminalLine>& output_lines);

    // 检查是否是内置命令
    static bool isBuiltin(const std::string& command);

  private:
    // 各个内置命令的实现
    static std::string executeHelp();
    static std::string executeClear(std::vector<TerminalLine>& output_lines);
    static std::string executePwd(const std::string& current_directory);
    static std::string executeCd(const std::vector<std::string>& args,
                                 std::string& current_directory);
    static std::string executeLs(const std::vector<std::string>& args,
                                 const std::string& current_directory);
    static std::string executeCat(const std::vector<std::string>& args,
                                  const std::string& current_directory);
    static std::string executeEcho(const std::vector<std::string>& args);
    static std::string executeWhoami();
    static std::string executeHostname();
    static std::string executeJobs();
    static std::string executeFg(const std::vector<std::string>& args);
    static std::string executeBg(const std::vector<std::string>& args);
    static std::string executeKill(const std::vector<std::string>& args);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_BUILTIN_H
