#include "utils/file_type_detector.h"
#include "utils/assembly_analyzer.h"
#include <algorithm>
#include <filesystem>

namespace pnana {
namespace utils {

std::string FileTypeDetector::detectFileType(const std::string& filename,
                                             const std::string& extension) {
    // 检查文件名（用于特殊文件名如 CMakeLists.txt, Portfile 等）
    std::string filename_lower = filename;
    std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);

    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

    // 特殊文件名检测（大小写不敏感）
    if (filename_lower == "cmakelists.txt" || filename_lower == "cmake.in" ||
        filename_lower == "cmake.in.in" || filename_lower.find("cmakelists") != std::string::npos) {
        return "cmake";
    }
    if (filename_lower == "portfile") {
        return "tcl"; // MacPorts portfiles are TCL
    }

    // 文件扩展名检测（大小写不敏感）
    // C/C++
    if (ext_lower == "cpp" || ext_lower == "cc" || ext_lower == "cxx" || ext_lower == "h" ||
        ext_lower == "hpp" || ext_lower == "hxx" || ext_lower == "hh" || ext_lower == "c++" ||
        ext_lower == "h++")
        return "cpp";
    if (ext_lower == "c")
        return "c";

    // Python
    if (ext_lower == "py" || ext_lower == "pyw" || ext_lower == "pyi")
        return "python";

    // JavaScript/TypeScript
    if (ext_lower == "js" || ext_lower == "jsx" || ext_lower == "mjs")
        return "javascript";
    if (ext_lower == "ts" || ext_lower == "tsx")
        return "typescript";

    // Java
    if (ext_lower == "java")
        return "java";

    // Go
    if (ext_lower == "go")
        return "go";

    // Rust
    if (ext_lower == "rs")
        return "rust";

    // Markdown
    if (ext_lower == "md" || ext_lower == "markdown")
        return "markdown";

    // JSON
    if (ext_lower == "json" || ext_lower == "jsonc")
        return "json";

    // HTML/CSS
    if (ext_lower == "html" || ext_lower == "htm")
        return "html";
    if (ext_lower == "css")
        return "css";

    // Shell
    if (ext_lower == "sh" || ext_lower == "bash" || ext_lower == "zsh" || ext_lower == "shell")
        return "shell";

    // Lua
    if (ext_lower == "lua")
        return "lua";

    // CMake
    if (ext_lower == "cmake")
        return "cmake";

    // TCL
    if (ext_lower == "tcl" || ext_lower == "tk")
        return "tcl";

    // Fortran
    if (ext_lower == "f90" || ext_lower == "f95" || ext_lower == "f03" || ext_lower == "f08" ||
        ext_lower == "f" || ext_lower == "for" || ext_lower == "ftn" || ext_lower == "fpp" ||
        ext_lower == "fortran")
        return "fortran";

    // Haskell
    if (ext_lower == "hs" || ext_lower == "lhs" || ext_lower == "haskell")
        return "haskell";

    // 新增语言支持
    // YAML
    if (ext_lower == "yaml" || ext_lower == "yml")
        return "yaml";

    // XML
    if (ext_lower == "xml" || ext_lower == "svg" || ext_lower == "xhtml")
        return "xml";

    // SQL
    if (ext_lower == "sql" || ext_lower == "mysql" || ext_lower == "postgresql" ||
        ext_lower == "sqlite" || ext_lower == "oracle" || ext_lower == "mssql")
        return "sql";

    // Ruby
    if (ext_lower == "rb" || ext_lower == "ruby" || ext_lower == "rake" || ext_lower == "gemspec")
        return "ruby";

    // PHP
    if (ext_lower == "php" || ext_lower == "phtml" || ext_lower == "php3" || ext_lower == "php4" ||
        ext_lower == "php5" || ext_lower == "php7")
        return "php";

    // Swift
    if (ext_lower == "swift")
        return "swift";

    // Kotlin
    if (ext_lower == "kt" || ext_lower == "kotlin" || ext_lower == "kts")
        return "kotlin";

    // Scala
    if (ext_lower == "scala" || ext_lower == "sc")
        return "scala";

