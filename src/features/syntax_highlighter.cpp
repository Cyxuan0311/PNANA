#include "features/syntax_highlighter.h"
#include <algorithm>
#include <cctype>

using namespace ftxui;

namespace pnana {
namespace features {

SyntaxHighlighter::SyntaxHighlighter(ui::Theme& theme)
    : theme_(theme), current_file_type_("text"),
      in_multiline_comment_(false), in_multiline_string_(false) {
    initializeLanguages();
}

void SyntaxHighlighter::initializeLanguages() {
    // C/C++ 关键字
    keywords_["cpp"] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "inline", "int", "long", "register", "return", "short", "signed",
        "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
        "void", "volatile", "while", "class", "namespace", "template", "typename",
        "public", "private", "protected", "virtual", "override", "final",
        "using", "try", "catch", "throw", "new", "delete", "this", "nullptr",
        "true", "false", "const_cast", "static_cast", "dynamic_cast", "reinterpret_cast"
    };
    
    types_["cpp"] = {
        "bool", "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "size_t", "ssize_t", "string", "vector", "map", "set",
        "shared_ptr", "unique_ptr", "weak_ptr"
    };
    
    // Python 关键字
    keywords_["python"] = {
        "and", "as", "assert", "async", "await", "break", "class", "continue",
        "def", "del", "elif", "else", "except", "False", "finally", "for",
        "from", "global", "if", "import", "in", "is", "lambda", "None",
        "nonlocal", "not", "or", "pass", "raise", "return", "True", "try",
        "while", "with", "yield", "self"
    };
    
    types_["python"] = {
        "int", "float", "str", "bool", "list", "dict", "tuple", "set",
        "bytes", "bytearray", "object", "type"
    };
    
    // JavaScript 关键字
    keywords_["javascript"] = {
        "async", "await", "break", "case", "catch", "class", "const", "continue",
        "debugger", "default", "delete", "do", "else", "enum", "export", "extends",
        "false", "finally", "for", "function", "if", "implements", "import", "in",
        "instanceof", "interface", "let", "new", "null", "package", "private",
        "protected", "public", "return", "static", "super", "switch", "this",
        "throw", "true", "try", "typeof", "var", "void", "while", "with", "yield"
    };
    
    types_["javascript"] = {
        "Array", "Boolean", "Date", "Error", "Function", "JSON", "Math",
        "Number", "Object", "Promise", "RegExp", "String", "Symbol", "Map", "Set"
    };
    
    // Shell 关键字
    keywords_["shell"] = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for", "while",
        "do", "done", "in", "function", "return", "exit", "break", "continue",
        "echo", "export", "source", "cd", "pwd", "ls", "cat", "grep", "sed", "awk"
    };
}

void SyntaxHighlighter::setFileType(const std::string& file_type) {
    if (current_file_type_ != file_type) {
    current_file_type_ = file_type;
        resetMultiLineState();
    }
}

void SyntaxHighlighter::resetMultiLineState() {
    in_multiline_comment_ = false;
    in_multiline_string_ = false;
}

ftxui::Element SyntaxHighlighter::highlightLine(const std::string& line) {
    if (line.empty()) {
        return text("");
    }
    
    // 根据文件类型分词
    std::vector<Token> tokens = tokenize(line);
    
    if (tokens.empty()) {
        return text(line) | color(theme_.getColors().foreground);
    }
    
    // 渲染每个token
    Elements elements;
    for (const auto& token : tokens) {
        Color token_color = getColorForToken(token.type);
        elements.push_back(text(token.text) | color(token_color));
    }
    
    return hbox(elements);
}

std::vector<Token> SyntaxHighlighter::tokenize(const std::string& line) {
    if (current_file_type_ == "cpp" || current_file_type_ == "c") {
        return tokenizeCpp(line);
    } else if (current_file_type_ == "python") {
        return tokenizePython(line);
    } else if (current_file_type_ == "javascript" || current_file_type_ == "typescript") {
        return tokenizeJavaScript(line);
    } else if (current_file_type_ == "json") {
        return tokenizeJSON(line);
    } else if (current_file_type_ == "markdown") {
        return tokenizeMarkdown(line);
    } else if (current_file_type_ == "shell") {
        return tokenizeShell(line);
    }
    
    // 默认：不高亮
    return {{line, TokenType::NORMAL, 0, line.length()}};
}

