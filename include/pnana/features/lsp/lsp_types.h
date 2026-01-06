#ifndef PNANA_FEATURES_LSP_LSP_TYPES_H
#define PNANA_FEATURES_LSP_LSP_TYPES_H

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

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_LSP_TYPES_H