    // R
    if (ext_lower == "r" || ext_lower == "rmd" || ext_lower == "rscript")
        return "r";

    // Perl
    if (ext_lower == "pl" || ext_lower == "pm" || ext_lower == "perl" || ext_lower == "pod")
        return "perl";

    // Dockerfile
    if (filename_lower == "dockerfile" || filename_lower == "containerfile")
        return "dockerfile";

    // Makefile
    if (filename_lower == "makefile" || filename_lower == "makefile.in" || ext_lower == "mk")
        return "makefile";

    // Vim
    if (filename_lower == "vimrc" || ext_lower == "vim")
        return "vim";

    // PowerShell
    if (ext_lower == "ps1" || ext_lower == "powershell" || ext_lower == "psm1" ||
        ext_lower == "psd1")
        return "powershell";

    // Elixir
    if (ext_lower == "ex" || ext_lower == "exs")
        return "elixir";

    // Clojure
    if (ext_lower == "clj" || ext_lower == "cljs" || ext_lower == "cljc" || ext_lower == "edn")
        return "clojure";

    // Erlang
    if (ext_lower == "erl" || ext_lower == "hrl" || ext_lower == "escript")
        return "erlang";

    // Julia
    if (ext_lower == "jl")
        return "julia";

    // Dart
    if (ext_lower == "dart")
        return "dart";

    // Nim
    if (ext_lower == "nim" || ext_lower == "nims" || ext_lower == "nimble")
        return "nim";

    // Meson
    if (filename_lower == "meson.build" || filename_lower == "meson_options.txt")
        return "meson";

    // TOML
    if (ext_lower == "toml")
        return "toml";

    // Crystal
    if (ext_lower == "cr")
        return "crystal";

    // Zig
    if (ext_lower == "zig")
        return "zig";

    // OCaml
    if (ext_lower == "ml" || ext_lower == "mli")
        return "ocaml";

    // Coq
    if (ext_lower == "v")
        return "coq";

    // Agda
    if (ext_lower == "agda" || ext_lower == "lagda")
        return "agda";

    // Idris
    if (ext_lower == "idr" || ext_lower == "lidr")
        return "idris";

    // PureScript
    if (ext_lower == "purs")
        return "purescript";

    // Reason
    if (ext_lower == "re" || ext_lower == "rei")
        return "reason";

    // SML
    if (ext_lower == "sml" || ext_lower == "sig")
        return "sml";

    // Groovy
    if (ext_lower == "groovy" || ext_lower == "gvy" || ext_lower == "gy" || ext_lower == "gsh")
        return "groovy";

    // CoffeeScript
    if (ext_lower == "coffee" || ext_lower == "litcoffee")
        return "coffeescript";

    // Pug (Jade)
    if (ext_lower == "pug" || ext_lower == "jade")
        return "pug";

    // Stylus
    if (ext_lower == "styl")
        return "stylus";

    // Sass/SCSS
    if (ext_lower == "sass" || ext_lower == "scss")
        return "sass";

    // Less
    if (ext_lower == "less")
        return "less";

    // PostCSS
    if (ext_lower == "pcss" || filename_lower == "postcss.config.js" ||
        filename_lower == "postcss.config.ts")
        return "postcss";

    // GraphQL
    if (ext_lower == "graphql" || ext_lower == "gql")
        return "graphql";

    // Vue.js
    if (ext_lower == "vue")
        return "vue";

    // Svelte
    if (ext_lower == "svelte")
        return "svelte";

    // F#
    if (ext_lower == "fs" || ext_lower == "fsi" || ext_lower == "fsx" || ext_lower == "fsscript")
        return "fsharp";

    // C#
    if (ext_lower == "cs" || ext_lower == "csx")
        return "csharp";

    // Visual Basic
    if (ext_lower == "vb" || ext_lower == "vbs")
        return "vb";

