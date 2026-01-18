#include "features/SyntaxHighlighter/syntax_highlighter_tree_sitter.h"
#include <algorithm>
#include <cstring>
#include <tree_sitter/api.h>

// Tree-sitter 语言定义（需要链接对应的语言库）
// 注意：这些函数从对应的语言库中获取
// 如果语言库未链接，需要在 CMakeLists.txt 中链接对应的库
extern "C" {
// C/C++
#ifdef BUILD_TREE_SITTER_CPP
TSLanguage* tree_sitter_cpp();
#endif
#ifdef BUILD_TREE_SITTER_C
TSLanguage* tree_sitter_c();
#endif

// Python
#ifdef BUILD_TREE_SITTER_PYTHON
TSLanguage* tree_sitter_python();
#endif

// JavaScript/TypeScript
#ifdef BUILD_TREE_SITTER_JAVASCRIPT
TSLanguage* tree_sitter_javascript();
#endif
#ifdef BUILD_TREE_SITTER_TYPESCRIPT
TSLanguage* tree_sitter_typescript();
#endif

// 数据格式
#ifdef BUILD_TREE_SITTER_JSON
TSLanguage* tree_sitter_json();
#endif
#ifdef BUILD_TREE_SITTER_MARKDOWN
TSLanguage* tree_sitter_markdown();
#endif

// Shell
#ifdef BUILD_TREE_SITTER_BASH
TSLanguage* tree_sitter_bash();
#endif

// 其他语言
#ifdef BUILD_TREE_SITTER_RUST
TSLanguage* tree_sitter_rust();
#endif
#ifdef BUILD_TREE_SITTER_GO
TSLanguage* tree_sitter_go();
#endif
#ifdef BUILD_TREE_SITTER_JAVA
TSLanguage* tree_sitter_java();
#endif

// CMake
#ifdef BUILD_TREE_SITTER_CMAKE
TSLanguage* tree_sitter_cmake();
#endif

// TCL
#ifdef BUILD_TREE_SITTER_TCL
TSLanguage* tree_sitter_tcl();
#endif

// Fortran
#ifdef BUILD_TREE_SITTER_FORTRAN
TSLanguage* tree_sitter_fortran();
#endif

// Haskell
#ifdef BUILD_TREE_SITTER_HASKELL
TSLanguage* tree_sitter_haskell();
#endif

// Lua
#ifdef BUILD_TREE_SITTER_LUA
TSLanguage* tree_sitter_lua();
#endif

// 新增语言支持
// YAML
#ifdef BUILD_TREE_SITTER_YAML
TSLanguage* tree_sitter_yaml();
#endif

// XML
#ifdef BUILD_TREE_SITTER_XML
TSLanguage* tree_sitter_xml();
#endif

// CSS
#ifdef BUILD_TREE_SITTER_CSS
TSLanguage* tree_sitter_css();
#endif

// SQL
#ifdef BUILD_TREE_SITTER_SQL
TSLanguage* tree_sitter_sql();
#endif

// Ruby
#ifdef BUILD_TREE_SITTER_RUBY
TSLanguage* tree_sitter_ruby();
#endif

// PHP
#ifdef BUILD_TREE_SITTER_PHP
TSLanguage* tree_sitter_php();
#endif

// Swift
#ifdef BUILD_TREE_SITTER_SWIFT
TSLanguage* tree_sitter_swift();
#endif

// Kotlin
#ifdef BUILD_TREE_SITTER_KOTLIN
TSLanguage* tree_sitter_kotlin();
#endif

// Scala
#ifdef BUILD_TREE_SITTER_SCALA
TSLanguage* tree_sitter_scala();
#endif

// R
#ifdef BUILD_TREE_SITTER_R
TSLanguage* tree_sitter_r();
#endif

// Perl
#ifdef BUILD_TREE_SITTER_PERL
TSLanguage* tree_sitter_perl();
#endif

// Dockerfile
#ifdef BUILD_TREE_SITTER_DOCKERFILE
TSLanguage* tree_sitter_dockerfile();
#endif

// Makefile
#ifdef BUILD_TREE_SITTER_MAKEFILE
TSLanguage* tree_sitter_make();
#endif

// Vim
#ifdef BUILD_TREE_SITTER_VIM
TSLanguage* tree_sitter_vim();
#endif

// PowerShell
#ifdef BUILD_TREE_SITTER_POWERSHELL
TSLanguage* tree_sitter_powershell();
#endif

// Meson
#ifdef BUILD_TREE_SITTER_MESON
TSLanguage* tree_sitter_meson();
#endif

// TOML
#ifdef BUILD_TREE_SITTER_TOML
TSLanguage* tree_sitter_toml();
#endif

// Nim
#ifdef BUILD_TREE_SITTER_NIM
TSLanguage* tree_sitter_nim();
#endif
}