std::vector<Token> SyntaxHighlighter::tokenizeCpp(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    // 处理多行注释的延续
    if (in_multiline_comment_) {
        size_t end_pos = line.find("*/");
        if (end_pos != std::string::npos) {
            tokens.push_back({line.substr(0, end_pos + 2), TokenType::COMMENT, 0, end_pos + 2});
            in_multiline_comment_ = false;
            i = end_pos + 2;
        } else {
            tokens.push_back({line, TokenType::COMMENT, 0, line.length()});
            return tokens;
        }
    }
    
    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i])) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }
        
        // 多行注释
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '*') {
            size_t start = i;
            i += 2;
            size_t end_pos = line.find("*/", i);
            if (end_pos != std::string::npos) {
                i = end_pos + 2;
                tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            } else {
                tokens.push_back({line.substr(start), TokenType::COMMENT, start, line.length()});
                in_multiline_comment_ = true;
                break;
            }
            continue;
        }
        
        // 单行注释
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '/') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }
        
        // 预处理器
        if (line[i] == '#') {
            size_t start = i;
            i++;
            // 跳过空白
            while (i < line.length() && std::isspace(line[i])) i++;
            // 读取指令
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) i++;
            // 读取参数（直到行尾或注释）
            while (i < line.length() && line[i] != '\n' && 
                   !(i + 1 < line.length() && line[i] == '/' && line[i + 1] == '/')) {
                i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::PREPROCESSOR, start, i});
            continue;
        }
        
        // 原始字符串字面量 (C++11)
        if (i + 1 < line.length() && line[i] == 'R' && line[i + 1] == '"') {
            size_t start = i;
            i += 2;
            // 查找分隔符
            size_t delim_start = i;
            while (i < line.length() && line[i] != '(') {
                if (line[i] == '"') {
                    // 没有分隔符的简单原始字符串
                    i = delim_start;
            break;
                }
                i++;
            }
            if (i < line.length() && line[i] == '(') {
                std::string delimiter = line.substr(delim_start, i - delim_start);
                i++; // 跳过 '('
                // 查找结束标记
                std::string end_marker = ")" + delimiter + "\"";
                size_t end_pos = line.find(end_marker, i);
                if (end_pos != std::string::npos) {
                    i = end_pos + end_marker.length();
                    tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
                    continue;
                }
            }
            // 回退到普通字符串处理
            i = start;
        }
        
        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length()) {
                if (line[i] == '\\' && i + 1 < line.length()) {
                    i += 2; // 跳过转义字符
                } else if (line[i] == quote) {
                    i++;
                    break;
                } else {
                    i++;
                }
            }
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 数字（支持十六进制、八进制、科学计数法）
        if (std::isdigit(line[i]) || (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            i = parseNumber(line, start);
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }
        
        // 多字符操作符
        if (isMultiCharOperator(line, i)) {
            size_t start = i;
            std::string op;
            if (i + 1 < line.length()) {
                std::string two_char = line.substr(i, 2);
                if (two_char == "==" || two_char == "!=" || two_char == "<=" || two_char == ">=" ||
                    two_char == "&&" || two_char == "||" || two_char == "<<" || two_char == ">>" ||
                    two_char == "++" || two_char == "--" || two_char == "+=" || two_char == "-=" ||
                    two_char == "*=" || two_char == "/=" || two_char == "%=" || two_char == "&=" ||
                    two_char == "|=" || two_char == "^=" || two_char == "->") {
                    op = two_char;
                    i += 2;
                } else {
                    op = std::string(1, line[i]);
                    i++;
                }
            } else {
                op = std::string(1, line[i]);
                i++;
            }
            tokens.push_back({op, TokenType::OPERATOR, start, i});
            continue;
        }
        
        // 标识符/关键字
        if (std::isalpha(line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) i++;
            std::string word = line.substr(start, i - start);
            
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            } else if (isType(word)) {
                type = TokenType::TYPE;
            } else {
                // 检查是否是函数调用（跳过可能的模板参数）
                size_t check_pos = i;
                int template_depth = 0;
                while (check_pos < line.length()) {
                    if (line[check_pos] == '<') template_depth++;
                    else if (line[check_pos] == '>') template_depth--;
                    else if (line[check_pos] == '(' && template_depth == 0) {
                type = TokenType::FUNCTION;
                        break;
                    } else if (std::isspace(line[check_pos])) {
                        check_pos++;
                        continue;
                    } else if (template_depth == 0 && !std::isalnum(line[check_pos]) && 
                               line[check_pos] != '_' && line[check_pos] != ':' && line[check_pos] != '&' &&
                               line[check_pos] != '*' && line[check_pos] != '[' && line[check_pos] != ']') {
                        break;
                    }
                    check_pos++;
                }
            }
            
            tokens.push_back({word, type, start, i});
            continue;
        }
        
        // 单字符操作符
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }
        
        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }
    
    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizePython(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i])) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }
        
        // 注释
        if (line[i] == '#') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }
        
        // 原始字符串 (r"..." 或 r'...')
        if (i + 1 < line.length() && (line[i] == 'r' || line[i] == 'R') && 
            (line[i + 1] == '"' || line[i + 1] == '\'')) {
            char quote = line[i + 1];
            size_t start = i;
            i += 2;
            
            // 三引号原始字符串
            if (i + 1 < line.length() && line[i] == quote && line[i + 1] == quote) {
                i += 2;
                while (i + 2 < line.length() && 
                       !(line[i] == quote && line[i + 1] == quote && line[i + 2] == quote)) {
                    i++;
                }
                if (i + 2 < line.length()) i += 3;
            } else {
                while (i < line.length() && line[i] != quote) {
                    i++; // 原始字符串不处理转义
                }
                if (i < line.length()) i++;
            }
            
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // f-string (f"..." 或 f'...')
        if (i + 1 < line.length() && (line[i] == 'f' || line[i] == 'F') && 
            (line[i + 1] == '"' || line[i + 1] == '\'')) {
            char quote = line[i + 1];
            size_t start = i;
            i += 2;
            
            // 三引号 f-string
            if (i + 1 < line.length() && line[i] == quote && line[i + 1] == quote) {
                i += 2;
                while (i + 2 < line.length() && 
                       !(line[i] == quote && line[i + 1] == quote && line[i + 2] == quote)) {
                    if (line[i] == '{') {
                        // f-string 表达式
                        i++;
                        int brace_count = 1;
                        while (i < line.length() && brace_count > 0) {
                            if (line[i] == '{') brace_count++;
                            else if (line[i] == '}') brace_count--;
                            i++;
                        }
                    } else {
                        i++;
                    }
                }
                if (i + 2 < line.length()) i += 3;
            } else {
                while (i < line.length() && line[i] != quote) {
                    if (line[i] == '\\' && i + 1 < line.length()) {
                        i += 2;
                    } else if (line[i] == '{') {
                        // f-string 表达式
                        i++;
                        int brace_count = 1;
                        while (i < line.length() && brace_count > 0) {
                            if (line[i] == '{') brace_count++;
                            else if (line[i] == '}') brace_count--;
                            i++;
                        }
                    } else {
                        i++;
                    }
                }
                if (i < line.length()) i++;
            }
            
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            
            // 三引号字符串
            if (i + 1 < line.length() && line[i] == quote && line[i + 1] == quote) {
                i += 2;
                while (i + 2 < line.length() && 
                       !(line[i] == quote && line[i + 1] == quote && line[i + 2] == quote)) {
                    if (line[i] == '\\' && i + 1 < line.length()) i += 2;
                    else i++;
                }
                if (i + 2 < line.length()) i += 3;
            } else {
                while (i < line.length() && line[i] != quote) {
                    if (line[i] == '\\' && i + 1 < line.length()) i += 2;
                    else i++;
                }
                if (i < line.length()) i++;
            }
            
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 数字（支持复数、科学计数法）
        if (std::isdigit(line[i]) || (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.')) i++;
            
            // 科学计数法
            if (i < line.length() && (line[i] == 'e' || line[i] == 'E')) {
                i++;
                if (i < line.length() && (line[i] == '+' || line[i] == '-')) i++;
                while (i < line.length() && std::isdigit(line[i])) i++;
            }
            
            // 复数后缀
            if (i < line.length() && (line[i] == 'j' || line[i] == 'J')) {
                i++;
            }
            
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }
        
        // 多字符操作符 (Python特有)
        if (i + 1 < line.length()) {
            std::string two_char = line.substr(i, 2);
            if (two_char == "==" || two_char == "!=" || two_char == "<=" || two_char == ">=" ||
                two_char == "**" || two_char == "//" || two_char == "<<" || two_char == ">>" ||
                two_char == "+=" || two_char == "-=" || two_char == "*=" || two_char == "/=" ||
                two_char == "%=" || two_char == "&=" || two_char == "|=" || two_char == "^=" ||
                two_char == "//=" || two_char == "**=" || two_char == "->" || two_char == "::") {
                tokens.push_back({two_char, TokenType::OPERATOR, i, i + 2});
                i += 2;
                continue;
            }
        }
        
        // 标识符/关键字
        if (std::isalpha(line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) i++;
            std::string word = line.substr(start, i - start);
            
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            } else if (isType(word)) {
                type = TokenType::TYPE;
            } else if (i < line.length() && line[i] == '(') {
                type = TokenType::FUNCTION;
            }
            
            tokens.push_back({word, type, start, i});
            continue;
        }
        
        // 单字符操作符
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }
        
        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }
    
    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeJavaScript(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i])) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }
        
        // 单行注释
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '/') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }
        
        // 多行注释
        if (i + 1 < line.length() && line[i] == '/' && line[i + 1] == '*') {
            size_t start = i;
            i += 2;
            size_t end_pos = line.find("*/", i);
            if (end_pos != std::string::npos) {
                i = end_pos + 2;
                tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            } else {
                tokens.push_back({line.substr(start), TokenType::COMMENT, start, line.length()});
                break;
            }
            continue;
        }
        
        // 模板字符串 (反引号)
        if (line[i] == '`') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '`') {
                if (line[i] == '\\' && i + 1 < line.length()) {
                    i += 2;
                } else if (line[i] == '$' && i + 1 < line.length() && line[i + 1] == '{') {
                    // 模板表达式开始
                    i += 2;
                    int brace_count = 1;
                    while (i < line.length() && brace_count > 0) {
                        if (line[i] == '{') brace_count++;
                        else if (line[i] == '}') brace_count--;
                        i++;
                    }
                } else {
                    i++;
                }
            }
            if (i < line.length()) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length()) i += 2;
                else i++;
            }
            if (i < line.length()) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 数字
        if (std::isdigit(line[i]) || (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            i = parseNumber(line, start);
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }
        
        // 箭头函数 =>
        if (i + 1 < line.length() && line[i] == '=' && line[i + 1] == '>') {
            tokens.push_back({line.substr(i, 2), TokenType::OPERATOR, i, i + 2});
            i += 2;
            continue;
        }
        
        // 多字符操作符
        if (isMultiCharOperator(line, i)) {
            size_t start = i;
            std::string op;
            if (i + 1 < line.length()) {
                std::string two_char = line.substr(i, 2);
                if (two_char == "==" || two_char == "!=" || two_char == "<=" || two_char == ">=" ||
                    two_char == "&&" || two_char == "||" || two_char == "++" || two_char == "--" ||
                    two_char == "+=" || two_char == "-=" || two_char == "*=" || two_char == "/=" ||
                    two_char == "===" || two_char == "!==") {
                    op = two_char;
                    i += 2;
                    // 检查 === 和 !==
                    if (op == "==" && i < line.length() && line[i] == '=') {
                        op = "===";
                        i++;
                    } else if (op == "!=" && i < line.length() && line[i] == '=') {
                        op = "!==";
                        i++;
                    }
                } else {
                    op = std::string(1, line[i]);
                    i++;
                }
            } else {
                op = std::string(1, line[i]);
                i++;
            }
            tokens.push_back({op, TokenType::OPERATOR, start, i});
            continue;
        }
        
        // 标识符/关键字
        if (std::isalpha(line[i]) || line[i] == '_' || line[i] == '$') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '$')) i++;
            std::string word = line.substr(start, i - start);
            
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            } else if (isType(word)) {
                type = TokenType::TYPE;
            } else if (i < line.length() && line[i] == '(') {
                type = TokenType::FUNCTION;
            }
            
            tokens.push_back({word, type, start, i});
            continue;
        }
        
        // 单字符操作符
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }
        
        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }
    
    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeJSON(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    while (i < line.length()) {
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i])) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }
        
        // 字符串（键或值）
        if (line[i] == '"') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < line.length()) i += 2;
                else i++;
            }
            if (i < line.length()) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 数字
        if (std::isdigit(line[i]) || line[i] == '-') {
            size_t start = i;
            if (line[i] == '-') i++;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.')) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }
        
        // 布尔值和null
        if (line.substr(i, 4) == "true" || line.substr(i, 5) == "false" || line.substr(i, 4) == "null") {
            size_t len = (line.substr(i, 4) == "null" || line.substr(i, 4) == "true") ? 4 : 5;
            tokens.push_back({line.substr(i, len), TokenType::KEYWORD, i, i + len});
            i += len;
            continue;
        }
        
        // 其他（括号、逗号等）
        tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
        i++;
    }
    
    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeMarkdown(const std::string& line) {
    std::vector<Token> tokens;
    
    if (line.empty()) return tokens;
    
    size_t i = 0;
    
    // 跳过前导空白
    while (i < line.length() && std::isspace(line[i])) i++;
    
    // 标题 (# ## ### 等)
    if (i < line.length() && line[i] == '#') {
        size_t start = i;
        while (i < line.length() && line[i] == '#') i++;
        tokens.push_back({line.substr(start, i - start), TokenType::KEYWORD, start, i});
        
        // 标题文本
        while (i < line.length() && std::isspace(line[i])) i++;
        if (i < line.length()) {
            tokens.push_back({line.substr(i), TokenType::FUNCTION, i, line.length()});
        }
        return tokens;
    }
    
    // 代码块标记
    if (line.find("```") == 0 || line.find("~~~") == 0) {
        tokens.push_back({line, TokenType::STRING, 0, line.length()});
        return tokens;
    }
    
    // 行内代码 `code`
    while (i < line.length()) {
        if (line[i] == '`') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '`') i++;
            if (i < line.length()) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 粗体 **text** 或 __text__
        if (i + 1 < line.length() && ((line[i] == '*' && line[i + 1] == '*') ||
            (line[i] == '_' && line[i + 1] == '_'))) {
            size_t start = i;
            char marker = line[i];
            i += 2;
            while (i + 1 < line.length() && !(line[i] == marker && line[i + 1] == marker)) i++;
            if (i + 1 < line.length()) {
                tokens.push_back({line.substr(start, 2), TokenType::KEYWORD, start, start + 2});
                if (i > start + 2) {
                    tokens.push_back({line.substr(start + 2, i - start - 2), TokenType::FUNCTION, start + 2, i});
                }
                tokens.push_back({line.substr(i, 2), TokenType::KEYWORD, i, i + 2});
                i += 2;
            } else {
                tokens.push_back({line.substr(start, 2), TokenType::NORMAL, start, start + 2});
                i = start + 2;
            }
            continue;
        }
        
        // 斜体 *text* 或 _text_
        if (line[i] == '*' || line[i] == '_') {
            size_t start = i;
            char marker = line[i];
            i++;
            while (i < line.length() && line[i] != marker && !std::isspace(line[i])) i++;
            if (i < line.length() && line[i] == marker) {
                tokens.push_back({std::string(1, marker), TokenType::KEYWORD, start, start + 1});
                if (i > start + 1) {
                    tokens.push_back({line.substr(start + 1, i - start - 1), TokenType::TYPE, start + 1, i});
                }
                tokens.push_back({std::string(1, marker), TokenType::KEYWORD, i, i + 1});
                i++;
            } else {
                tokens.push_back({std::string(1, marker), TokenType::NORMAL, start, start + 1});
                i = start + 1;
            }
            continue;
        }
        
        // 链接 [text](url)
        if (line[i] == '[') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != ']') i++;
            if (i < line.length() && i + 1 < line.length() && line[i + 1] == '(') {
                tokens.push_back({line.substr(start, 1), TokenType::KEYWORD, start, start + 1});
                if (i > start + 1) {
                    tokens.push_back({line.substr(start + 1, i - start - 1), TokenType::FUNCTION, start + 1, i});
                }
                tokens.push_back({line.substr(i, 1), TokenType::KEYWORD, i, i + 1});
                i++;
                // 跳过 URL
                while (i < line.length() && line[i] != ')') i++;
                if (i < line.length()) {
                    tokens.push_back({line.substr(i, 1), TokenType::STRING, i, i + 1});
                    i++;
                }
                continue;
            }
        }
        
        // 列表标记 (-, *, +, 数字.)
        if ((line[i] == '-' || line[i] == '*' || line[i] == '+') && 
            (i + 1 >= line.length() || std::isspace(line[i + 1]))) {
            tokens.push_back({std::string(1, line[i]), TokenType::KEYWORD, i, i + 1});
            i++;
            continue;
        }
        
        // 普通文本
        size_t text_start = i;
        while (i < line.length() && line[i] != '`' && line[i] != '*' && line[i] != '_' && 
               line[i] != '[' && line[i] != '-' && line[i] != '+' && line[i] != '#') {
            i++;
        }
        if (i > text_start) {
            tokens.push_back({line.substr(text_start, i - text_start), TokenType::NORMAL, text_start, i});
        }
    }
    
    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeShell(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;
    
    while (i < line.length()) {
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i])) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }
        
        // 注释
        if (line[i] == '#') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }
        
        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length()) i += 2;
                else i++;
            }
            if (i < line.length()) i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }
        
        // 变量
        if (line[i] == '$') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == '{') {
                while (i < line.length() && line[i] != '}') i++;
                if (i < line.length()) i++;
            } else {
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_')) i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }
        
        // 命令/关键字
        if (std::isalpha(line[i])) {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '-')) i++;
            std::string word = line.substr(start, i - start);
            
            TokenType type = isKeyword(word) ? TokenType::KEYWORD : TokenType::NORMAL;
            tokens.push_back({word, type, start, i});
            continue;
        }
        
        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
        i++;
    }
    
    return tokens;
}

