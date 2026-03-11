#ifndef PNANA_UTILS_CLANGD_FLAGS_H
#define PNANA_UTILS_CLANGD_FLAGS_H

#include <string>
#include <vector>

namespace pnana {
namespace utils {

/**
 * 检测 C++ 标准库 include 路径，用于 clangd fallbackFlags
 * 无 compile_commands.json 时，clangd 依赖 fallbackFlags 定位 <vector>、<string> 等
 * 参考 nvim/clangd 做法，解决 WSL 下 bits/c++config.h 找不到的问题
 */
std::vector<std::string> getClangdFallbackFlags();

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_CLANGD_FLAGS_H