using namespace ftxui;

namespace pnana {
namespace features {

SyntaxHighlighterTreeSitter::SyntaxHighlighterTreeSitter(ui::Theme& theme)
    : theme_(theme), parser_(nullptr), current_language_(nullptr), current_file_type_("text") {
    parser_ = ts_parser_new();
    if (!parser_) {
        // 如果创建失败，parser_ 保持 nullptr
        return;
    }
    initializeLanguages();
}

SyntaxHighlighterTreeSitter::~SyntaxHighlighterTreeSitter() {
    if (parser_) {
        ts_parser_delete(parser_);
    }
}

void SyntaxHighlighterTreeSitter::initializeLanguages() {
// 初始化语言映射
// 注意：这些语言库需要在编译时链接（在 CMakeLists.txt 中）
// 如果语言库未链接，对应的语言将使用原生语法高亮器（自动回退）

// C/C++
#ifdef BUILD_TREE_SITTER_CPP
    TSLanguage* cpp_lang = tree_sitter_cpp();
    if (cpp_lang) {
        language_map_["cpp"] = cpp_lang;
        language_map_["cxx"] = cpp_lang;
        language_map_["cc"] = cpp_lang;
        language_map_["c++"] = cpp_lang;
        language_map_["hpp"] = cpp_lang;
        language_map_["hxx"] = cpp_lang;
        language_map_["hh"] = cpp_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_C
    TSLanguage* c_lang = tree_sitter_c();
    if (c_lang) {
        language_map_["c"] = c_lang;
        language_map_["h"] = c_lang;
    }
#endif

// Python
#ifdef BUILD_TREE_SITTER_PYTHON
    TSLanguage* python_lang = tree_sitter_python();
    if (python_lang) {
        language_map_["py"] = python_lang;
        language_map_["python"] = python_lang;
        language_map_["pyw"] = python_lang;
        language_map_["pyi"] = python_lang;
    }
#endif

// JavaScript
#ifdef BUILD_TREE_SITTER_JAVASCRIPT
    TSLanguage* js_lang = tree_sitter_javascript();
    if (js_lang) {
        language_map_["js"] = js_lang;
        language_map_["javascript"] = js_lang;
        language_map_["jsx"] = js_lang;
        language_map_["mjs"] = js_lang;
    }
#endif

// TypeScript
#ifdef BUILD_TREE_SITTER_TYPESCRIPT
    TSLanguage* ts_lang = tree_sitter_typescript();
    if (ts_lang) {
        language_map_["ts"] = ts_lang;
        language_map_["typescript"] = ts_lang;
        language_map_["tsx"] = ts_lang;
    }
#endif

// JSON
#ifdef BUILD_TREE_SITTER_JSON
    TSLanguage* json_lang = tree_sitter_json();
    if (json_lang) {
        language_map_["json"] = json_lang;
        language_map_["jsonc"] = json_lang;
    }
#endif

// Markdown
#ifdef BUILD_TREE_SITTER_MARKDOWN
    TSLanguage* md_lang = tree_sitter_markdown();
    if (md_lang) {
        language_map_["md"] = md_lang;
        language_map_["markdown"] = md_lang;
    }
#endif

// Shell/Bash
#ifdef BUILD_TREE_SITTER_BASH
    TSLanguage* bash_lang = tree_sitter_bash();
    if (bash_lang) {
        language_map_["sh"] = bash_lang;
        language_map_["bash"] = bash_lang;
        language_map_["shell"] = bash_lang;
        language_map_["zsh"] = bash_lang;
    }
#endif

// Rust
#ifdef BUILD_TREE_SITTER_RUST
    TSLanguage* rust_lang = tree_sitter_rust();
    if (rust_lang) {
        language_map_["rs"] = rust_lang;
        language_map_["rust"] = rust_lang;
    }
#endif

// Go
#ifdef BUILD_TREE_SITTER_GO
    TSLanguage* go_lang = tree_sitter_go();
    if (go_lang) {
        language_map_["go"] = go_lang;
    }
#endif

// Java
#ifdef BUILD_TREE_SITTER_JAVA
    TSLanguage* java_lang = tree_sitter_java();
    if (java_lang) {
        language_map_["java"] = java_lang;
    }
#endif

// CMake
#ifdef BUILD_TREE_SITTER_CMAKE
    TSLanguage* cmake_lang = tree_sitter_cmake();
    if (cmake_lang) {
        language_map_["cmake"] = cmake_lang;
        language_map_["cmake.in"] = cmake_lang;
        language_map_["cmake.in.in"] = cmake_lang;
    }
#endif

// TCL
#ifdef BUILD_TREE_SITTER_TCL
    TSLanguage* tcl_lang = tree_sitter_tcl();
    if (tcl_lang) {
        language_map_["tcl"] = tcl_lang;
        language_map_["tk"] = tcl_lang;
        language_map_["portfile"] = tcl_lang; // MacPorts portfiles
    }
#endif

// Fortran
#ifdef BUILD_TREE_SITTER_FORTRAN
    TSLanguage* fortran_lang = tree_sitter_fortran();
    if (fortran_lang) {
        language_map_["f90"] = fortran_lang;
        language_map_["f95"] = fortran_lang;
        language_map_["f03"] = fortran_lang;
        language_map_["f08"] = fortran_lang;
        language_map_["f"] = fortran_lang;
        language_map_["for"] = fortran_lang;
        language_map_["ftn"] = fortran_lang;
        language_map_["fpp"] = fortran_lang;
        language_map_["fortran"] = fortran_lang;
    }
#endif

// Haskell
#ifdef BUILD_TREE_SITTER_HASKELL
    TSLanguage* haskell_lang = tree_sitter_haskell();
    if (haskell_lang) {
        language_map_["hs"] = haskell_lang;
        language_map_["haskell"] = haskell_lang;
        language_map_["lhs"] = haskell_lang; // Literate Haskell
    }
#endif

// Lua
#ifdef BUILD_TREE_SITTER_LUA
    TSLanguage* lua_lang = tree_sitter_lua();
    if (lua_lang) {
        language_map_["lua"] = lua_lang;
        language_map_["lua5.1"] = lua_lang;
        language_map_["lua5.2"] = lua_lang;
        language_map_["lua5.3"] = lua_lang;
        language_map_["lua5.4"] = lua_lang;
    }
#endif

// 新增语言支持
// YAML
#ifdef BUILD_TREE_SITTER_YAML
    TSLanguage* yaml_lang = tree_sitter_yaml();
    if (yaml_lang) {
        language_map_["yaml"] = yaml_lang;
        language_map_["yml"] = yaml_lang;
    }
#endif

// XML
#ifdef BUILD_TREE_SITTER_XML
    TSLanguage* xml_lang = tree_sitter_xml();
    if (xml_lang) {
        language_map_["xml"] = xml_lang;
        language_map_["html"] = xml_lang;
        language_map_["htm"] = xml_lang;
        language_map_["xhtml"] = xml_lang;
        language_map_["svg"] = xml_lang;
    }
#endif

// CSS
#ifdef BUILD_TREE_SITTER_CSS
    TSLanguage* css_lang = tree_sitter_css();
    if (css_lang) {
        language_map_["css"] = css_lang;
        language_map_["scss"] = css_lang;
        language_map_["sass"] = css_lang;
        language_map_["less"] = css_lang;
    }
#endif

// SQL
#ifdef BUILD_TREE_SITTER_SQL
    TSLanguage* sql_lang = tree_sitter_sql();
    if (sql_lang) {
        language_map_["sql"] = sql_lang;
        language_map_["mysql"] = sql_lang;
        language_map_["postgresql"] = sql_lang;
        language_map_["sqlite"] = sql_lang;
        language_map_["oracle"] = sql_lang;
        language_map_["mssql"] = sql_lang;
    }
#endif

// Ruby
#ifdef BUILD_TREE_SITTER_RUBY
    TSLanguage* ruby_lang = tree_sitter_ruby();
    if (ruby_lang) {
        language_map_["rb"] = ruby_lang;
        language_map_["ruby"] = ruby_lang;
        language_map_["rake"] = ruby_lang;
        language_map_["gemspec"] = ruby_lang;
    }
#endif

// PHP
#ifdef BUILD_TREE_SITTER_PHP
    TSLanguage* php_lang = tree_sitter_php();
    if (php_lang) {
        language_map_["php"] = php_lang;
        language_map_["phtml"] = php_lang;
        language_map_["php3"] = php_lang;
        language_map_["php4"] = php_lang;
        language_map_["php5"] = php_lang;
        language_map_["php7"] = php_lang;
    }
#endif

// Swift
#ifdef BUILD_TREE_SITTER_SWIFT
    TSLanguage* swift_lang = tree_sitter_swift();
    if (swift_lang) {
        language_map_["swift"] = swift_lang;
    }
#endif

// Kotlin
#ifdef BUILD_TREE_SITTER_KOTLIN
    TSLanguage* kotlin_lang = tree_sitter_kotlin();
    if (kotlin_lang) {
        language_map_["kt"] = kotlin_lang;
        language_map_["kotlin"] = kotlin_lang;
        language_map_["kts"] = kotlin_lang;
    }
#endif

// Scala
#ifdef BUILD_TREE_SITTER_SCALA
    TSLanguage* scala_lang = tree_sitter_scala();
    if (scala_lang) {
        language_map_["scala"] = scala_lang;
        language_map_["sc"] = scala_lang;
    }
#endif

// R
#ifdef BUILD_TREE_SITTER_R
    TSLanguage* r_lang = tree_sitter_r();
    if (r_lang) {
        language_map_["r"] = r_lang;
        language_map_["R"] = r_lang;
        language_map_["rmd"] = r_lang;
        language_map_["rscript"] = r_lang;
    }
#endif

// Perl
#ifdef BUILD_TREE_SITTER_PERL
    TSLanguage* perl_lang = tree_sitter_perl();
    if (perl_lang) {
        language_map_["pl"] = perl_lang;
        language_map_["pm"] = perl_lang;
        language_map_["perl"] = perl_lang;
        language_map_["pod"] = perl_lang;
    }
#endif

// Dockerfile
#ifdef BUILD_TREE_SITTER_DOCKERFILE
    TSLanguage* dockerfile_lang = tree_sitter_dockerfile();
    if (dockerfile_lang) {
        language_map_["dockerfile"] = dockerfile_lang;
        language_map_["Dockerfile"] = dockerfile_lang;
        language_map_["containerfile"] = dockerfile_lang;
    }
#endif

// Makefile
#ifdef BUILD_TREE_SITTER_MAKEFILE
    TSLanguage* makefile_lang = tree_sitter_make();
    if (makefile_lang) {
        language_map_["makefile"] = makefile_lang;
        language_map_["Makefile"] = makefile_lang;
        language_map_["make"] = makefile_lang;
        language_map_["mk"] = makefile_lang;
    }
#endif

// Vim
#ifdef BUILD_TREE_SITTER_VIM
    TSLanguage* vim_lang = tree_sitter_vim();
    if (vim_lang) {
        language_map_["vim"] = vim_lang;
        language_map_["vimrc"] = vim_lang;
        language_map_["nvim"] = vim_lang;
        language_map_["vimscript"] = vim_lang;
    }
#endif

// PowerShell
#ifdef BUILD_TREE_SITTER_POWERSHELL
    TSLanguage* powershell_lang = tree_sitter_powershell();
    if (powershell_lang) {
        language_map_["ps1"] = powershell_lang;
        language_map_["powershell"] = powershell_lang;
        language_map_["psm1"] = powershell_lang;
        language_map_["psd1"] = powershell_lang;
    }
#endif

// Meson
#ifdef BUILD_TREE_SITTER_MESON
    TSLanguage* meson_lang = tree_sitter_meson();
    if (meson_lang) {
        language_map_["meson"] = meson_lang;
        language_map_["meson.build"] = meson_lang;
        language_map_["meson_options.txt"] = meson_lang;
    }
#endif

// TOML
#ifdef BUILD_TREE_SITTER_TOML
    TSLanguage* toml_lang = tree_sitter_toml();
    if (toml_lang) {
        language_map_["toml"] = toml_lang;
        language_map_["Cargo.lock"] = toml_lang;   // Rust Cargo.lock files
        language_map_["Pipfile.lock"] = toml_lang; // Python Pipfile.lock
        language_map_["poetry.lock"] = toml_lang;  // Python Poetry lock files
    }
#endif

// Nim
#ifdef BUILD_TREE_SITTER_NIM
    TSLanguage* nim_lang = tree_sitter_nim();
    if (nim_lang) {
        language_map_["nim"] = nim_lang;
        language_map_["nims"] = nim_lang;   // Nim script files
        language_map_["nimble"] = nim_lang; // Nimble package files
    }
#endif
}

TSLanguage* SyntaxHighlighterTreeSitter::getLanguageForFileType(const std::string& file_type) {
    auto it = language_map_.find(file_type);
    if (it != language_map_.end()) {
        return it->second;
    }
    return nullptr;
}

void SyntaxHighlighterTreeSitter::setFileType(const std::string& file_type) {
    if (current_file_type_ == file_type) {
        return;
    }

    current_file_type_ = file_type;
    TSLanguage* lang = getLanguageForFileType(file_type);

    if (lang && parser_) {
        ts_parser_set_language(parser_, lang);
        current_language_ = lang;
    } else {
        current_language_ = nullptr;
    }
}

bool SyntaxHighlighterTreeSitter::supportsFileType(const std::string& file_type) const {
    return language_map_.find(file_type) != language_map_.end();
}

void SyntaxHighlighterTreeSitter::reset() {
    if (parser_ && current_language_) {
        ts_parser_set_language(parser_, current_language_);
    }
}

ftxui::Element SyntaxHighlighterTreeSitter::highlightLine(const std::string& line) {
    if (line.empty() || !parser_ || !current_language_) {
        return text(line) | color(theme_.getColors().foreground);
    }

    return parseAndHighlight(line);
}

ftxui::Element SyntaxHighlighterTreeSitter::highlightLines(const std::vector<std::string>& lines) {
    if (lines.empty() || !parser_ || !current_language_) {
        Elements elements;
        for (const auto& line : lines) {
            elements.push_back(text(line) | color(theme_.getColors().foreground));
        }
        return vbox(elements);
    }

    // 合并所有行为一个字符串进行解析（更高效）
    std::string code;
    for (const auto& line : lines) {
        code += line + "\n";
    }

    return parseAndHighlight(code);
}

ftxui::Element SyntaxHighlighterTreeSitter::parseAndHighlight(const std::string& code) {
    if (code.empty() || !parser_ || !current_language_) {
        return text(code) | color(theme_.getColors().foreground);
    }

    // 解析代码
    TSTree* tree = ts_parser_parse_string(parser_, nullptr, code.c_str(), code.length());
    if (!tree) {
        return text(code) | color(theme_.getColors().foreground);
    }

    TSNode root_node = ts_tree_root_node(tree);

    // 遍历语法树并生成高亮元素
    Elements elements;
    size_t current_pos = 0;
    traverseTree(root_node, code, elements, current_pos);

    // 处理未覆盖的文本
    if (current_pos < code.length()) {
        std::string remaining = code.substr(current_pos);
        elements.push_back(text(remaining) | color(theme_.getColors().foreground));
    }

    ts_tree_delete(tree);

    return hbox(elements);
}

void SyntaxHighlighterTreeSitter::traverseTree(TSNode node, const std::string& source,
                                               std::vector<ftxui::Element>& elements,
                                               size_t& current_pos) const {
    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);

    // 处理节点前的文本
    if (current_pos < start_byte) {
        std::string before = source.substr(current_pos, start_byte - current_pos);
        elements.push_back(text(before) | color(theme_.getColors().foreground));
        current_pos = start_byte;
    }

    // 获取节点类型
    const char* node_type_cstr = ts_node_type(node);
    std::string node_type = node_type_cstr ? node_type_cstr : "";

    // 获取节点文本
    std::string node_text = getNodeText(node, source);

    // 获取颜色
    Color node_color = getColorForNodeType(node_type);

    // 如果是叶子节点，直接添加
    if (ts_node_child_count(node) == 0) {
        elements.push_back(text(node_text) | color(node_color));
        current_pos = end_byte;
    } else {
        // 递归处理子节点
        uint32_t child_count = ts_node_child_count(node);
        for (uint32_t i = 0; i < child_count; ++i) {
            TSNode child = ts_node_child(node, i);
            traverseTree(child, source, elements, current_pos);
        }
    }
}

std::string SyntaxHighlighterTreeSitter::getNodeText(TSNode node, const std::string& source) const {
    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);

