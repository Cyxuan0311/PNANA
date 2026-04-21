#include "features/indent/auto_indent_engine.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <cstring>

#ifdef BUILD_TREE_SITTER_SUPPORT
extern "C" {
#ifdef BUILD_TREE_SITTER_CPP
TSLanguage* tree_sitter_cpp();
#endif
#ifdef BUILD_TREE_SITTER_C
TSLanguage* tree_sitter_c();
#endif
#ifdef BUILD_TREE_SITTER_PYTHON
TSLanguage* tree_sitter_python();
#endif
#ifdef BUILD_TREE_SITTER_JAVASCRIPT
TSLanguage* tree_sitter_javascript();
#endif
#ifdef BUILD_TREE_SITTER_TYPESCRIPT
TSLanguage* tree_sitter_typescript();
#endif
#ifdef BUILD_TREE_SITTER_JSON
TSLanguage* tree_sitter_json();
#endif
#ifdef BUILD_TREE_SITTER_BASH
TSLanguage* tree_sitter_bash();
#endif
#ifdef BUILD_TREE_SITTER_RUST
TSLanguage* tree_sitter_rust();
#endif
#ifdef BUILD_TREE_SITTER_GO
TSLanguage* tree_sitter_go();
#endif
#ifdef BUILD_TREE_SITTER_JAVA
TSLanguage* tree_sitter_java();
#endif
#ifdef BUILD_TREE_SITTER_RUBY
TSLanguage* tree_sitter_ruby();
#endif
#ifdef BUILD_TREE_SITTER_KOTLIN
TSLanguage* tree_sitter_kotlin();
#endif
#ifdef BUILD_TREE_SITTER_SWIFT
TSLanguage* tree_sitter_swift();
#endif
#ifdef BUILD_TREE_SITTER_CSHARP
TSLanguage* tree_sitter_c_sharp();
#endif
#ifdef BUILD_TREE_SITTER_PHP
TSLanguage* tree_sitter_php();
#endif
#ifdef BUILD_TREE_SITTER_LUA
TSLanguage* tree_sitter_lua();
#endif
#ifdef BUILD_TREE_SITTER_ZIG
TSLanguage* tree_sitter_zig();
#endif
#ifdef BUILD_TREE_SITTER_YAML
TSLanguage* tree_sitter_yaml();
#endif
#ifdef BUILD_TREE_SITTER_SQL
TSLanguage* tree_sitter_sql();
#endif
#ifdef BUILD_TREE_SITTER_CSS
TSLanguage* tree_sitter_css();
#endif
#ifdef BUILD_TREE_SITTER_XML
TSLanguage* tree_sitter_xml();
#endif
#ifdef BUILD_TREE_SITTER_HTML
TSLanguage* tree_sitter_html();
#endif
}
#endif