ftxui::Color SyntaxHighlighter::getColorForToken(TokenType type) const {
    auto& colors = theme_.getColors();
    
    switch (type) {
        case TokenType::KEYWORD:
            return colors.keyword;
        case TokenType::STRING:
            return colors.string;
        case TokenType::COMMENT:
            return colors.comment;
        case TokenType::NUMBER:
            return colors.number;
        case TokenType::FUNCTION:
            return colors.function;
        case TokenType::TYPE:
            return colors.type;
        case TokenType::OPERATOR:
            return colors.operator_color;
        case TokenType::PREPROCESSOR:
            return colors.keyword;  // 使用关键字颜色
        case TokenType::NORMAL:
        default:
            return colors.foreground;
    }
}

bool SyntaxHighlighter::isKeyword(const std::string& word) const {
    auto it = keywords_.find(current_file_type_);
    if (it == keywords_.end()) return false;
    
    return std::find(it->second.begin(), it->second.end(), word) != it->second.end();
}

bool SyntaxHighlighter::isType(const std::string& word) const {
    auto it = types_.find(current_file_type_);
    if (it == types_.end()) return false;
    
    return std::find(it->second.begin(), it->second.end(), word) != it->second.end();
}

bool SyntaxHighlighter::isOperator(char ch) const {
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' ||
           ch == '=' || ch == '<' || ch == '>' || ch == '!' || ch == '&' ||
           ch == '|' || ch == '^' || ch == '~' || ch == '?' || ch == ':' ||
           ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' ||
           ch == '}' || ch == ',' || ch == ';' || ch == '.' || ch == '-';
}