    if (start_byte >= source.length()) {
        return "";
    }

    size_t start = static_cast<size_t>(start_byte);
    size_t end = std::min(static_cast<size_t>(end_byte), source.length());

    return source.substr(start, end - start);
}

ftxui::Color SyntaxHighlighterTreeSitter::getColorForNodeType(const std::string& node_type) const {
    auto& colors = theme_.getColors();

    // 关键字
    if (node_type.find("keyword") != std::string::npos || node_type == "if" ||
        node_type == "else" || node_type == "for" || node_type == "while" ||
        node_type == "return" || node_type == "class" || node_type == "function" ||
        node_type == "const" || node_type == "let" || node_type == "var" || node_type == "import" ||
        node_type == "export") {
        return colors.keyword;
    }

    // 字符串
    if (node_type.find("string") != std::string::npos || node_type == "string_content" ||
        node_type == "string_literal") {
        return colors.string;
    }

    // 注释
    if (node_type.find("comment") != std::string::npos) {
        return colors.comment;
    }

    // 数字
    if (node_type.find("number") != std::string::npos || node_type == "integer" ||
        node_type == "float") {
        return colors.number;
    }

    // 函数
    if (node_type.find("function") != std::string::npos || node_type == "call_expression" ||
        node_type == "method_invocation") {
        return colors.function;
    }

    // 类型
    if (node_type.find("type") != std::string::npos || node_type == "type_identifier" ||
        node_type == "class_declaration") {
        return colors.type;
    }

    // 操作符
    if (node_type.find("operator") != std::string::npos || node_type == "+" || node_type == "-" ||
        node_type == "*" || node_type == "/" || node_type == "=" || node_type == "==") {
        return colors.operator_color;
    }

    // 预处理器（使用 keyword 颜色）
    if (node_type.find("preproc") != std::string::npos || node_type == "preprocessor_directive") {
        return colors.keyword;
    }

    // 默认颜色
    return colors.foreground;
}

} // namespace features
} // namespace pnana
