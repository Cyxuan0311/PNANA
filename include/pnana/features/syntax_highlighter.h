#ifndef PNANA_FEATURES_SYNTAX_HIGHLIGHTER_H
#define PNANA_FEATURES_SYNTAX_HIGHLIGHTER_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include <regex>
#include <map>

namespace pnana {
namespace features {

// 语法元素类型
enum class TokenType {
    NORMAL,
    KEYWORD,
    STRING,
    COMMENT,
    NUMBER,
    FUNCTION,
    TYPE,
    OPERATOR,
    PREPROCESSOR
};

// Token
struct Token {
    std::string text;
    TokenType type;
    size_t start;
    size_t end;
};

// 语法高亮器
class SyntaxHighlighter {
public:
    explicit SyntaxHighlighter(ui::Theme& theme);
    
    // 设置文件类型
    void setFileType(const std::string& file_type);
    
    // 重置多行状态（切换文件时调用）
    void resetMultiLineState();
    
    // 高亮一行代码
    ftxui::Element highlightLine(const std::string& line);
    
    // 获取颜色
    ftxui::Color getColorForToken(TokenType type) const;
    
private:
    ui::Theme& theme_;
    std::string current_file_type_;
    
    // 不同语言的关键字
    std::map<std::string, std::vector<std::string>> keywords_;
    std::map<std::string, std::vector<std::string>> types_;
    
    // 多行注释状态（用于跨行高亮）
    bool in_multiline_comment_;
    bool in_multiline_string_;
    
    // 初始化语言定义
    void initializeLanguages();
    
    // 分词
    std::vector<Token> tokenize(const std::string& line);
    
    // 特定语言的分词器
    std::vector<Token> tokenizeCpp(const std::string& line);
    std::vector<Token> tokenizePython(const std::string& line);
    std::vector<Token> tokenizeJavaScript(const std::string& line);
    std::vector<Token> tokenizeJSON(const std::string& line);
    std::vector<Token> tokenizeMarkdown(const std::string& line);
    std::vector<Token> tokenizeShell(const std::string& line);
    
    // 辅助方法
    bool isKeyword(const std::string& word) const;
    bool isType(const std::string& word) const;
    bool isOperator(char ch) const;
    bool isMultiCharOperator(const std::string& text, size_t pos) const;
    bool isNumber(const std::string& text) const;
    
    // 字符串处理
    size_t parseString(const std::string& line, size_t start, char quote, TokenType& type);
    size_t parseRawString(const std::string& line, size_t start);
    size_t parseNumber(const std::string& line, size_t start);
    
    // 注释处理
    size_t parseComment(const std::string& line, size_t start, bool& is_multiline);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SYNTAX_HIGHLIGHTER_H

