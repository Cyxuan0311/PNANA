#include "features/comment/comment_syntax.h"
#include <algorithm>
#include <string>

namespace pnana {
namespace features {
namespace comment {

namespace {

// 将文件类型转为小写便于匹配
std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

} // namespace

std::pair<std::string, std::string> getCommentSyntax(const std::string& file_type) {
    std::string ft = toLower(file_type);

    // # 注释：Python, Shell, Ruby, YAML, Dockerfile, Makefile, Perl, PowerShell 等
    if (ft == "python" || ft == "shell" || ft == "ruby" || ft == "yaml" || ft == "dockerfile" ||
        ft == "makefile" || ft == "perl" || ft == "powershell" || ft == "r" || ft == "elixir" ||
        ft == "crystal" || ft == "nim" || ft == "groovy" || ft == "gradle" || ft == "cmake" ||
        ft == "tcl" || ft == "toml" || ft == "cfg") {
        return {"#", ""};
    }

    // -- 注释：Lua, SQL, Haskell, Ada
    if (ft == "lua" || ft == "sql" || ft == "haskell" || ft == "ada" || ft == "nelua") {
        return {"--", ""};
    }

    // ; 注释：INI, Assembly
    if (ft == "ini" || ft == "asm" || ft == "nasm" || ft == "s") {
        return {";", ""};
    }

    // " 注释：Vim
    if (ft == "vim") {
        return {"\"", ""};
    }

    // <!-- --> 注释：HTML, XML
    if (ft == "html" || ft == "xml" || ft == "xhtml" || ft == "svg") {
        return {"<!-- ", " -->"};
    }

    // /* */ 行内注释（单行使用）：CSS, SCSS, Less
    if (ft == "css" || ft == "scss" || ft == "less" || ft == "sass") {
        return {"/* ", " */"};
    }

    // // 注释：C/C++, JavaScript, TypeScript, Java, Go, Rust, C#, PHP, Swift, Kotlin, Dart 等
    if (ft == "cpp" || ft == "c" || ft == "javascript" || ft == "typescript" || ft == "java" ||
        ft == "go" || ft == "rust" || ft == "csharp" || ft == "php" || ft == "swift" ||
        ft == "kotlin" || ft == "dart" || ft == "zig" || ft == "odin" || ft == "v" ||
        ft == "objective-c" || ft == "objective-cpp" || ft == "jsonc" || ft == "glsl" ||
        ft == "hlsl" || ft == "wgsl" || ft == "proto" || ft == "thrift" || ft == "text") {
        return {"//", ""};
    }

    // 默认使用 //
    return {"//", ""};
}

std::pair<std::string, int> toggleLineComment(const std::string& line, const std::string& prefix,
                                              const std::string& suffix) {
    size_t first_non_space = line.find_first_not_of(" \t");
    if (first_non_space == std::string::npos) {
        first_non_space = 0;
    }

    const bool has_suffix = !suffix.empty();

    // 检查是否已注释（行注释：行首为 prefix；块注释：行首 prefix 且行尾 suffix）
    bool is_commented = false;
    if (line.length() >= first_non_space + prefix.length()) {
        if (line.substr(first_non_space, prefix.length()) == prefix) {
            if (has_suffix) {
                size_t suffix_pos = line.find(suffix, first_non_space + prefix.length());
                is_commented = (suffix_pos != std::string::npos);
            } else {
                is_commented = true;
            }
        }
    }

    if (is_commented) {
        // 取消注释
        std::string result = line;
        result.erase(first_non_space, prefix.length());
        int offset = -static_cast<int>(prefix.length());

        if (has_suffix) {
            size_t suffix_pos = result.find(suffix, first_non_space);
            if (suffix_pos != std::string::npos) {
                result.erase(suffix_pos, suffix.length());
                // offset 不变，因为 suffix 在行尾
            }
        }

        return {result, offset};
    } else {
        // 添加注释
        std::string result = line;
        std::string to_insert = prefix;
        if (!has_suffix) {
            to_insert += " ";
        }
        result.insert(first_non_space, to_insert);
        int offset = static_cast<int>(to_insert.length());

        if (has_suffix) {
            result += suffix;
        }

        return {result, offset};
    }
}

std::pair<std::string, int> toggleCommentForLine(const std::string& line,
                                                 const std::string& file_type) {
    auto [prefix, suffix] = getCommentSyntax(file_type);
    return toggleLineComment(line, prefix, suffix);
}

} // namespace comment
} // namespace features
} // namespace pnana