bool SyntaxHighlighter::isMultiCharOperator(const std::string& text, size_t pos) const {
    if (pos + 1 >= text.length()) return false;
    
    char ch1 = text[pos];
    char ch2 = text[pos + 1];
    
    // 双字符操作符
    if ((ch1 == '=' && ch2 == '=') || (ch1 == '!' && ch2 == '=') ||
        (ch1 == '<' && ch2 == '=') || (ch1 == '>' && ch2 == '=') ||
        (ch1 == '&' && ch2 == '&') || (ch1 == '|' && ch2 == '|') ||
        (ch1 == '<' && ch2 == '<') || (ch1 == '>' && ch2 == '>') ||
        (ch1 == '+' && ch2 == '+') || (ch1 == '-' && ch2 == '-') ||
        (ch1 == '+' && ch2 == '=') || (ch1 == '-' && ch2 == '=') ||
        (ch1 == '*' && ch2 == '=') || (ch1 == '/' && ch2 == '=') ||
        (ch1 == '%' && ch2 == '=') || (ch1 == '&' && ch2 == '=') ||
        (ch1 == '|' && ch2 == '=') || (ch1 == '^' && ch2 == '=') ||
        (ch1 == '-' && ch2 == '>')) {
        return true;
    }
    
    return false;
}

