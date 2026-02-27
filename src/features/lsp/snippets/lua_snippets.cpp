#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getLuaSnippets() {
    return {
        // Functions
        {"function", "function ${1:functionName}(${2:parameters})\n    ${3:// code}\nend",
         "Function definition"},
        {"func", "function ${1:functionName}(${2:parameters})\n    ${3:// code}\nend", "Function"},
        {"local", "local function ${1:functionName}(${2:parameters})\n    ${3:// code}\nend",
         "Local function"},
        {"anon", "function(${1:parameters})\n    ${2:// code}\nend", "Anonymous function"},
        {"return", "return ${1:value}", "Return statement"},

        // Variables
        {"local", "local ${1:variable} = ${2:value}", "Local variable"},
        {"var", "${1:variable} = ${2:value}", "Global variable"},

        // Control Flow
        {"if", "if ${1:condition} then\n    ${2:// code}\nend", "If statement"},
        {"ifelse", "if ${1:condition} then\n    ${2:// code}\nelse\n    ${3:// code}\nend",
         "If-else statement"},
        {"elseif", "elseif ${1:condition} then\n    ${2:// code}", "Else-if statement"},
        {"while", "while ${1:condition} do\n    ${2:// code}\nend", "While loop"},
        {"repeat", "repeat\n    ${1:// code}\nuntil ${2:condition}", "Repeat-until loop"},
        {"for", "for ${1:i} = ${2:1}, ${3:10} do\n    ${4:// code}\nend", "For loop"},
        {"fori", "for ${1:i} = ${2:start}, ${3:end}, ${4:step} do\n    ${5:// code}\nend",
         "For loop with step"},
        {"forin", "for ${1:key}, ${2:value} in pairs(${3:table}) do\n    ${4:// code}\nend",
         "For-in loop"},
        {"forip", "for ${1:key}, ${2:value} in ipairs(${3:table}) do\n    ${4:// code}\nend",
         "For-ipairs loop"},

        // Tables
        {"table", "${1:table} = {${2}}", "Table"},
        {"tablek", "${1:table} = {\n    [${2:key}] = ${3:value}\n}", "Table with key"},
        {"tablea", "${1:table} = {${2:value1}, ${3:value2}}", "Array table"},
        {"emptyt", "${1:table} = {}", "Empty table"},

        // Metatables
        {"metatable", "setmetatable(${1:table}, {\n    __index = ${2:parent}\n})", "Set metatable"},
        {"metaindex", "__index = function(${1:table}, ${2:key})\n    ${3:// code}\nend",
         "Metatable __index"},
        {"metacall", "__call = function(${1:table}, ${2:...})\n    ${3:// code}\nend",
         "Metatable __call"},

        // Error Handling
        {"pcall", "local ${1:success}, ${2:result} = pcall(${3:function}, ${4:args})",
         "Protected call"},
        {"xpcall", "xpcall(${1:function}, ${2:error_handler}, ${3:args})",
         "Extended protected call"},
        {"error", "error(\"${1:message}\")", "Error"},
        {"assert", "assert(${1:condition}, \"${2:message}\")", "Assert"},

        // String Operations
        {"string", "\"${1:text}\"", "String"},
        {"stringq", "'${1:text}'", "String (single quotes)"},
        {"stringm", "[[${1:multiline text}]]", "Multiline string"},
        {"format", "string.format(\"${1:format}\", ${2:args})", "String format"},
        {"gsub", "string.gsub(${1:str}, \"${2:pattern}\", \"${3:replacement}\")", "String gsub"},
        {"match", "string.match(${1:str}, \"${2:pattern}\")", "String match"},
        {"find", "string.find(${1:str}, \"${2:pattern}\")", "String find"},
        {"len", "string.len(${1:str})", "String length"},
        {"sub", "string.sub(${1:str}, ${2:start}, ${3:end})", "String substring"},
        {"upper", "string.upper(${1:str})", "String uppercase"},
        {"lower", "string.lower(${1:str})", "String lowercase"},

        // Table Operations
        {"insert", "table.insert(${1:table}, ${2:value})", "Table insert"},
        {"remove", "table.remove(${1:table}, ${2:index})", "Table remove"},
        {"concat", "table.concat(${1:table}, \"${2:,}\")", "Table concat"},
        {"sort", "table.sort(${1:table})", "Table sort"},
        {"unpack", "table.unpack(${1:table})", "Table unpack"},

        // Modules
        {"module",
         "local ${1:module} = {}\n\nfunction ${1:module}.${2:function}()\n    ${3:// "
         "code}\nend\n\nreturn ${1:module}",
         "Module"},
        {"require", "local ${1:module} = require(\"${2:module}\")", "Require module"},

        // Common Patterns
        {"nilcheck", "if ${1:value} == nil then\n    ${2:// handle}\nend", "Nil check"},
        {"typecheck", "if type(${1:value}) == \"${2:type}\" then\n    ${3:// code}\nend",
         "Type check"},
        {"getn", "#${1:table}", "Get table length"},
        {"next", "next(${1:table}, ${2:key})", "Next key-value pair"},
        {"pairs", "pairs(${1:table})", "Pairs iterator"},
        {"ipairs", "ipairs(${1:table})", "Ipairs iterator"},

        // Comments
        {"cmt", "-- ${1:comment}", "Single line comment"},
        {"cmtb", "--[[\n${1:comment}\n]]", "Block comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
