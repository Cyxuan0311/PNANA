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

    // Assembly - 灰（底层）
    if (type_lower == "asm" || type_lower == "assembly") {
        return ftxui::Color::RGB(130, 128, 115); // 灰
    }

    // LLVM IR - 橙黄（LLVM 风格）
    if (type_lower == "llvm" || type_lower == "ll") {
        return ftxui::Color::RGB(185, 130, 45); // 橙黄
    }
    if (type_lower == "x86") {
        return ftxui::Color::RGB(120, 130, 140); // 灰蓝
    }
    if (type_lower == "arm") {
        return ftxui::Color::RGB(110, 135, 130); // 灰青
    }
    if (type_lower == "riscv") {
        return ftxui::Color::RGB(135, 125, 120); // 灰
    }
    if (type_lower == "mips") {
        return ftxui::Color::RGB(125, 130, 135); // 灰
    }

    // ========== 高级编程语言 ==========
    // Python - 蓝色/青色（Python 官方颜色 #3776AB）
    if (type_lower == "python") {
        return ftxui::Color::RGB(55, 118, 171); // 蓝色
    }

    // JavaScript - 黄色（JS 官方 #F7DF1E）
    if (type_lower == "javascript") {
        return ftxui::Color::RGB(247, 223, 30); // 黄
    }
    // TypeScript - 蓝色（TS 官方 #3178C6）
    if (type_lower == "typescript") {
        return ftxui::Color::RGB(49, 120, 198); // #3178C6 蓝
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

    // Lua - 深蓝（Lua 官方风格）
    if (type_lower == "lua") {
        return ftxui::Color::RGB(46, 80, 126); // #2E507E 深蓝
    }

    // R - 蓝（R 统计语言）
    if (type_lower == "r") {
        return ftxui::Color::RGB(25, 118, 210); // #1976D2 蓝
    }

    // MATLAB - 橙（MATLAB 品牌）
    if (type_lower == "matlab") {
        return ftxui::Color::RGB(237, 139, 0); // #ED8B00
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

    // Erlang - 红（Erlang 品牌）
    if (type_lower == "erlang") {
        return ftxui::Color::RGB(163, 31, 52); // #A31F34
    }

    // Haskell - 紫（Haskell 风格）
    if (type_lower == "haskell") {
        return ftxui::Color::RGB(120, 90, 160); // #785AA0 紫
    }

    // OCaml - 琥珀（OCaml 品牌）
    if (type_lower == "ocaml") {
        return ftxui::Color::RGB(236, 140, 0); // #EC8C00
    }

    // Crystal - 白/银（Crystal 风格）
    if (type_lower == "crystal") {
        return ftxui::Color::RGB(200, 205, 210); // #C8CDD2 银白
    }

    // TCL - 蓝灰
    if (type_lower == "tcl") {
        return ftxui::Color::RGB(90, 130, 180); // #5A82B4 蓝灰
    }

    // Fortran - 蓝色
    if (type_lower == "fortran") {
        return ftxui::Color::RGB(77, 144, 142); // 青色
    }

    // Meson - 蓝色
    if (type_lower == "meson") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝色
    }

    // Coq - 青（证明助手）
    if (type_lower == "coq") {
        return ftxui::Color::RGB(0, 140, 150); // #008C96 青
    }

    // Agda - 紫青（与 Coq 区分）
    if (type_lower == "agda") {
        return ftxui::Color::RGB(70, 100, 150); // #466496 紫青
    }

    // Idris - 紫（与 Haskell 区分）
    if (type_lower == "idris") {
        return ftxui::Color::RGB(130, 90, 160); // #825AA0 紫
    }

    // PureScript - 紫灰（可读）
    if (type_lower == "purescript") {
        return ftxui::Color::RGB(137, 99, 181); // #8963B5 紫
    }

    // Reason - 粉红（ReasonML）
    if (type_lower == "reason") {
        return ftxui::Color::RGB(221, 74, 104); // #DD4A68 粉红
    }

    // SML - 橙色
    if (type_lower == "sml") {
        return ftxui::Color::RGB(237, 139, 0); // 橙色
    }

    // Carbon - 蓝（Google Carbon）
    if (type_lower == "carbon") {
        return ftxui::Color::RGB(50, 120, 200); // #3278C8
    }

    // Vala - 紫（GNOME/Vala）
    if (type_lower == "vala") {
        return ftxui::Color::RGB(149, 88, 178); // #9558B2
    }

    // Genie - 黄绿（Genie 风格）
    if (type_lower == "genie") {
        return ftxui::Color::RGB(150, 190, 80); // #96BE50
    }

    // D language - 红色
    if (type_lower == "dlang") {
        return ftxui::Color::RGB(255, 85, 110); // 红色
    }

    // Pony - 淡紫
    if (type_lower == "pony") {
        return ftxui::Color::RGB(180, 140, 200); // #B48CC8 淡紫
    }

    // V language - 青绿（V 官方风格）
    if (type_lower == "vlang") {
        return ftxui::Color::RGB(0, 170, 160); // #00AAA0 青绿
    }

    // Odin - 青（Odin 风格）
    if (type_lower == "odin") {
        return ftxui::Color::RGB(90, 160, 200); // #5AA0C8 青
    }

    // Jai - 青蓝
    if (type_lower == "jai") {
        return ftxui::Color::RGB(80, 140, 200); // #508CC8 青蓝
    }

    // Nelua - 蓝（Lua 系）
    if (type_lower == "nelua") {
        return ftxui::Color::RGB(60, 110, 170); // #3C6EAA 蓝
    }

    // Wren - 琥珀
    if (type_lower == "wren") {
        return ftxui::Color::RGB(220, 160, 60); // #DCA03C 琥珀
    }

    // MoonScript - 紫灰（Lua 系）
    if (type_lower == "moonscript") {
        return ftxui::Color::RGB(140, 110, 180); // #8C6EB4 紫灰
    }

    // Fantom - 深蓝
    if (type_lower == "fantom") {
        return ftxui::Color::RGB(40, 100, 160); // #2864A0 深蓝
    }

    // Smalltalk - 绿（Smalltalk 传统）
    if (type_lower == "smalltalk") {
        return ftxui::Color::RGB(89, 166, 89); // #59A659 绿
    }

    // APL - 青
    if (type_lower == "apl") {
        return ftxui::Color::RGB(0, 140, 180); // #008CB4 青
    }

    // J language - 蓝灰
    if (type_lower == "jlang") {
        return ftxui::Color::RGB(70, 120, 170); // #4678AA
    }

    // K language - 深蓝
    if (type_lower == "klang") {
        return ftxui::Color::RGB(50, 100, 160); // #3264A0
    }

    // Q language - 蓝
    if (type_lower == "qlang") {
        return ftxui::Color::RGB(80, 130, 190); // #5082BE
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

    // Less - 深蓝（Less 官方 #1D365D）
    if (type_lower == "less") {
        return ftxui::Color::RGB(29, 54, 93); // #1D365D
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

    // Svelte - 橙（Svelte 品牌）
    if (type_lower == "svelte") {
        return ftxui::Color::RGB(255, 62, 0); // #FF3E00
    }

    // CoffeeScript - 深棕蓝
    if (type_lower == "coffeescript") {
        return ftxui::Color::RGB(45, 80, 130); // #2D5082
    }

    // Pug (Jade) - 绿色
    if (type_lower == "pug" || type_lower == "jade") {
        return ftxui::Color::RGB(169, 204, 41); // 绿色
    }

    // PostCSS - 红灰
    if (type_lower == "postcss") {
        return ftxui::Color::RGB(220, 50, 90); // #DC325A
    }

    // GraphQL - 粉（GraphQL 品牌）
    if (type_lower == "graphql" || type_lower == "gql") {
        return ftxui::Color::RGB(225, 29, 132); // #E11D84
    }

    // ========== 依赖与项目配置（按检测器返回的 type） ==========
    // Cargo (Rust)
    if (type_lower == "cargo.toml") {
        return ftxui::Color::RGB(206, 65, 43); // 同 Rust 橙
    }
    if (type_lower == "cargo.lock") {
        return ftxui::Color::RGB(160, 100, 80); // 棕灰
    }
    // Ruby
    if (type_lower == "gemfile") {
        return ftxui::Color::RGB(204, 52, 45); // 同 Ruby 红
    }
    // PHP
    if (type_lower == "composer.json") {
        return ftxui::Color::RGB(119, 123, 180); // 同 PHP 紫蓝
    }
    // Go
    if (type_lower == "go.mod") {
        return ftxui::Color::RGB(0, 173, 216); // 同 Go 青
    }
    if (type_lower == "go.sum") {
        return ftxui::Color::RGB(80, 140, 160); // 灰青
    }
    // Python
    if (type_lower == "requirements.txt") {
        return ftxui::Color::RGB(55, 118, 171); // 同 Python 蓝
    }
    if (type_lower == "pyproject.toml") {
        return ftxui::Color::RGB(45, 95, 140); // 深蓝
    }
    // Meson
    if (type_lower == "meson.build") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝
    }
    // Docker Compose
    if (type_lower == "docker-compose.yml") {
        return ftxui::Color::RGB(13, 183, 237); // 同 Docker 蓝
    }
    // 文档类
    if (type_lower == "readme") {
        return ftxui::Color::RGB(200, 220, 240); // 淡蓝白
    }
    if (type_lower == "license" || type_lower == "licence") {
        return ftxui::Color::RGB(120, 120, 120); // 灰
    }
    if (type_lower == "changelog") {
        return ftxui::Color::RGB(130, 140, 150); // 灰蓝
    }
    // Protobuf
    if (type_lower == "proto") {
        return ftxui::Color::RGB(100, 130, 160); // #6482A0 灰蓝
    }

    // ========== 数据格式 ==========
    // JSON - 黄
    if (type_lower == "json") {
        return ftxui::Color::RGB(255, 200, 0); // 黄
    }
    // JSONC / JSON5 - 琥珀（带注释的 JSON）
    if (type_lower == "jsonc" || type_lower == "json5") {
        return ftxui::Color::RGB(255, 190, 60); // #FFBE3C 琥珀
    }
    // RON (Rusty Object Notation) - 橙褐
    if (type_lower == "ron") {
        return ftxui::Color::RGB(210, 130, 70); // #D28246
    }

    // XML - 橙
    if (type_lower == "xml") {
        return ftxui::Color::RGB(255, 165, 0); // 橙
    }
    // XAML - 蓝（微软标记）
    if (type_lower == "xaml") {
        return ftxui::Color::RGB(0, 120, 215); // #0078D7
    }

    // YAML - 红紫（配置文件，易与 JSON 区分）
    if (type_lower == "yaml" || type_lower == "yml") {
        return ftxui::Color::RGB(203, 108, 128); // #CB6C80 玫红
    }

    // TOML - 淡蓝（Rust/Python 配置）
    if (type_lower == "toml") {
        return ftxui::Color::RGB(156, 220, 254); // #9CDCFE 淡蓝
    }

    // INI - 灰青（配置文件）
    if (type_lower == "ini") {
        return ftxui::Color::RGB(110, 130, 140); // #6E828C 灰青
    }

    // CSV/TSV - 绿（数据表）
    if (type_lower == "csv") {
        return ftxui::Color::RGB(80, 180, 120); // #50B478 绿
    }
    if (type_lower == "tsv") {
        return ftxui::Color::RGB(70, 160, 110); // 略深绿
    }

    // ========== 脚本语言 ==========
    // Shell/Bash/Sh - 绿（终端）
    if (type_lower == "shell" || type_lower == "bash" || type_lower == "sh") {
        return ftxui::Color::RGB(65, 180, 90); // #41B45A 绿
    }

    // PowerShell - 蓝（PowerShell 官方 #5391FE）
    if (type_lower == "powershell") {
        return ftxui::Color::RGB(83, 145, 254); // #5391FE
    }

    // Zsh - 黄绿
    if (type_lower == "zsh") {
        return ftxui::Color::RGB(220, 200, 60); // #DCC83C 黄绿
    }

    // Fish - 青（Fish 官方风格）
    if (type_lower == "fish") {
        return ftxui::Color::RGB(70, 160, 200); // #46A0C8 青
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
    // Git - 橙灰（Git 品牌）
    if (type_lower == "git") {
        return ftxui::Color::RGB(240, 80, 51); // #F05033 橙红
    }
    if (type_lower == "gitignore" || type_lower == "gitattributes" || type_lower == "gitmodules") {
        return ftxui::Color::RGB(130, 125, 115); // 灰
    }

    // 环境变量 - 绿灰（env 常见联想）
    if (type_lower == "env" || type_lower == "dotenv") {
        return ftxui::Color::RGB(100, 150, 110); // #64966E 绿灰
    }

    // 编辑器配置 - 灰
    if (type_lower == "editorconfig") {
        return ftxui::Color::RGB(110, 130, 140); // 灰青
    }
    // Vim - 绿（Vim 经典绿）
    if (type_lower == "vim" || type_lower == "vimrc") {
        return ftxui::Color::RGB(1, 135, 95); // #01875F 维姆绿
    }

    // ========== 文档与标记 ==========
    // Markdown - 白/米
    if (type_lower == "markdown" || type_lower == "md") {
        return ftxui::Color::RGB(235, 235, 230); // 米白
    }

    // Text - 白
    if (type_lower == "text") {
        return ftxui::Color::RGB(220, 220, 215); // #DCDCD7
    }

    // LaTeX / TeX - 青
    if (type_lower == "latex" || type_lower == "tex") {
        return ftxui::Color::RGB(0, 128, 128); // 青
    }
    // BibTeX - 琥珀
    if (type_lower == "bib") {
        return ftxui::Color::RGB(220, 170, 50); // #DCAA32
    }
    // reStructuredText - 蓝
    if (type_lower == "rst") {
        return ftxui::Color::RGB(20, 120, 160); // #1478A0
    }
    // AsciiDoc - 蓝灰
    if (type_lower == "asciidoc" || type_lower == "adoc") {
        return ftxui::Color::RGB(80, 110, 140); // #506E8C
    }
    // Org - 绿（Org-mode）
    if (type_lower == "org") {
        return ftxui::Color::RGB(119, 170, 100); // #77AA64
    }

    // ========== 数据库与存储 ==========
    // SQL - 蓝
    if (type_lower == "sql") {
        return ftxui::Color::RGB(0, 122, 204); // 蓝
    }
    // 数据库文件 - 灰蓝
    if (type_lower == "db" || type_lower == "sqlite") {
        return ftxui::Color::RGB(60, 120, 160); // #3C78A0
    }

    // ========== 函数式编程语言 ==========
    // Racket - 红（Racket 品牌）
    if (type_lower == "racket") {
        return ftxui::Color::RGB(158, 1, 66); // #9E0142 红
    }

    // Scheme - 深红（与 Racket 区分）
    if (type_lower == "scheme") {
        return ftxui::Color::RGB(180, 40, 70); // #B42846
    }

    // Common Lisp - 紫
    if (type_lower == "commonlisp" || type_lower == "lisp") {
        return ftxui::Color::RGB(149, 88, 178); // 紫
    }

    // Emacs Lisp - 紫红（与 Common Lisp 区分）
    if (type_lower == "emacslisp") {
        return ftxui::Color::RGB(160, 100, 170); // #A064AA
    }

    // Prolog - 青蓝
    if (type_lower == "prolog") {
        return ftxui::Color::RGB(0, 110, 180); // #006EB4
    }

    // Mercury - 橙
    if (type_lower == "mercury") {
        return ftxui::Color::RGB(230, 120, 30); // #E6781E
    }

    // Alloy - 灰蓝（形式化建模）
    if (type_lower == "alloy") {
        return ftxui::Color::RGB(80, 110, 150); // #506E96
    }

    // Dafny - 蓝紫（验证语言）
    if (type_lower == "dafny") {
        return ftxui::Color::RGB(60, 90, 180); // #3C5AB4
    }

    // Lean - 绿（Lean 官方风格）
    if (type_lower == "lean") {
        return ftxui::Color::RGB(80, 200, 120); // #50C878 翠绿
    }

    // Ballerina - 红（Ballerina 品牌）
    if (type_lower == "ballerina") {
        return ftxui::Color::RGB(220, 50, 70); // #DC3246
    }

    // ========== 区块链语言 ==========
    // Solidity - 灰蓝（Solidity 风格）
    if (type_lower == "solidity") {
        return ftxui::Color::RGB(100, 130, 160); // #6482A0 灰蓝
    }

    // Vyper - 蓝（Python 系区块链）
    if (type_lower == "vyper") {
        return ftxui::Color::RGB(0, 130, 200); // #0082C8
    }

    // Cadence - 紫（Flow 链）
    if (type_lower == "cadence") {
        return ftxui::Color::RGB(130, 80, 200); // #8250C8
    }

    // Clarity - 青绿（Stacks 链）
    if (type_lower == "clarity") {
        return ftxui::Color::RGB(50, 160, 140); // #32A08C
    }

    // ========== 硬件描述语言 ==========
    // Verilog - 青蓝（硬件描述）
    if (type_lower == "verilog") {
        return ftxui::Color::RGB(0, 154, 166); // #009AA6 青
    }

    // VHDL - 深紫（硬件描述，与 Verilog 区分）
    if (type_lower == "vhdl") {
        return ftxui::Color::RGB(108, 82, 156); // #6C529C 紫
    }

    // WebAssembly - 黄色
    if (type_lower == "webassembly" || type_lower == "wasm" || type_lower == "wat") {
        return ftxui::Color::RGB(255, 200, 0); // 黄色
    }

    // SPIR-V - 灰紫（着色器中间表示）
    if (type_lower == "spirv") {
        return ftxui::Color::RGB(100, 90, 140); // #645A8C
    }

    // ========== 其他语言 ==========
    // Octave - 蓝（与 MATLAB 区分，略偏青）
    if (type_lower == "octave") {
        return ftxui::Color::RGB(0, 115, 180); // #0073B4
    }

    // Visual Basic - 蓝（微软系）
    if (type_lower == "vb" || type_lower == "vbs") {
        return ftxui::Color::RGB(0, 102, 204); // #0066CC
    }

    // ========== 媒体文件 ==========
    // 图像 - 紫（通用）
    if (type_lower == "image") {
        return ftxui::Color::RGB(180, 140, 255); // 紫
    }
    // PNG/JPG/GIF/WebP - 青绿
    if (type_lower == "png" || type_lower == "jpg" || type_lower == "jpeg" || type_lower == "gif" ||
        type_lower == "webp" || type_lower == "bmp") {
        return ftxui::Color::RGB(80, 180, 160); // #50B4A0 青绿
    }
    // SVG - 橙（矢量）
    if (type_lower == "svg") {
        return ftxui::Color::RGB(255, 152, 0); // #FF9800 橙
    }

    // 音频 - 粉
    if (type_lower == "audio" || type_lower == "mp3" || type_lower == "wav" ||
        type_lower == "flac" || type_lower == "aac" || type_lower == "ogg") {
        return ftxui::Color::RGB(233, 130, 160); // #E982A0 粉
    }

    // 视频 - 红
    if (type_lower == "video" || type_lower == "mp4" || type_lower == "avi" ||
        type_lower == "mkv" || type_lower == "mov" || type_lower == "webm") {
        return ftxui::Color::RGB(220, 50, 47); // 红
    }

    // ========== 压缩与归档 ==========
    // ZIP - 琥珀
    if (type_lower == "zip") {
        return ftxui::Color::RGB(255, 180, 50); // #FFB432 琥珀
    }
    // TAR/GZ/BZ2/XZ - 棕红
    if (type_lower == "tar" || type_lower == "gz" || type_lower == "bz2" || type_lower == "xz" ||
        type_lower == "tgz") {
        return ftxui::Color::RGB(200, 90, 60); // #C85A3C 棕红
    }
    // RAR/7z - 深红
    if (type_lower == "rar" || type_lower == "7z") {
        return ftxui::Color::RGB(200, 60, 70); // #C83C46
    }

    // ========== 二进制文件 ==========
    // 可执行文件 - 深灰色
    if (type_lower == "exe" || type_lower == "bin" || type_lower == "dll" || type_lower == "so" ||
        type_lower == "dylib") {
        return ftxui::Color::RGB(100, 100, 100); // 深灰色
    }

    // PDF - 红（Adobe）
    if (type_lower == "pdf") {
        return ftxui::Color::RGB(230, 50, 50); // #E63232 红
    }

    // ========== 日志与临时 ==========
    if (type_lower == "log") {
        return ftxui::Color::RGB(130, 128, 118); // 灰
    }
    if (type_lower == "lock") {
        return ftxui::Color::RGB(100, 105, 110); // 深灰
    }
    if (type_lower == "tmp" || type_lower == "temp" || type_lower == "bak" || type_lower == "swp") {
        return ftxui::Color::RGB(100, 100, 100); // 深灰
    }

    // ========== 更多配置与基础设施 ==========
    // 通用配置扩展（供检测器扩展）
    if (type_lower == "config" || type_lower == "conf") {
        return ftxui::Color::RGB(115, 135, 145); // #738791 灰青
    }
    if (type_lower == "properties") {
        return ftxui::Color::RGB(130, 140, 130); // #8C8C82 灰绿
    }
    // HCL / Terraform - 紫（HashiCorp）
    if (type_lower == "hcl" || type_lower == "terraform" || type_lower == "tf") {
        return ftxui::Color::RGB(127, 80, 210); // #7F50D2
    }
    // Bicep - 橙蓝（Azure）
    if (type_lower == "bicep") {
        return ftxui::Color::RGB(230, 110, 50); // #E66E32
    }
    // Prisma - 青（ORM）
    if (type_lower == "prisma") {
        return ftxui::Color::RGB(0, 170, 170); // #00AAAA
    }
    // Nix - 青蓝
    if (type_lower == "nix") {
        return ftxui::Color::RGB(80, 135, 180); // #5087B4
    }
    // Just / Justfile - 灰黄
    if (type_lower == "just") {
        return ftxui::Color::RGB(200, 180, 100); // #C8B464
    }
    // Gherkin - 绿（BDD）
    if (type_lower == "gherkin" || type_lower == "feature") {
        return ftxui::Color::RGB(90, 160, 90); // #5AA05A
    }
    // 字幕 - 灰
    if (type_lower == "srt" || type_lower == "vtt" || type_lower == "ass" || type_lower == "ssa") {
        return ftxui::Color::RGB(120, 125, 130); // #787D82
    }
    // WMV - 视频格式单独色
    if (type_lower == "wmv") {
        return ftxui::Color::RGB(200, 60, 60); // #C83C3C
    }
    // 可执行/二进制 - 与 exe 区分
    if (type_lower == "out") {
        return ftxui::Color::RGB(90, 95, 100); // #5A5F64
    }

    // 默认颜色 - 灰（未知类型）
    return ftxui::Color::RGB(140, 135, 120); // 灰
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
