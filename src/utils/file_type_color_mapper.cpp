#include "utils/file_type_color_mapper.h"
#include "utils/file_type_detector.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace pnana {
namespace utils {

// 预定义颜色映射表，提升可维护性
static const std::unordered_map<std::string, ftxui::Color> FILE_TYPE_COLOR_MAP = {
    // ========== 系统/底层编程语言 ==========
    {"c", ftxui::Color::RGB(100, 150, 255)},         // 蓝色
    {"cpp", ftxui::Color::RGB(100, 150, 255)},       // 蓝色
    {"cxx", ftxui::Color::RGB(100, 150, 255)},       // 蓝色
    {"cc", ftxui::Color::RGB(100, 150, 255)},        // 蓝色
    {"rust", ftxui::Color::RGB(206, 65, 43)},        // 橙色（Rust 官方）
    {"rs", ftxui::Color::RGB(206, 65, 43)},          // Rust 别名
    {"go", ftxui::Color::RGB(0, 173, 216)},          // 青色（Go 官方）
    {"zig", ftxui::Color::RGB(247, 164, 29)},        // 黄色（Zig 官方）
    {"nim", ftxui::Color::RGB(255, 200, 0)},         // 黄色
    {"asm", ftxui::Color::RGB(130, 128, 115)},       // 灰色
    {"assembly", ftxui::Color::RGB(130, 128, 115)},  // 汇编
    {"llvm", ftxui::Color::RGB(185, 130, 45)},       // 橙黄
    {"ll", ftxui::Color::RGB(185, 130, 45)},         // LLVM IR
    {"x86", ftxui::Color::RGB(120, 130, 140)},       // 灰蓝
    {"arm", ftxui::Color::RGB(110, 135, 130)},       // 灰青
    {"riscv", ftxui::Color::RGB(135, 125, 120)},     // 灰色
    {"mips", ftxui::Color::RGB(125, 130, 135)},      // 灰蓝
    {"webassembly", ftxui::Color::RGB(255, 200, 0)}, // 黄色
    {"wasm", ftxui::Color::RGB(255, 200, 0)},        // WASM
    {"wat", ftxui::Color::RGB(255, 200, 0)},         // WASM 文本
    {"spirv", ftxui::Color::RGB(100, 90, 140)},      // 灰紫

    // ========== 高级编程语言 ==========
    {"python", ftxui::Color::RGB(55, 118, 171)},       // 蓝色（Python 官方）
    {"py", ftxui::Color::RGB(55, 118, 171)},           // Python 别名
    {"javascript", ftxui::Color::RGB(247, 223, 30)},   // 黄色（JS 官方）
    {"js", ftxui::Color::RGB(247, 223, 30)},           // JS 别名
    {"typescript", ftxui::Color::RGB(49, 120, 198)},   // 蓝色（TS 官方）
    {"ts", ftxui::Color::RGB(49, 120, 198)},           // TS 别名
    {"java", ftxui::Color::RGB(237, 139, 0)},          // 橙色（Java 官方）
    {"kotlin", ftxui::Color::RGB(127, 82, 255)},       // 紫色（Kotlin 官方）
    {"kt", ftxui::Color::RGB(127, 82, 255)},           // Kotlin 别名
    {"scala", ftxui::Color::RGB(220, 50, 47)},         // 红色（Scala 官方）
    {"clojure", ftxui::Color::RGB(91, 227, 106)},      // 绿色
    {"clj", ftxui::Color::RGB(91, 227, 106)},          // Clojure 别名
    {"groovy", ftxui::Color::RGB(100, 200, 150)},      // 绿色
    {"swift", ftxui::Color::RGB(250, 115, 67)},        // 橙色（Swift 官方）
    {"objective-c", ftxui::Color::RGB(100, 150, 255)}, // 蓝色
    {"objc", ftxui::Color::RGB(100, 150, 255)},        // OC 别名
    {"csharp", ftxui::Color::RGB(81, 43, 212)},        // 紫色（C# 官方）
    {"c#", ftxui::Color::RGB(81, 43, 212)},            // C# 别名
    {"fsharp", ftxui::Color::RGB(110, 180, 220)},      // 青色
    {"f#", ftxui::Color::RGB(110, 180, 220)},          // F# 别名
    {"php", ftxui::Color::RGB(119, 123, 180)},         // 蓝色（PHP 官方）
    {"ruby", ftxui::Color::RGB(204, 52, 45)},          // 红色（Ruby 官方）
    {"rb", ftxui::Color::RGB(204, 52, 45)},            // Ruby 别名
    {"perl", ftxui::Color::RGB(0, 118, 192)},          // 蓝色
    {"pl", ftxui::Color::RGB(0, 118, 192)},            // Perl 别名
    {"lua", ftxui::Color::RGB(46, 80, 126)},           // 深蓝
    {"r", ftxui::Color::RGB(25, 118, 210)},            // 蓝色
    {"matlab", ftxui::Color::RGB(237, 139, 0)},        // 橙色
    {"m", ftxui::Color::RGB(237, 139, 0)},             // MATLAB 别名
    {"julia", ftxui::Color::RGB(149, 88, 178)},        // 紫色（Julia 官方）
    {"dart", ftxui::Color::RGB(1, 117, 194)},          // 蓝色（Dart 官方）
    {"elixir", ftxui::Color::RGB(75, 39, 95)},         // 紫色（Elixir 官方）
    {"ex", ftxui::Color::RGB(75, 39, 95)},             // Elixir 别名
    {"exs", ftxui::Color::RGB(75, 39, 95)},            // Elixir 别名
    {"erlang", ftxui::Color::RGB(163, 31, 52)},        // 红色
    {"erl", ftxui::Color::RGB(163, 31, 52)},           // Erlang 别名
    {"haskell", ftxui::Color::RGB(120, 90, 160)},      // 紫色
    {"hs", ftxui::Color::RGB(120, 90, 160)},           // Haskell 别名
    {"ocaml", ftxui::Color::RGB(236, 140, 0)},         // 琥珀色
    {"crystal", ftxui::Color::RGB(200, 205, 210)},     // 银白
    {"tcl", ftxui::Color::RGB(90, 130, 180)},          // 蓝灰
    {"fortran", ftxui::Color::RGB(77, 144, 142)},      // 青色
    {"f90", ftxui::Color::RGB(77, 144, 142)},          // Fortran 别名
    {"f95", ftxui::Color::RGB(77, 144, 142)},          // Fortran 别名
    {"meson", ftxui::Color::RGB(0, 122, 204)},         // 蓝色
    {"coq", ftxui::Color::RGB(0, 140, 150)},           // 青色
    {"agda", ftxui::Color::RGB(70, 100, 150)},         // 紫青
    {"idris", ftxui::Color::RGB(130, 90, 160)},        // 紫色
    {"purescript", ftxui::Color::RGB(137, 99, 181)},   // 紫灰
    {"reason", ftxui::Color::RGB(221, 74, 104)},       // 粉红
    {"sml", ftxui::Color::RGB(237, 139, 0)},           // 橙色
    {"carbon", ftxui::Color::RGB(50, 120, 200)},       // 蓝色
    {"vala", ftxui::Color::RGB(149, 88, 178)},         // 紫色
    {"genie", ftxui::Color::RGB(150, 190, 80)},        // 黄绿
    {"dlang", ftxui::Color::RGB(255, 85, 110)},        // 红色
    {"pony", ftxui::Color::RGB(180, 140, 200)},        // 淡紫
    {"vlang", ftxui::Color::RGB(0, 170, 160)},         // 青绿
    {"odin", ftxui::Color::RGB(90, 160, 200)},         // 青色
    {"jai", ftxui::Color::RGB(80, 140, 200)},          // 青蓝
    {"nelua", ftxui::Color::RGB(60, 110, 170)},        // 蓝色
    {"wren", ftxui::Color::RGB(220, 160, 60)},         // 琥珀
    {"moonscript", ftxui::Color::RGB(140, 110, 180)},  // 紫灰
    {"fantom", ftxui::Color::RGB(40, 100, 160)},       // 深蓝
    {"smalltalk", ftxui::Color::RGB(89, 166, 89)},     // 绿色
    {"apl", ftxui::Color::RGB(0, 140, 180)},           // 青色
    {"jlang", ftxui::Color::RGB(70, 120, 170)},        // 蓝灰
    {"klang", ftxui::Color::RGB(50, 100, 160)},        // 深蓝
    {"qlang", ftxui::Color::RGB(80, 130, 190)},        // 蓝色

    // ========== Web 技术 ==========
    {"html", ftxui::Color::RGB(227, 76, 38)},         // 橙色（HTML5 官方）
    {"htm", ftxui::Color::RGB(227, 76, 38)},          // HTML 别名
    {"css", ftxui::Color::RGB(38, 77, 228)},          // 蓝色（CSS 官方）
    {"scss", ftxui::Color::RGB(207, 100, 154)},       // 粉色
    {"sass", ftxui::Color::RGB(207, 100, 154)},       // Sass 别名
    {"less", ftxui::Color::RGB(29, 54, 93)},          // 深蓝（Less 官方）
    {"stylus", ftxui::Color::RGB(100, 200, 150)},     // 绿色
    {"vue", ftxui::Color::RGB(79, 192, 141)},         // 绿色（Vue 官方）
    {"vuejs", ftxui::Color::RGB(79, 192, 141)},       // Vue 别名
    {"jsx", ftxui::Color::RGB(97, 218, 251)},         // 青色（React 官方）
    {"tsx", ftxui::Color::RGB(97, 218, 251)},         // React TSX
    {"react", ftxui::Color::RGB(97, 218, 251)},       // React 别名
    {"angular", ftxui::Color::RGB(221, 0, 49)},       // 红色（Angular 官方）
    {"svelte", ftxui::Color::RGB(255, 62, 0)},        // 橙色（Svelte 官方）
    {"coffeescript", ftxui::Color::RGB(45, 80, 130)}, // 深棕蓝
    {"coffee", ftxui::Color::RGB(45, 80, 130)},       // CoffeeScript 别名
    {"pug", ftxui::Color::RGB(169, 204, 41)},         // 绿色
    {"jade", ftxui::Color::RGB(169, 204, 41)},        // Pug 别名
    {"postcss", ftxui::Color::RGB(220, 50, 90)},      // 红灰
    {"graphql", ftxui::Color::RGB(225, 29, 132)},     // 粉色（GraphQL 官方）
    {"gql", ftxui::Color::RGB(225, 29, 132)},         // GraphQL 别名
    {"astro", ftxui::Color::RGB(250, 76, 138)},       // 粉色（Astro 官方）
    {"sveltekit", ftxui::Color::RGB(255, 62, 0)},     // SvelteKit
    {"nextjs", ftxui::Color::RGB(0, 0, 0)},           // 黑色（Next.js 官方）
    {"nuxt", ftxui::Color::RGB(0, 222, 140)},         // 绿色（Nuxt 官方）
    {"remix", ftxui::Color::RGB(66, 153, 225)},       // 蓝色（Remix 官方）
    {"solid", ftxui::Color::RGB(60, 189, 157)},       // 绿色（Solid 官方）
    {"qwik", ftxui::Color::RGB(51, 102, 255)},        // 蓝色（Qwik 官方）

    // ========== 构建/配置文件 ==========
    {"cargo.toml", ftxui::Color::RGB(206, 65, 43)},           // Rust 橙
    {"cargo.lock", ftxui::Color::RGB(160, 100, 80)},          // 棕灰
    {"gemfile", ftxui::Color::RGB(204, 52, 45)},              // Ruby 红
    {"gemfile.lock", ftxui::Color::RGB(180, 80, 70)},         // Ruby 锁文件
    {"composer.json", ftxui::Color::RGB(119, 123, 180)},      // PHP 蓝
    {"composer.lock", ftxui::Color::RGB(100, 105, 150)},      // PHP 锁文件
    {"go.mod", ftxui::Color::RGB(0, 173, 216)},               // Go 青
    {"go.sum", ftxui::Color::RGB(80, 140, 160)},              // 灰青
    {"requirements.txt", ftxui::Color::RGB(55, 118, 171)},    // Python 蓝
    {"pyproject.toml", ftxui::Color::RGB(45, 95, 140)},       // 深蓝
    {"setup.py", ftxui::Color::RGB(55, 118, 171)},            // Python 安装文件
    {"setup.cfg", ftxui::Color::RGB(70, 120, 160)},           // Python 配置
    {"poetry.lock", ftxui::Color::RGB(60, 100, 140)},         // Poetry 锁文件
    {"meson.build", ftxui::Color::RGB(0, 122, 204)},          // 蓝色
    {"meson.options", ftxui::Color::RGB(20, 100, 160)},       // Meson 配置
    {"docker-compose.yml", ftxui::Color::RGB(13, 183, 237)},  // Docker 蓝
    {"docker-compose.yaml", ftxui::Color::RGB(13, 183, 237)}, // Docker 蓝
    {"readme", ftxui::Color::RGB(200, 220, 240)},             // 淡蓝白
    {"license", ftxui::Color::RGB(120, 120, 120)},            // 灰色
    {"licence", ftxui::Color::RGB(120, 120, 120)},            // 灰色（英式拼写）
    {"changelog", ftxui::Color::RGB(130, 140, 150)},          // 灰蓝
    {"proto", ftxui::Color::RGB(100, 130, 160)},              // 灰蓝
    {"makefile", ftxui::Color::RGB(255, 200, 0)},             // 黄色
    {"cmake", ftxui::Color::RGB(64, 128, 174)},               // 蓝色
    {"cmakeLists.txt", ftxui::Color::RGB(64, 128, 174)},      // CMake 文件
    {"dockerfile", ftxui::Color::RGB(13, 183, 237)},          // 蓝色（Docker 官方）
    {"git", ftxui::Color::RGB(240, 80, 51)},                  // 橙红（Git 官方）
    {"gitignore", ftxui::Color::RGB(130, 125, 115)},          // 灰色
    {"gitattributes", ftxui::Color::RGB(130, 125, 115)},      // 灰色
    {"gitmodules", ftxui::Color::RGB(130, 125, 115)},         // 灰色
    {"env", ftxui::Color::RGB(100, 150, 110)},                // 绿灰
    {"dotenv", ftxui::Color::RGB(100, 150, 110)},             // 绿灰
    {"editorconfig", ftxui::Color::RGB(110, 130, 140)},       // 灰青
    {"vim", ftxui::Color::RGB(1, 135, 95)},                   // 维姆绿
    {"vimrc", ftxui::Color::RGB(1, 135, 95)},                 // Vim 配置
    {"nvim", ftxui::Color::RGB(1, 135, 95)},                  // Neovim
    {"zshrc", ftxui::Color::RGB(220, 200, 60)},               // Zsh 配置
    {"bashrc", ftxui::Color::RGB(65, 180, 90)},               // Bash 配置
    {"bash_profile", ftxui::Color::RGB(65, 180, 90)},         // Bash 配置
    {"fishrc", ftxui::Color::RGB(70, 160, 200)},              // Fish 配置
    {"terraform", ftxui::Color::RGB(127, 80, 210)},           // 紫色
    {"tf", ftxui::Color::RGB(127, 80, 210)},                  // Terraform 别名
    {"tfvars", ftxui::Color::RGB(110, 70, 190)},              // Terraform 变量
    {"tfstate", ftxui::Color::RGB(90, 60, 170)},              // Terraform 状态
    {"ansible", ftxui::Color::RGB(150, 100, 200)},            // 紫（Ansible 风格）
    {"playbook", ftxui::Color::RGB(150, 100, 200)},           // Ansible 剧本
    {"gradle", ftxui::Color::RGB(120, 180, 0)},               // 绿色（Gradle 官方）
    {"gradlew", ftxui::Color::RGB(120, 180, 0)},              // Gradle 包装器
    {"maven", ftxui::Color::RGB(200, 100, 0)},                // 橙色（Maven 官方）
    {"pom", ftxui::Color::RGB(200, 100, 0)},                  // Maven POM
    {"npm", ftxui::Color::RGB(200, 70, 0)},                   // 红色（NPM 官方）
    {"package.json", ftxui::Color::RGB(200, 70, 0)},          // NPM 包
    {"package-lock.json", ftxui::Color::RGB(180, 60, 0)},     // NPM 锁文件
    {"yarn.lock", ftxui::Color::RGB(40, 180, 100)},           // 绿色（Yarn 官方）
    {"pnpm-lock.yaml", ftxui::Color::RGB(255, 100, 0)},       // 橙色（PNPM 官方）
    {"bun.lockb", ftxui::Color::RGB(255, 50, 100)},           // 红色（Bun 官方）
    {"nx", ftxui::Color::RGB(150, 80, 220)},                  // 紫色（NX 官方）
    {"turbo", ftxui::Color::RGB(0, 180, 255)},                // 青色（Turbo 官方）
    {"bazel", ftxui::Color::RGB(200, 50, 50)},                // 红色（Bazel 官方）
    {"bzl", ftxui::Color::RGB(200, 50, 50)},                  // Bazel 别名
    {"buck", ftxui::Color::RGB(80, 180, 220)},                // 蓝色（Buck 官方）
    {"nix", ftxui::Color::RGB(80, 135, 180)},                 // 青蓝
    {"flake.nix", ftxui::Color::RGB(90, 145, 190)},           // Nix Flake
    {"justfile", ftxui::Color::RGB(200, 180, 100)},           // 灰黄
    {"just", ftxui::Color::RGB(200, 180, 100)},               // Just 别名

    // ========== 数据格式 ==========
    {"json", ftxui::Color::RGB(255, 200, 0)},         // 黄色
    {"jsonc", ftxui::Color::RGB(255, 190, 60)},       // 琥珀
    {"json5", ftxui::Color::RGB(255, 190, 60)},       // JSON5
    {"ron", ftxui::Color::RGB(210, 130, 70)},         // 橙褐
    {"xml", ftxui::Color::RGB(255, 165, 0)},          // 橙色
    {"xaml", ftxui::Color::RGB(0, 120, 215)},         // 蓝色
    {"yaml", ftxui::Color::RGB(203, 108, 128)},       // 玫红
    {"yml", ftxui::Color::RGB(203, 108, 128)},        // YAML 别名
    {"toml", ftxui::Color::RGB(156, 220, 254)},       // 淡蓝
    {"ini", ftxui::Color::RGB(110, 130, 140)},        // 灰青
    {"conf", ftxui::Color::RGB(115, 135, 145)},       // 灰青
    {"config", ftxui::Color::RGB(115, 135, 145)},     // 配置文件
    {"properties", ftxui::Color::RGB(130, 140, 130)}, // 灰绿
    {"csv", ftxui::Color::RGB(80, 180, 120)},         // 绿色
    {"tsv", ftxui::Color::RGB(70, 160, 110)},         // 深绿
    {"parquet", ftxui::Color::RGB(60, 140, 100)},     // 深绿
    {"avro", ftxui::Color::RGB(50, 120, 90)},         // 更深绿
    {"protobuf", ftxui::Color::RGB(100, 130, 160)},   // 灰蓝
    {"msgpack", ftxui::Color::RGB(90, 120, 150)},     // 蓝灰
    {"bson", ftxui::Color::RGB(80, 110, 140)},        // 深蓝灰
    {"hcl", ftxui::Color::RGB(127, 80, 210)},         // 紫色
    {"toml", ftxui::Color::RGB(156, 220, 254)},       // 淡蓝

    // ========== 脚本语言 ==========
    {"shell", ftxui::Color::RGB(65, 180, 90)},       // 绿色
    {"bash", ftxui::Color::RGB(65, 180, 90)},        // Bash
    {"sh", ftxui::Color::RGB(65, 180, 90)},          // Shell
    {"powershell", ftxui::Color::RGB(83, 145, 254)}, // 蓝色（PowerShell 官方）
    {"ps1", ftxui::Color::RGB(83, 145, 254)},        // PS1 别名
    {"zsh", ftxui::Color::RGB(220, 200, 60)},        // 黄绿
    {"fish", ftxui::Color::RGB(70, 160, 200)},       // 青色
    {"batch", ftxui::Color::RGB(83, 145, 254)},      // 批处理
    {"bat", ftxui::Color::RGB(83, 145, 254)},        // BAT 别名
    {"cmd", ftxui::Color::RGB(83, 145, 254)},        // CMD 别名

    // ========== 文档与标记 ==========
    {"markdown", ftxui::Color::RGB(235, 235, 230)}, // 米白
    {"md", ftxui::Color::RGB(235, 235, 230)},       // MD 别名
    {"text", ftxui::Color::RGB(220, 220, 215)},     // 白色
    {"txt", ftxui::Color::RGB(220, 220, 215)},      // TXT 别名
    {"latex", ftxui::Color::RGB(0, 128, 128)},      // 青色
    {"tex", ftxui::Color::RGB(0, 128, 128)},        // TeX 别名
    {"bib", ftxui::Color::RGB(220, 170, 50)},       // 琥珀
    {"rst", ftxui::Color::RGB(20, 120, 160)},       // 蓝色
    {"asciidoc", ftxui::Color::RGB(80, 110, 140)},  // 蓝灰
    {"adoc", ftxui::Color::RGB(80, 110, 140)},      // AsciiDoc 别名
    {"org", ftxui::Color::RGB(119, 170, 100)},      // 绿色
    {"docx", ftxui::Color::RGB(40, 100, 200)},      // 蓝色（Word）
    {"doc", ftxui::Color::RGB(40, 100, 200)},       // DOC 别名
    {"odt", ftxui::Color::RGB(80, 140, 40)},        // 绿色（OpenDocument）
    {"pdf", ftxui::Color::RGB(230, 50, 50)},        // 红色（PDF）
    {"epub", ftxui::Color::RGB(180, 80, 40)},       // 橙色（EPUB）
    {"mobi", ftxui::Color::RGB(160, 70, 30)},       // 深橙（MOBI）
    {"notion", ftxui::Color::RGB(50, 200, 200)},    // 青色（Notion 风格）
    {"obsidian", ftxui::Color::RGB(80, 80, 200)},   // 紫色（Obsidian 风格）

    // ========== 数据库 ==========
    {"sql", ftxui::Color::RGB(0, 122, 204)},        // 蓝色
    {"db", ftxui::Color::RGB(60, 120, 160)},        // 灰蓝
    {"sqlite", ftxui::Color::RGB(60, 120, 160)},    // SQLite
    {"mysql", ftxui::Color::RGB(0, 160, 220)},      // 青色（MySQL 官方）
    {"postgres", ftxui::Color::RGB(0, 120, 200)},   // 蓝色（PostgreSQL 官方）
    {"pg", ftxui::Color::RGB(0, 120, 200)},         // PG 别名
    {"redis", ftxui::Color::RGB(255, 60, 60)},      // 红色（Redis 官方）
    {"mongodb", ftxui::Color::RGB(60, 160, 60)},    // 绿色（MongoDB 官方）
    {"cassandra", ftxui::Color::RGB(100, 60, 160)}, // 紫色（Cassandra 官方）
    {"cql", ftxui::Color::RGB(100, 60, 160)},       // CQL 别名
    {"clickhouse", ftxui::Color::RGB(200, 80, 0)},  // 橙色（ClickHouse 官方）
    {"prisma", ftxui::Color::RGB(0, 170, 170)},     // 青色
    {"dbt", ftxui::Color::RGB(0, 140, 200)},        // 蓝色（dbt 官方）

    // ========== 函数式编程语言 ==========
    {"racket", ftxui::Color::RGB(158, 1, 66)},       // 红色
    {"rkt", ftxui::Color::RGB(158, 1, 66)},          // Racket 别名
    {"scheme", ftxui::Color::RGB(180, 40, 70)},      // 深红
    {"commonlisp", ftxui::Color::RGB(149, 88, 178)}, // 紫色
    {"lisp", ftxui::Color::RGB(149, 88, 178)},       // Lisp 别名
    {"emacslisp", ftxui::Color::RGB(160, 100, 170)}, // 紫红
    {"elisp", ftxui::Color::RGB(160, 100, 170)},     // Elisp 别名
    {"prolog", ftxui::Color::RGB(0, 110, 180)},      // 青蓝
    {"mercury", ftxui::Color::RGB(230, 120, 30)},    // 橙色
    {"alloy", ftxui::Color::RGB(80, 110, 150)},      // 灰蓝
    {"dafny", ftxui::Color::RGB(60, 90, 180)},       // 蓝紫
    {"lean", ftxui::Color::RGB(80, 200, 120)},       // 翠绿
    {"ballerina", ftxui::Color::RGB(220, 50, 70)},   // 红色

    // ========== 区块链语言 ==========
    {"solidity", ftxui::Color::RGB(100, 130, 160)}, // 灰蓝
    {"sol", ftxui::Color::RGB(100, 130, 160)},      // Solidity 别名
    {"vyper", ftxui::Color::RGB(0, 130, 200)},      // 蓝色
    {"cadence", ftxui::Color::RGB(130, 80, 200)},   // 紫色
    {"clarity", ftxui::Color::RGB(50, 160, 140)},   // 青绿
    {"move", ftxui::Color::RGB(80, 100, 200)},      // 深蓝
    {"sway", ftxui::Color::RGB(100, 80, 180)},      // 紫蓝

    // ========== 硬件描述语言 ==========
    {"verilog", ftxui::Color::RGB(0, 154, 166)},       // 青色
    {"v", ftxui::Color::RGB(0, 154, 166)},             // Verilog 别名
    {"vhdl", ftxui::Color::RGB(108, 82, 156)},         // 紫色
    {"systemverilog", ftxui::Color::RGB(0, 130, 140)}, // 深青
    {"sv", ftxui::Color::RGB(0, 130, 140)},            // SV 别名
    {"chisel", ftxui::Color::RGB(237, 139, 0)},        // 橙色（基于 Scala）

    // ========== 媒体文件 ==========
    {"image", ftxui::Color::RGB(180, 140, 255)}, // 紫色
    {"png", ftxui::Color::RGB(80, 180, 160)},    // 青绿
    {"jpg", ftxui::Color::RGB(80, 180, 160)},    // JPG
    {"jpeg", ftxui::Color::RGB(80, 180, 160)},   // JPEG
    {"gif", ftxui::Color::RGB(80, 180, 160)},    // GIF
    {"webp", ftxui::Color::RGB(80, 180, 160)},   // WebP
    {"bmp", ftxui::Color::RGB(80, 180, 160)},    // BMP
    {"tiff", ftxui::Color::RGB(90, 190, 170)},   // TIFF
    {"svg", ftxui::Color::RGB(255, 152, 0)},     // 橙色
    {"ico", ftxui::Color::RGB(100, 190, 180)},   // ICO
    {"heic", ftxui::Color::RGB(70, 170, 150)},   // HEIC
    {"audio", ftxui::Color::RGB(233, 130, 160)}, // 粉色
    {"mp3", ftxui::Color::RGB(233, 130, 160)},   // MP3
    {"wav", ftxui::Color::RGB(233, 130, 160)},   // WAV
    {"flac", ftxui::Color::RGB(233, 130, 160)},  // FLAC
    {"aac", ftxui::Color::RGB(233, 130, 160)},   // AAC
    {"ogg", ftxui::Color::RGB(233, 130, 160)},   // OGG
    {"m4a", ftxui::Color::RGB(223, 120, 150)},   // M4A
    {"video", ftxui::Color::RGB(220, 50, 47)},   // 红色
    {"mp4", ftxui::Color::RGB(220, 50, 47)},     // MP4
    {"avi", ftxui::Color::RGB(220, 50, 47)},     // AVI
    {"mkv", ftxui::Color::RGB(220, 50, 47)},     // MKV
    {"mov", ftxui::Color::RGB(220, 50, 47)},     // MOV
    {"webm", ftxui::Color::RGB(220, 50, 47)},    // WebM
    {"wmv", ftxui::Color::RGB(200, 60, 60)},     // WMV
    {"flv", ftxui::Color::RGB(190, 50, 50)},     // FLV
    {"mpeg", ftxui::Color::RGB(180, 40, 40)},    // MPEG
    {"mpg", ftxui::Color::RGB(180, 40, 40)},     // MPG
    {"font", ftxui::Color::RGB(160, 180, 200)},  // 浅蓝
    {"ttf", ftxui::Color::RGB(160, 180, 200)},   // TTF
    {"otf", ftxui::Color::RGB(160, 180, 200)},   // OTF
    {"woff", ftxui::Color::RGB(150, 170, 190)},  // WOFF
    {"woff2", ftxui::Color::RGB(150, 170, 190)}, // WOFF2

    // ========== 压缩与归档 ==========
    {"zip", ftxui::Color::RGB(255, 180, 50)},   // 琥珀
    {"tar", ftxui::Color::RGB(200, 90, 60)},    // 棕红
    {"gz", ftxui::Color::RGB(200, 90, 60)},     // GZ
    {"bz2", ftxui::Color::RGB(200, 90, 60)},    // BZ2
    {"xz", ftxui::Color::RGB(200, 90, 60)},     // XZ
    {"tgz", ftxui::Color::RGB(200, 90, 60)},    // TGZ
    {"rar", ftxui::Color::RGB(200, 60, 70)},    // 深红
    {"7z", ftxui::Color::RGB(200, 60, 70)},     // 7Z
    {"zstd", ftxui::Color::RGB(190, 80, 50)},   // ZSTD
    {"lz4", ftxui::Color::RGB(180, 70, 40)},    // LZ4
    {"snappy", ftxui::Color::RGB(170, 60, 30)}, // Snappy

    // ========== 二进制文件 ==========
    {"exe", ftxui::Color::RGB(100, 100, 100)},   // 深灰
    {"bin", ftxui::Color::RGB(100, 100, 100)},   // BIN
    {"dll", ftxui::Color::RGB(100, 100, 100)},   // DLL
    {"so", ftxui::Color::RGB(100, 100, 100)},    // SO
    {"dylib", ftxui::Color::RGB(100, 100, 100)}, // DYLIB
    {"o", ftxui::Color::RGB(90, 90, 90)},        // 目标文件
    {"obj", ftxui::Color::RGB(90, 90, 90)},      // OBJ
    {"lib", ftxui::Color::RGB(90, 90, 90)},      // LIB
    {"a", ftxui::Color::RGB(90, 90, 90)},        // 静态库
    {"out", ftxui::Color::RGB(90, 95, 100)},     // 可执行输出

    // ========== 日志与临时文件 ==========
    {"log", ftxui::Color::RGB(130, 128, 118)},  // 灰色
    {"lock", ftxui::Color::RGB(100, 105, 110)}, // 深灰
    {"tmp", ftxui::Color::RGB(100, 100, 100)},  // 深灰
    {"temp", ftxui::Color::RGB(100, 100, 100)}, // 深灰
    {"bak", ftxui::Color::RGB(100, 100, 100)},  // 备份文件
    {"swp", ftxui::Color::RGB(100, 100, 100)},  // 交换文件
    {"swo", ftxui::Color::RGB(100, 100, 100)},  // Vim 交换文件
    {"~", ftxui::Color::RGB(100, 100, 100)},    // 临时文件后缀

    // ========== 测试框架 ==========
    {"test", ftxui::Color::RGB(80, 180, 80)},         // 绿色
    {"spec", ftxui::Color::RGB(80, 180, 80)},         // 测试规范
    {"jest", ftxui::Color::RGB(255, 50, 100)},        // 红色（Jest 官方）
    {"pytest", ftxui::Color::RGB(55, 118, 171)},      // Python 蓝
    {"rspec", ftxui::Color::RGB(204, 52, 45)},        // Ruby 红
    {"cypress", ftxui::Color::RGB(0, 200, 200)},      // 青色（Cypress 官方）
    {"playwright", ftxui::Color::RGB(100, 100, 255)}, // 蓝色（Playwright 官方）
    {"vitest", ftxui::Color::RGB(79, 192, 141)},      // Vue 绿（Vitest 官方）

    // ========== 其他类型 ==========
    {"gherkin", ftxui::Color::RGB(90, 160, 90)},    // 绿色
    {"feature", ftxui::Color::RGB(90, 160, 90)},    // 特性文件
    {"srt", ftxui::Color::RGB(120, 125, 130)},      // 字幕
    {"vtt", ftxui::Color::RGB(120, 125, 130)},      // 字幕
    {"ass", ftxui::Color::RGB(120, 125, 130)},      // 字幕
    {"ssa", ftxui::Color::RGB(120, 125, 130)},      // 字幕
    {"terraform", ftxui::Color::RGB(127, 80, 210)}, // 紫色
    {"bicep", ftxui::Color::RGB(230, 110, 50)},     // 橙蓝
    {"terraform", ftxui::Color::RGB(127, 80, 210)}, // 紫色
    {"hcl", ftxui::Color::RGB(127, 80, 210)},       // 紫色
    {"nix", ftxui::Color::RGB(80, 135, 180)},       // 青蓝
    {"just", ftxui::Color::RGB(200, 180, 100)},     // 灰黄
    {"gherkin", ftxui::Color::RGB(90, 160, 90)},    // 绿色
    {"feature", ftxui::Color::RGB(90, 160, 90)},    // 特性文件
};

FileTypeColorMapper::FileTypeColorMapper(const ui::Theme& theme) : theme_(theme) {}

ftxui::Color FileTypeColorMapper::getFileColor(const std::string& filename,
                                               bool is_directory) const {
    auto& colors = theme_.getColors();

    // 目录特殊处理
    if (is_directory) {
        if (filename == "..") {
            return colors.comment; // 上级目录用注释色
        }
        return colors.function; // 普通目录用函数色
    }

    // 获取文件扩展名和类型
    std::string ext = getFileExtension(filename);
    std::string file_type = FileTypeDetector::detectFileType(filename, ext);

    // 转换为小写
    std::transform(file_type.begin(), file_type.end(), file_type.begin(), ::tolower);

    // 优先从预定义映射表查找
    auto it = FILE_TYPE_COLOR_MAP.find(file_type);
    if (it != FILE_TYPE_COLOR_MAP.end()) {
        return it->second;
    }

    // 如果找不到精确匹配，尝试扩展名匹配
    if (!ext.empty()) {
        it = FILE_TYPE_COLOR_MAP.find(ext);
        if (it != FILE_TYPE_COLOR_MAP.end()) {
            return it->second;
        }
    }

    // 默认颜色 - 灰色（未知类型）
    return ftxui::Color::RGB(140, 135, 120);
}

// 优化后的颜色获取方法（简化逻辑，提升性能）
ftxui::Color FileTypeColorMapper::getColorByFileType(const std::string& file_type) const {
    std::string type_lower = file_type;
    std::transform(type_lower.begin(), type_lower.end(), type_lower.begin(), ::tolower);

    // 从预定义映射表查找
    auto it = FILE_TYPE_COLOR_MAP.find(type_lower);
    if (it != FILE_TYPE_COLOR_MAP.end()) {
        return it->second;
    }

    // 默认颜色
    return ftxui::Color::RGB(140, 135, 120);
}

std::string FileTypeColorMapper::getFileExtension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == 0) {
        return "";
    }

    std::string ext = filename.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

} // namespace utils
} // namespace pnana