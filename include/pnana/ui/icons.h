#ifndef PNANA_UI_ICONS_H
#define PNANA_UI_ICONS_H

#include <string>

namespace pnana {
namespace ui {
namespace icons {

// Nerd Font图标常量
// 使用 JetBrains Nerd Font 的 Unicode 字符
// 参考: https://www.nerdfonts.com/cheat-sheet

// 文件和文件夹
constexpr const char* FOLDER = "\uf07b";      // nf-fa-folder
constexpr const char* FOLDER_OPEN = "\uf07c"; // nf-fa-folder_open
constexpr const char* FOLDER_UP = "\uf062";   // nf-fa-arrow_up (上级目录)
constexpr const char* FILE = "\uf15b";        // nf-fa-file
constexpr const char* FILE_TEXT = "\uf15c";   // nf-fa-file_text_o

// 编程语言 (使用 Devicons 或 Font Awesome)
constexpr const char* CPP = "\ue61d";        // nf-dev-cplusplus
constexpr const char* C = "\ue61e";          // nf-dev-c
constexpr const char* PYTHON = "\ue63c";     // nf-dev-python
constexpr const char* JAVASCRIPT = "\ue74e"; // nf-dev-javascript
constexpr const char* TYPESCRIPT = "\ue628"; // nf-dev-typescript
constexpr const char* JAVA = "\ue256";       // nf-dev-java
constexpr const char* GO = "\ue627";         // nf-dev-go
constexpr const char* RUST = "\ue7a8";       // nf-dev-rust
constexpr const char* RUBY = "\ue739";       // nf-dev-ruby
constexpr const char* PHP = "\ue73d";        // nf-dev-php
constexpr const char* LUA = "\ue620";        // nf-dev-lua
constexpr const char* HTML = "\ue736";       // nf-dev-html5
constexpr const char* CSS = "\ue749";        // nf-dev-css3
constexpr const char* JSON = "\ue60b";       // nf-dev-json
constexpr const char* MARKDOWN = "\ue73e";   // nf-dev-markdown
constexpr const char* YAML = "\uf481";       // nf-mdi-code_braces
constexpr const char* XML = "\ue72a";        // nf-dev-xml
constexpr const char* SQL = "\uf1c0";        // nf-fa-database
constexpr const char* SHELL = "\uf489";      // nf-mdi-console
constexpr const char* DOCKER = "\ue7b0";     // nf-dev-docker
constexpr const char* GIT = "\ue702";        // nf-dev-git
constexpr const char* GIT_BRANCH = "\uf126"; // nf-fa-code-fork (分支图标)
constexpr const char* GITIGNORE = "\ue702";  // nf-dev-git
constexpr const char* CMAKE = "\uf489";      // nf-mdi-console
constexpr const char* MAKEFILE = "\uf489";   // nf-mdi-console

// 新增语言图标
constexpr const char* SWIFT = "\ue755";      // nf-dev-swift
constexpr const char* KOTLIN = "\ue634";     // nf-dev-kotlin
constexpr const char* SCALA = "\ue737";      // nf-dev-scala
constexpr const char* R = "\ue68c";          // nf-dev-r (R语言)
constexpr const char* PERL = "\ue769";       // nf-dev-perl
constexpr const char* VIM = "\ue62b";        // nf-dev-vim
constexpr const char* POWERSHELL = "\uf489"; // nf-mdi-console
constexpr const char* HASKELL = "\ue777";    // nf-dev-haskell
constexpr const char* TCL = "\uf489";        // nf-mdi-console
constexpr const char* FORTRAN = "\uf489";    // nf-mdi-console

// 更多编程语言图标
constexpr const char* ELIXIR = "\ue62d";        // nf-dev-elixir
constexpr const char* CLOJURE = "\ue76a";       // nf-dev-clojure
constexpr const char* ERLANG = "\ue7b1";        // nf-dev-erlang
constexpr const char* F_SHARP = "\ue7a7";       // nf-dev-fsharp
constexpr const char* JULIA = "\ue624";         // nf-dev-julia
constexpr const char* DART = "\ue798";          // nf-dev-dart
constexpr const char* NIM = "\ue677";           // nf-dev-nim
constexpr const char* CRYSTAL = "\ue62f";       // nf-dev-crystal
constexpr const char* ZIG = "\ue6a9";           // nf-dev-zig
constexpr const char* OCAML = "\ue67a";         // nf-dev-ocaml
constexpr const char* COQ = "\ue695";           // nf-dev-coq
constexpr const char* AGDA = "\ue6a8";          // nf-dev-agda
constexpr const char* IDRIS = "\ue75c";         // nf-dev-idris
constexpr const char* PURESCRIPT = "\ue630";    // nf-dev-purescript
constexpr const char* REASON = "\ue68d";        // nf-dev-reason
constexpr const char* SML = "\ue677";           // nf-dev-sml
constexpr const char* GROOVY = "\ue775";        // nf-dev-groovy
constexpr const char* CLOJURESCRIPT = "\ue76a"; // nf-dev-clojurescript
constexpr const char* COFFEESCRIPT = "\ue751";  // nf-dev-coffeescript
constexpr const char* PUG = "\ue720";           // nf-dev-pug
constexpr const char* STYLUS = "\ue600";        // nf-dev-stylus
constexpr const char* SASS = "\ue603";          // nf-dev-sass
constexpr const char* LESS = "\ue758";          // nf-dev-less
constexpr const char* POSTCSS = "\ue730";       // nf-dev-postcss
constexpr const char* GRAPHQL = "\ue711";       // nf-dev-graphql
constexpr const char* APOLLO = "\ue659";        // nf-dev-apollo
constexpr const char* NEXTJS = "\ue6a0";        // nf-dev-nextjs
constexpr const char* NUXTJS = "\ue6c0";        // nf-dev-nuxtjs
constexpr const char* VUE = "\ue6a0";           // nf-dev-vue
constexpr const char* ANGULAR = "\ue753";       // nf-dev-angular
constexpr const char* REACT = "\ue7ba";         // nf-dev-react
constexpr const char* SVELTE = "\ue697";        // nf-dev-svelte
constexpr const char* EMBER = "\ue61b";         // nf-dev-ember
constexpr const char* METEOR = "\ue754";        // nf-dev-meteor
constexpr const char* AURELIA = "\ue617";       // nf-dev-aurelia
constexpr const char* BACKBONE = "\ue752";      // nf-dev-backbone
constexpr const char* KNOCKOUT = "\ue690";      // nf-dev-knockout
constexpr const char* POLYMER = "\ue681";       // nf-dev-polymer
constexpr const char* LIT = "\ue6a4";           // nf-dev-lit
constexpr const char* STENCIL = "\ue6a5";       // nf-dev-stencil
constexpr const char* IONIC = "\ue6a6";         // nf-dev-ionic
constexpr const char* CAPACITOR = "\ue670";     // nf-dev-capacitor
constexpr const char* CORDOVA = "\ue955";       // nf-dev-cordova
constexpr const char* ELECTRON = "\ue62e";      // nf-dev-electron
constexpr const char* NWJS = "\ue682";          // nf-dev-nwjs
constexpr const char* TAURI = "\ue68f";         // nf-dev-tauri
constexpr const char* TUXEDO = "\ue771";        // nf-dev-tuxedo
constexpr const char* DOTNET = "\ue77f";        // nf-dev-dotnet
constexpr const char* CSHARP = "\ue648";        // nf-dev-csharp
constexpr const char* VB = "\ue70f";            // nf-dev-visualstudio
constexpr const char* F_SHARP_ALT = "\ue7a7";   // nf-dev-fsharp
constexpr const char* ASSEMBLY = "\uf471";      // nf-mdi-chip
constexpr const char* WEBASSEMBLY = "\ue6a1";   // nf-dev-webassembly
constexpr const char* VERILOG = "\uf453";       // nf-mdi-memory
constexpr const char* VHDL = "\uf453";          // nf-mdi-memory
constexpr const char* MATLAB = "\ue67c";        // nf-dev-matlab
constexpr const char* OCTAVE = "\ue67c";        // nf-dev-matlab
constexpr const char* RACKET = "\ue7b3";        // nf-dev-racket
constexpr const char* SCHEME = "\ue7b3";        // nf-dev-racket
constexpr const char* COMMON_LISP = "\ue7b1";   // nf-dev-clisp
constexpr const char* EMACS_LISP = "\ue7b1";    // nf-dev-clisp
constexpr const char* LOGO = "\ue672";          // nf-dev-logo
constexpr const char* PROLOG = "\ue7a1";        // nf-dev-prolog
constexpr const char* MERCURY = "\ue67b";       // nf-dev-mercury
constexpr const char* ALLOY = "\ue68e";         // nf-dev-alloy
constexpr const char* Z3 = "\ue68e";            // nf-dev-alloy
constexpr const char* DAFNY = "\ue7b2";         // nf-dev-dafny
constexpr const char* WHY3 = "\ue7b2";          // nf-dev-dafny
constexpr const char* COQ_ALT = "\ue695";       // nf-dev-coq
constexpr const char* ISABELLE = "\ue695";      // nf-dev-coq
constexpr const char* HOL = "\ue695";           // nf-dev-coq
constexpr const char* LEAN = "\ue758";          // nf-dev-lean
constexpr const char* TLA = "\ue695";           // nf-dev-coq
constexpr const char* B = "\ue68e";             // nf-dev-alloy
constexpr const char* EVENT_B = "\ue68e";       // nf-dev-alloy
constexpr const char* ABEL = "\ue759";          // nf-dev-abel
constexpr const char* BALLERINA = "\ue61f";     // nf-dev-ballerina
constexpr const char* CADENCE = "\ue672";       // nf-dev-cadence
constexpr const char* CLARITY = "\ue636";       // nf-dev-clarity
constexpr const char* MOVE = "\ue672";          // nf-dev-move
constexpr const char* SOLIDITY = "\ue656";      // nf-dev-solidity
constexpr const char* VYPER = "\ue656";         // nf-dev-solidity
constexpr const char* RUST_ALT = "\ue7a8";      // nf-dev-rust
constexpr const char* GO_ALT = "\ue627";        // nf-dev-go
constexpr const char* CARBON = "\ue671";        // nf-dev-carbon
constexpr const char* VALA = "\ue69e";          // nf-dev-vala
constexpr const char* GENIE = "\ue69e";         // nf-dev-vala
constexpr const char* NIM_ALT = "\ue677";       // nf-dev-nim
constexpr const char* D = "\ue7af";             // nf-dev-dlang
constexpr const char* NIMROD = "\ue677";        // nf-dev-nim
constexpr const char* PONY = "\ue6b3";          // nf-dev-pony
constexpr const char* V_LANG = "\ue6ac";        // nf-dev-v
constexpr const char* ODIN = "\ue6ac";          // nf-dev-v
constexpr const char* JAI = "\ue7ab";           // nf-dev-jai
constexpr const char* ZIG_ALT = "\ue6a9";       // nf-dev-zig
constexpr const char* NELUA = "\ue67b";         // nf-dev-nelua
constexpr const char* WREN = "\ue67b";          // nf-dev-nelua
constexpr const char* MOONSCRIPT = "\ue66b";    // nf-dev-moon
constexpr const char* FAN = "\ue69f";           // nf-dev-fantom
constexpr const char* KRYPTON = "\ue69f";       // nf-dev-fantom
constexpr const char* FANTOM = "\ue69f";        // nf-dev-fantom
constexpr const char* SMALLTALK = "\ue6b8";     // nf-dev-smalltalk
constexpr const char* SELF = "\ue6b8";          // nf-dev-smalltalk
constexpr const char* NEWSPEAK = "\ue6b8";      // nf-dev-smalltalk
constexpr const char* IO = "\ue6b8";            // nf-dev-smalltalk
constexpr const char* SELF_ALT = "\ue6b8";      // nf-dev-smalltalk
constexpr const char* PHARO = "\ue6b8";         // nf-dev-smalltalk
constexpr const char* SQUEAK = "\ue6b8";        // nf-dev-smalltalk
constexpr const char* RED = "\ue6b8";           // nf-dev-smalltalk
constexpr const char* REBOL = "\ue6b8";         // nf-dev-smalltalk
constexpr const char* APL = "\ue6a8";           // nf-dev-apl
constexpr const char* J = "\ue6a8";             // nf-dev-apl
constexpr const char* K = "\ue6a8";             // nf-dev-apl
constexpr const char* Q = "\ue6a8";             // nf-dev-apl
constexpr const char* KDB = "\ue6a8";           // nf-dev-apl
constexpr const char* QLANG = "\ue6a8";         // nf-dev-apl
constexpr const char* KDB_ALT = "\ue6a8";       // nf-dev-apl
constexpr const char* Q_ALT = "\ue6a8";         // nf-dev-apl

// 更多编程语言图标（补充）
constexpr const char* BASH = "\uf489";              // nf-mdi-console
constexpr const char* ZSH = "\uf489";               // nf-mdi-console
constexpr const char* FISH = "\uf489";              // nf-mdi-console
constexpr const char* POWERSHELL_ALT = "\uf489";    // nf-mdi-console
constexpr const char* BATCH = "\uf489";             // nf-mdi-console
constexpr const char* WINDOWS_CMD = "\uf489";       // nf-mdi-console
constexpr const char* APPLESCRIPT = "\uf302";       // nf-fa-apple
constexpr const char* VISUAL_BASIC = "\ue70f";      // nf-dev-visualstudio
constexpr const char* DELPHI = "\ue70f";            // nf-dev-visualstudio
constexpr const char* PASCAL = "\ue70f";            // nf-dev-visualstudio
constexpr const char* ADA = "\ue70f";               // nf-dev-visualstudio
constexpr const char* COBOL = "\uf489";             // nf-mdi-console
constexpr const char* FORTH = "\uf489";             // nf-mdi-console
constexpr const char* LISP = "\ue7b1";              // nf-dev-clisp
constexpr const char* SCHEME_ALT = "\ue7b3";        // nf-dev-racket
constexpr const char* CLOJURE_ALT = "\ue76a";       // nf-dev-clojure
constexpr const char* ERLANG_ALT = "\ue7b1";        // nf-dev-erlang
constexpr const char* ELIXIR_ALT = "\ue62d";        // nf-dev-elixir
constexpr const char* HASKELL_ALT = "\ue777";       // nf-dev-haskell
constexpr const char* OCAML_ALT = "\ue67a";         // nf-dev-ocaml
constexpr const char* F_SHARP_ALT2 = "\ue7a7";      // nf-dev-fsharp
constexpr const char* ML = "\ue677";                // nf-dev-sml
constexpr const char* COQ_ALT2 = "\ue695";          // nf-dev-coq
constexpr const char* AGDA_ALT = "\ue6a8";          // nf-dev-agda
constexpr const char* IDRIS_ALT = "\ue75c";         // nf-dev-idris
constexpr const char* LEAN_ALT = "\ue758";          // nf-dev-lean
constexpr const char* ISABELLE_ALT = "\ue695";      // nf-dev-coq
constexpr const char* HOL_ALT = "\ue695";           // nf-dev-coq
constexpr const char* TLA_ALT = "\ue695";           // nf-dev-coq
constexpr const char* ALLOY_ALT = "\ue68e";         // nf-dev-alloy
constexpr const char* Z3_ALT = "\ue68e";            // nf-dev-alloy
constexpr const char* DAFNY_ALT = "\ue7b2";         // nf-dev-dafny
constexpr const char* WHY3_ALT = "\ue7b2";          // nf-dev-dafny
constexpr const char* BALLERINA_ALT = "\ue61f";     // nf-dev-ballerina
constexpr const char* CADENCE_ALT = "\ue672";       // nf-dev-cadence
constexpr const char* CLARITY_ALT = "\ue636";       // nf-dev-clarity
constexpr const char* MOVE_ALT = "\ue672";          // nf-dev-move
constexpr const char* SOLIDITY_ALT = "\ue656";      // nf-dev-solidity
constexpr const char* VYPER_ALT = "\ue656";         // nf-dev-solidity
constexpr const char* CARBON_ALT = "\ue671";        // nf-dev-carbon
constexpr const char* VALA_ALT = "\ue69e";          // nf-dev-vala
constexpr const char* GENIE_ALT = "\ue69e";         // nf-dev-vala
constexpr const char* D_ALT = "\ue7af";             // nf-dev-dlang
constexpr const char* PONY_ALT = "\ue6b3";          // nf-dev-pony
constexpr const char* V_ALT = "\ue6ac";             // nf-dev-v
constexpr const char* ODIN_ALT = "\ue6ac";          // nf-dev-v
constexpr const char* JAI_ALT = "\ue7ab";           // nf-dev-jai
constexpr const char* ZIG_ALT2 = "\ue6a9";          // nf-dev-zig
constexpr const char* NELUA_ALT = "\ue67b";         // nf-dev-nelua
constexpr const char* WREN_ALT = "\ue67b";          // nf-dev-nelua
constexpr const char* MOONSCRIPT_ALT = "\ue66b";    // nf-dev-moon
constexpr const char* FANTOM_ALT = "\ue69f";        // nf-dev-fantom
constexpr const char* SMALLTALK_ALT = "\ue6b8";     // nf-dev-smalltalk
constexpr const char* APL_ALT = "\ue6a8";           // nf-dev-apl
constexpr const char* J_ALT = "\ue6a8";             // nf-dev-apl
constexpr const char* K_ALT = "\ue6a8";             // nf-dev-apl
constexpr const char* Q_ALT2 = "\ue6a8";            // nf-dev-apl
constexpr const char* KDB_ALT2 = "\ue6a8";          // nf-dev-apl
constexpr const char* QLANG_ALT = "\ue6a8";         // nf-dev-apl
constexpr const char* MATLAB_ALT = "\ue67c";        // nf-dev-matlab
constexpr const char* OCTAVE_ALT = "\ue67c";        // nf-dev-matlab
constexpr const char* R_ALT = "\ue68c";             // nf-dev-r
constexpr const char* JULIA_ALT = "\ue624";         // nf-dev-julia
constexpr const char* LUA_ALT = "\ue620";           // nf-dev-lua
constexpr const char* PYTHON_ALT = "\ue63c";        // nf-dev-python
constexpr const char* RUBY_ALT = "\ue739";          // nf-dev-ruby
constexpr const char* PERL_ALT = "\ue769";          // nf-dev-perl
constexpr const char* PHP_ALT = "\ue73d";           // nf-dev-php
constexpr const char* JAVASCRIPT_ALT = "\ue74e";    // nf-dev-javascript
constexpr const char* TYPESCRIPT_ALT = "\ue628";    // nf-dev-typescript
constexpr const char* JAVA_ALT = "\ue256";          // nf-dev-java
constexpr const char* KOTLIN_ALT = "\ue634";        // nf-dev-kotlin
constexpr const char* SCALA_ALT = "\ue737";         // nf-dev-scala
constexpr const char* CSHARP_ALT = "\ue648";        // nf-dev-csharp
constexpr const char* GO_ALT2 = "\ue627";           // nf-dev-go
constexpr const char* RUST_ALT2 = "\ue7a8";         // nf-dev-rust
constexpr const char* SWIFT_ALT = "\ue755";         // nf-dev-swift
constexpr const char* OBJECTIVE_C = "\ue755";       // nf-dev-swift
constexpr const char* CPP_ALT = "\ue61d";           // nf-dev-cplusplus
constexpr const char* C_ALT = "\ue61e";             // nf-dev-c
constexpr const char* CSHARP_ALT2 = "\ue648";       // nf-dev-csharp
constexpr const char* VB_ALT = "\ue70f";            // nf-dev-visualstudio
constexpr const char* F_SHARP_ALT3 = "\ue7a7";      // nf-dev-fsharp
constexpr const char* NIM_ALT2 = "\ue677";          // nf-dev-nim
constexpr const char* CRYSTAL_ALT = "\ue62f";       // nf-dev-crystal
constexpr const char* DART_ALT = "\ue798";          // nf-dev-dart
constexpr const char* GROOVY_ALT = "\ue775";        // nf-dev-groovy
constexpr const char* CLOJURESCRIPT_ALT = "\ue76a"; // nf-dev-clojurescript
constexpr const char* COFFEESCRIPT_ALT = "\ue751";  // nf-dev-coffeescript
constexpr const char* PUG_ALT = "\ue720";           // nf-dev-pug
constexpr const char* STYLUS_ALT = "\ue600";        // nf-dev-stylus
constexpr const char* SASS_ALT = "\ue603";          // nf-dev-sass
constexpr const char* LESS_ALT = "\ue758";          // nf-dev-less
constexpr const char* POSTCSS_ALT = "\ue730";       // nf-dev-postcss
constexpr const char* GRAPHQL_ALT = "\ue711";       // nf-dev-graphql
constexpr const char* VUE_ALT = "\ue6a0";           // nf-dev-vue
constexpr const char* ANGULAR_ALT = "\ue753";       // nf-dev-angular
constexpr const char* REACT_ALT = "\ue7ba";         // nf-dev-react
constexpr const char* SVELTE_ALT = "\ue697";        // nf-dev-svelte
constexpr const char* IONIC_ALT = "\ue6a6";         // nf-dev-ionic
constexpr const char* CAPACITOR_ALT = "\ue670";     // nf-dev-capacitor
constexpr const char* CORDOVA_ALT = "\ue955";       // nf-dev-cordova
constexpr const char* ELECTRON_ALT = "\ue62e";      // nf-dev-electron
constexpr const char* NWJS_ALT = "\ue682";          // nf-dev-nwjs
constexpr const char* TAURI_ALT = "\ue68f";         // nf-dev-tauri
constexpr const char* DOTNET_ALT = "\ue77f";        // nf-dev-dotnet
constexpr const char* ASSEMBLY_ALT = "\uf471";      // nf-mdi-chip
constexpr const char* WEBASSEMBLY_ALT = "\ue6a1";   // nf-dev-webassembly
constexpr const char* VERILOG_ALT = "\uf453";       // nf-mdi-memory
constexpr const char* VHDL_ALT = "\uf453";          // nf-mdi-memory
constexpr const char* TCL_ALT = "\uf489";           // nf-mdi-console
constexpr const char* FORTRAN_ALT = "\uf489";       // nf-mdi-console
constexpr const char* RACKET_ALT = "\ue7b3";        // nf-dev-racket
constexpr const char* COMMON_LISP_ALT = "\ue7b1";   // nf-dev-clisp
constexpr const char* EMACS_LISP_ALT = "\ue7b1";    // nf-dev-clisp
constexpr const char* PROLOG_ALT = "\ue7a1";        // nf-dev-prolog
constexpr const char* MERCURY_ALT = "\ue67b";       // nf-dev-mercury

// 状态图标
constexpr const char* MODIFIED = "\uf111"; // nf-fa-circle (修改标记)
constexpr const char* SAVED = "\uf00c";    // nf-fa-check (已保存)
constexpr const char* UNSAVED = "\uf12a";  // nf-fa-exclamation (未保存)
constexpr const char* CLOSE = "\uf00d";    // nf-fa-times (关闭)

// 操作图标
constexpr const char* SEARCH = "\uf002";  // nf-fa-search (搜索)
constexpr const char* REPLACE = "\uf0e7"; // nf-fa-exchange (替换)
constexpr const char* SAVE = "\uf0c7";    // nf-fa-floppy_o (保存)
constexpr const char* OPEN = "\uf07c";    // nf-fa-folder_open (打开)
constexpr const char* NEW = "\uf016";     // nf-fa-file_o (新建)
constexpr const char* UNDO = "\uf0e2";    // nf-fa-undo (撤销)
constexpr const char* REDO = "\uf01e";    // nf-fa-repeat (重做)
constexpr const char* COPY = "\uf0c5";    // nf-fa-files_o (复制)
constexpr const char* CUT = "\uf0c4";     // nf-fa-cut (剪切)
constexpr const char* PASTE = "\uf0ea";   // nf-fa-clipboard (粘贴)

// 导航图标
constexpr const char* ARROW_UP = "\uf062";    // nf-fa-arrow_up
constexpr const char* ARROW_DOWN = "\uf063";  // nf-fa-arrow_down
constexpr const char* ARROW_LEFT = "\uf060";  // nf-fa-arrow_left
constexpr const char* ARROW_RIGHT = "\uf061"; // nf-fa-arrow_right
constexpr const char* GO_TO = "\uf0ac";       // nf-fa-external_link (跳转)

// UI元素
constexpr const char* THEME = "\uf1fc";    // nf-fa-paint_brush (主题)
constexpr const char* SETTINGS = "\uf013"; // nf-fa-cog (设置)
constexpr const char* HELP = "\uf059";     // nf-fa-question_circle (帮助)
constexpr const char* INFO = "\uf05a";     // nf-fa-info_circle (信息)
constexpr const char* WARNING = "\uf071";  // nf-fa-exclamation_triangle (警告)
constexpr const char* ERROR = "\uf06a";    // nf-fa-exclamation_circle (错误)
constexpr const char* SUCCESS = "\uf00c";  // nf-fa-check_circle (成功)

// 编辑器功能
constexpr const char* LINE_NUMBER = "\uf0cb"; // nf-fa-list_alt (行号)
constexpr const char* WORD_WRAP = "\uf0ea";   // nf-fa-arrows_alt (自动换行)
constexpr const char* FULLSCREEN = "\uf065";  // nf-fa-expand (全屏)
constexpr const char* SPLIT = "\uf0c9";       // nf-fa-columns (分屏)
constexpr const char* CODE = "\uf121";        // nf-fa-code (代码)
constexpr const char* FUNCTION = "\uf1c0";    // nf-fa-cube (函数)
constexpr const char* TAB = "\uf02e";         // nf-fa-tag (标签)
constexpr const char* SELECT = "\uf0b2";      // nf-fa-mouse_pointer (选择)
constexpr const char* HIGHLIGHT = "\uf0eb";   // nf-fa-lightbulb_o (语法高亮)
constexpr const char* LOCATION = "\uf041";    // nf-fa-map_marker (位置)
constexpr const char* CLOCK = "\uf017";       // nf-fa-clock_o (时钟)

// 文件浏览器
constexpr const char* REFRESH = "\uf021"; // nf-fa-refresh (刷新)
constexpr const char* HOME = "\uf015";    // nf-fa-home (主目录)

// 其他文件类型图标
constexpr const char* IMAGE = "\uf1c5";      // nf-fa-file_image_o (图片)
constexpr const char* PDF = "\uf1c1";        // nf-fa-file_pdf_o (PDF)
constexpr const char* ARCHIVE = "\uf1c6";    // nf-fa-file_archive_o (压缩包)
constexpr const char* VIDEO = "\uf1c8";      // nf-fa-file_video_o (视频)
constexpr const char* AUDIO = "\uf1c7";      // nf-fa-file_audio_o (音频)
constexpr const char* DATABASE = "\uf1c0";   // nf-fa-database (数据库)
constexpr const char* CONFIG = "\uf013";     // nf-fa-cog (配置文件)
constexpr const char* LOCK = "\uf023";       // nf-fa-lock (锁定文件)
constexpr const char* EXECUTABLE = "\uf292"; // nf-fa-terminal (可执行文件)

// 构建和依赖管理文件
constexpr const char* PACKAGE_JSON = "\ue60e"; // nf-dev-npm (npm/package.json)
constexpr const char* PACKAGE_LOCK = "\ue60e"; // nf-dev-npm (package-lock.json)
constexpr const char* YARN = "\uf1e6";         // nf-fa-yarn (yarn.lock)
constexpr const char* CARGO = "\ue7a8";        // nf-dev-rust (Cargo.toml)
constexpr const char* PIP = "\ue63c";          // nf-dev-python (requirements.txt)
constexpr const char* MAVEN = "\ue256";        // nf-dev-java (pom.xml)
constexpr const char* GRADLE = "\ue256";       // nf-dev-java (build.gradle)
constexpr const char* GEMFILE = "\ue739";      // nf-dev-ruby (Gemfile)
constexpr const char* COMPOSER = "\ue73d";     // nf-dev-php (composer.json)
constexpr const char* GO_MOD = "\ue627";       // nf-dev-go (go.mod)
constexpr const char* GO_SUM = "\ue627";       // nf-dev-go (go.sum)

// 环境配置文件
constexpr const char* ENV = "\uf462";          // nf-mdi-key (环境变量文件)
constexpr const char* DOCKERIGNORE = "\ue7b0"; // nf-dev-docker (.dockerignore)
constexpr const char* EDITORCONFIG = "\uf013"; // nf-fa-cog (.editorconfig)
constexpr const char* PRETTIER = "\ue60b";     // nf-dev-json (.prettierrc)
constexpr const char* ESLINT = "\ue60b";       // nf-dev-json (.eslintrc)
constexpr const char* BABEL = "\ue60b";        // nf-dev-json (.babelrc)
constexpr const char* TSCONFIG = "\ue628";     // nf-dev-typescript (tsconfig.json)

// 文档文件
constexpr const char* README = "\ue73e";       // nf-dev-markdown (README)
constexpr const char* LICENSE = "\uf1c9";      // nf-fa-file_text_o (LICENSE)
constexpr const char* CHANGELOG = "\uf1c9";    // nf-fa-file_text_o (CHANGELOG)
constexpr const char* CONTRIBUTING = "\uf1c9"; // nf-fa-file_text_o (CONTRIBUTING)
constexpr const char* AUTHORS = "\uf1c9";      // nf-fa-file_text_o (AUTHORS)
constexpr const char* TODO = "\uf0ae";         // nf-fa-tasks (TODO)

// 测试文件
constexpr const char* TEST = "\uf188"; // nf-fa-flask (测试文件)
constexpr const char* SPEC = "\uf188"; // nf-fa-flask (spec文件)

// 数据文件
constexpr const char* CSV = "\uf1c3";   // nf-fa-file_text_o (CSV)
constexpr const char* TSV = "\uf1c3";   // nf-fa-file_text_o (TSV)
constexpr const char* EXCEL = "\uf1c3"; // nf-fa-file_excel_o (Excel)

// 特殊配置文件
constexpr const char* TRAVIS = "\ue77e";         // nf-dev-travis (.travis.yml)
constexpr const char* JENKINS = "\ue767";        // nf-dev-jenkins (Jenkinsfile)
constexpr const char* GITHUB_ACTIONS = "\ue702"; // nf-dev-git (GitHub Actions)
constexpr const char* CI = "\uf013";             // nf-fa-cog (CI配置文件)

// 锁文件和清单文件
constexpr const char* YARN_LOCK = "\uf1e6";    // nf-fa-yarn (yarn.lock)
constexpr const char* PNPM_LOCK = "\ue60e";    // nf-dev-npm (pnpm-lock.yaml)
constexpr const char* GEMFILE_LOCK = "\ue739"; // nf-dev-ruby (Gemfile.lock)
constexpr const char* POETRY = "\ue63c";       // nf-dev-python (poetry.lock)

// 特殊目录
constexpr const char* NODE_MODULES = "\ue60e"; // nf-dev-npm (node_modules)
constexpr const char* VENV = "\ue63c";         // nf-dev-python (虚拟环境)
constexpr const char* DOTFILES = "\uf013";     // nf-fa-cog (隐藏配置文件)

// 日志和临时文件
constexpr const char* LOG = "\uf1c9";   // nf-fa-file_text_o (日志文件)
constexpr const char* TEMP = "\uf016";  // nf-fa-file_o (临时文件)
constexpr const char* CACHE = "\uf0eb"; // nf-fa-lightbulb_o (缓存文件)

// 证书和密钥文件
constexpr const char* CERTIFICATE = "\uf023"; // nf-fa-lock (证书文件)
constexpr const char* KEY = "\uf084";         // nf-fa-key (密钥文件)
constexpr const char* PEM = "\uf023";         // nf-fa-lock (PEM文件)

// 字体和样式文件
constexpr const char* FONT = "\uf031"; // nf-fa-font (字体文件)

// 其他特殊文件
constexpr const char* DOCKER_COMPOSE = "\ue7b0"; // nf-dev-docker (docker-compose.yml)
constexpr const char* KUBERNETES = "\ufd31";     // nf-mdi-kubernetes (Kubernetes)
constexpr const char* TERRAFORM = "\uf1c0";      // nf-fa-cube (Terraform)
constexpr const char* ANSIBLE = "\uf013";        // nf-fa-cog (Ansible)
constexpr const char* VAGRANT = "\uf1c0";        // nf-fa-cube (Vagrant)

// 欢迎界面
constexpr const char* ROCKET = "\uf135"; // nf-fa-rocket (快速开始)
constexpr const char* STAR = "\uf005";   // nf-fa-star (特性)
constexpr const char* BULB = "\uf0eb";   // nf-fa-lightbulb_o (提示)
constexpr const char* BOOK = "\uf02d";   // nf-fa-book (文档)

// 终端
constexpr const char* TERMINAL = "\uf120"; // nf-fa-terminal (终端)

} // namespace icons
} // namespace ui
} // namespace pnana

#endif // PNANA_UI_ICONS_H
