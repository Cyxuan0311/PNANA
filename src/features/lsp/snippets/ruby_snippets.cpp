#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getRubySnippets() {
    return {
        // Methods
        {"def", "def ${1:method_name}${2:(${3:parameters})}\n    ${4:// code}\nend",
         "Method definition"},
        {"defs", "def self.${1:method_name}${2:(${3:parameters})}\n    ${4:// code}\nend",
         "Class method definition"},
        {"defc", "def ${1:method_name}(${2:&block})\n    ${3:block.call}\nend",
         "Method with block"},

        // Classes and Modules
        {"class", "class ${1:ClassName}\n    ${2:// code}\nend", "Class definition"},
        {"classe", "class ${1:ClassName} < ${2:SuperClass}\n    ${3:// code}\nend",
         "Class with inheritance"},
        {"module", "module ${1:ModuleName}\n    ${2:// code}\nend", "Module definition"},
        {"include", "include ${1:ModuleName}", "Include module"},
        {"extend", "extend ${1:ModuleName}", "Extend module"},

        // Conditionals
        {"if", "if ${1:condition}\n    ${2:// code}\nend", "If statement"},
        {"ifelse", "if ${1:condition}\n    ${2:// code}\nelse\n    ${3:// code}\nend",
         "If-else statement"},
        {"elsif", "elsif ${1:condition}\n    ${2:// code}\nend", "Elsif statement"},
        {"unless", "unless ${1:condition}\n    ${2:// code}\nend", "Unless statement"},
        {"case",
         "case ${1:value}\nwhen ${2:condition}\n    ${3:// code}\nelse\n    ${4:// code}\nend",
         "Case statement"},
        {"ternary", "${1:condition} ? ${2:true_value} : ${3:false_value}", "Ternary operator"},

        // Loops
        {"for", "for ${1:item} in ${2:collection}\n    ${3:// code}\nend", "For loop"},
        {"each", "${1:collection}.each do |${2:item}|\n    ${3:// code}\nend", "Each iterator"},
        {"times", "${1:n}.times do |${2:i}|\n    ${3:// code}\nend", "Times iterator"},
        {"while", "while ${1:condition}\n    ${2:// code}\nend", "While loop"},
        {"until", "until ${1:condition}\n    ${2:// code}\nend", "Until loop"},
        {"loop", "loop do\n    ${1:// code}\nend", "Infinite loop"},

        // Blocks and Iterators
        {"do", "do |${1:param}|\n    ${2:// code}\nend", "Do block"},
        {"block", "{ |${1:param}| ${2:// code} }", "Block literal"},
        {"map", "${1:collection}.map { |${2:item}| ${3:item} }", "Map iterator"},
        {"select", "${1:collection}.select { |${2:item}| ${3:condition} }", "Select iterator"},
        {"reject", "${1:collection}.reject { |${2:item}| ${3:condition} }", "Reject iterator"},
        {"reduce", "${1:collection}.reduce(${2:0}) { |${3:acc}, ${4:item}| ${5:acc + item} }",
         "Reduce iterator"},

        // Error Handling
        {"begin",
         "begin\n    ${1:// code}\nrescue ${2:Exception} => ${3:e}\n    ${4:// handle}\nend",
         "Begin-rescue block"},
        {"rescue", "rescue ${1:Exception} => ${2:e}\n    ${3:// handle}\nend", "Rescue clause"},
        {"ensure", "ensure\n    ${1:// cleanup}\nend", "Ensure clause"},
        {"raise", "raise ${1:Exception}, \"${2:message}\"", "Raise exception"},

        // Variables and Constants
        {"var", "${1:name} = ${2:value}", "Variable assignment"},
        {"const", "${1:CONSTANT} = ${2:value}", "Constant definition"},
        {"ivar", "@${1:instance_variable}", "Instance variable"},
        {"cvar", "@@${1:class_variable}", "Class variable"},
        {"gvar", "${1:$global_variable}", "Global variable"},

        // Symbols and Strings
        {"symbol", ":${1:symbol}", "Symbol"},
        {"string", "\"${1:text}\"", "String literal"},
        {"stringi", "'${1:text}'", "String literal (interpolated)"},
        {"heredoc", "<<-${1:EOF}\n${2:text}\n${1:EOF}", "Heredoc"},

        // Arrays and Hashes
        {"array", "${1:array} = [${2}]", "Array declaration"},
        {"hash", "${1:hash} = {${2:key} => ${3:value}}", "Hash declaration"},
        {"hashn", "${1:hash} = {\n    ${2:key}: ${3:value}\n}", "Hash with new syntax"},

        // Common Patterns
        {"return", "return ${1:value}", "Return statement"},
        {"returnn", "return", "Return nil"},
        {"puts", "puts ${1:value}", "Print line"},
        {"p", "p ${1:value}", "Print inspect"},
        {"nil", "nil", "Nil value"},

        // Comments
        {"cmt", "# ${1:comment}", "Single line comment"},
        {"cmtb", "=begin\n${1:comment}\n=end", "Block comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