    // Assembly - 基于内容智能检测不同架构和语法
    if (ext_lower == "asm" || ext_lower == "s" || ext_lower == "S") {
        // 首先尝试基于文件名进行初步检测
        std::string preliminary_type = "asm";
        if (filename_lower.find("riscv") != std::string::npos ||
            filename_lower.find("risc-v") != std::string::npos) {
            preliminary_type = "riscv";
        } else if (filename_lower.find("mips") != std::string::npos) {
            preliminary_type = "mips";
        } else if (filename_lower.find("arm") != std::string::npos ||
                   filename_lower.find("aarch64") != std::string::npos) {
            preliminary_type = "arm";
        } else if (filename_lower.find("x86") != std::string::npos ||
                   filename_lower.find("x64") != std::string::npos ||
                   filename_lower.find("amd64") != std::string::npos) {
            preliminary_type = "x86";
        }

        // 尝试读取文件内容进行精确检测
        try {
            std::filesystem::path filepath = std::filesystem::path(filename);
            if (std::filesystem::exists(filepath) && std::filesystem::is_regular_file(filepath)) {
                auto analysis_result = AssemblyAnalyzer::analyzeFile(filename, 50); // 分析前50行

                // 如果内容分析置信度足够高，使用分析结果
                if (analysis_result.confidence >= 0.6f) {
                    switch (analysis_result.arch) {
                        case AssemblyAnalyzer::Architecture::X86:
                            return "x86";
                        case AssemblyAnalyzer::Architecture::ARM:
                            return "arm";
                        case AssemblyAnalyzer::Architecture::RISCV:
                            return "riscv";
                        case AssemblyAnalyzer::Architecture::MIPS:
                            return "mips";
                        case AssemblyAnalyzer::Architecture::GENERIC:
                            return "asm";
                        default:
                            break;
                    }
                }
            }
        } catch (const std::exception&) {
            // 如果分析失败，使用基于文件名的检测结果
        }

        // 返回基于文件名的检测结果或默认类型
        return preliminary_type;
    }

    // 特定架构的汇编文件
    if (ext_lower == "spv" || filename_lower.find("spir-v") != std::string::npos) {
        return "spirv";
    }

    // WebAssembly
    if (ext_lower == "wat" || ext_lower == "wasm")
        return "webassembly";

    // Verilog
    if (ext_lower == "v" || ext_lower == "vh")
        return "verilog";

    // VHDL
    if (ext_lower == "vhdl" || ext_lower == "vhd")
        return "vhdl";

    // MATLAB
    if (ext_lower == "m")
        return "matlab";

    // Octave
    if (ext_lower == "m" && filename_lower.find("octave") != std::string::npos)
        return "octave";

    // Racket
    if (ext_lower == "rkt" || ext_lower == "rktl")
        return "racket";

    // Scheme
    if (ext_lower == "scm" || ext_lower == "ss")
        return "scheme";

    // Common Lisp
    if (ext_lower == "lisp" || ext_lower == "lsp" || ext_lower == "cl")
        return "commonlisp";

    // Emacs Lisp
    if (ext_lower == "el" || ext_lower == "elc")
        return "emacslisp";

    // Prolog
    if (ext_lower == "pl" || ext_lower == "pro")
        return "prolog";

    // Mercury
    if (ext_lower == "m")
        return "mercury";

    // Alloy
    if (ext_lower == "als")
        return "alloy";

    // Dafny
    if (ext_lower == "dfy")
        return "dafny";

    // Lean
    if (ext_lower == "lean")
        return "lean";

    // Ballerina
    if (ext_lower == "bal")
        return "ballerina";

    // Cadence
    if (ext_lower == "cdc")
        return "cadence";

    // Clarity
    if (ext_lower == "clar")
        return "clarity";

    // Solidity
    if (ext_lower == "sol")
        return "solidity";

    // Vyper
    if (ext_lower == "vy")
        return "vyper";

    // Carbon
    if (ext_lower == "carbon")
        return "carbon";

    // Vala
    if (ext_lower == "vala" || ext_lower == "vapi")
        return "vala";

    // Genie
    if (ext_lower == "gs")
        return "genie";

    // D language
    if (ext_lower == "d" || ext_lower == "di")
        return "dlang";

    // Pony
    if (ext_lower == "pony")
        return "pony";