size_t SyntaxHighlighter::parseNumber(const std::string& line, size_t start) {
    size_t i = start;
    
    // 十六进制
    if (i + 1 < line.length() && line[i] == '0' && (line[i + 1] == 'x' || line[i + 1] == 'X')) {
        i += 2;
        while (i < line.length() && (std::isxdigit(line[i]) || line[i] == '\'')) i++;
        // 后缀
        if (i < line.length() && (line[i] == 'u' || line[i] == 'U' || 
            line[i] == 'l' || line[i] == 'L')) {
            i++;
            if (i < line.length() && (line[i] == 'l' || line[i] == 'L')) i++;
        }
        return i;
    }
    
    // 八进制或二进制
    if (i < line.length() && line[i] == '0') {
        i++;
        if (i < line.length() && (line[i] == 'b' || line[i] == 'B')) {
            // 二进制
            i++;
            while (i < line.length() && (line[i] == '0' || line[i] == '1' || line[i] == '\'')) i++;
        } else {
            // 八进制
            while (i < line.length() && ((line[i] >= '0' && line[i] <= '7') || line[i] == '\'')) i++;
        }
        // 后缀
        if (i < line.length() && (line[i] == 'u' || line[i] == 'U' || 
            line[i] == 'l' || line[i] == 'L')) {
            i++;
            if (i < line.length() && (line[i] == 'l' || line[i] == 'L')) i++;
        }
        return i;
    }
    
    // 十进制或浮点数
    while (i < line.length() && (std::isdigit(line[i]) || line[i] == '\'')) i++;
    
    // 浮点数
    if (i < line.length() && line[i] == '.') {
        i++;
        while (i < line.length() && (std::isdigit(line[i]) || line[i] == '\'')) i++;
    }
    
    // 科学计数法
    if (i < line.length() && (line[i] == 'e' || line[i] == 'E')) {
        i++;
        if (i < line.length() && (line[i] == '+' || line[i] == '-')) i++;
        while (i < line.length() && std::isdigit(line[i])) i++;
    }
    
    // 后缀
    if (i < line.length() && (line[i] == 'f' || line[i] == 'F' || 
        line[i] == 'l' || line[i] == 'L')) {
        i++;
    }
    
    return i;
}