namespace pnana {
namespace features {

AutoIndentEngine::AutoIndentEngine(const core::ConfigManager& config_manager)
    : config_manager_(config_manager), file_type_("text"), indent_config_{4, true, true, {}}
#ifdef BUILD_TREE_SITTER_SUPPORT
      ,
      parser_(nullptr), current_language_(nullptr)
#endif
{
#ifdef BUILD_TREE_SITTER_SUPPORT
    parser_ = ts_parser_new();
    if (parser_) {
        initializeLanguages();
    }
#endif
}

AutoIndentEngine::~AutoIndentEngine() {
#ifdef BUILD_TREE_SITTER_SUPPORT
    if (parser_) {
        ts_parser_delete(parser_);
    }
#endif
}

void AutoIndentEngine::setFileType(const std::string& file_type) {
    if (file_type_ == file_type) {
        return;
    }
    file_type_ = file_type;

#ifdef BUILD_TREE_SITTER_SUPPORT
    TSLanguage* lang = getLanguageForFileType(file_type);

    if (lang && parser_) {
        ts_parser_set_language(parser_, lang);
        current_language_ = lang;

        IndentQuery& query = indent_query_map_[file_type];
        query.loadForLanguage(file_type, lang);
    } else {
        current_language_ = nullptr;
    }
#endif

    auto default_cfg = getDefaultConfigForLanguage(file_type);
    indent_config_.indent_size = default_cfg.indent_size;
    indent_config_.insert_spaces = default_cfg.insert_spaces;
    indent_config_.smart_indent = default_cfg.smart_indent;
    indent_config_.file_extensions = default_cfg.file_extensions;
}

void AutoIndentEngine::setFileType(const std::string& file_type,
                                   const core::LanguageIndentConfig& user_config) {
    if (file_type_ == file_type && indent_config_.indent_size == user_config.indent_size) {
        return;
    }
    file_type_ = file_type;

#ifdef BUILD_TREE_SITTER_SUPPORT
    TSLanguage* lang = getLanguageForFileType(file_type);

    if (lang && parser_) {
        ts_parser_set_language(parser_, lang);
        current_language_ = lang;

        IndentQuery& query = indent_query_map_[file_type];
        query.loadForLanguage(file_type, lang);
    } else {
        current_language_ = nullptr;
    }
#endif

    indent_config_ = user_config;
}

void AutoIndentEngine::setIndentConfig(const core::LanguageIndentConfig& config) {
    indent_config_ = config;
}

std::string AutoIndentEngine::computeIndent(const std::vector<std::string>& lines,
                                            size_t cursor_row, size_t cursor_col) const {
    if (lines.empty() || cursor_row >= lines.size()) {
        return "";
    }

#ifdef BUILD_TREE_SITTER_SUPPORT
    if (current_language_ && indent_config_.smart_indent) {
        int level = computeIndentFromTree(lines, cursor_row, cursor_col);
        if (level >= 0) {
            return indentToString(level);
        }
    }
#else
    (void)cursor_col;
#endif

    int level = computeIndentFallback(lines, cursor_row, cursor_col);
    return indentToString(level);
}

std::string AutoIndentEngine::computeIndentAfterNewline(const std::vector<std::string>& lines,
                                                        size_t cursor_row,
                                                        size_t cursor_col) const {
    if (lines.empty() || cursor_row == 0) {
        return "";
    }

    size_t prev_row = cursor_row - 1;
    if (prev_row >= lines.size()) {
        return "";
    }

    const std::string& prev_line = lines[prev_row];

    if (prev_line.empty()) {
        return indentToString(spacesToIndentLevel(countLeadingSpaces(lines[cursor_row - 1])));
    }

#ifdef BUILD_TREE_SITTER_SUPPORT
    if (current_language_ && indent_config_.smart_indent) {
        int level = computeIndentFromTree(lines, cursor_row, cursor_col);
        if (level >= 0) {
            return indentToString(level);
        }
    }
#else
    (void)cursor_col;
#endif

    int level = computeIndentFallback(lines, cursor_row, cursor_col);
    return indentToString(level);
}

core::LanguageIndentConfig AutoIndentEngine::getDefaultConfigForLanguage(
    const std::string& language_id) const {
    core::LanguageIndentConfig cfg;
    const auto& global_cfg = config_manager_.getConfig();
    cfg.indent_size = global_cfg.editor.tab_size;
    cfg.insert_spaces = global_cfg.editor.insert_spaces;
    cfg.smart_indent = true;
    (void)language_id;
    return cfg;
}

bool AutoIndentEngine::isTreeSitterEnabled() const {
#ifdef BUILD_TREE_SITTER_SUPPORT
    return parser_ != nullptr && current_language_ != nullptr;
#else
    return false;
#endif
}

#ifdef BUILD_TREE_SITTER_SUPPORT
void AutoIndentEngine::initializeLanguages() {
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

#ifdef BUILD_TREE_SITTER_PYTHON
    TSLanguage* python_lang = tree_sitter_python();
    if (python_lang) {
        language_map_["py"] = python_lang;
        language_map_["python"] = python_lang;
        language_map_["pyw"] = python_lang;
        language_map_["pyi"] = python_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_JAVASCRIPT
    TSLanguage* js_lang = tree_sitter_javascript();
    if (js_lang) {
        language_map_["js"] = js_lang;
        language_map_["javascript"] = js_lang;
        language_map_["jsx"] = js_lang;
        language_map_["mjs"] = js_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_TYPESCRIPT
    TSLanguage* ts_lang = tree_sitter_typescript();
    if (ts_lang) {
        language_map_["ts"] = ts_lang;
        language_map_["typescript"] = ts_lang;
        language_map_["tsx"] = ts_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_JSON
    TSLanguage* json_lang = tree_sitter_json();
    if (json_lang) {
        language_map_["json"] = json_lang;
        language_map_["jsonc"] = json_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_BASH
    TSLanguage* bash_lang = tree_sitter_bash();
    if (bash_lang) {
        language_map_["sh"] = bash_lang;
        language_map_["bash"] = bash_lang;
        language_map_["shell"] = bash_lang;
        language_map_["zsh"] = bash_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_RUST
    TSLanguage* rust_lang = tree_sitter_rust();
    if (rust_lang) {
        language_map_["rs"] = rust_lang;
        language_map_["rust"] = rust_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_GO
    TSLanguage* go_lang = tree_sitter_go();
    if (go_lang) {
        language_map_["go"] = go_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_JAVA
    TSLanguage* java_lang = tree_sitter_java();
    if (java_lang) {
        language_map_["java"] = java_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_RUBY
    TSLanguage* ruby_lang = tree_sitter_ruby();
    if (ruby_lang) {
        language_map_["rb"] = ruby_lang;
        language_map_["ruby"] = ruby_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_KOTLIN
    TSLanguage* kotlin_lang = tree_sitter_kotlin();
    if (kotlin_lang) {
        language_map_["kt"] = kotlin_lang;
        language_map_["kotlin"] = kotlin_lang;
        language_map_["kts"] = kotlin_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_SWIFT
    TSLanguage* swift_lang = tree_sitter_swift();
    if (swift_lang) {
        language_map_["swift"] = swift_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_CSHARP
    TSLanguage* csharp_lang = tree_sitter_c_sharp();
    if (csharp_lang) {
        language_map_["cs"] = csharp_lang;
        language_map_["csharp"] = csharp_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_PHP
    TSLanguage* php_lang = tree_sitter_php();
    if (php_lang) {
        language_map_["php"] = php_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_LUA
    TSLanguage* lua_lang = tree_sitter_lua();
    if (lua_lang) {
        language_map_["lua"] = lua_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_ZIG
    TSLanguage* zig_lang = tree_sitter_zig();
    if (zig_lang) {
        language_map_["zig"] = zig_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_YAML
    TSLanguage* yaml_lang = tree_sitter_yaml();
    if (yaml_lang) {
        language_map_["yaml"] = yaml_lang;
        language_map_["yml"] = yaml_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_SQL
    TSLanguage* sql_lang = tree_sitter_sql();
    if (sql_lang) {
        language_map_["sql"] = sql_lang;
    }
#endif

#ifdef BUILD_TREE_SITTER_CSS
    TSLanguage* css_lang = tree_sitter_css();
    if (css_lang) {
        language_map_["css"] = css_lang;
        language_map_["scss"] = css_lang;
        language_map_["sass"] = css_lang;
        language_map_["less"] = css_lang;
    }
#endif

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
}

TSLanguage* AutoIndentEngine::getLanguageForFileType(const std::string& file_type) {
    auto it = language_map_.find(file_type);
    if (it != language_map_.end()) {
        return it->second;
    }
    return nullptr;
}

IndentQuery* AutoIndentEngine::getIndentQueryForFileType(const std::string& file_type) {
    auto it = indent_query_map_.find(file_type);
    if (it != indent_query_map_.end()) {
        if (it->second.isLoaded()) {
            return &it->second;
        }
    }
    return nullptr;
}

int AutoIndentEngine::computeIndentFromTree(const std::vector<std::string>& lines,
                                            size_t cursor_row, size_t cursor_col) const {
    (void)cursor_col;

    if (lines.empty() || cursor_row > lines.size()) {
        return -1;
    }

    std::string code;
    std::vector<size_t> line_starts;
    for (size_t i = 0; i < lines.size(); i++) {
        line_starts.push_back(code.size());
        code += lines[i] + "\n";
    }

    TSTree* tree = ts_parser_parse_string(parser_, nullptr, code.c_str(), code.length());
    if (!tree) {
        return -1;
    }

    TSNode root = ts_tree_root_node(tree);

    uint32_t query_row, query_col;

    if (file_type_ == "py" || file_type_ == "python" || file_type_ == "pyw" ||
        file_type_ == "pyi") {
        size_t search_row = cursor_row;
        if (search_row > 0) {
            search_row = cursor_row - 1;
            while (search_row > 0 && lines[search_row].empty()) {
                search_row--;
            }
        }
        query_row = static_cast<uint32_t>(search_row);
        query_col =
            (search_row < lines.size()) ? static_cast<uint32_t>(lines[search_row].size()) : 0;
    } else {
        query_row = static_cast<uint32_t>(cursor_row);
        query_col = 0;
    }

    TSPoint start_point = {query_row, 0};
    TSPoint end_point = {query_row, query_col};

    TSNode node = ts_node_named_descendant_for_point_range(root, start_point, end_point);
    if (ts_node_is_null(node)) {
        ts_tree_delete(tree);
        return -1;
    }

    const char* node_type = ts_node_type(node);

    if (file_type_ == "py" || file_type_ == "python" || file_type_ == "pyw" ||
        file_type_ == "pyi") {
        std::string initial_type(node_type);
        if (initial_type == "function_definition" || initial_type == "class_definition" ||
            initial_type == "if_statement" || initial_type == "for_statement" ||
            initial_type == "while_statement" || initial_type == "try_statement" ||
            initial_type == "with_statement") {
            uint32_t child_count = ts_node_child_count(node);
            for (uint32_t i = 0; i < child_count; i++) {
                TSNode child = ts_node_child(node, i);
                const char* child_type = ts_node_type(child);
                if (std::string(child_type) == "block") {
                    TSPoint child_start = ts_node_start_point(child);
                    TSPoint child_end = ts_node_end_point(child);
                    if (query_row >= child_start.row && query_row <= child_end.row) {
                        node = child;
                        node_type = child_type;
                        break;
                    }
                }
            }
        }
    }

    IndentQuery* query = const_cast<AutoIndentEngine*>(this)->getIndentQueryForFileType(file_type_);
    if (query && query->isLoaded()) {
        auto captures = query->queryAtRow(tree, query_row);
        if (!captures.empty()) {
            int level = query->computeIndentLevel(captures, query_row);
            ts_tree_delete(tree);
            return level;
        }
    }

    int indent_level = 0;
    TSNode current = node;
    TSNode innermost_block = TSNode{};

    if (file_type_ == "py" || file_type_ == "python" || file_type_ == "pyw" ||
        file_type_ == "pyi") {
        std::string start_type(ts_node_type(current));
        if (start_type == "function_definition" || start_type == "class_definition" ||
            start_type == "if_statement" || start_type == "for_statement" ||
            start_type == "while_statement" || start_type == "try_statement" ||
            start_type == "with_statement") {
            uint32_t child_count = ts_node_child_count(current);
            for (uint32_t i = 0; i < child_count; i++) {
                TSNode child = ts_node_child(current, i);
                const char* child_type = ts_node_type(child);
                if (std::string(child_type) == "block") {
                    TSPoint child_start = ts_node_start_point(child);
                    TSPoint child_end = ts_node_end_point(child);
                    if (query_row >= child_start.row && query_row <= child_end.row) {
                        if (ts_node_is_null(innermost_block)) {
                            innermost_block = child;
                        }
                        break;
                    }
                }
            }
        }
    }

    while (!ts_node_is_null(current)) {
        const char* type = ts_node_type(current);
        std::string type_str(type);

        bool is_block = false;

        if (file_type_ == "sh" || file_type_ == "bash" || file_type_ == "shell" ||
            file_type_ == "zsh") {
            is_block = (type_str == "compound_statement" || type_str == "if_statement" ||
                        type_str == "for_statement" || type_str == "while_statement" ||
                        type_str == "case_statement" || type_str == "case_item" ||
                        type_str == "do_group" || type_str == "function_definition");
        } else if (file_type_ == "py" || file_type_ == "python" || file_type_ == "pyw" ||
                   file_type_ == "pyi") {
            is_block =
                (type_str == "block" || type_str == "if_statement" || type_str == "for_statement" ||
                 type_str == "while_statement" || type_str == "try_statement" ||
                 type_str == "except_clause" || type_str == "finally_clause" ||
                 type_str == "with_statement" || type_str == "match_statement" ||
                 type_str == "elif_clause" || type_str == "else_clause");
        } else if (file_type_ == "c" || file_type_ == "cpp" || file_type_ == "cxx" ||
                   file_type_ == "cc" || file_type_ == "c++" || file_type_ == "h" ||
                   file_type_ == "hpp" || file_type_ == "hxx" || file_type_ == "hh") {
            is_block = (type_str == "compound_statement" || type_str == "function_definition" ||
                        type_str == "class_definition" || type_str == "field_declaration_list" ||
                        type_str == "class_specifier" || type_str == "struct_specifier" ||
                        type_str == "namespace_definition" || type_str == "enumerator_list" ||
                        type_str == "switch_statement");
        } else if (file_type_ == "rs" || file_type_ == "rust") {
            is_block = (type_str == "block" || type_str == "field_declaration_list" ||
                        type_str == "declaration_list" || type_str == "match_block" ||
                        type_str == "match_arm" || type_str == "function_item" ||
                        type_str == "impl_item" || type_str == "struct_item" ||
                        type_str == "enum_item" || type_str == "trait_item" ||
                        type_str == "mod_item" || type_str == "for_expression" ||
                        type_str == "while_expression" || type_str == "loop_expression" ||
                        type_str == "if_expression" || type_str == "else_clause");
        } else if (file_type_ == "go" || file_type_ == "golang") {
            is_block =
                (type_str == "block" || type_str == "field_declaration_list" ||
                 type_str == "const_declaration" || type_str == "var_declaration" ||
                 type_str == "import_declaration" || type_str == "import_spec_list" ||
                 type_str == "type_declaration" || type_str == "type_spec" ||
                 type_str == "struct_type" || type_str == "interface_type" ||
                 type_str == "function_declaration" || type_str == "method_declaration" ||
                 type_str == "if_statement" || type_str == "for_statement" ||
                 type_str == "switch_statement" || type_str == "select_statement" ||
                 type_str == "expression_switch_statement" || type_str == "expression_case" ||
                 type_str == "communication_case" || type_str == "default_case");
        } else if (file_type_ == "java") {
            is_block = (type_str == "block" || type_str == "class_body" ||
                        type_str == "method_declaration" || type_str == "constructor_declaration" ||
                        type_str == "if_statement" || type_str == "for_statement" ||
                        type_str == "enhanced_for_statement" || type_str == "while_statement" ||
                        type_str == "do_statement" || type_str == "switch_expression" ||
                        type_str == "switch_block" || type_str == "try_statement" ||
                        type_str == "catch_clause" || type_str == "finally_clause" ||
                        type_str == "lambda_expression" || type_str == "annotation_type_body" ||
                        type_str == "enum_body" || type_str == "interface_body");
        } else {
            is_block = (type_str == "compound_statement" || type_str == "block" ||
                        type_str == "statement_block" || type_str == "function_definition" ||
                        type_str == "class_definition" || type_str == "if_statement" ||
                        type_str == "for_statement" || type_str == "while_statement" ||
                        type_str == "switch_statement" || type_str == "try_statement" ||
                        type_str == "catch_clause" || type_str == "else_clause" ||
                        type_str == "do_statement" || type_str == "with_statement" ||
                        type_str == "match_statement" || type_str == "lambda" ||
                        type_str == "arrow_function" || type_str == "method_definition" ||
                        type_str == "constructor_declaration" || type_str == "enum_body" ||
                        type_str == "interface_body" || type_str == "object_type" ||
                        type_str == "type_body");
        }

        bool is_root = (type_str == "program" || type_str == "translation_unit" ||
                        type_str == "module" || type_str == "source_file");

        if (!is_root && is_block) {
            if (ts_node_is_null(innermost_block)) {
                innermost_block = current;
            }
        }
        current = ts_node_parent(current);
    }

    if (!ts_node_is_null(innermost_block)) {
        TSPoint innermost_start = ts_node_start_point(innermost_block);
        if (innermost_start.row < lines.size()) {
            const std::string& innermost_line = lines[innermost_start.row];
            int innermost_indent = countLeadingSpaces(innermost_line);
            int innermost_level = innermost_indent / indent_config_.indent_size;
            indent_level = innermost_level + 1;
        }
    } else {
        indent_level = 0;
    }

    ts_tree_delete(tree);

    return indent_level;
}
#endif

int AutoIndentEngine::computeIndentFallback(const std::vector<std::string>& lines,
                                            size_t cursor_row, size_t cursor_col) const {
    (void)cursor_col;

    if (lines.empty() || cursor_row >= lines.size()) {
        return 0;
    }

    if (cursor_row == 0) {
        return 0;
    }

    const std::string& current_line = lines[cursor_row];
    const std::string& prev_line = lines[cursor_row - 1];

    int leading_spaces = countLeadingSpaces(prev_line);
    int base_indent = spacesToIndentLevel(leading_spaces);

    if (prev_line.empty()) {
        return base_indent;
    }

    std::string trimmed_prev = prev_line;
    trimmed_prev.erase(0, trimmed_prev.find_first_not_of(" \t"));

    std::string trimmed_current = current_line;
    trimmed_current.erase(0, trimmed_current.find_first_not_of(" \t"));

    auto has_open_char = [](const std::string& s) {
        return s.find('{') != std::string::npos || s.find('(') != std::string::npos ||
               s.find('[') != std::string::npos;
    };

    auto ends_with_open_char = [](const std::string& s) {
        if (s.empty())
            return false;
        char last = s.back();
        return last == '{' || last == '(' || last == '[' || last == ':';
    };

    auto starts_with_close_char = [](const std::string& s) {
        if (s.empty())
            return false;
        return s[0] == '}' || s[0] == ')' || s[0] == ']';
    };

    bool prev_opens = ends_with_open_char(trimmed_prev) || has_open_char(trimmed_prev);
    bool curr_closes = starts_with_close_char(trimmed_current);

    int result = base_indent;
    if (curr_closes) {
        result = std::max(0, base_indent - 1);
    } else if (prev_opens) {
        result = base_indent + 1;
    }

    return result;
}

std::string AutoIndentEngine::indentToString(int level) const {
    if (level <= 0) {
        return "";
    }

    std::string result;
    if (indent_config_.insert_spaces) {
        result = std::string(level * indent_config_.indent_size, ' ');
    } else {
        result = std::string(static_cast<size_t>(level), '\t');
    }

    return result;
}

int AutoIndentEngine::countLeadingSpaces(const std::string& line) const {
    int count = 0;
    for (char ch : line) {
        if (ch == ' ') {
            count++;
        } else if (ch == '\t') {
            count += indent_config_.indent_size;
        } else {
            break;
        }
    }
    return count;
}

int AutoIndentEngine::spacesToIndentLevel(int spaces) const {
    if (indent_config_.indent_size <= 0) {
        return 0;
    }
    return spaces / indent_config_.indent_size;
}

} // namespace features
} // namespace pnana