    // V language
    if (ext_lower == "v")
        return "vlang";

    // Odin
    if (ext_lower == "odin")
        return "odin";

    // Jai
    if (ext_lower == "jai")
        return "jai";

    // Nelua
    if (ext_lower == "nelua")
        return "nelua";

    // Wren
    if (ext_lower == "wren")
        return "wren";

    // MoonScript
    if (ext_lower == "moon")
        return "moonscript";

    // Fantom
    if (ext_lower == "fan")
        return "fantom";

    // Smalltalk
    if (ext_lower == "st")
        return "smalltalk";

    // APL
    if (ext_lower == "apl")
        return "apl";

    // J language
    if (ext_lower == "ijx" || ext_lower == "ijt")
        return "jlang";

    // K language
    if (ext_lower == "k")
        return "klang";

    // Q language
    if (ext_lower == "q")
        return "qlang";

    return "text";
}

// 获取完整的文件类型信息（用于图标显示）
std::string FileTypeDetector::getFileTypeForIcon(const std::string& filename,
                                                 const std::string& extension) {
    // 先检测文件类型
    std::string file_type = detectFileType(filename, extension);

    // 对于特殊文件名，返回特定的类型字符串用于图标匹配
    std::string filename_lower = filename;
    std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);

    if (filename_lower == "cargo.toml") {
        return "cargo.toml";
    }
    if (filename_lower == "cargo.lock") {
        return "cargo.lock";
    }
    if (filename_lower == "gemfile") {
        return "gemfile";
    }
    if (filename_lower == "composer.json") {
        return "composer.json";
    }
    if (filename_lower == "go.mod") {
        return "go.mod";
    }
    if (filename_lower == "go.sum") {
        return "go.sum";
    }
    if (filename_lower == "requirements.txt") {
        return "requirements.txt";
    }
    if (filename_lower == "pyproject.toml") {
        return "pyproject.toml";
    }
    if (filename_lower == "meson.build") {
        return "meson.build";
    }
    if (filename_lower.find("docker-compose") != std::string::npos) {
        return "docker-compose.yml";
    }
    if (filename_lower == "dockerfile") {
        return "dockerfile";
    }
    if (filename_lower.find("readme") != std::string::npos) {
        return "readme";
    }
    if (filename_lower.find("license") != std::string::npos) {
        return "license";
    }
    if (filename_lower.find("changelog") != std::string::npos) {
        return "changelog";
    }

    // 对于图片文件，使用扩展名
    std::string ext_lower = extension;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

    if (ext_lower == "jpg" || ext_lower == "jpeg" || ext_lower == "png" || ext_lower == "gif" ||
        ext_lower == "bmp" || ext_lower == "webp") {
        return ext_lower;
    }

    // 对于视频文件
    if (ext_lower == "mp4" || ext_lower == "avi" || ext_lower == "mkv" || ext_lower == "mov" ||
        ext_lower == "wmv") {
        return ext_lower;
    }

    // 对于音频文件
    if (ext_lower == "mp3" || ext_lower == "wav" || ext_lower == "flac" || ext_lower == "aac" ||
        ext_lower == "ogg") {
        return ext_lower;
    }

    // 对于压缩文件
    if (ext_lower == "zip" || ext_lower == "tar" || ext_lower == "gz" || ext_lower == "rar" ||
        ext_lower == "7z") {
        return ext_lower;
    }

    // 对于数据库文件
    if (ext_lower == "db" || ext_lower == "sqlite") {
        return ext_lower;
    }

    // 对于可执行文件
    if (ext_lower == "exe" || ext_lower == "bin" || ext_lower == "out") {
        return ext_lower;
    }

    // 对于配置文件
    if (ext_lower == "config" || ext_lower == "conf" || ext_lower == "ini") {
        return ext_lower;
    }

    // 对于日志文件
    if (ext_lower == "log") {
        return ext_lower;
    }

    // 对于数据文件
    if (ext_lower == "csv" || ext_lower == "tsv") {
        return ext_lower;
    }

    // 返回标准文件类型
    return file_type;
}

} // namespace utils
} // namespace pnana
