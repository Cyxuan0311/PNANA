#include "utils/clangd_flags.h"
#include <sys/stat.h>
#include <sys/utsname.h>

namespace pnana {
namespace utils {

namespace {
// 获取当前架构的 clang target，解决 WSL 下 bits/c++config.h 找不到的问题
std::string getClangTarget() {
    struct utsname u;
    if (uname(&u) == 0) {
        std::string m = u.machine;
        if (m == "x86_64" || m == "amd64")
            return "x86_64-linux-gnu";
        if (m == "aarch64" || m == "arm64")
            return "aarch64-linux-gnu";
        if (m == "i686" || m == "i386")
            return "i686-linux-gnu";
    }
    return "x86_64-linux-gnu"; // 默认
}
} // namespace

std::vector<std::string> getClangdFallbackFlags() {
    const char* cxx_bases[] = {"/usr/include/c++", "/usr/local/include/c++"};
    const char* versions[] = {"17", "16", "15", "14", "13", "12", "11", "10", "9"};
    const char* archs[] = {"x86_64-linux-gnu", "aarch64-linux-gnu", "i686-linux-gnu", ""};
    for (const char* base : cxx_bases) {
        for (const char* ver : versions) {
            std::string path = std::string(base) + "/" + ver;
            struct stat st;
            if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                // 使用检测到的版本作为 -std，与系统 libstdc++ 一致（如 /usr/include/c++/11 ->
                // -std=c++11）
                // --target 解决 WSL 下 clangd 找不到 bits/c++config.h 的问题
                std::string target = "--target=" + getClangTarget();
                std::vector<std::string> flags = {target, "-std=c++" + std::string(ver), "-xc++"};
                flags.push_back("-isystem");
                flags.push_back(path);
                // 架构子目录：/usr/include/c++/11/x86_64-linux-gnu（bits/c++config.h 可能在此）
                for (const char* arch : archs) {
                    if (arch[0] != '\0') {
                        std::string arch_path = path + "/" + arch;
                        if (stat(arch_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                            flags.push_back("-isystem");
                            flags.push_back(arch_path);
                            break;
                        }
                    }
                }
                // GCC 架构路径：/usr/include/x86_64-linux-gnu/c++/11（部分发行版将 c++config.h
                // 放此处）
                std::string arch_cxx =
                    "/usr/include/" + getClangTarget() + "/c++/" + std::string(ver);
                if (stat(arch_cxx.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                    flags.push_back("-isystem");
                    flags.push_back(arch_cxx);
                }
                // 架构相关系统头：/usr/include/x86_64-linux-gnu
                std::string arch_include = "/usr/include/" + getClangTarget();
                if (stat(arch_include.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                    flags.push_back("-isystem");
                    flags.push_back(arch_include);
                }
                // 系统头文件（limits.h 等），WSL/Linux 通用
                std::string usr_include = "/usr/include";
                if (stat(usr_include.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                    flags.push_back("-isystem");
                    flags.push_back(usr_include);
                }
                return flags;
            }
        }
    }
    // 未找到 C++ 库时仍加 /usr/include，减少基础头文件报错
    std::vector<std::string> fallback = {"--target=" + getClangTarget(), "-std=c++17", "-xc++"};
    struct stat st;
    if (stat("/usr/include", &st) == 0 && S_ISDIR(st.st_mode)) {
        fallback.push_back("-isystem");
        fallback.push_back("/usr/include");
    }
    return fallback;
}

} // namespace utils
} // namespace pnana
