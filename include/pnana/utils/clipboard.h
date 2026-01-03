#ifndef PNANA_UTILS_CLIPBOARD_H
#define PNANA_UTILS_CLIPBOARD_H

#include <string>

namespace pnana {
namespace utils {

// 系统剪贴板工具类
// 支持 Linux (xclip/wl-clipboard) 和 macOS (pbcopy/pbpaste)
class Clipboard {
  public:
    // 复制文本到系统剪贴板
    static bool copyToSystem(const std::string& text);

    // 从系统剪贴板读取文本
    static std::string pasteFromSystem();

    // 检查系统剪贴板是否可用
    static bool isAvailable();

  private:
    // 检测平台并返回相应的命令
    static std::string getCopyCommand();
    static std::string getPasteCommand();

    // 执行系统命令
    static bool executeCommand(const std::string& command, const std::string& input = "");
    static std::string executeCommandWithOutput(const std::string& command);
};

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_CLIPBOARD_H
