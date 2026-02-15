#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getShellSnippets() {
    return {
        // Shebang
        {"shebang", "#!/bin/bash",
         "Bash shebang"},
        {"shebangsh", "#!/bin/sh",
         "Shell shebang"},

        // Variables
        {"var", "${1:VAR}=${2:value}",
         "Variable assignment"},
        {"varexport", "export ${1:VAR}=${2:value}",
         "Export variable"},
        {"varread", "read ${1:VAR}",
         "Read variable"},
        {"varreadp", "read -p \"${1:prompt}\" ${2:VAR}",
         "Read with prompt"},
        {"dollar", "${${1:VAR}}",
         "Variable reference"},
        {"dollarcurly", "${${1:VAR}}",
         "Variable reference (curly braces)"},
        {"dollarpos", "${${1:$2}}",
         "Positional parameter"},

        // Conditionals
        {"if", "if [ ${1:condition} ]; then\n    ${2:// code}\nfi",
         "If statement"},
        {"ifelse", "if [ ${1:condition} ]; then\n    ${2:// code}\nelse\n    ${3:// code}\nfi",
         "If-else statement"},
        {"elif", "elif [ ${1:condition} ]; then\n    ${2:// code}",
         "Else-if statement"},
        {"case", "case ${1:value} in\n    ${2:pattern})\n        ${3:// code}\n        ;;\n    *)\n        ${4:// code}\n        ;;\nesac",
         "Case statement"},

        // Loops
        {"for", "for ${1:var} in ${2:list}; do\n    ${3:// code}\ndone",
         "For loop"},
        {"fori", "for (( ${1:i}=${2:0}; ${1:i}<${3:n}; ${1:i}++ )); do\n    ${4:// code}\ndone",
         "For loop with index"},
        {"while", "while [ ${1:condition} ]; do\n    ${2:// code}\ndone",
         "While loop"},
        {"until", "until [ ${1:condition} ]; do\n    ${2:// code}\ndone",
         "Until loop"},

        // Functions
        {"func", "${1:function_name}() {\n    ${2:// code}\n}",
         "Function definition"},
        {"funcp", "${1:function_name}() {\n    local ${2:param}=\"${3:$1}\"\n    ${4:// code}\n}",
         "Function with parameter"},

        // File Operations
        {"test", "[ -${1:f} \"${2:file}\" ]",
         "File test"},
        {"testd", "[ -d \"${1:dir}\" ]",
         "Directory test"},
        {"testf", "[ -f \"${1:file}\" ]",
         "File exists test"},
        {"teste", "[ -e \"${1:file}\" ]",
         "File exists test"},
        {"testr", "[ -r \"${1:file}\" ]",
         "Readable test"},
        {"testw", "[ -w \"${1:file}\" ]",
         "Writable test"},
        {"testx", "[ -x \"${1:file}\" ]",
         "Executable test"},

        // String Operations
        {"echo", "echo \"${1:text}\"",
         "Echo"},
        {"echon", "echo -n \"${1:text}\"",
         "Echo without newline"},
        {"printf", "printf \"${1:format}\" ${2:args}",
         "Printf"},
        {"substr", "${${1:VAR}:${2:start}:${3:length}}",
         "Substring"},
        {"length", "${#${1:VAR}}",
         "String length"},
        {"replace", "${${1:VAR}/${2:pattern}/${3:replacement}}",
         "String replace"},
        {"replaceall", "${${1:VAR}//${2:pattern}/${3:replacement}}",
         "String replace all"},

        // Arrays
        {"array", "${1:ARRAY}=(${2:item1 item2 item3})",
         "Array declaration"},
        {"arrayel", "${${1:ARRAY}[${2:0}]}",
         "Array element"},
        {"arraylen", "${#${1:ARRAY}[@]}",
         "Array length"},
        {"arrayall", "${${1:ARRAY}[@]}",
         "All array elements"},

        // Redirection
        {"redirect", "${1:command} > ${2:file}",
         "Redirect output"},
        {"redirecta", "${1:command} >> ${2:file}",
         "Append output"},
        {"redirecte", "${1:command} 2> ${2:file}",
         "Redirect error"},
        {"redirectb", "${1:command} &> ${2:file}",
         "Redirect both"},
        {"pipe", "${1:command1} | ${2:command2}",
         "Pipe"},

        // Command Substitution
        {"cmdsub", "$(${1:command})",
         "Command substitution"},
        {"cmdsubb", "`${1:command}`",
         "Command substitution (backticks)"},

        // Arithmetic
        {"arith", "$((${1:expression}))",
         "Arithmetic expansion"},
        {"arithlet", "let ${1:var}=${2:expression}",
         "Let arithmetic"},

        // Common Commands
        {"cd", "cd \"${1:directory}\"",
         "Change directory"},
        {"mkdir", "mkdir -p \"${1:directory}\"",
         "Create directory"},
        {"rm", "rm -rf \"${1:file}\"",
         "Remove file"},
        {"cp", "cp -r \"${1:source}\" \"${2:dest}\"",
         "Copy file"},
        {"mv", "mv \"${1:source}\" \"${2:dest}\"",
         "Move file"},
        {"cat", "cat \"${1:file}\"",
         "Cat file"},
        {"grep", "grep \"${1:pattern}\" \"${2:file}\"",
         "Grep"},
        {"sed", "sed 's/${1:pattern}/${2:replacement}/g' \"${3:file}\"",
         "Sed replace"},
        {"awk", "awk '{print ${1:$1}}' \"${2:file}\"",
         "Awk"},
        {"find", "find \"${1:path}\" -name \"${2:pattern}\"",
         "Find files"},

        // Error Handling
        {"sete", "set -e",
         "Exit on error"},
        {"setx", "set -x",
         "Debug mode"},
        {"trap", "trap '${1:cleanup}' EXIT",
         "Trap signal"},
        {"trap_err", "trap '${1:cleanup}' ERR",
         "Trap error"},

        // Comments
        {"cmt", "# ${1:comment}",
         "Comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana

