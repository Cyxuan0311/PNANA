#ifndef PNANA_FEATURES_LSP_LSP_TYPES_H
#define PNANA_FEATURES_LSP_LSP_TYPES_H

#include <string>
#include <utility>
#include <vector>

namespace pnana {
namespace features {

// LSP 位置结构
struct LspPosition {
    int line;
    int character;

    LspPosition(int l = 0, int c = 0) : line(l), character(c) {}
};

// LSP 范围结构
struct LspRange {
    LspPosition start;
    LspPosition end;

    LspRange() = default;
    LspRange(const LspPosition& s, const LspPosition& e) : start(s), end(e) {}
};

// LSP 诊断严重程度枚举
enum DiagnosticSeverity { ERROR = 1, WARNING = 2, INFORMATION = 3, HINT = 4 };

// 代码片段占位符
struct SnippetPlaceholder {
    int index;
    std::string default_value;
    std::pair<size_t, size_t> position; // 在代码片段中的位置

    SnippetPlaceholder(int idx = 0, const std::string& def_val = "")
        : index(idx), default_value(def_val), position({0, 0}) {}
};

// LSP 折叠范围类型
enum class FoldingRangeKind {
    Comment,
    Imports,
    Region,
    Unknown // For cases where kind is not specified or recognized
};

// LSP 折叠范围
struct FoldingRange {
    int startLine;
    int startCharacter;
    int endLine;
    int endCharacter;
    FoldingRangeKind kind; // "comment", "imports", "region"

    FoldingRange(int sl = 0, int sc = 0, int el = 0, int ec = 0,
                 FoldingRangeKind k = FoldingRangeKind::Unknown)
        : startLine(sl), startCharacter(sc), endLine(el), endCharacter(ec), kind(k) {}

    bool containsLine(int line) const {
        return line >= startLine && line <= endLine;
    }

    bool isValid() const {
        return startLine >= 0 && endLine >= startLine;
    }
};

// 代码片段
struct Snippet {
    std::string prefix;
    std::string body;
    std::string description;
    std::vector<SnippetPlaceholder> placeholders;

    Snippet(const std::string& p = "", const std::string& b = "", const std::string& d = "")
        : prefix(p), body(b), description(d) {}
};

// LSP 文档符号（用于符号导航）
struct DocumentSymbol {
    std::string name;
    std::string kind; // "Function", "Class", "Namespace", "Method", "Variable", etc.
    LspRange range;
    std::string detail;                   // 可选详细信息（如函数签名）
    std::vector<DocumentSymbol> children; // 嵌套符号（如类中的方法）
    int depth;                            // 嵌套深度（用于UI显示）

    DocumentSymbol() : depth(0) {}
    DocumentSymbol(const std::string& n, const std::string& k, const LspRange& r, int d = 0)
        : name(n), kind(k), range(r), depth(d) {}
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_LSP_TYPES_H
