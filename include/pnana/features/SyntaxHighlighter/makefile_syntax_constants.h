#ifndef MAKEFILE_SYNTAX_CONSTANTS_H_
#define MAKEFILE_SYNTAX_CONSTANTS_H_

#include <string>
#include <unordered_set>

// Makefile语法常量定义
// 基于tree-sitter-make的语法结构提取

namespace pnana {
namespace features {

// Makefile指令关键字
static const std::unordered_set<std::string> MAKEFILE_DIRECTIVE_KEYWORDS = {
    "define", "endef",  "include",  "sinclude", "-include", "vpath",
    "VPATH",  "export", "unexport", "override", "undefine", "private",
    "endif",  "else",   "ifeq",     "ifneq",    "ifdef",    "ifndef"};

// Makefile函数名
static const std::unordered_set<std::string> MAKEFILE_FUNCTIONS = {
    "subst", "patsubst", "strip",    "findstring", "filter",    "filter-out",
    "sort",  "word",     "words",    "wordlist",   "firstword", "lastword",
    "dir",   "notdir",   "suffix",   "basename",   "addsuffix", "addprefix",
    "join",  "wildcard", "realpath", "abspath",    "error",     "warning",
    "info",  "origin",   "flavor",   "foreach",    "if",        "or",
    "and",   "call",     "eval",     "file",       "value",     "shell"};

// Makefile自动变量
static const std::string MAKEFILE_AUTOMATIC_VARS = "$@%<^?+*|/.";

// Makefile运算符
static const std::string MAKEFILE_OPERATORS = ":&|;=+-!?";

// Makefile括号字符
static const std::string MAKEFILE_BRACKETS = "()[]{}";

// Makefile分隔符
static const std::string MAKEFILE_DELIMITERS = ",;";

// 检查是否是指令关键字
inline bool isMakefileDirectiveKeyword(const std::string& word) {
    return MAKEFILE_DIRECTIVE_KEYWORDS.count(word) > 0;
}

// 检查是否是函数名
inline bool isMakefileFunction(const std::string& word) {
    return MAKEFILE_FUNCTIONS.count(word) > 0;
}

// 检查是否是自动变量字符
inline bool isMakefileAutomaticVar(char c) {
    return MAKEFILE_AUTOMATIC_VARS.find(c) != std::string::npos;
}

// 检查是否是运算符
inline bool isMakefileOperator(char c) {
    return MAKEFILE_OPERATORS.find(c) != std::string::npos;
}

// 检查是否是括号
inline bool isMakefileBracket(char c) {
    return MAKEFILE_BRACKETS.find(c) != std::string::npos;
}

// 检查是否是分隔符
inline bool isMakefileDelimiter(char c) {
    return MAKEFILE_DELIMITERS.find(c) != std::string::npos;
}

} // namespace features
} // namespace pnana

#endif // MAKEFILE_SYNTAX_CONSTANTS_H_
