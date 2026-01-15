#include "features/SyntaxHighlighter/syntax_highlighter.h"
// 条件包含 Tree-sitter 头文件（如果启用）
#ifdef BUILD_TREE_SITTER_SUPPORT
#include "features/SyntaxHighlighter/syntax_highlighter_tree_sitter.h"
#endif
#include <algorithm>
#include <cctype>
#include <cstring>

using namespace ftxui;

namespace {
// UTF-8字符边界检测辅助函数
// 返回UTF-8字符的字节数（1-4）
inline size_t getUtf8CharLength(unsigned char first_byte) {
    if ((first_byte & 0x80) == 0)
        return 1; // ASCII字符
    if ((first_byte & 0xE0) == 0xC0)
        return 2; // 2字节字符
    if ((first_byte & 0xF0) == 0xE0)
        return 3; // 3字节字符
    if ((first_byte & 0xF8) == 0xF0)
        return 4; // 4字节字符
    return 1;     // 无效的UTF-8，按单字节处理
}

// 检查字符是否是ASCII字母或数字（用于标识符）
inline bool isAsciiAlnum(unsigned char c) {
    return std::isalnum(c) && c < 128;
}

// 检查字符是否是ASCII字母（用于标识符）
inline bool isAsciiAlpha(unsigned char c) {
    return std::isalpha(c) && c < 128;
}

// 安全地获取字符（处理UTF-8多字节字符）
[[maybe_unused]] inline size_t skipUtf8Char(const std::string& str, size_t pos) {
    if (pos >= str.length())
        return pos;
    unsigned char c = static_cast<unsigned char>(str[pos]);
    size_t len = getUtf8CharLength(c);
    return std::min(pos + len, str.length());
}
} // namespace