size_t SyntaxHighlighter::parseString(const std::string& line, size_t start, char quote, TokenType& type) {
    size_t i = start + 1;
    type = TokenType::STRING;
    
    while (i < line.length()) {
        if (line[i] == '\\' && i + 1 < line.length()) {
            i += 2; // 跳过转义字符
        } else if (line[i] == quote) {
            i++;
            break;
        } else {
            i++;
        }
    }
    
    return i;
}

size_t SyntaxHighlighter::parseRawString(const std::string& line, size_t start) {
    size_t i = start + 2; // 跳过 R"
    
    // 查找分隔符
    size_t delim_start = i;
    while (i < line.length() && line[i] != '(') {
        if (line[i] == '"') {
            // 没有分隔符的简单原始字符串
            i = delim_start;
            break;
        }
        i++;
    }
    
    if (i < line.length() && line[i] == '(') {
        std::string delimiter = line.substr(delim_start, i - delim_start);
        i++; // 跳过 '('
        
        // 查找结束标记
        std::string end_marker = ")" + delimiter + "\"";
        size_t end_pos = line.find(end_marker, i);
        if (end_pos != std::string::npos) {
            return end_pos + end_marker.length();
        }
    }
    
    return start; // 解析失败，返回起始位置
}

size_t SyntaxHighlighter::parseComment(const std::string& line, size_t start, bool& is_multiline) {
    if (start + 1 >= line.length()) return start;
    
    if (line[start] == '/' && line[start + 1] == '/') {
        // 单行注释
        is_multiline = false;
        return line.length();
    } else if (line[start] == '/' && line[start + 1] == '*') {
        // 多行注释
        size_t end_pos = line.find("*/", start + 2);
        if (end_pos != std::string::npos) {
            is_multiline = false;
            return end_pos + 2;
        } else {
            is_multiline = true;
            return line.length();
        }
    }
    
    return start;
}

bool SyntaxHighlighter::isNumber(const std::string& text) const {
    if (text.empty()) return false;
    
    for (char ch : text) {
        if (!std::isdigit(ch) && ch != '.' && ch != 'x' && ch != 'X' &&
            ch != 'f' && ch != 'F' && ch != 'l' && ch != 'L') {
            return false;
        }
    }
    return true;
}

} // namespace features
} // namespace pnana

