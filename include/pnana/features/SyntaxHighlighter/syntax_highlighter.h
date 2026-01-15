#ifndef PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_H
#define PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

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
    NATIVE,     // 使用原有的分词器实现
    TREE_SITTER // 使用 Tree-sitter（如果可用）
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
    explicit SyntaxHighlighter(
        ui::Theme& theme, SyntaxHighlightBackend backend = SyntaxHighlightBackend::TREE_SITTER);

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
    SyntaxHighlightBackend getBackend() const {
        return backend_;
    }

    // 检查 Tree-sitter 是否可用
    static bool isTreeSitterAvailable();

  private:
    ui::Theme& theme_;
    std::string current_file_type_;
    SyntaxHighlightBackend backend_;

#ifdef BUILD_TREE_SITTER_SUPPORT
    // Tree-sitter 后端（如果可用）
    std::unique_ptr<SyntaxHighlighterTreeSitter> tree_sitter_highlighter_;
#endif

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
    std::vector<Token> tokenizeLua(const std::string& line);
    std::vector<Token> tokenizeCMake(const std::string& line);
    std::vector<Token> tokenizeTCL(const std::string& line);
    std::vector<Token> tokenizeFortran(const std::string& line);
    std::vector<Token> tokenizeHaskell(const std::string& line);

    // 新增语言的分词器
    std::vector<Token> tokenizeYAML(const std::string& line);
    std::vector<Token> tokenizeXML(const std::string& line);
    std::vector<Token> tokenizeCSS(const std::string& line);
    std::vector<Token> tokenizeSQL(const std::string& line);
    std::vector<Token> tokenizeRuby(const std::string& line);
    std::vector<Token> tokenizePHP(const std::string& line);
    std::vector<Token> tokenizeSwift(const std::string& line);
    std::vector<Token> tokenizeKotlin(const std::string& line);
    std::vector<Token> tokenizeScala(const std::string& line);
    std::vector<Token> tokenizeR(const std::string& line);
    std::vector<Token> tokenizePerl(const std::string& line);
    std::vector<Token> tokenizeDockerfile(const std::string& line);
    std::vector<Token> tokenizeMakefile(const std::string& line);
    std::vector<Token> tokenizeVim(const std::string& line);
    std::vector<Token> tokenizePowerShell(const std::string& line);

    // 通用基础tokenize函数
    std::vector<Token> tokenizeGeneric(const std::string& line);

    // 新增语言的tokenize函数
    std::vector<Token> tokenizeElixir(const std::string& line);
    std::vector<Token> tokenizeClojure(const std::string& line);
    std::vector<Token> tokenizeErlang(const std::string& line);
    std::vector<Token> tokenizeJulia(const std::string& line);
    std::vector<Token> tokenizeDart(const std::string& line);
    std::vector<Token> tokenizeNim(const std::string& line);
    std::vector<Token> tokenizeMeson(const std::string& line);
    std::vector<Token> tokenizeTOML(const std::string& line);
    std::vector<Token> tokenizeHTML(const std::string& line);
    std::vector<Token> tokenizeCrystal(const std::string& line);
    std::vector<Token> tokenizeZig(const std::string& line);
    std::vector<Token> tokenizeOCaml(const std::string& line);
    std::vector<Token> tokenizeCoq(const std::string& line);
    std::vector<Token> tokenizeAgda(const std::string& line);
    std::vector<Token> tokenizeIdris(const std::string& line);
    std::vector<Token> tokenizePureScript(const std::string& line);
    std::vector<Token> tokenizeReason(const std::string& line);
    std::vector<Token> tokenizeSML(const std::string& line);
    std::vector<Token> tokenizeGroovy(const std::string& line);
    std::vector<Token> tokenizeCoffeeScript(const std::string& line);
    std::vector<Token> tokenizePug(const std::string& line);
    std::vector<Token> tokenizeStylus(const std::string& line);
    std::vector<Token> tokenizeSass(const std::string& line);
    std::vector<Token> tokenizeLess(const std::string& line);
    std::vector<Token> tokenizePostCSS(const std::string& line);
    std::vector<Token> tokenizeGraphQL(const std::string& line);
    std::vector<Token> tokenizeVue(const std::string& line);
    std::vector<Token> tokenizeSvelte(const std::string& line);
    std::vector<Token> tokenizeFSharp(const std::string& line);
    std::vector<Token> tokenizeCSharp(const std::string& line);
    std::vector<Token> tokenizeVB(const std::string& line);
    std::vector<Token> tokenizeAssembly(const std::string& line);
    std::vector<Token> tokenizeWebAssembly(const std::string& line);
    std::vector<Token> tokenizeVerilog(const std::string& line);
    std::vector<Token> tokenizeVHDL(const std::string& line);
    std::vector<Token> tokenizeMATLAB(const std::string& line);
    std::vector<Token> tokenizeOctave(const std::string& line);
    std::vector<Token> tokenizeRacket(const std::string& line);
    std::vector<Token> tokenizeScheme(const std::string& line);
    std::vector<Token> tokenizeCommonLisp(const std::string& line);
    std::vector<Token> tokenizeEmacsLisp(const std::string& line);
    std::vector<Token> tokenizeProlog(const std::string& line);
    std::vector<Token> tokenizeMercury(const std::string& line);
    std::vector<Token> tokenizeAlloy(const std::string& line);
    std::vector<Token> tokenizeDafny(const std::string& line);
    std::vector<Token> tokenizeLean(const std::string& line);
    std::vector<Token> tokenizeBallerina(const std::string& line);
    std::vector<Token> tokenizeCadence(const std::string& line);
    std::vector<Token> tokenizeClarity(const std::string& line);
    std::vector<Token> tokenizeSolidity(const std::string& line);
    std::vector<Token> tokenizeVyper(const std::string& line);
    std::vector<Token> tokenizeCarbon(const std::string& line);
    std::vector<Token> tokenizeVala(const std::string& line);
    std::vector<Token> tokenizeGenie(const std::string& line);
    std::vector<Token> tokenizeD(const std::string& line);
    std::vector<Token> tokenizePony(const std::string& line);
    std::vector<Token> tokenizeV(const std::string& line);
    std::vector<Token> tokenizeOdin(const std::string& line);
    std::vector<Token> tokenizeJai(const std::string& line);
    std::vector<Token> tokenizeNelua(const std::string& line);
    std::vector<Token> tokenizeWren(const std::string& line);
    std::vector<Token> tokenizeMoonScript(const std::string& line);
    std::vector<Token> tokenizeFantom(const std::string& line);
    std::vector<Token> tokenizeSmalltalk(const std::string& line);
    std::vector<Token> tokenizeAPL(const std::string& line);
    std::vector<Token> tokenizeJ(const std::string& line);
    std::vector<Token> tokenizeK(const std::string& line);
    std::vector<Token> tokenizeQ(const std::string& line);

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