namespace pnana {
namespace features {

SyntaxHighlighter::SyntaxHighlighter(ui::Theme& theme, SyntaxHighlightBackend backend)
    : theme_(theme), current_file_type_("text"), backend_(backend), in_multiline_comment_(false),
      in_multiline_string_(false) {
    initializeLanguages();

    // 如果使用 Tree-sitter 后端且可用，初始化 Tree-sitter
#ifdef BUILD_TREE_SITTER_SUPPORT
    if (backend_ == SyntaxHighlightBackend::TREE_SITTER && isTreeSitterAvailable()) {
        try {
            tree_sitter_highlighter_ = std::make_unique<SyntaxHighlighterTreeSitter>(theme_);
        } catch (...) {
            // Tree-sitter 初始化失败，回退到原生实现
            backend_ = SyntaxHighlightBackend::NATIVE;
            tree_sitter_highlighter_.reset();
        }
    } else {
        backend_ = SyntaxHighlightBackend::NATIVE;
    }
#else
    // Tree-sitter 未编译，强制使用原生实现
    backend_ = SyntaxHighlightBackend::NATIVE;
#endif
}

SyntaxHighlighter::~SyntaxHighlighter() = default;

void SyntaxHighlighter::initializeLanguages() {
    // C/C++ 关键字
    keywords_["cpp"] = {"auto",        "break",        "case",
                        "char",        "const",        "continue",
                        "default",     "do",           "double",
                        "else",        "enum",         "extern",
                        "float",       "for",          "goto",
                        "if",          "inline",       "int",
                        "long",        "register",     "return",
                        "short",       "signed",       "sizeof",
                        "static",      "struct",       "switch",
                        "typedef",     "union",        "unsigned",
                        "void",        "volatile",     "while",
                        "class",       "namespace",    "template",
                        "typename",    "public",       "private",
                        "protected",   "virtual",      "override",
                        "final",       "using",        "try",
                        "catch",       "throw",        "new",
                        "delete",      "this",         "nullptr",
                        "true",        "false",        "const_cast",
                        "static_cast", "dynamic_cast", "reinterpret_cast"};

    types_["cpp"] = {"bool",     "int8_t",   "int16_t",  "int32_t",    "int64_t",    "uint8_t",
                     "uint16_t", "uint32_t", "uint64_t", "size_t",     "ssize_t",    "string",
                     "vector",   "map",      "set",      "shared_ptr", "unique_ptr", "weak_ptr"};

    // Python 关键字
    keywords_["python"] = {
        "and",  "as",     "assert", "async",  "await",  "break",   "class",    "continue", "def",
        "del",  "elif",   "else",   "except", "False",  "finally", "for",      "from",     "global",
        "if",   "import", "in",     "is",     "lambda", "None",    "nonlocal", "not",      "or",
        "pass", "raise",  "return", "True",   "try",    "while",   "with",     "yield",    "self"};

    types_["python"] = {"int",   "float", "str",   "bool",      "list",   "dict",
                        "tuple", "set",   "bytes", "bytearray", "object", "type"};

    // JavaScript 关键字
    keywords_["javascript"] = {
        "async",      "await",    "break",   "case",       "catch",     "class",    "const",
        "continue",   "debugger", "default", "delete",     "do",        "else",     "enum",
        "export",     "extends",  "false",   "finally",    "for",       "function", "if",
        "implements", "import",   "in",      "instanceof", "interface", "let",      "new",
        "null",       "package",  "private", "protected",  "public",    "return",   "static",
        "super",      "switch",   "this",    "throw",      "true",      "try",      "typeof",
        "var",        "void",     "while",   "with",       "yield"};

    types_["javascript"] = {"Array",  "Boolean", "Date",   "Error",  "Function",
                            "JSON",   "Math",    "Number", "Object", "Promise",
                            "RegExp", "String",  "Symbol", "Map",    "Set"};

    // Shell 关键字
    keywords_["shell"] = {"if",   "then",  "else",     "elif", "fi",     "case",     "esac",
                          "for",  "while", "do",       "done", "in",     "function", "return",
                          "exit", "break", "continue", "echo", "export", "source",   "cd",
                          "pwd",  "ls",    "cat",      "grep", "sed",    "awk"};

    // Lua 关键字
    keywords_["lua"] = {"and",      "break",  "do",   "else", "elseif", "end",  "false", "for",
                        "function", "goto",   "if",   "in",   "local",  "nil",  "not",   "or",
                        "repeat",   "return", "then", "true", "until",  "while"};

    types_["lua"] = {"number",   "string",   "boolean", "table",   "function",  "thread",
                     "userdata", "nil",      "type",    "pairs",   "ipairs",    "next",
                     "tostring", "tonumber", "print",   "require", "package",   "io",
                     "os",       "math",     "string",  "table",   "coroutine", "debug"};

    // CMake 关键字
    keywords_["cmake"] = {"if",
                          "else",
                          "elseif",
                          "endif",
                          "foreach",
                          "endforeach",
                          "while",
                          "endwhile",
                          "function",
                          "endfunction",
                          "macro",
                          "endmacro",
                          "break",
                          "continue",
                          "return",
                          "cmake_minimum_required",
                          "project",
                          "add_executable",
                          "add_library",
                          "target_link_libraries",
                          "include",
                          "include_directories",
                          "find_package",
                          "set",
                          "unset",
                          "list",
                          "string",
                          "math",
                          "file",
                          "message",
                          "option",
                          "add_subdirectory",
                          "install",
                          "enable_testing",
                          "add_test",
                          "set_property",
                          "get_property",
                          "target_compile_options",
                          "target_include_directories"};

    types_["cmake"] = {"CMAKE_C_COMPILER",         "CMAKE_CXX_COMPILER", "CMAKE_BUILD_TYPE",
                       "CMAKE_SOURCE_DIR",         "CMAKE_BINARY_DIR",   "CMAKE_CURRENT_SOURCE_DIR",
                       "CMAKE_CURRENT_BINARY_DIR", "CMAKE_VERSION",      "PROJECT_NAME",
                       "PROJECT_VERSION",          "BUILD_SHARED_LIBS"};

    // TCL 关键字
    keywords_["tcl"] = {
        "if",       "else",    "elseif",  "then",    "for",     "foreach", "while",     "break",
        "continue", "proc",    "return",  "global",  "upvar",   "uplevel", "namespace", "variable",
        "set",      "unset",   "append",  "lappend", "incr",    "expr",    "eval",      "subst",
        "catch",    "error",   "throw",   "try",     "finally", "switch",  "case",      "default",
        "package",  "require", "provide", "source",  "uplevel", "upvar",   "array",     "dict",
        "list",     "string",  "regexp",  "regsub",  "file",    "open",    "close",     "puts",
        "gets",     "read",    "seek",    "tell",    "flush",   "eof",     "fconfigure"};

    types_["tcl"] = {"tcl_version", "tcl_patchLevel", "tcl_library", "tcl_platform",
                     "env",         "auto_path",      "tcl_pkgPath", "errorCode",
                     "errorInfo",   "argv",           "argc"};

    // Fortran 关键字
    keywords_["fortran"] = {
        "program",  "end",       "subroutine",  "function",   "module",    "use",       "contains",
        "implicit", "none",      "integer",     "real",       "double",    "precision", "complex",
        "logical",  "character", "allocatable", "dimension",  "parameter", "data",      "if",
        "then",     "else",      "elseif",      "endif",      "select",    "case",      "default",
        "do",       "while",     "enddo",       "continue",   "cycle",     "exit",      "goto",
        "return",   "call",      "intent",      "in",         "out",       "inout",     "optional",
        "save",     "public",    "private",     "interface",  "abstract",  "type",      "endtype",
        "class",    "procedure", "allocate",    "deallocate", "nullify",   "associate", "block",
        "endblock"};

    types_["fortran"] = {"integer", "real",      "double", "precision", "complex",
                         "logical", "character", "kind",   "len",       "allocatable",
                         "pointer", "target",    "intent", "optional"};

    // Haskell 关键字
    keywords_["haskell"] = {
        "data",   "type",   "newtype",   "class",   "instance", "where",  "let",    "in",
        "if",     "then",   "else",      "case",    "of",       "do",     "where",  "deriving",
        "import", "module", "qualified", "as",      "hiding",   "infix",  "infixl", "infixr",
        "type",   "family", "instance",  "default", "foreign",  "export", "ccall",  "safe",
        "unsafe", "forall", "forall",    "mdo",     "rec",      "proc",   "->",     "=>",
        "::",     "=",      "<-",        "->",      "|",        "..",     "...",    "\\",
        "|",      "guard",  "when",      "unless",  "return",   "fail",   ">>=",    ">>",
        "=<<",    "<*>",    "<$>",       "<$"};

    types_["haskell"] = {"Int",         "Integer",  "Float",      "Double",      "Bool",
                         "Char",        "String",   "Maybe",      "Either",      "IO",
                         "[]",          "()",       "Ordering",   "Eq",          "Ord",
                         "Show",        "Read",     "Enum",       "Bounded",     "Num",
                         "Real",        "Integral", "Fractional", "Floating",    "Functor",
                         "Applicative", "Monad",    "Monoid",     "Semigroup",   "Foldable",
                         "Traversable", "MonadIO",  "MonadState", "MonadReader", "MonadWriter"};

    // 新增语言支持 - YAML
    keywords_["yaml"] = {"true", "false", "null", "yes", "no", "on", "off"};

    // XML/HTML
    keywords_["xml"] = {"DOCTYPE", "ELEMENT", "ATTLIST", "ENTITY",   "NOTATION",
                        "CDATA",   "xml",     "version", "encoding", "standalone"};

    // CSS
    keywords_["css"] = {
        "import",     "media",    "keyframes", "animation", "transition", "transform", "background",
        "color",      "font",     "margin",    "padding",   "border",     "width",     "height",
        "display",    "position", "float",     "clear",     "overflow",   "z-index",   "opacity",
        "visibility", "cursor",   "text",      "line",      "letter",     "word",      "vertical",
        "horizontal", "align",    "justify",   "flex",      "grid",       "box",       "shadow",
        "radius",     "gradient", "important"};

    // SQL
    keywords_["sql"] = {"SELECT",
                        "FROM",
                        "WHERE",
                        "INSERT",
                        "UPDATE",
                        "DELETE",
                        "CREATE",
                        "DROP",
                        "ALTER",
                        "TABLE",
                        "INDEX",
                        "VIEW",
                        "DATABASE",
                        "SCHEMA",
                        "COLUMN",
                        "CONSTRAINT",
                        "PRIMARY",
                        "FOREIGN",
                        "UNIQUE",
                        "NOT",
                        "NULL",
                        "DEFAULT",
                        "AUTO_INCREMENT",
                        "JOIN",
                        "INNER",
                        "LEFT",
                        "RIGHT",
                        "FULL",
                        "OUTER",
                        "ON",
                        "GROUP",
                        "BY",
                        "HAVING",
                        "ORDER",
                        "ASC",
                        "DESC",
                        "LIMIT",
                        "OFFSET",
                        "UNION",
                        "ALL",
                        "DISTINCT",
                        "AS",
                        "AND",
                        "OR",
                        "IN",
                        "EXISTS",
                        "BETWEEN",
                        "LIKE",
                        "IS",
                        "BEGIN",
                        "COMMIT",
                        "ROLLBACK",
                        "TRANSACTION",
                        "SAVEPOINT",
                        "CASE",
                        "WHEN",
                        "THEN",
                        "ELSE",
                        "END",
                        "IF",
                        "ELSEIF",
                        "WHILE",
                        "FOR",
                        "LOOP",
                        "REPEAT",
                        "UNTIL",
                        "DECLARE",
                        "SET",
                        "CALL",
                        "FUNCTION",
                        "PROCEDURE",
                        "TRIGGER",
                        "EVENT",
                        "GRANT",
                        "REVOKE",
                        "PRIVILEGES",
                        "USER",
                        "ROLE"};

    // Ruby
    keywords_["ruby"] = {"BEGIN", "END",    "alias",    "and",      "begin",       "break",
                         "case",  "class",  "def",      "defined?", "do",          "else",
                         "elsif", "end",    "ensure",   "false",    "for",         "if",
                         "in",    "module", "next",     "nil",      "not",         "or",
                         "redo",  "rescue", "retry",    "return",   "self",        "super",
                         "then",  "true",   "undef",    "unless",   "until",       "when",
                         "while", "yield",  "__FILE__", "__LINE__", "__ENCODING__"};

    types_["ruby"] = {"Array",         "Hash",         "String",        "Integer",   "Float",
                      "Symbol",        "Regexp",       "Range",         "Time",      "Date",
                      "DateTime",      "File",         "Dir",           "IO",        "Exception",
                      "StandardError", "RuntimeError", "ArgumentError", "TypeError", "NameError"};

    // PHP
    keywords_["php"] = {"__halt_compiler",
                        "abstract",
                        "and",
                        "array",
                        "as",
                        "break",
                        "callable",
                        "case",
                        "catch",
                        "class",
                        "clone",
                        "const",
                        "continue",
                        "declare",
                        "default",
                        "die",
                        "do",
                        "echo",
                        "else",
                        "elseif",
                        "empty",
                        "enddeclare",
                        "endfor",
                        "endforeach",
                        "endif",
                        "endswitch",
                        "endwhile",
                        "eval",
                        "exit",
                        "extends",
                        "final",
                        "finally",
                        "for",
                        "foreach",
                        "function",
                        "global",
                        "goto",
                        "if",
                        "implements",
                        "include",
                        "include_once",
                        "instanceof",
                        "insteadof",
                        "interface",
                        "isset",
                        "list",
                        "namespace",
                        "new",
                        "or",
                        "print",
                        "private",
                        "protected",
                        "public",
                        "require",
                        "require_once",
                        "return",
                        "static",
                        "switch",
                        "throw",
                        "trait",
                        "try",
                        "unset",
                        "use",
                        "var",
                        "while",
                        "xor",
                        "yield",
                        "yield from",
                        "__CLASS__",
                        "__DIR__",
                        "__FILE__",
                        "__FUNCTION__",
                        "__LINE__",
                        "__METHOD__",
                        "__NAMESPACE__",
                        "__TRAIT__",
                        "true",
                        "false",
                        "null"};

    types_["php"] = {
        "int",  "float",  "string", "bool",     "array",     "object", "callable",       "iterable",
        "void", "mixed",  "never",  "stdClass", "Exception", "Error",  "Throwable",      "DateTime",
        "PDO",  "mysqli", "curl",   "json",     "xml",       "zip",    "ReflectionClass"};

    // Swift
    keywords_["swift"] = {
        "associatedtype", "class",    "deinit",    "enum",      "extension", "fileprivate",
        "func",           "import",   "init",      "inout",     "internal",  "let",
        "open",           "operator", "private",   "protocol",  "public",    "rethrows",
        "static",         "struct",   "subscript", "typealias", "var",       "break",
        "case",           "continue", "default",   "defer",     "do",        "else",
        "fallthrough",    "for",      "guard",     "if",        "in",        "repeat",
        "return",         "switch",   "where",     "while",     "as",        "Any",
        "catch",          "false",    "is",        "nil",       "rethrows",  "super",
        "self",           "Self",     "throw",     "throws",    "true",      "try"};

    types_["swift"] = {"Int",      "Int8",   "Int16",     "Int32",    "Int64",      "UInt",
                       "UInt8",    "UInt16", "UInt32",    "UInt64",   "Float",      "Double",
                       "Bool",     "String", "Character", "Array",    "Dictionary", "Set",
                       "Optional", "Any",    "AnyObject", "AnyClass", "Void",       "Never",
                       "Error",    "Result", "URL",       "Data",     "Date",       "UUID"};

    // Kotlin
    keywords_["kotlin"] = {
        "as",       "as?",       "break",      "class",       "continue",  "do",
        "else",     "false",     "for",        "fun",         "if",        "in",
        "!in",      "interface", "is",         "!is",         "null",      "object",
        "package",  "return",    "super",      "this",        "throw",     "true",
        "try",      "typealias", "typeof",     "val",         "var",       "when",
        "while",    "by",        "catch",      "constructor", "delegate",  "dynamic",
        "field",    "file",      "finally",    "get",         "import",    "init",
        "param",    "property",  "receiver",   "set",         "setparam",  "where",
        "actual",   "abstract",  "annotation", "companion",   "const",     "crossinline",
        "data",     "enum",      "expect",     "external",    "final",     "infix",
        "inline",   "inner",     "internal",   "lateinit",    "noinline",  "open",
        "operator", "out",       "override",   "private",     "protected", "public",
        "reified",  "sealed",    "suspend",    "tailrec",     "vararg",    "it"};

    types_["kotlin"] = {"Any",
                        "Unit",
                        "Nothing",
                        "Int",
                        "Long",
                        "Short",
                        "Byte",
                        "Float",
                        "Double",
                        "Boolean",
                        "Char",
                        "String",
                        "Array",
                        "List",
                        "MutableList",
                        "Set",
                        "MutableSet",
                        "Map",
                        "MutableMap",
                        "Pair",
                        "Triple",
                        "Collection",
                        "MutableCollection",
                        "Iterable",
                        "MutableIterable",
                        "Sequence",
                        "Exception",
                        "RuntimeException",
                        "IllegalArgumentException",
                        "NullPointerException"};

    // Scala
    keywords_["scala"] = {
        "abstract", "case",      "catch",   "class",  "def",     "do",     "else",     "extends",
        "false",    "final",     "finally", "for",    "forSome", "if",     "implicit", "import",
        "lazy",     "macro",     "match",   "new",    "null",    "object", "override", "package",
        "private",  "protected", "return",  "sealed", "super",   "this",   "throw",    "trait",
        "try",      "true",      "type",    "val",    "var",     "while",  "with",     "yield",
        "_",        ":",         ":=",      "=",      "=>",      "<-",     "<:",       "<%",
        ">:",       "#",         "@"};

    types_["scala"] = {"Any",     "AnyRef",   "AnyVal",   "Nothing", "Null",   "Unit",
                       "Boolean", "Byte",     "Char",     "Short",   "Int",    "Long",
                       "Float",   "Double",   "String",   "List",    "Array",  "Seq",
                       "Set",     "Map",      "Option",   "Either",  "Try",    "Future",
                       "Promise", "Iterable", "Iterator", "Stream",  "Vector", "ArrayBuffer"};

    // R
    keywords_["r"] = {
        "if",     "else",        "repeat",   "while",       "function",      "for",     "in",
        "next",   "break",       "TRUE",     "FALSE",       "NULL",          "Inf",     "NaN",
        "NA",     "NA_integer_", "NA_real_", "NA_complex_", "NA_character_", "...",     "..1",
        "..2",    "..3",         "..4",      "..5",         "..6",           "..7",     "..8",
        "..9",    "library",     "require",  "source",      "attach",        "detach",  "ls",
        "rm",     "save",        "load",     "data",        "print",         "cat",     "paste",
        "substr", "nchar",       "grep",     "gsub",        "strsplit",      "toupper", "tolower"};

    types_["r"] = {"numeric",    "integer", "double",  "complex",    "character", "logical",
                   "factor",     "matrix",  "array",   "data.frame", "list",      "vector",
                   "table",      "Date",    "POSIXct", "POSIXlt",    "difftime",  "environment",
                   "expression", "name",    "call",    "formula",    "lm",        "glm",
                   "anova",      "summary", "plot"};

    // Perl
    keywords_["perl"] = {
        "if",       "unless", "while", "until",   "for",       "foreach",   "when",     "given",
        "default",  "break",  "next",  "last",    "redo",      "continue",  "return",   "sub",
        "my",       "our",    "local", "state",   "use",       "no",        "package",  "require",
        "BEGIN",    "END",    "INIT",  "CHECK",   "UNITCHECK", "DESTROY",   "AUTOLOAD", "import",
        "unimport", "isa",    "can",   "DOES",    "VERSION",   "new",       "bless",    "ref",
        "tie",      "tied",   "untie", "tied",    "caller",    "wantarray", "eval",     "do",
        "require",  "use",    "no",    "package", "import",    "unimport",  "strict",   "warnings"};

    types_["perl"] = {"SCALAR", "ARRAY",      "HASH",      "CODE",   "REF",
                      "GLOB",   "LVALUE",     "FORMAT",    "IO",     "VSTRING",
                      "REGEXP", "FileHandle", "DirHandle", "Socket", "Pipe"};

    // Dockerfile
    keywords_["dockerfile"] = {
        "FROM",        "MAINTAINER", "RUN",         "CMD",     "LABEL", "EXPOSE",  "ENV",
        "ADD",         "COPY",       "ENTRYPOINT",  "VOLUME",  "USER",  "WORKDIR", "ARG",
        "ONBUILD",     "STOPSIGNAL", "HEALTHCHECK", "SHELL",   "AS",    "FROM",    "MAINTAINER",
        "RUN",         "CMD",        "LABEL",       "EXPOSE",  "ENV",   "ADD",     "COPY",
        "ENTRYPOINT",  "VOLUME",     "USER",        "WORKDIR", "ARG",   "ONBUILD", "STOPSIGNAL",
        "HEALTHCHECK", "SHELL"};

    // Makefile
    keywords_["makefile"] = {"include",
                             "define",
                             "endef",
                             "ifdef",
                             "ifndef",
                             "ifeq",
                             "ifneq",
                             "else",
                             "endif",
                             "override",
                             "export",
                             "unexport",
                             "private",
                             "vpath",
                             "GPATH",
                             "VPATH",
                             "CC",
                             "CXX",
                             "CFLAGS",
                             "CXXFLAGS",
                             "CPPFLAGS",
                             "LDFLAGS",
                             "LDLIBS",
                             "AR",
                             "AS",
                             "LD",
                             "NM",
                             "OBJCOPY",
                             "OBJDUMP",
                             "RANLIB",
                             "STRIP",
                             "SIZE",
                             "STRINGS",
                             "READELF",
                             "OBJDUMP",
                             "ADDR2LINE",
                             "all",
                             "clean",
                             "install",
                             "uninstall",
                             "dist",
                             "distclean",
                             "check",
                             "test",
                             "help",
                             ".PHONY",
                             ".SUFFIXES",
                             ".DEFAULT",
                             ".PRECIOUS",
                             ".INTERMEDIATE",
                             ".SECONDARY",
                             ".SECONDEXPANSION",
                             ".DELETE_ON_ERROR",
                             ".IGNORE",
                             ".LOW_RESOLUTION_TIME",
                             ".SILENT",
                             ".EXPORT_ALL_VARIABLES",
                             ".NOTPARALLEL",
                             ".ONESHELL",
                             ".POSIX"};

    // Vimscript
    keywords_["vim"] = {
        "if",       "else",         "elseif",    "endif",       "for",        "in",
        "endfor",   "while",        "endwhile",  "try",         "catch",      "finally",
        "endtry",   "throw",        "function",  "endfunction", "return",     "break",
        "continue", "let",          "unlet",     "const",       "call",       "execute",
        "eval",     "echo",         "echon",     "echomsg",     "echoerr",    "echohl",
        "normal",   "silent",       "verbose",   "command",     "delcommand", "autocmd",
        "augroup",  "au",           "doautocmd", "noautocmd",   "syntax",     "highlight",
        "hi",       "match",        "matchadd",  "matchdelete", "setlocal",   "setglobal",
        "set",      "setlocal",     "setglobal", "map",         "nmap",       "vmap",
        "imap",     "cmap",         "omap",      "noremap",     "nnoremap",   "vnoremap",
        "inoremap", "cnoremap",     "onoremap",  "unmap",       "nunmap",     "vunmap",
        "iunmap",   "cunmap",       "ounmap",    "abbreviate",  "ab",         "iabbrev",
        "cabbrev",  "unabbreviate", "unab",      "iunabbrev",   "cunabbrev"};

    types_["vim"] = {"g:",          "b:",          "w:",          "t:",         "s:",
                     "l:",          "a:",          "v:",          "exists",     "has",
                     "executable",  "fnamemodify", "fnamemodify", "expand",     "glob",
                     "globpath",    "findfile",    "finddir",     "readfile",   "writefile",
                     "tempname",    "getcwd",      "getpid",      "hostname",   "strftime",
                     "localtime",   "reltime",     "reltimestr",  "input",      "inputlist",
                     "inputsecret", "confirm",     "getchar",     "getcharmod", "feedkeys"};

    // PowerShell
    keywords_["powershell"] = {"begin",
                               "break",
                               "catch",
                               "class",
                               "continue",
                               "data",
                               "define",
                               "do",
                               "dynamicparam",
                               "else",
                               "elseif",
                               "end",
                               "enum",
                               "exit",
                               "filter",
                               "finally",
                               "for",
                               "foreach",
                               "from",
                               "function",
                               "if",
                               "in",
                               "inlinescript",
                               "parallel",
                               "param",
                               "process",
                               "return",
                               "sequence",
                               "switch",
                               "throw",
                               "trap",
                               "try",
                               "until",
                               "using",
                               "var",
                               "while",
                               "workflow",
                               "configuration",
                               "dscresource",
                               "write-host",
                               "write-output",
                               "write-error",
                               "write-warning",
                               "write-verbose",
                               "write-debug",
                               "write-information",
                               "write-progress",
                               "read-host",
                               "get-content",
                               "set-content",
                               "add-content",
                               "clear-content",
                               "out-file",
                               "out-string",
                               "out-default",
                               "format-table",
                               "format-list",
                               "format-wide",
                               "sort-object",
                               "where-object",
                               "foreach-object",
                               "group-object",
                               "measure-object",
                               "select-object",
                               "export-csv",
                               "import-csv",
                               "convertto-json",
                               "convertfrom-json",
                               "new-object",
                               "get-member",
                               "get-command",
                               "get-help",
                               "get-alias",
                               "set-alias",
                               "new-alias",
                               "export-alias",
                               "import-alias",
                               "get-variable",
                               "set-variable",
                               "new-variable",
                               "clear-variable",
                               "remove-variable",
                               "get-item",
                               "set-item",
                               "new-item",
                               "remove-item",
                               "copy-item",
                               "move-item",
                               "rename-item",
                               "get-childitem",
                               "test-path",
                               "convert-path",
                               "join-path",
                               "split-path",
                               "resolve-path"};

    types_["powershell"] = {"Object",
                            "String",
                            "Char",
                            "Byte",
                            "Int32",
                            "Int64",
                            "UInt32",
                            "UInt64",
                            "Single",
                            "Double",
                            "Decimal",
                            "Boolean",
                            "DateTime",
                            "TimeSpan",
                            "Guid",
                            "Array",
                            "ArrayList",
                            "Hashtable",
                            "OrderedDictionary",
                            "Queue",
                            "Stack",
                            "List",
                            "Dictionary",
                            "XmlDocument",
                            "XmlElement",
                            "PSObject",
                            "PSCustomObject",
                            "ScriptBlock",
                            "Regex",
                            "Match",
                            "Group",
                            "Capture",
                            "FileInfo",
                            "DirectoryInfo",
                            "PathInfo",
                            "FileSystemInfo",
                            "Process",
                            "Service",
                            "EventLog",
                            "RegistryKey"};

    // Meson 构建系统
    keywords_["meson"] = {"project",
                          "meson",
                          "subdir",
                          "subdir_done",
                          "executable",
                          "library",
                          "shared_library",
                          "static_library",
                          "both_libraries",
                          "dependency",
                          "find_program",
                          "find_library",
                          "find_path",
                          "pkgconfig",
                          "cmake",
                          "python3",
                          "python",
                          "compiler",
                          "subproject",
                          "get_option",
                          "option",
                          "files",
                          "include_directories",
                          "declare_dependency",
                          "summary",
                          "warning",
                          "error",
                          "message",
                          "assert",
                          "is_disabler",
                          "disabler",
                          "if",
                          "else",
                          "elif",
                          "endif",
                          "foreach",
                          "endforeach",
                          "break",
                          "continue",
                          "return",
                          "and",
                          "or",
                          "not",
                          "true",
                          "false",
                          "import",
                          "run_command",
                          "run_target",
                          "custom_target",
                          "generator",
                          "configure_file",
                          "install_data",
                          "install_headers",
                          "install_man",
                          "install_subdir",
                          "subdir",
                          "join_paths",
                          "environment",
                          "jar",
                          "build_machine",
                          "host_machine",
                          "target_machine"};

    types_["meson"] = {"str", "int", "bool", "list", "dict", "exe", "lib", "dep", "inc", "tgt"};

    // TOML 配置格式
    keywords_["toml"] = {"true", "false", "null", "nan", "inf", "-inf"};

    // HTML
    keywords_["html"] = {
        "html",    "head",     "body",      "div",     "span",     "p",           "a",
        "img",     "br",       "hr",        "table",   "tr",       "td",          "th",
        "thead",   "tbody",    "tfoot",     "ul",      "ol",       "li",          "dl",
        "dt",      "dd",       "h1",        "h2",      "h3",       "h4",          "h5",
        "h6",      "form",     "input",     "button",  "select",   "option",      "textarea",
        "label",   "fieldset", "legend",    "script",  "style",    "link",        "meta",
        "title",   "DOCTYPE",  "xml",       "version", "encoding", "standalone",  "xmlns",
        "class",   "id",       "name",      "type",    "value",    "href",        "src",
        "alt",     "title",    "width",     "height",  "border",   "cellpadding", "cellspacing",
        "colspan", "rowspan",  "align",     "valign",  "bgcolor",  "color",       "face",
        "size",    "action",   "method",    "enctype", "target",   "rel",         "media",
        "charset", "content",  "http-equiv"};

    // Nim 编程语言
    keywords_["nim"] = {
        "addr",     "and",       "as",      "asm",   "atomic",   "bind",      "block",  "break",
        "case",     "cast",      "concept", "const", "continue", "converter", "defer",  "discard",
        "distinct", "div",       "do",      "elif",  "else",     "end",       "enum",   "except",
        "export",   "finally",   "for",     "from",  "func",     "if",        "import", "in",
        "include",  "interface", "is",      "isnot", "iterator", "let",       "macro",  "method",
        "mixin",    "mod",       "nil",     "not",   "notin",    "object",    "of",     "or",
        "out",      "proc",      "ptr",     "raise", "ref",      "return",    "shl",    "shr",
        "static",   "template",  "try",     "tuple", "type",     "using",     "var",    "when",
        "while",    "with",      "without", "xor",   "yield"};

    types_["nim"] = {"int",       "int8",    "int16",    "int32",    "int64",   "uint",    "uint8",
                     "uint16",    "uint32",  "uint64",   "float",    "float32", "float64", "bool",
                     "char",      "string",  "cstring",  "pointer",  "byte",    "seq",     "array",
                     "openarray", "varargs", "set",      "enum",     "object",  "tuple",   "ref",
                     "ptr",       "proc",    "iterator", "distinct", "void"};
}

void SyntaxHighlighter::setFileType(const std::string& file_type) {
    if (current_file_type_ != file_type) {
        current_file_type_ = file_type;

        // 如果使用 Tree-sitter，更新其文件类型
        // 即使 Tree-sitter 不支持该文件类型，也调用 setFileType 以确保状态正确重置
#ifdef BUILD_TREE_SITTER_SUPPORT
        if (backend_ == SyntaxHighlightBackend::TREE_SITTER && tree_sitter_highlighter_) {
            tree_sitter_highlighter_->setFileType(file_type);
            // 如果 Tree-sitter 不支持该文件类型，setFileType 会将 current_language_ 设置为 nullptr
            // 在 highlightLine 中会检查 supportsFileType，如果不支持则使用本地语法高亮
        }
#endif

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

    // 根据后端选择不同的实现
#ifdef BUILD_TREE_SITTER_SUPPORT
    if (backend_ == SyntaxHighlightBackend::TREE_SITTER && tree_sitter_highlighter_) {
        // 检查 Tree-sitter 是否支持当前文件类型
        if (tree_sitter_highlighter_->supportsFileType(current_file_type_)) {
            try {
                return tree_sitter_highlighter_->highlightLine(line);
            } catch (...) {
                // Tree-sitter 处理失败，回退到原生实现
                return highlightLineNative(line);
            }
        } else {
            // Tree-sitter 不支持该文件类型，直接使用本地语法高亮
            return highlightLineNative(line);
        }
    }
#endif

    // 使用原生实现
    return highlightLineNative(line);
}

ftxui::Element SyntaxHighlighter::highlightLineNative(const std::string& line) {
    if (line.empty()) {
        return text("");
    }

    // 对于超长行，限制处理长度以提高性能（支持长行但不卡顿）
    const size_t MAX_LINE_LENGTH = 10000; // 最大处理长度
    std::string processed_line = line;
    bool is_truncated = false;
    if (line.length() > MAX_LINE_LENGTH) {
        processed_line = line.substr(0, MAX_LINE_LENGTH);
        is_truncated = true;
    }

    // 根据文件类型分词
    std::vector<Token> tokens = tokenize(processed_line);

    if (tokens.empty()) {
        Element result = text(processed_line) | color(theme_.getColors().foreground);
        if (is_truncated) {
            // 如果被截断，添加省略号
            return hbox({result, text("...") | color(theme_.getColors().comment)});
        }
        return result;
    }

    // 渲染每个token
    Elements elements;
    for (const auto& token : tokens) {
        Color token_color = getColorForToken(token.type);
        elements.push_back(text(token.text) | color(token_color));
    }

    // 如果被截断，添加省略号
    if (is_truncated) {
        elements.push_back(text("...") | color(theme_.getColors().comment));
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
    } else if (current_file_type_ == "lua") {
        return tokenizeLua(line);
    } else if (current_file_type_ == "cmake") {
        return tokenizeCMake(line);
    } else if (current_file_type_ == "tcl") {
        return tokenizeTCL(line);
    } else if (current_file_type_ == "fortran") {
        return tokenizeFortran(line);
    } else if (current_file_type_ == "haskell") {
        return tokenizeHaskell(line);
    } else if (current_file_type_ == "yaml" || current_file_type_ == "yml") {
        return tokenizeYAML(line);
    } else if (current_file_type_ == "xml") {
        return tokenizeXML(line);
    } else if (current_file_type_ == "html" || current_file_type_ == "htm") {
        return tokenizeHTML(line);
    } else if (current_file_type_ == "meson") {
        return tokenizeMeson(line);
    } else if (current_file_type_ == "toml") {
        return tokenizeTOML(line);
    } else if (current_file_type_ == "css" || current_file_type_ == "scss" ||
               current_file_type_ == "sass") {
        return tokenizeCSS(line);
    } else if (current_file_type_ == "sql") {
        return tokenizeSQL(line);
    } else if (current_file_type_ == "ruby" || current_file_type_ == "rb") {
        return tokenizeRuby(line);
    } else if (current_file_type_ == "php") {
        return tokenizePHP(line);
    } else if (current_file_type_ == "swift") {
        return tokenizeSwift(line);
    } else if (current_file_type_ == "kotlin" || current_file_type_ == "kt") {
        return tokenizeKotlin(line);
    } else if (current_file_type_ == "scala") {
        return tokenizeScala(line);
    } else if (current_file_type_ == "r" || current_file_type_ == "R") {
        return tokenizeR(line);
    } else if (current_file_type_ == "perl" || current_file_type_ == "pl" ||
               current_file_type_ == "pm") {
        return tokenizePerl(line);
    } else if (current_file_type_ == "dockerfile") {
        return tokenizeDockerfile(line);
    } else if (current_file_type_ == "makefile") {
        return tokenizeMakefile(line);
    } else if (current_file_type_ == "vim" || current_file_type_ == "vimrc") {
        return tokenizeVim(line);
    } else if (current_file_type_ == "powershell" || current_file_type_ == "ps1") {
        return tokenizePowerShell(line);
    } else if (current_file_type_ == "elixir") {
        return tokenizeElixir(line);
    } else if (current_file_type_ == "clojure") {
        return tokenizeClojure(line);
    } else if (current_file_type_ == "erlang") {
        return tokenizeErlang(line);
    } else if (current_file_type_ == "julia") {
        return tokenizeJulia(line);
    } else if (current_file_type_ == "dart") {
        return tokenizeDart(line);
    } else if (current_file_type_ == "nim") {
        return tokenizeNim(line);
    } else if (current_file_type_ == "crystal") {
        return tokenizeCrystal(line);
    } else if (current_file_type_ == "zig") {
        return tokenizeZig(line);
    } else if (current_file_type_ == "ocaml") {
        return tokenizeOCaml(line);
    } else if (current_file_type_ == "coq") {
        return tokenizeCoq(line);
    } else if (current_file_type_ == "agda") {
        return tokenizeAgda(line);
    } else if (current_file_type_ == "idris") {
        return tokenizeIdris(line);
    } else if (current_file_type_ == "purescript") {
        return tokenizePureScript(line);
    } else if (current_file_type_ == "reason") {
        return tokenizeReason(line);
    } else if (current_file_type_ == "sml") {
        return tokenizeSML(line);
    } else if (current_file_type_ == "groovy") {
        return tokenizeGroovy(line);
    } else if (current_file_type_ == "coffeescript") {
        return tokenizeCoffeeScript(line);
    } else if (current_file_type_ == "pug") {
        return tokenizePug(line);
    } else if (current_file_type_ == "stylus") {
        return tokenizeStylus(line);
    } else if (current_file_type_ == "sass") {
        return tokenizeSass(line);
    } else if (current_file_type_ == "less") {
        return tokenizeLess(line);
    } else if (current_file_type_ == "postcss") {
        return tokenizePostCSS(line);
    } else if (current_file_type_ == "graphql") {
        return tokenizeGraphQL(line);
    } else if (current_file_type_ == "vue") {
        return tokenizeVue(line);
    } else if (current_file_type_ == "svelte") {
        return tokenizeSvelte(line);
    } else if (current_file_type_ == "fsharp") {
        return tokenizeFSharp(line);
    } else if (current_file_type_ == "csharp") {
        return tokenizeCSharp(line);
    } else if (current_file_type_ == "vb") {
        return tokenizeVB(line);
    } else if (current_file_type_ == "assembly") {
        return tokenizeAssembly(line);
    } else if (current_file_type_ == "webassembly") {
        return tokenizeWebAssembly(line);
    } else if (current_file_type_ == "verilog") {
        return tokenizeVerilog(line);
    } else if (current_file_type_ == "vhdl") {
        return tokenizeVHDL(line);
    } else if (current_file_type_ == "matlab") {
        return tokenizeMATLAB(line);
    } else if (current_file_type_ == "octave") {
        return tokenizeOctave(line);
    } else if (current_file_type_ == "racket") {
        return tokenizeRacket(line);
    } else if (current_file_type_ == "scheme") {
        return tokenizeScheme(line);
    } else if (current_file_type_ == "commonlisp") {
        return tokenizeCommonLisp(line);
    } else if (current_file_type_ == "emacslisp") {
        return tokenizeEmacsLisp(line);
    } else if (current_file_type_ == "prolog") {
        return tokenizeProlog(line);
    } else if (current_file_type_ == "mercury") {
        return tokenizeMercury(line);
    } else if (current_file_type_ == "alloy") {
        return tokenizeAlloy(line);
    } else if (current_file_type_ == "dafny") {
        return tokenizeDafny(line);
    } else if (current_file_type_ == "lean") {
        return tokenizeLean(line);
    } else if (current_file_type_ == "ballerina") {
        return tokenizeBallerina(line);
    } else if (current_file_type_ == "cadence") {
        return tokenizeCadence(line);
    } else if (current_file_type_ == "clarity") {
        return tokenizeClarity(line);
    } else if (current_file_type_ == "solidity") {
        return tokenizeSolidity(line);
    } else if (current_file_type_ == "vyper") {
        return tokenizeVyper(line);
    } else if (current_file_type_ == "carbon") {
        return tokenizeCarbon(line);
    } else if (current_file_type_ == "vala") {
        return tokenizeVala(line);
    } else if (current_file_type_ == "genie") {
        return tokenizeGenie(line);
    } else if (current_file_type_ == "dlang") {
        return tokenizeD(line);
    } else if (current_file_type_ == "pony") {
        return tokenizePony(line);
    } else if (current_file_type_ == "vlang") {
        return tokenizeV(line);
    } else if (current_file_type_ == "odin") {
        return tokenizeOdin(line);
    } else if (current_file_type_ == "jai") {
        return tokenizeJai(line);
    } else if (current_file_type_ == "nelua") {
        return tokenizeNelua(line);
    } else if (current_file_type_ == "wren") {
        return tokenizeWren(line);
    } else if (current_file_type_ == "moonscript") {
        return tokenizeMoonScript(line);
    } else if (current_file_type_ == "fantom") {
        return tokenizeFantom(line);
    } else if (current_file_type_ == "smalltalk") {
        return tokenizeSmalltalk(line);
    } else if (current_file_type_ == "apl") {
        return tokenizeAPL(line);
    } else if (current_file_type_ == "jlang") {
        return tokenizeJ(line);
    } else if (current_file_type_ == "klang") {
        return tokenizeK(line);
    } else if (current_file_type_ == "qlang") {
        return tokenizeQ(line);
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
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
            while (i < line.length() && std::isspace(line[i]))
                i++;
            // 读取指令（只匹配ASCII字符）
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    // UTF-8多字节字符，停止
                    break;
                } else {
                    break;
                }
            }
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
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
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

        // 标识符/关键字（支持ASCII字符，正确处理UTF-8多字节字符如中文）
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_') {
            size_t start = i;
            // 只匹配ASCII字母数字和下划线，跳过UTF-8多字节字符（如中文）
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    // UTF-8多字节字符（如中文），作为普通文本处理，停止标识符匹配
                    break;
                } else {
                    // 其他ASCII字符，停止
                    break;
                }
            }
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
                    if (line[check_pos] == '<')
                        template_depth++;
                    else if (line[check_pos] == '>')
                        template_depth--;
                    else if (line[check_pos] == '(' && template_depth == 0) {
                        type = TokenType::FUNCTION;
                        break;
                    } else if (std::isspace(line[check_pos])) {
                        check_pos++;
                        continue;
                    } else if (template_depth == 0 && !std::isalnum(line[check_pos]) &&
                               line[check_pos] != '_' && line[check_pos] != ':' &&
                               line[check_pos] != '&' && line[check_pos] != '*' &&
                               line[check_pos] != '[' && line[check_pos] != ']') {
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
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (i + 2 < line.length())
                    i += 3;
            } else {
                while (i < line.length() && line[i] != quote) {
                    i++; // 原始字符串不处理转义
                }
                if (i < line.length())
                    i++;
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
                            if (line[i] == '{')
                                brace_count++;
                            else if (line[i] == '}')
                                brace_count--;
                            i++;
                        }
                    } else {
                        i++;
                    }
                }
                if (i + 2 < line.length())
                    i += 3;
            } else {
                while (i < line.length() && line[i] != quote) {
                    if (line[i] == '\\' && i + 1 < line.length()) {
                        i += 2;
                    } else if (line[i] == '{') {
                        // f-string 表达式
                        i++;
                        int brace_count = 1;
                        while (i < line.length() && brace_count > 0) {
                            if (line[i] == '{')
                                brace_count++;
                            else if (line[i] == '}')
                                brace_count--;
                            i++;
                        }
                    } else {
                        i++;
                    }
                }
                if (i < line.length())
                    i++;
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
                    if (line[i] == '\\' && i + 1 < line.length())
                        i += 2;
                    else
                        i++;
                }
                if (i + 2 < line.length())
                    i += 3;
            } else {
                while (i < line.length() && line[i] != quote) {
                    if (line[i] == '\\' && i + 1 < line.length())
                        i += 2;
                    else
                        i++;
                }
                if (i < line.length())
                    i++;
            }

            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字（支持复数、科学计数法）
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;

            // 科学计数法
            if (i < line.length() && (line[i] == 'e' || line[i] == 'E')) {
                i++;
                if (i < line.length() && (line[i] == '+' || line[i] == '-'))
                    i++;
                while (i < line.length() && std::isdigit(line[i]))
                    i++;
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

        // 标识符/关键字（支持ASCII字符，正确处理UTF-8多字节字符如中文）
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_') {
            size_t start = i;
            // 只匹配ASCII字母数字和下划线，跳过UTF-8多字节字符（如中文）
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    // UTF-8多字节字符（如中文），作为普通文本处理，停止标识符匹配
                    break;
                } else {
                    // 其他ASCII字符，停止
                    break;
                }
            }
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
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                        if (line[i] == '{')
                            brace_count++;
                        else if (line[i] == '}')
                            brace_count--;
                        i++;
                    }
                } else {
                    i++;
                }
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
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

        // 标识符/关键字（支持ASCII字符，正确处理UTF-8多字节字符如中文）
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_' || line[i] == '$') {
            size_t start = i;
            // 只匹配ASCII字母数字、下划线和$，跳过UTF-8多字节字符（如中文）
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_' || ch == '$') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    // UTF-8多字节字符（如中文），作为普通文本处理，停止标识符匹配
                    break;
                } else {
                    // 其他ASCII字符，停止
                    break;
                }
            }
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
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 字符串（键或值）
        if (line[i] == '"') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) || line[i] == '-') {
            size_t start = i;
            if (line[i] == '-')
                i++;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 布尔值和null
        if (line.substr(i, 4) == "true" || line.substr(i, 5) == "false" ||
            line.substr(i, 4) == "null") {
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

    if (line.empty())
        return tokens;

    size_t i = 0;

    // 跳过前导空白
    while (i < line.length() && std::isspace(line[i]))
        i++;

    // 标题 (# ## ### 等)
    if (i < line.length() && line[i] == '#') {
        size_t start = i;
        while (i < line.length() && line[i] == '#')
            i++;
        tokens.push_back({line.substr(start, i - start), TokenType::KEYWORD, start, i});

        // 标题文本
        while (i < line.length() && std::isspace(line[i]))
            i++;
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
        size_t old_i = i; // 记录当前位置，防止无限循环

        if (line[i] == '`') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '`')
                i++;
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 粗体 **text** 或 __text__
        if (i + 1 < line.length() &&
            ((line[i] == '*' && line[i + 1] == '*') || (line[i] == '_' && line[i + 1] == '_'))) {
            size_t start = i;
            char marker = line[i];
            i += 2;
            while (i + 1 < line.length() && !(line[i] == marker && line[i + 1] == marker))
                i++;
            if (i + 1 < line.length()) {
                tokens.push_back({line.substr(start, 2), TokenType::KEYWORD, start, start + 2});
                if (i > start + 2) {
                    tokens.push_back(
                        {line.substr(start + 2, i - start - 2), TokenType::FUNCTION, start + 2, i});
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
            while (i < line.length() && line[i] != marker && !std::isspace(line[i]))
                i++;
            if (i < line.length() && line[i] == marker) {
                tokens.push_back({std::string(1, marker), TokenType::KEYWORD, start, start + 1});
                if (i > start + 1) {
                    tokens.push_back(
                        {line.substr(start + 1, i - start - 1), TokenType::TYPE, start + 1, i});
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
            while (i < line.length() && line[i] != ']')
                i++;
            if (i < line.length() && i + 1 < line.length() && line[i + 1] == '(') {
                tokens.push_back({line.substr(start, 1), TokenType::KEYWORD, start, start + 1});
                if (i > start + 1) {
                    tokens.push_back(
                        {line.substr(start + 1, i - start - 1), TokenType::FUNCTION, start + 1, i});
                }
                tokens.push_back({line.substr(i, 1), TokenType::KEYWORD, i, i + 1});
                i++;
                // 跳过 URL
                while (i < line.length() && line[i] != ')')
                    i++;
                if (i < line.length()) {
                    tokens.push_back({line.substr(i, 1), TokenType::STRING, i, i + 1});
                    i++;
                }
                continue;
            } else {
                // 没有匹配的]，作为普通字符处理
                tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
                i++;
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
            tokens.push_back(
                {line.substr(text_start, i - text_start), TokenType::NORMAL, text_start, i});
        }

        // 防止无限循环：如果i没有增加，强制增加1
        if (i == old_i) {
            tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
            i++;
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
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 变量
        if (line[i] == '$') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == '{') {
                while (i < line.length() && line[i] != '}')
                    i++;
                if (i < line.length())
                    i++;
            } else {
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                    i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        // 命令/关键字（支持ASCII字符，正确处理UTF-8多字节字符如中文）
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c)) {
            size_t start = i;
            // 只匹配ASCII字母数字、下划线和-，跳过UTF-8多字节字符（如中文）
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_' || ch == '-') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    // UTF-8多字节字符（如中文），作为普通文本处理，停止标识符匹配
                    break;
                } else {
                    // 其他ASCII字符，停止
                    break;
                }
            }
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

std::vector<Token> SyntaxHighlighter::tokenizeLua(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 单行注释
        if (line[i] == '-' && i + 1 < line.length() && line[i + 1] == '-') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        // 多行注释 --[[ ... ]]
        if (i + 3 < line.length() && line.substr(i, 4) == "--[[") {
            size_t start = i;
            i += 4;
            size_t end_pos = line.find("]]", i);
            if (end_pos != std::string::npos) {
                i = end_pos + 2;
                tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            } else {
                tokens.push_back({line.substr(start), TokenType::COMMENT, start, line.length()});
                break;
            }
            continue;
        }

        // 字符串（支持单引号和双引号）
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

        // 长字符串 [[ ... ]]
        if (i + 1 < line.length() && line[i] == '[' && line[i + 1] == '[') {
            size_t start = i;
            i += 2;
            size_t end_pos = line.find("]]", i);
            if (end_pos != std::string::npos) {
                i = end_pos + 2;
                tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            } else {
                tokens.push_back({line.substr(start), TokenType::STRING, start, line.length()});
                break;
            }
            continue;
        }

        // 数字（支持十六进制、科学计数法）
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            i = parseNumber(line, start);
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 多字符操作符
        if (i + 1 < line.length()) {
            std::string two_char = line.substr(i, 2);
            if (two_char == "==" || two_char == "~=" || two_char == "<=" || two_char == ">=" ||
                two_char == ".." || two_char == "//" || two_char == "::") {
                tokens.push_back({two_char, TokenType::OPERATOR, i, i + 2});
                i += 2;
                continue;
            }
        }

        // 标识符/关键字（支持ASCII字符，正确处理UTF-8多字节字符如中文）
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_') {
            size_t start = i;
            // 只匹配ASCII字母数字和下划线，跳过UTF-8多字节字符（如中文）
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    // UTF-8多字节字符（如中文），作为普通文本处理，停止标识符匹配
                    break;
                } else {
                    // 其他ASCII字符，停止
                    break;
                }
            }
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
            return colors.keyword; // 使用关键字颜色
        case TokenType::NORMAL:
        default:
            return colors.foreground;
    }
}

bool SyntaxHighlighter::isKeyword(const std::string& word) const {
    auto it = keywords_.find(current_file_type_);
    if (it == keywords_.end())
        return false;

    return std::find(it->second.begin(), it->second.end(), word) != it->second.end();
}

bool SyntaxHighlighter::isType(const std::string& word) const {
    auto it = types_.find(current_file_type_);
    if (it == types_.end())
        return false;

    return std::find(it->second.begin(), it->second.end(), word) != it->second.end();
}

bool SyntaxHighlighter::isOperator(char ch) const {
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || ch == '=' ||
           ch == '<' || ch == '>' || ch == '!' || ch == '&' || ch == '|' || ch == '^' ||
           ch == '~' || ch == '?' || ch == ':' || ch == '(' || ch == ')' || ch == '[' ||
           ch == ']' || ch == '{' || ch == '}' || ch == ',' || ch == ';' || ch == '.' || ch == '-';
}

bool SyntaxHighlighter::isMultiCharOperator(const std::string& text, size_t pos) const {
    if (pos + 1 >= text.length())
        return false;

    char ch1 = text[pos];
    char ch2 = text[pos + 1];

    // 双字符操作符
    if ((ch1 == '=' && ch2 == '=') || (ch1 == '!' && ch2 == '=') || (ch1 == '<' && ch2 == '=') ||
        (ch1 == '>' && ch2 == '=') || (ch1 == '&' && ch2 == '&') || (ch1 == '|' && ch2 == '|') ||
        (ch1 == '<' && ch2 == '<') || (ch1 == '>' && ch2 == '>') || (ch1 == '+' && ch2 == '+') ||
        (ch1 == '-' && ch2 == '-') || (ch1 == '+' && ch2 == '=') || (ch1 == '-' && ch2 == '=') ||
        (ch1 == '*' && ch2 == '=') || (ch1 == '/' && ch2 == '=') || (ch1 == '%' && ch2 == '=') ||
        (ch1 == '&' && ch2 == '=') || (ch1 == '|' && ch2 == '=') || (ch1 == '^' && ch2 == '=') ||
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
        while (i < line.length() && (std::isxdigit(line[i]) || line[i] == '\''))
            i++;
        // 后缀
        if (i < line.length() &&
            (line[i] == 'u' || line[i] == 'U' || line[i] == 'l' || line[i] == 'L')) {
            i++;
            if (i < line.length() && (line[i] == 'l' || line[i] == 'L'))
                i++;
        }
        return i;
    }

    // 八进制或二进制
    if (i < line.length() && line[i] == '0') {
        i++;
        if (i < line.length() && (line[i] == 'b' || line[i] == 'B')) {
            // 二进制
            i++;
            while (i < line.length() && (line[i] == '0' || line[i] == '1' || line[i] == '\''))
                i++;
        } else {
            // 八进制
            while (i < line.length() && ((line[i] >= '0' && line[i] <= '7') || line[i] == '\''))
                i++;
        }
        // 后缀
        if (i < line.length() &&
            (line[i] == 'u' || line[i] == 'U' || line[i] == 'l' || line[i] == 'L')) {
            i++;
            if (i < line.length() && (line[i] == 'l' || line[i] == 'L'))
                i++;
        }
        return i;
    }

    // 十进制或浮点数
    while (i < line.length() && (std::isdigit(line[i]) || line[i] == '\''))
        i++;

    // 浮点数
    if (i < line.length() && line[i] == '.') {
        i++;
        while (i < line.length() && (std::isdigit(line[i]) || line[i] == '\''))
            i++;
    }

    // 科学计数法
    if (i < line.length() && (line[i] == 'e' || line[i] == 'E')) {
        i++;
        if (i < line.length() && (line[i] == '+' || line[i] == '-'))
            i++;
        while (i < line.length() && std::isdigit(line[i]))
            i++;
    }

    // 后缀
    if (i < line.length() &&
        (line[i] == 'f' || line[i] == 'F' || line[i] == 'l' || line[i] == 'L')) {
        i++;
    }

    return i;
}

size_t SyntaxHighlighter::parseString(const std::string& line, size_t start, char quote,
                                      TokenType& type) {
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
    if (start + 1 >= line.length())
        return start;

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
    if (text.empty())
        return false;

    for (char ch : text) {
        if (!std::isdigit(ch) && ch != '.' && ch != 'x' && ch != 'X' && ch != 'f' && ch != 'F' &&
            ch != 'l' && ch != 'L') {
            return false;
        }
    }
    return true;
}

void SyntaxHighlighter::setBackend(SyntaxHighlightBackend backend) {
    if (backend_ == backend) {
        return;
    }

    backend_ = backend;

#ifdef BUILD_TREE_SITTER_SUPPORT
    if (backend_ == SyntaxHighlightBackend::TREE_SITTER && isTreeSitterAvailable()) {
        try {
            tree_sitter_highlighter_ = std::make_unique<SyntaxHighlighterTreeSitter>(theme_);
            if (!current_file_type_.empty()) {
                tree_sitter_highlighter_->setFileType(current_file_type_);
            }
        } catch (...) {
            // Tree-sitter 初始化失败，回退到原生实现
            backend_ = SyntaxHighlightBackend::NATIVE;
            tree_sitter_highlighter_.reset();
        }
    } else {
        tree_sitter_highlighter_.reset();
        backend_ = SyntaxHighlightBackend::NATIVE;
    }
#else
    // Tree-sitter 未编译，强制使用原生实现
    if (backend_ == SyntaxHighlightBackend::TREE_SITTER) {
        backend_ = SyntaxHighlightBackend::NATIVE;
    }
#endif
}

bool SyntaxHighlighter::isTreeSitterAvailable() {
#ifdef BUILD_TREE_SITTER_SUPPORT
    return true;
#else
    return false;
#endif
}

std::vector<Token> SyntaxHighlighter::tokenizeCMake(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        if (line[i] == '#') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        if (line[i] == '$') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == '{') {
                i++;
                while (i < line.length() && line[i] != '}')
                    i++;
                if (i < line.length())
                    i++;
            } else if (i + 4 < line.length() && line.substr(i, 4) == "ENV{") {
                i += 4;
                while (i < line.length() && line[i] != '}')
                    i++;
                if (i < line.length())
                    i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            i = parseNumber(line, start);
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    break;
                } else {
                    break;
                }
            }
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

        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeTCL(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        if (line[i] == '#') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        if (line[i] == '{') {
            size_t start = i;
            i++;
            int brace_count = 1;
            while (i < line.length() && brace_count > 0) {
                if (line[i] == '{')
                    brace_count++;
                else if (line[i] == '}')
                    brace_count--;
                i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        if (line[i] == '"') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        if (line[i] == '$') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == '{') {
                i++;
                while (i < line.length() && line[i] != '}')
                    i++;
                if (i < line.length())
                    i++;
            } else {
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                    i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            i = parseNumber(line, start);
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_' || line[i] == ':') {
            size_t start = i;
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_' || ch == ':') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    break;
                } else {
                    break;
                }
            }
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

        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeFortran(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    if (line.length() > 0 &&
        (line[0] == 'C' || line[0] == 'c' || line[0] == '*' || line[0] == '!')) {
        tokens.push_back({line, TokenType::COMMENT, 0, line.length()});
        return tokens;
    }

    while (i < line.length()) {
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        if (line[i] == '!') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == quote && i + 1 < line.length() && line[i + 1] == quote) {
                    i += 2;
                } else {
                    i++;
                }
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            if (i < line.length() &&
                (line[i] == 'e' || line[i] == 'E' || line[i] == 'd' || line[i] == 'D')) {
                i++;
                if (i < line.length() && (line[i] == '+' || line[i] == '-'))
                    i++;
                while (i < line.length() && std::isdigit(line[i]))
                    i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    break;
                } else {
                    break;
                }
            }
            std::string word = line.substr(start, i - start);
            std::string word_lower = word;
            std::transform(word_lower.begin(), word_lower.end(), word_lower.begin(), ::tolower);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word_lower)) {
                type = TokenType::KEYWORD;
            } else if (isType(word_lower)) {
                type = TokenType::TYPE;
            } else if (i < line.length() && line[i] == '(') {
                type = TokenType::FUNCTION;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeHaskell(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        if (i + 1 < line.length() && line[i] == '-' && line[i + 1] == '-') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        if (i + 1 < line.length() && line[i] == '{' && line[i + 1] == '-') {
            size_t start = i;
            i += 2;
            while (i + 1 < line.length() && !(line[i] == '-' && line[i + 1] == '}')) {
                i++;
            }
            if (i + 1 < line.length())
                i += 2;
            tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            continue;
        }

        if (line[i] == '"') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        if (line[i] == '\'') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == '\\' && i + 1 < line.length()) {
                i += 2;
            } else if (i < line.length()) {
                i++;
            }
            if (i < line.length() && line[i] == '\'')
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            i = parseNumber(line, start);
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        if (i + 1 < line.length()) {
            std::string two_char = line.substr(i, 2);
            if (two_char == "->" || two_char == "=>" || two_char == "::" || two_char == "++" ||
                two_char == "==" || two_char == "/=" || two_char == "<=" || two_char == ">=" ||
                two_char == "&&" || two_char == "||" || two_char == ">>" || two_char == ">>=" ||
                two_char == "=<<" || two_char == "<$>" || two_char == "<*>") {
                tokens.push_back({two_char, TokenType::OPERATOR, i, i + 2});
                i += 2;
                continue;
            }
        }

        unsigned char c = static_cast<unsigned char>(line[i]);
        if (isAsciiAlpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length()) {
                unsigned char ch = static_cast<unsigned char>(line[i]);
                if (isAsciiAlnum(ch) || ch == '_' || ch == '\'') {
                    i++;
                } else if ((ch & 0x80) != 0) {
                    break;
                } else {
                    break;
                }
            }
            std::string word = line.substr(start, i - start);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            } else if (isType(word)) {
                type = TokenType::TYPE;
            } else if (i < line.length() && line[i] == '(') {
                type = TokenType::FUNCTION;
            } else if (!word.empty() && std::isupper(static_cast<unsigned char>(word[0]))) {
                type = TokenType::TYPE;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeYAML(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 布尔值和null
        if (line.substr(i, 4) == "true" || line.substr(i, 5) == "false" ||
            line.substr(i, 4) == "null" || line.substr(i, 3) == "yes" ||
            line.substr(i, 2) == "no" || line.substr(i, 2) == "on" || line.substr(i, 3) == "off") {
            size_t len = 0;
            if (line.substr(i, 5) == "false")
                len = 5;
            else if (line.substr(i, 4) == "true" || line.substr(i, 4) == "null")
                len = 4;
            else if (line.substr(i, 3) == "yes" || line.substr(i, 3) == "off")
                len = 3;
            else if (line.substr(i, 2) == "no" || line.substr(i, 2) == "on")
                len = 2;
            tokens.push_back({line.substr(i, len), TokenType::KEYWORD, i, i + len});
            i += len;
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '-' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            if (line[i] == '-')
                i++;
            while (i < line.length() &&
                   (std::isdigit(line[i]) || line[i] == '.' || line[i] == 'e' || line[i] == 'E' ||
                    line[i] == '+' || line[i] == '-'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeXML(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // XML注释
        if (i + 3 < line.length() && line.substr(i, 4) == "<!--") {
            size_t start = i;
            i += 4;
            size_t end_pos = line.find("-->", i);
            if (end_pos != std::string::npos) {
                i = end_pos + 3;
                tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            } else {
                tokens.push_back({line.substr(start), TokenType::COMMENT, start, line.length()});
                break;
            }
            continue;
        }

        // XML标签开始
        if (line[i] == '<') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == '/') {
                // 闭合标签
                i++;
            }

            // 标签名
            while (i < line.length() &&
                   (std::isalnum(line[i]) || line[i] == '_' || line[i] == '-' || line[i] == ':'))
                i++;

            // 属性
            while (i < line.length() && line[i] != '>' && line[i] != '/') {
                if (line[i] == '=') {
                    // 属性值
                    i++;
                    while (i < line.length() && std::isspace(line[i]))
                        i++;
                    if (i < line.length() && (line[i] == '"' || line[i] == '\'')) {
                        char quote = line[i];
                        i++;
                        while (i < line.length() && line[i] != quote) {
                            if (line[i] == '&' && i + 1 < line.length() && line[i + 1] == ' ') {
                                i++; // 转义字符
                            }
                            i++;
                        }
                        if (i < line.length())
                            i++;
                    } else {
                        while (i < line.length() && !std::isspace(line[i]) && line[i] != '>' &&
                               line[i] != '/')
                            i++;
                    }
                } else {
                    i++;
                }
            }

            if (i < line.length() && line[i] == '/')
                i++;
            if (i < line.length() && line[i] == '>')
                i++;

            tokens.push_back({line.substr(start, i - start), TokenType::KEYWORD, start, i});
            continue;
        }

        // 实体引用
        if (line[i] == '&') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != ';')
                i++;
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        // 字符串内容
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '&' && i + 1 < line.length() && line[i + 1] == ' ') {
                    i++; // 转义字符
                }
                i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeCSS(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 注释
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

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1])) ||
            (line[i] == '-' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            if (line[i] == '-')
                i++;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            // 单位
            while (i < line.length() && (std::isalpha(line[i]) || line[i] == '%'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // CSS选择器和属性名
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_' || line[i] == '-' || line[i] == '#') {
            size_t start = i;
            while (i < line.length() &&
                   (std::isalnum(line[i]) || line[i] == '_' || line[i] == '-' || line[i] == '#' ||
                    line[i] == '.' || line[i] == ':'))
                i++;
            std::string word = line.substr(start, i - start);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeSQL(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 注释
        if (i + 1 < line.length() && line[i] == '-' && line[i + 1] == '-') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }
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

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_' || line[i] == '@') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '$'))
                i++;
            std::string word = line.substr(start, i - start);
            std::string upper_word = word;
            std::transform(upper_word.begin(), upper_word.end(), upper_word.begin(), ::toupper);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(upper_word)) {
                type = TokenType::KEYWORD;
            } else if (isType(upper_word)) {
                type = TokenType::TYPE;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeRuby(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 符号
        if (line[i] == ':') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == ':') {
                i++;
                tokens.push_back({"::", TokenType::OPERATOR, start, i});
                continue;
            } else if (i < line.length() &&
                       (std::isalpha(line[i]) || line[i] == '_' || line[i] == '@')) {
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' ||
                                             line[i] == '@' || line[i] == '?' || line[i] == '!'))
                    i++;
                tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
                continue;
            }
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() &&
                   (std::isdigit(line[i]) || line[i] == '.' || line[i] == 'e' || line[i] == 'E' ||
                    line[i] == '+' || line[i] == '-'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_' || line[i] == '@' || line[i] == '$') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' ||
                                         line[i] == '@' || line[i] == '$' || line[i] == '?'))
                i++;
            std::string word = line.substr(start, i - start);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            } else if (isType(word)) {
                type = TokenType::TYPE;
            } else if (i < line.length() && line[i] == '(') {
                type = TokenType::FUNCTION;
            } else if (word[0] == '@' || word[0] == '$') {
                type = TokenType::TYPE;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizePHP(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    // 检查是否在PHP代码块中（简化处理）
    bool in_php =
        (line.find("<?php") != std::string::npos) || (line.find("<?=") != std::string::npos);

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // PHP开始标签
        if (i + 4 < line.length() && line.substr(i, 5) == "<?php") {
            tokens.push_back({line.substr(i, 5), TokenType::KEYWORD, i, i + 5});
            i += 5;
            in_php = true;
            continue;
        }
        if (i + 1 < line.length() && line.substr(i, 2) == "<?") {
            tokens.push_back({line.substr(i, 2), TokenType::KEYWORD, i, i + 2});
            i += 2;
            in_php = true;
            continue;
        }

        // PHP结束标签
        if (i + 1 < line.length() && line.substr(i, 2) == "?>") {
            tokens.push_back({line.substr(i, 2), TokenType::KEYWORD, i, i + 2});
            i += 2;
            in_php = false;
            continue;
        }

        // 如果不在PHP代码中，按HTML处理
        if (!in_php) {
            return tokenizeXML(line);
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

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 变量
        if (line[i] == '$') {
            size_t start = i;
            i++;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
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

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeSwift(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
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

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeKotlin(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
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

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeScala(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
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

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeR(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() &&
                   (std::isdigit(line[i]) || line[i] == '.' || line[i] == 'e' || line[i] == 'E' ||
                    line[i] == '+' || line[i] == '-'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_' || line[i] == '.') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '.'))
                i++;
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

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizePerl(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 变量
        if (line[i] == '$' || line[i] == '@' || line[i] == '%') {
            size_t start = i;
            i++;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/关键字
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
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

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeDockerfile(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 指令
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c)) {
            size_t start = i;
            while (i < line.length() && std::isalpha(line[i]))
                i++;
            std::string word = line.substr(start, i - start);
            std::string upper_word = word;
            std::transform(upper_word.begin(), upper_word.end(), upper_word.begin(), ::toupper);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(upper_word)) {
                type = TokenType::KEYWORD;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeMakefile(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 注释
        if (line[i] == '#') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        // 变量引用
        if (line[i] == '$') {
            size_t start = i;
            i++;
            if (i < line.length() && line[i] == '(') {
                i++;
                while (i < line.length() && line[i] != ')')
                    i++;
                if (i < line.length())
                    i++;
            } else if (i < line.length() && line[i] == '{') {
                i++;
                while (i < line.length() && line[i] != '}')
                    i++;
                if (i < line.length())
                    i++;
            } else {
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                    i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        // 规则目标
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_' || line[i] == '/' || line[i] == '.') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' ||
                                         line[i] == '/' || line[i] == '.' || line[i] == '-'))
                i++;
            std::string word = line.substr(start, i - start);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            } else if (isType(word)) {
                type = TokenType::TYPE;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeVim(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 注释
        if (line[i] == '"') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        // 字符串
        if (line[i] == '\'') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != '\'') {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 变量
        if (line[i] == '&' || line[i] == 'g' || line[i] == 'b' || line[i] == 'w' ||
            line[i] == 't' || line[i] == 's' || line[i] == 'l' || line[i] == 'a' ||
            line[i] == 'v') {
            if (i + 1 < line.length() && line[i + 1] == ':') {
                size_t start = i;
                i += 2;
                while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                    i++;
                tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
                continue;
            }
        }

        // 数字
        if (std::isdigit(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isdigit(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/命令
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
            std::string word = line.substr(start, i - start);
            TokenType type = TokenType::NORMAL;
            if (isKeyword(word)) {
                type = TokenType::KEYWORD;
            } else if (isType(word)) {
                type = TokenType::TYPE;
            }
            tokens.push_back({word, type, start, i});
            continue;
        }

        // 其他
        tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizePowerShell(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 单行注释
        if (line[i] == '#') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        // 多行注释
        if (i + 1 < line.length() && line[i] == '<' && line[i + 1] == '#') {
            size_t start = i;
            i += 2;
            size_t end_pos = line.find("#>", i);
            if (end_pos != std::string::npos) {
                i = end_pos + 2;
                tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            } else {
                tokens.push_back({line.substr(start), TokenType::COMMENT, start, line.length()});
                break;
            }
            continue;
        }

        // 字符串
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '`' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 变量
        if (line[i] == '$') {
            size_t start = i;
            i++;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符/命令
        unsigned char c = static_cast<unsigned char>(line[i]);
        if (std::isalpha(c) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '-'))
                i++;
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

        // 其他
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

// 通用的基础tokenize函数，为新添加的语言提供基本的语法高亮
std::vector<Token> SyntaxHighlighter::tokenizeGeneric(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白字符
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // 注释处理（通用规则）
        if (line.substr(i, 2) == "//" || line.substr(i, 2) == "#!") {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }
        if (line.substr(i, 2) == "/*") {
            size_t start = i;
            i += 2;
            while (i < line.length() - 1 && line.substr(i, 2) != "*/")
                i++;
            if (line.substr(i, 2) == "*/")
                i += 2;
            tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            continue;
        }
        if (line[i] == '#' || line[i] == ';') {
            tokens.push_back({line.substr(i), TokenType::COMMENT, i, line.length()});
            break;
        }

        // 字符串处理
        if (line[i] == '"' || line[i] == '\'' || line[i] == '`') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i++; // 转义字符
                i++;
            }
            if (i < line.length())
                i++; // 包含结束引号
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '.' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            bool hasDot = (line[i] == '.');
            i++;
            while (i < line.length() && (std::isdigit(line[i]) || (!hasDot && line[i] == '.'))) {
                if (line[i] == '.')
                    hasDot = true;
                i++;
            }
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符和关键字
        if (std::isalpha(line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
            std::string word = line.substr(start, i - start);
            TokenType type = isKeyword(word) ? TokenType::KEYWORD : TokenType::NORMAL;
            tokens.push_back({word, type, start, i});
            continue;
        }

        // 运算符
        if (isOperator(line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

// 新增语言的tokenize函数实现（使用通用实现）
std::vector<Token> SyntaxHighlighter::tokenizeElixir(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeClojure(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeErlang(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeJulia(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeDart(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeNim(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeCrystal(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeZig(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeOCaml(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeCoq(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeAgda(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeIdris(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizePureScript(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeReason(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeSML(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeGroovy(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeCoffeeScript(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizePug(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeStylus(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeSass(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeLess(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizePostCSS(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeGraphQL(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeVue(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeSvelte(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeFSharp(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeCSharp(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeVB(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeAssembly(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeWebAssembly(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeVerilog(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeVHDL(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeMATLAB(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeOctave(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeRacket(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeScheme(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeCommonLisp(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeEmacsLisp(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeProlog(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeMercury(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeAlloy(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeDafny(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeLean(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeBallerina(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeCadence(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeClarity(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeSolidity(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeVyper(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeCarbon(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeVala(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeGenie(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeD(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizePony(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeV(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeOdin(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeJai(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeNelua(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeWren(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeMoonScript(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeFantom(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeSmalltalk(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeAPL(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeJ(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeK(const std::string& line) {
    return tokenizeGeneric(line);
}
std::vector<Token> SyntaxHighlighter::tokenizeQ(const std::string& line) {
    return tokenizeGeneric(line);
}

std::vector<Token> SyntaxHighlighter::tokenizeMeson(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 多行字符串开始
        if (line.substr(i, 3) == "'''") {
            size_t start = i;
            i += 3;
            while (i < line.length() && line.substr(i, 3) != "'''") {
                i++;
            }
            if (i < line.length())
                i += 3;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '-' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            if (line[i] == '-')
                i++;
            while (i < line.length() && (std::isdigit(line[i]) || line[i] == '.'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 标识符和关键字
        if (std::isalpha(line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
            std::string word = line.substr(start, i - start);

            // 检查是否是关键字
            if (std::find(keywords_["meson"].begin(), keywords_["meson"].end(), word) !=
                keywords_["meson"].end()) {
                tokens.push_back({word, TokenType::KEYWORD, start, i});
            }
            // 检查是否是类型
            else if (std::find(types_["meson"].begin(), types_["meson"].end(), word) !=
                     types_["meson"].end()) {
                tokens.push_back({word, TokenType::TYPE, start, i});
            }
            // 检查是否是布尔值
            else if (word == "true" || word == "false") {
                tokens.push_back({word, TokenType::KEYWORD, start, i});
            } else {
                tokens.push_back({word, TokenType::NORMAL, start, i});
            }
            continue;
        }

        // 操作符和标点符号
        if (strchr("()[]{},.:;=+-*/%!&|^~<>?", line[i])) {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        // 其他字符
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeTOML(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
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
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 多行字符串开始
        if (line.substr(i, 3) == "\"\"\"") {
            size_t start = i;
            i += 3;
            while (i < line.length() && line.substr(i, 3) != "\"\"\"") {
                i++;
            }
            if (i < line.length())
                i += 3;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 数字
        if (std::isdigit(line[i]) ||
            (line[i] == '-' && i + 1 < line.length() && std::isdigit(line[i + 1])) ||
            (line[i] == '+' && i + 1 < line.length() && std::isdigit(line[i + 1]))) {
            size_t start = i;
            if (line[i] == '-' || line[i] == '+')
                i++;
            while (i < line.length() &&
                   (std::isdigit(line[i]) || line[i] == '.' || line[i] == 'e' || line[i] == 'E' ||
                    line[i] == '-' || line[i] == '+'))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NUMBER, start, i});
            continue;
        }

        // 数组和内联表
        if (line[i] == '[' || line[i] == ']' || line[i] == '{') {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        // 等号
        if (line[i] == '=') {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        // 标识符和关键字
        if (std::isalpha(line[i]) || line[i] == '_') {
            size_t start = i;
            while (i < line.length() && (std::isalnum(line[i]) || line[i] == '_' || line[i] == '-'))
                i++;
            std::string word = line.substr(start, i - start);

            // 检查是否是关键字
            if (std::find(keywords_["toml"].begin(), keywords_["toml"].end(), word) !=
                keywords_["toml"].end()) {
                tokens.push_back({word, TokenType::KEYWORD, start, i});
            } else {
                tokens.push_back({word, TokenType::NORMAL, start, i});
            }
            continue;
        }

        // 点号（用于表路径）
        if (line[i] == '.') {
            tokens.push_back({std::string(1, line[i]), TokenType::OPERATOR, i, i + 1});
            i++;
            continue;
        }

        // 其他字符
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

std::vector<Token> SyntaxHighlighter::tokenizeHTML(const std::string& line) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < line.length()) {
        // 跳过空白
        if (std::isspace(line[i])) {
            size_t start = i;
            while (i < line.length() && std::isspace(line[i]))
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::NORMAL, start, i});
            continue;
        }

        // HTML 注释
        if (line.substr(i, 4) == "<!--") {
            size_t start = i;
            i += 4;
            while (i < line.length() && line.substr(i, 3) != "-->") {
                i++;
            }
            if (i < line.length())
                i += 3;
            tokens.push_back({line.substr(start, i - start), TokenType::COMMENT, start, i});
            continue;
        }

        // HTML 标签开始
        if (line[i] == '<') {
            if (i + 1 < line.length()) {
                if (line[i + 1] == '/') { // 结束标签
                    size_t start = i;
                    i += 2; // 跳过 </
                    while (i < line.length() && line[i] != '>' && !std::isspace(line[i]))
                        i++;
                    if (i < line.length() && line[i] == '>') {
                        i++;
                        tokens.push_back(
                            {line.substr(start, i - start), TokenType::KEYWORD, start, i});
                        continue;
                    }
                } else if (std::isalpha(line[i + 1])) { // 开始标签
                    size_t start = i;
                    i++; // 跳过 <
                    while (i < line.length() && line[i] != '>' && !std::isspace(line[i]))
                        i++;
                    if (i < line.length()) {
                        std::string tag_content = line.substr(start + 1, i - start - 1);
                        if (std::find(keywords_["html"].begin(), keywords_["html"].end(),
                                      tag_content) != keywords_["html"].end()) {
                            tokens.push_back({line.substr(start, i - start + 1), TokenType::KEYWORD,
                                              start, i + 1});
                        } else {
                            tokens.push_back(
                                {line.substr(start, i - start + 1), TokenType::TYPE, start, i + 1});
                        }
                        if (line[i] == '>')
                            i++;
                        continue;
                    }
                }
            }
        }

        // 结束标签 >
        if (line[i] == '>') {
            tokens.push_back({std::string(1, line[i]), TokenType::KEYWORD, i, i + 1});
            i++;
            continue;
        }

        // 属性值（引号中的字符串）
        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < line.length())
                    i += 2;
                else
                    i++;
            }
            if (i < line.length())
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::STRING, start, i});
            continue;
        }

        // 实体引用
        if (line[i] == '&') {
            size_t start = i;
            i++;
            while (i < line.length() && line[i] != ';' && std::isalnum(line[i]))
                i++;
            if (i < line.length() && line[i] == ';')
                i++;
            tokens.push_back({line.substr(start, i - start), TokenType::TYPE, start, i});
            continue;
        }

        // 其他字符
        tokens.push_back({std::string(1, line[i]), TokenType::NORMAL, i, i + 1});
        i++;
    }

    return tokens;
}

} // namespace features
} // namespace pnana
