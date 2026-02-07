#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getGoSnippets() {
    return {
        // Functions
        {"func", "func ${1:functionName}(${2:parameters}) ${3:returnType} {\n    ${4:// code}\n}",
         "Function definition"},
        {"funcm",
         "func (${1:receiver} *${2:Type}) ${3:methodName}(${4:params}) ${5:returnType} {\n    "
         "${6:// code}\n}",
         "Method definition"},
        {"main", "func main() {\n    ${1:// code}\n}", "Main function"},
        {"init", "func init() {\n    ${1:// code}\n}", "Init function"},

        // Types and Structs
        {"type", "type ${1:TypeName} ${2:underlying}", "Type definition"},
        {"struct", "type ${1:StructName} struct {\n    ${2:Field} ${3:Type}\n}",
         "Struct definition"},
        {"interface", "type ${1:InterfaceName} interface {\n    ${2:Method}()\n}",
         "Interface definition"},

        // Variables and Constants
        {"var", "var ${1:name} ${2:type} = ${3:value}", "Variable declaration"},
        {"const", "const ${1:name} = ${2:value}", "Constant declaration"},
        {"short", "${1:name} := ${2:value}", "Short variable declaration"},

        // Conditionals
        {"if", "if ${1:condition} {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if ${1:condition} {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"iferr", "if ${1:err} != nil {\n    ${2:return ${1:err}}\n}", "Error check"},
        {"switch",
         "switch ${1:value} {\ncase ${2:case}:\n    ${3:// code}\ndefault:\n    ${4:// code}\n}",
         "Switch statement"},
        {"select", "select {\ncase ${1:case}:\n    ${2:// code}\ndefault:\n    ${3:// code}\n}",
         "Select statement"},

        // Loops
        {"for", "for ${1:i} := ${2:0}; ${1:i} < ${3:n}; ${1:i}++ {\n    ${4:// code}\n}",
         "For loop"},
        {"forr", "for ${1:key}, ${2:value} := range ${3:collection} {\n    ${4:// code}\n}",
         "Range loop"},
        {"fori", "for ${1:condition} {\n    ${2:// code}\n}", "For loop (while-style)"},

        // Error Handling
        {"err", "if err != nil {\n    ${1:return err}\n}", "Error handling"},
        {"errc", "if err := ${1:function}(); err != nil {\n    ${2:return err}\n}",
         "Error check inline"},
        {"panic", "panic(\"${1:message}\")", "Panic"},
        {"recover",
         "defer func() {\n    if r := recover(); r != nil {\n        ${1:// handle}\n    }\n}()",
         "Recover from panic"},

        // Defer
        {"defer", "defer ${1:function}()", "Defer statement"},
        {"deferc", "defer func() {\n    ${1:// cleanup}\n}()", "Defer closure"},

        // Goroutines and Channels
        {"go", "go ${1:function}()", "Goroutine"},
        {"chan", "make(chan ${1:type})", "Channel"},
        {"chanb", "make(chan ${1:type}, ${2:buffer})", "Buffered channel"},
        {"close", "close(${1:channel})", "Close channel"},

        // Slices and Maps
        {"slice", "${1:name} := []${2:type}{${3}}", "Slice declaration"},
        {"make", "make(${1:type}, ${2:length})", "Make function"},
        {"append", "append(${1:slice}, ${2:element})", "Append to slice"},
        {"map", "${1:name} := make(map[${2:key}]${3:value})", "Map declaration"},
        {"mapl", "${1:name} := map[${2:key}]${3:value}{${4}}", "Map literal"},

        // Pointers
        {"ptr", "&${1:value}", "Address of"},
        {"deref", "*${1:pointer}", "Dereference"},
        {"new", "new(${1:type})", "New pointer"},

        // Testing
        {"test", "func Test${1:Name}(t *testing.T) {\n    ${2:// test code}\n}", "Test function"},
        {"bench", "func Benchmark${1:Name}(b *testing.B) {\n    ${2:// benchmark code}\n}",
         "Benchmark function"},
        {"t", "t.Run(\"${1:name}\", func(t *testing.T) {\n    ${2:// test}\n})", "Subtest"},

        // Common Patterns
        {"return", "return ${1:value}", "Return statement"},
        {"returnn", "return nil", "Return nil"},
        {"returnv", "return", "Return void"},
        {"fmt", "fmt.Printf(\"${1:format}\", ${2:args})", "Printf"},
        {"fmtp", "fmt.Println(${1:value})", "Println"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"doc", "// ${1:Name} ${2:description}", "Documentation comment"},

        // Context
        {"ctx", "ctx, cancel := context.WithCancel(${1:context.Background()})\ndefer cancel()",
         "Context with cancel"},
        {"ctxto",
         "ctx, cancel := context.WithTimeout(${1:context.Background()}, ${2:time.Second})\ndefer "
         "cancel()",
         "Context with timeout"},

        // HTTP Server
        {"httph",
         "http.HandleFunc(\"/${1:path}\", func(w http.ResponseWriter, r *http.Request) {\n    "
         "${2:// code}\n})",
         "HTTP handler function"},
        {"httpget",
         "resp, err := http.Get(${1:url})\nif err != nil {\n    ${2:return err}\n}\ndefer "
         "resp.Body.Close()",
         "HTTP GET request"},

        // Logging
        {"logp", "log.Printf(\"${1:format}\", ${2:args})", "log.Printf"},

        // Map lookup
        {"mapok", "if ${1:val}, ok := ${2:m}[${3:key}]; ok {\n    ${4:// code}\n}",
         "Map lookup with ok"},

        // Struct with JSON tag
        {"structj", "type ${1:Name} struct {\n    ${2:Field} ${3:Type} `json:\"${4:field}\"`\n}",
         "Struct with json tag"},

        // Worker goroutine
        {"worker",
         "go func() {\n    for ${1:item} := range ${2:ch} {\n        ${3:// work}\n    }\n}()",
         "Worker goroutine consuming channel"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
