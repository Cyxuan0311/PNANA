#include "utils/file_type_color_mapper.h"
#include "utils/file_type_detector.h"
#include <algorithm>
#include <cctype>

namespace pnana {
namespace utils {

FileTypeColorMapper::FileTypeColorMapper(const ui::Theme& theme) : theme_(theme) {}

ftxui::Color FileTypeColorMapper::getFileColor(const std::string& filename,
                                               bool is_directory) const {
    auto& colors = theme_.getColors();

    if (is_directory) {
        if (filename == "..") {
            return colors.comment; // 上级目录使用主题的注释色（灰色）
        }
        return colors.function; // 目录使用主题的函数色（青色/蓝色）
    }

    // 使用文件类型检测器获取文件类型
    std::string ext = getFileExtension(filename);
    std::string file_type = FileTypeDetector::detectFileType(filename, ext);

    // 根据文件类型获取颜色
    return getColorByFileType(file_type);
}

ftxui::Color FileTypeColorMapper::getColorByFileType(const std::string& file_type) const {
    // 转换为小写进行比较
    std::string type_lower = file_type;
    std::transform(type_lower.begin(), type_lower.end(), type_lower.begin(), ::tolower);

    // 使用固定的 RGB 颜色值，确保文件类型颜色不被主题覆盖
    // ========== 系统编程语言 ==========
    // C/C++ - 蓝色（系统级编程的代表色）
    if (type_lower == "c" || type_lower == "cpp") {
        return ftxui::Color::RGB(100, 150, 255); // 蓝色
    }

    // Rust - 橙色（Rust 官方颜色 #CE412B）
    if (type_lower == "rust") {
        return ftxui::Color::RGB(206, 65, 43); // 橙色
    }

    // Go - 青色（Go 官方颜色 #00ADD8）
    if (type_lower == "go") {
        return ftxui::Color::RGB(0, 173, 216); // 青色
    }

    // Zig - 黄色（Zig 官方颜色 #F7A41D）
    if (type_lower == "zig") {
        return ftxui::Color::RGB(247, 164, 29); // 黄色
    }

    // Nim - 黄色
    if (type_lower == "nim") {
        return ftxui::Color::RGB(255, 200, 0); // 黄色
    }

    // D - 红色
    if (type_lower == "d") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // Assembly - 深灰色（底层代码）
    if (type_lower == "asm" || type_lower == "assembly" || type_lower == "x86" ||
        type_lower == "arm" || type_lower == "riscv" || type_lower == "mips") {
        return ftxui::Color::RGB(140, 135, 120); // 灰色
    }

    // ========== 高级编程语言 ==========
    // Python - 蓝色/青色（Python 官方颜色 #3776AB）
    if (type_lower == "python") {
        return ftxui::Color::RGB(55, 118, 171); // 蓝色
    }

    // JavaScript/TypeScript - 黄色（JS 官方颜色 #F7DF1E）
    if (type_lower == "javascript" || type_lower == "typescript") {
        return ftxui::Color::RGB(247, 223, 30); // 黄色
    }

    // Java - 橙色（Java 官方颜色 #ED8B00）
    if (type_lower == "java") {
        return ftxui::Color::RGB(237, 139, 0); // 橙色
    }

    // Kotlin - 紫色（Kotlin 官方颜色 #7F52FF）
    if (type_lower == "kotlin") {
        return ftxui::Color::RGB(127, 82, 255); // 紫色
    }

    // Scala - 红色（Scala 官方颜色 #DC322F）
    if (type_lower == "scala") {
        return ftxui::Color::RGB(220, 50, 47); // 红色
    }

    // Clojure - 绿色
    if (type_lower == "clojure") {
        return ftxui::Color::RGB(91, 227, 106); // 绿色
    }

    // Groovy - 绿色
    if (type_lower == "groovy") {
        return ftxui::Color::RGB(100, 200, 150); // 绿色
    }

    // Swift - 橙色（Swift 官方颜色 #FA7343）
    if (type_lower == "swift") {
        return ftxui::Color::RGB(250, 115, 67); // 橙色
    }

    // Objective-C - 蓝色
    if (type_lower == "objective-c" || type_lower == "objc") {
        return ftxui::Color::RGB(100, 150, 255); // 蓝色
    }

    // C# - 紫色（.NET 官方颜色 #512BD4）
    if (type_lower == "csharp" || type_lower == "c#") {
        return ftxui::Color::RGB(81, 43, 212); // 紫色
    }

    // F# - 青色
    if (type_lower == "fsharp" || type_lower == "f#") {
        return ftxui::Color::RGB(110, 180, 220); // 青色
    }

    // PHP - 蓝色（PHP 官方颜色 #777BB4）
    if (type_lower == "php") {
        return ftxui::Color::RGB(119, 123, 180); // 蓝色
    }

    // Ruby - 红色（Ruby 官方颜色 #CC342D）
    if (type_lower == "ruby") {
        return ftxui::Color::RGB(204, 52, 45); // 红色
    }

    // Perl - 蓝色
    if (type_lower == "perl") {
        return ftxui::Color::RGB(0, 118, 192); // 蓝色
    }

    // Lua - 蓝色
    if (type_lower == "lua") {
        return ftxui::Color::RGB(0, 0, 128); // 深蓝色
    }

    // R - 蓝色
    if (type_lower == "r") {
        return ftxui::Color::RGB(26, 71, 111); // 蓝色
    }

    // MATLAB - 橙色
    if (type_lower == "matlab") {
        return ftxui::Color::RGB(237, 139, 0); // 橙色
    }

    // Julia - 紫色（Julia 官方颜色 #9558B2）
    if (type_lower == "julia") {
        return ftxui::Color::RGB(149, 88, 178); // 紫色
    }

    // Dart - 蓝色（Dart 官方颜色 #0175C2）
    if (type_lower == "dart") {
        return ftxui::Color::RGB(1, 117, 194); // 蓝色
    }

    // Elixir - 紫色（Elixir 官方颜色 #4B275F）
    if (type_lower == "elixir") {
        return ftxui::Color::RGB(75, 39, 95); // 紫色
    }

    // Erlang - 红色
    if (type_lower == "erlang") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // Haskell - 紫色
    if (type_lower == "haskell") {
        return ftxui::Color::RGB(94, 80, 134); // 紫色
    }

    // OCaml - 橙色
    if (type_lower == "ocaml") {
        return ftxui::Color::RGB(237, 139, 0); // 橙色
    }

    // Crystal - 白色/灰色
    if (type_lower == "crystal") {
        return ftxui::Color::RGB(235, 235, 230); // 白色
    }

    // TCL - 蓝色
    if (type_lower == "tcl") {
        return ftxui::Color::RGB(228, 228, 228); // 浅灰色
    }

    // Fortran - 蓝色
    if (type_lower == "fortran") {
        return ftxui::Color::RGB(77, 144, 142); // 青色
    }

    // Meson - 蓝色
    if (type_lower == "meson") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Coq - 蓝色
    if (type_lower == "coq") {
        return ftxui::Color::RGB(0, 128, 128); // 青色
    }

    // Agda - 蓝色
    if (type_lower == "agda") {
        return ftxui::Color::RGB(0, 128, 128); // 青色
    }

    // Idris - 紫色
    if (type_lower == "idris") {
        return ftxui::Color::RGB(94, 80, 134); // 紫色
    }

    // PureScript - 红色
    if (type_lower == "purescript") {
        return ftxui::Color::RGB(29, 34, 45); // 深色
    }

    // Reason - 橙色
    if (type_lower == "reason") {
        return ftxui::Color::RGB(221, 74, 104); // 红色
    }

    // SML - 橙色
    if (type_lower == "sml") {
        return ftxui::Color::RGB(237, 139, 0); // 橙色
    }

    // Carbon - 蓝色
    if (type_lower == "carbon") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Vala - 紫色
    if (type_lower == "vala") {
        return ftxui::Color::RGB(149, 88, 178); // 紫色
    }

    // Genie - 蓝色
    if (type_lower == "genie") {
        return ftxui::Color::RGB(100, 150, 255); // 蓝色
    }

    // D language - 红色
    if (type_lower == "dlang" || type_lower == "d") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // Pony - 蓝色
    if (type_lower == "pony") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // V language - 蓝色
    if (type_lower == "vlang" || type_lower == "v") {
        return ftxui::Color::RGB(91, 227, 106); // 绿色
    }

    // Odin - 蓝色
    if (type_lower == "odin") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Jai - 蓝色
    if (type_lower == "jai") {
        return ftxui::Color::RGB(100, 150, 255); // 蓝色
    }

    // Nelua - 蓝色
    if (type_lower == "nelua") {
        return ftxui::Color::RGB(100, 150, 255); // 蓝色
    }

    // Wren - 红色
    if (type_lower == "wren") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // MoonScript - 蓝色
    if (type_lower == "moonscript") {
        return ftxui::Color::RGB(100, 150, 255); // 蓝色
    }

    // Fantom - 蓝色
    if (type_lower == "fantom") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Smalltalk - 红色
    if (type_lower == "smalltalk") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // APL - 蓝色
    if (type_lower == "apl") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // J language - 蓝色
    if (type_lower == "jlang") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // K language - 蓝色
    if (type_lower == "klang") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Q language - 蓝色
    if (type_lower == "qlang") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // ========== Web 技术 ==========
    // HTML - 橙色（HTML5 官方颜色 #E34C26）
    if (type_lower == "html") {
        return ftxui::Color::RGB(227, 76, 38); // 橙色
    }

    // CSS - 蓝色（CSS 官方颜色 #264DE4）
    if (type_lower == "css") {
        return ftxui::Color::RGB(38, 77, 228); // 蓝色
    }

    // SCSS/Sass - 粉色
    if (type_lower == "scss" || type_lower == "sass") {
        return ftxui::Color::RGB(207, 100, 154); // 粉色
    }

    // Less - 蓝色
    if (type_lower == "less") {
        return ftxui::Color::RGB(29, 54, 93); // 蓝色
    }

    // Stylus - 绿色
    if (type_lower == "stylus") {
        return ftxui::Color::RGB(100, 200, 150); // 绿色
    }

    // Vue - 绿色（Vue 官方颜色 #4FC08D）
    if (type_lower == "vue") {
        return ftxui::Color::RGB(79, 192, 141); // 绿色
    }

    // React/JSX - 青色（React 官方颜色 #61DAFB）
    if (type_lower == "jsx" || type_lower == "react") {
        return ftxui::Color::RGB(97, 218, 251); // 青色
    }

    // Angular - 红色（Angular 官方颜色 #DD0031）
    if (type_lower == "angular") {
        return ftxui::Color::RGB(221, 0, 49); // 红色
    }

    // Svelte - 橙色
    if (type_lower == "svelte") {
        return ftxui::Color::RGB(255, 62, 0); // 橙色
    }

    // CoffeeScript - 棕色
    if (type_lower == "coffeescript") {
        return ftxui::Color::RGB(36, 71, 118); // 深蓝色
    }

    // Pug (Jade) - 绿色
    if (type_lower == "pug" || type_lower == "jade") {
        return ftxui::Color::RGB(169, 204, 41); // 绿色
    }

    // PostCSS - 红色
    if (type_lower == "postcss") {
        return ftxui::Color::RGB(221, 0, 49); // 红色
    }

    // GraphQL - 粉色
    if (type_lower == "graphql") {
        return ftxui::Color::RGB(225, 29, 132); // 粉色
    }

    // ========== 数据格式 ==========
    // JSON - 黄色
    if (type_lower == "json") {
        return ftxui::Color::RGB(255, 200, 0); // 黄色
    }

    // XML - 橙色
    if (type_lower == "xml") {
        return ftxui::Color::RGB(255, 165, 0); // 橙色
    }

    // YAML - 灰色（配置文件）
    if (type_lower == "yaml" || type_lower == "yml") {
        return ftxui::Color::RGB(140, 135, 120); // 灰色
    }

    // TOML - 蓝色
    if (type_lower == "toml") {
        return ftxui::Color::RGB(156, 220, 254); // 蓝色
    }

    // INI - 灰色（配置文件）
    if (type_lower == "ini") {
        return ftxui::Color::RGB(140, 135, 120); // 灰色
    }

    // CSV - 绿色（数据文件）
    if (type_lower == "csv" || type_lower == "tsv") {
        return ftxui::Color::RGB(100, 200, 150); // 绿色
    }

    // ========== 脚本语言 ==========
    // Shell/Bash - 绿色（终端颜色）
    if (type_lower == "shell" || type_lower == "bash" || type_lower == "sh") {
        return ftxui::Color::RGB(100, 200, 150); // 绿色
    }

    // PowerShell - 蓝色
    if (type_lower == "powershell") {
        return ftxui::Color::RGB(1, 36, 86); // 蓝色
    }

    // Zsh - 绿色
    if (type_lower == "zsh") {
        return ftxui::Color::RGB(100, 200, 150); // 绿色
    }

    // Fish - 蓝色
    if (type_lower == "fish") {
        return ftxui::Color::RGB(100, 150, 255); // 蓝色
    }

    // ========== 构建工具 ==========
    // CMake - 蓝色
    if (type_lower == "cmake") {
        return ftxui::Color::RGB(64, 128, 174); // 蓝色
    }

    // Makefile - 黄色
    if (type_lower == "makefile") {
        return ftxui::Color::RGB(255, 200, 0); // 黄色
    }

    // Dockerfile - 蓝色（Docker 官方颜色 #0DB7ED）
    if (type_lower == "dockerfile") {
        return ftxui::Color::RGB(13, 183, 237); // 蓝色
    }

    // ========== 配置文件 ==========
    // Git 配置文件 - 灰色
    if (type_lower == "git" || type_lower == "gitignore" || type_lower == "gitattributes") {
        return ftxui::Color::RGB(140, 135, 120); // 灰色
    }

    // 环境变量文件 - 灰色
    if (type_lower == "env" || type_lower == "dotenv") {
        return ftxui::Color::RGB(140, 135, 120); // 灰色
    }

    // 编辑器配置 - 灰色
    if (type_lower == "editorconfig" || type_lower == "vim" || type_lower == "vimrc") {
        return ftxui::Color::RGB(140, 135, 120); // 灰色
    }

    // ========== 文档 ==========
    // Markdown - 白色（文档）
    if (type_lower == "markdown" || type_lower == "md") {
        return ftxui::Color::RGB(235, 235, 230); // 白色
    }

    // Text - 白色
    if (type_lower == "text") {
        return ftxui::Color::RGB(235, 235, 230); // 白色
    }

    // LaTeX - 蓝色
    if (type_lower == "latex" || type_lower == "tex") {
        return ftxui::Color::RGB(0, 128, 128); // 青色
    }

    // ========== 数据库 ==========
    // SQL - 蓝色
    if (type_lower == "sql") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // ========== 函数式编程语言 ==========
    // Racket - 红色
    if (type_lower == "racket") {
        return ftxui::Color::RGB(158, 1, 66); // 红色
    }

    // Scheme - 红色
    if (type_lower == "scheme") {
        return ftxui::Color::RGB(158, 1, 66); // 红色
    }

    // Common Lisp - 紫色
    if (type_lower == "commonlisp" || type_lower == "lisp") {
        return ftxui::Color::RGB(149, 88, 178); // 紫色
    }

    // Emacs Lisp - 紫色
    if (type_lower == "emacslisp") {
        return ftxui::Color::RGB(149, 88, 178); // 紫色
    }

    // Prolog - 蓝色
    if (type_lower == "prolog") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Mercury - 橙色
    if (type_lower == "mercury") {
        return ftxui::Color::RGB(237, 139, 0); // 橙色
    }

    // Alloy - 蓝色
    if (type_lower == "alloy") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Dafny - 蓝色
    if (type_lower == "dafny") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Lean - 绿色
    if (type_lower == "lean") {
        return ftxui::Color::RGB(91, 227, 106); // 绿色
    }

    // Ballerina - 红色
    if (type_lower == "ballerina") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // ========== 区块链语言 ==========
    // Solidity - 灰色
    if (type_lower == "solidity") {
        return ftxui::Color::RGB(100, 100, 100); // 灰色
    }

    // Vyper - 蓝色
    if (type_lower == "vyper") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Cadence - 蓝色
    if (type_lower == "cadence") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Clarity - 蓝色
    if (type_lower == "clarity") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // ========== 硬件描述语言 ==========
    // Verilog - 蓝色
    if (type_lower == "verilog") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // VHDL - 蓝色
    if (type_lower == "vhdl") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // WebAssembly - 黄色
    if (type_lower == "webassembly" || type_lower == "wasm" || type_lower == "wat") {
        return ftxui::Color::RGB(255, 200, 0); // 黄色
    }

    // SPIR-V - 蓝色
    if (type_lower == "spirv") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // ========== 其他语言 ==========
    // Octave - 蓝色
    if (type_lower == "octave") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Visual Basic - 蓝色
    if (type_lower == "vb" || type_lower == "vbs") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // ========== 媒体文件 ==========
    // 图像文件 - 紫色
    if (type_lower == "image" || type_lower == "png" || type_lower == "jpg" ||
        type_lower == "jpeg" || type_lower == "gif" || type_lower == "svg") {
        return ftxui::Color::RGB(180, 140, 255); // 紫色
    }

    // 音频文件 - 红色
    if (type_lower == "audio" || type_lower == "mp3" || type_lower == "wav" ||
        type_lower == "flac") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // 视频文件 - 深红色
    if (type_lower == "video" || type_lower == "mp4" || type_lower == "avi" ||
        type_lower == "mkv") {
        return ftxui::Color::RGB(220, 50, 47); // 深红色
    }

    // ========== 压缩文件 ==========
    // 压缩文件 - 红色
    if (type_lower == "zip" || type_lower == "tar" || type_lower == "gz" || type_lower == "rar" ||
        type_lower == "7z") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // ========== 二进制文件 ==========
    // 可执行文件 - 深灰色
    if (type_lower == "exe" || type_lower == "bin" || type_lower == "dll" || type_lower == "so" ||
        type_lower == "dylib") {
        return ftxui::Color::RGB(100, 100, 100); // 深灰色
    }

    // PDF - 红色
    if (type_lower == "pdf") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // ========== 日志文件 ==========
    // 日志文件 - 灰色
    if (type_lower == "log") {
        return ftxui::Color::RGB(140, 135, 120); // 灰色
    }

    // 默认颜色 - 灰色（未知类型）
    return ftxui::Color::RGB(140, 135, 120); // 灰色
}

std::string FileTypeColorMapper::getFileExtension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == 0) {
        return "";
    }

    std::string ext = filename.substr(dot_pos + 1);
    // 转换为小写
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

} // namespace utils
} // namespace pnana
