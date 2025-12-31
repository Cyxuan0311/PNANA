#ifndef PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_H
#define PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include <regex>
#include <map>
#include <memory>

// 条件包含 Tree-sitter 头文件（如果启用）
#ifdef BUILD_TREE_SITTER_SUPPORT
#include "features/SyntaxHighlighter/syntax_highlighter_tree_sitter.h"
#else
// 前向声明（如果 Tree-sitter 未启用）
namespace pnana {
namespace features {
class SyntaxHighlighterTreeSitter;
} // namespace features
} // namespace pnana
#endif

namespace pnana {
namespace features {

// 语法高亮后端类型
enum class SyntaxHighlightBackend {
    NATIVE,      // 使用原有的分词器实现
    TREE_SITTER  // 使用 Tree-sitter（如果可用）
};

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

// 语法高亮器（统一接口，支持多种后端）
class SyntaxHighlighter {
public:
    explicit SyntaxHighlighter(ui::Theme& theme, SyntaxHighlightBackend backend = SyntaxHighlightBackend::TREE_SITTER);
    
    // 析构函数（需要显式声明，因为 unique_ptr 需要完整类型）
    ~SyntaxHighlighter();
    
    // 设置文件类型
    void setFileType(const std::string& file_type);
    
    // 重置多行状态（切换文件时调用）
    void resetMultiLineState();
    
    // 高亮一行代码
    ftxui::Element highlightLine(const std::string& line);
    
    // 获取颜色
    ftxui::Color getColorForToken(TokenType type) const;
    
    // 设置后端
    void setBackend(SyntaxHighlightBackend backend);
    
    // 获取当前后端
    SyntaxHighlightBackend getBackend() const { return backend_; }
    
    // 检查 Tree-sitter 是否可用
    static bool isTreeSitterAvailable();
    
private:
    ui::Theme& theme_;
    std::string current_file_type_;
    SyntaxHighlightBackend backend_;
    
    // Tree-sitter 后端（如果可用）
    std::unique_ptr<SyntaxHighlighterTreeSitter> tree_sitter_highlighter_;
    
    // 原有实现的数据成员
    std::map<std::string, std::vector<std::string>> keywords_;
    std::map<std::string, std::vector<std::string>> types_;
    bool in_multiline_comment_;
    bool in_multiline_string_;
    
    // 初始化语言定义（原有实现）
    void initializeLanguages();
    
    // 分词（原有实现）
    std::vector<Token> tokenize(const std::string& line);
    
    // 特定语言的分词器（原有实现）
    std::vector<Token> tokenizeCpp(const std::string& line);
    std::vector<Token> tokenizePython(const std::string& line);
    std::vector<Token> tokenizeJavaScript(const std::string& line);
    std::vector<Token> tokenizeJSON(const std::string& line);
    std::vector<Token> tokenizeMarkdown(const std::string& line);
    std::vector<Token> tokenizeShell(const std::string& line);
    
    // 辅助方法（原有实现）
    bool isKeyword(const std::string& word) const;
    bool isType(const std::string& word) const;
    bool isOperator(char ch) const;
    bool isMultiCharOperator(const std::string& text, size_t pos) const;
    bool isNumber(const std::string& text) const;
    
    // 字符串处理（原有实现）
    size_t parseString(const std::string& line, size_t start, char quote, TokenType& type);
    size_t parseRawString(const std::string& line, size_t start);
    size_t parseNumber(const std::string& line, size_t start);
    
    // 注释处理（原有实现）
    size_t parseComment(const std::string& line, size_t start, bool& is_multiline);
    
    // 使用原有实现高亮
    ftxui::Element highlightLineNative(const std::string& line);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_H

