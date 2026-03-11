#ifndef PNANA_UTILS_PROJECT_ROOT_FINDER_H
#define PNANA_UTILS_PROJECT_ROOT_FINDER_H

#include <string>

namespace pnana {
namespace utils {

// 查找 Rust 项目根（含 Cargo.toml 的目录），供 rust-analyzer 正确识别 workspace
// 从文件路径向上遍历，返回首个包含 Cargo.toml 的目录；未找到时返回空字符串
std::string findRustProjectRoot(const std::string& filepath);

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_PROJECT_ROOT_FINDER_H
