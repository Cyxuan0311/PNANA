#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getCppSnippets() {
    return {
        // Loops
        {"for", "for (auto& ${1:item} : ${2:container}) {\n    ${3:// code}\n}",
         "Range-based for loop"},
        {"fori", "for (size_t ${1:i} = 0; ${1:i} < ${2:n}; ++${1:i}) {\n    ${3:// code}\n}",
         "Index-based for loop"},
        {"forr", "for (int ${1:i} = ${2:n} - 1; ${1:i} >= 0; --${1:i}) {\n    ${3:// code}\n}",
         "Reverse for loop"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do {\n    ${1:// code}\n} while (${2:condition});", "Do-while loop"},

        // Conditionals
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"else", "else {\n    ${1:// code}\n}", "Else statement"},
        {"elseif", "else if (${1:condition}) {\n    ${2:// code}\n}", "Else-if statement"},
        {"switch",
         "switch (${1:expression}) {\ncase ${2:value}:\n    ${3:// code}\n    break;\ndefault:\n   "
         " ${4:// code}\n}",
         "Switch statement"},
        {"case", "case ${1:value}:\n    ${2:// code}\n    break;", "Case statement"},

        // Classes and Structures
        {"class",
         "class ${1:ClassName} {\npublic:\n    ${1}(${2});\n    ~${1}();\n\nprivate:\n    ${3}\n};",
         "Class definition"},
        {"struct", "struct ${1:StructName} {\n    ${2}\n};", "Struct definition"},
        {"namespace", "namespace ${1:name} {\n    ${2}\n}", "Namespace definition"},
        {"enum", "enum class ${1:EnumName} {\n    ${2}\n};", "Enum class definition"},

        // Functions
        {"func",
         "${1:return_type} ${2:function_name}(${3:parameters}) {\n    ${4:// code}\n    return "
         "${5:value};\n}",
         "Function definition"},
        {"lambda",
         "[${1:capture}](${2:params}) ${3:mutable} -> ${4:return_type} {\n    ${5:// code}\n}",
         "Lambda function"},
        {"main", "int main(int argc, char* argv[]) {\n    ${1:// code}\n    return 0;\n}",
         "Main function"},
        {"mainv", "int main() {\n    ${1:// code}\n    return 0;\n}", "Main function (no args)"},

        // Includes and Headers
        {"include", "#include \"${1:header.h}\"", "Include header"},
        {"includei", "#include <${1:header}>", "Include system header"},
        {"guard", "#ifndef ${1:HEADER_H}\n#define ${1:HEADER_H}\n\n${2}\n\n#endif // ${1:HEADER_H}",
         "Header guard"},
        {"pragma", "#pragma once\n\n${1}", "Pragma once"},

        // Smart Pointers
        {"unique", "std::unique_ptr<${1:Type}> ${2:ptr} = std::make_unique<${1:Type}>(${3});",
         "Unique pointer"},
        {"shared", "std::shared_ptr<${1:Type}> ${2:ptr} = std::make_shared<${1:Type}>(${3});",
         "Shared pointer"},
        {"weak", "std::weak_ptr<${1:Type}> ${2:ptr} = ${3:shared_ptr};", "Weak pointer"},

        // Containers
        {"vector", "std::vector<${1:Type}> ${2:vec}${3: = {${4}}};", "Vector declaration"},
        {"map", "std::map<${1:Key}, ${2:Value}> ${3:m}${4: = {${5}}};", "Map declaration"},
        {"unordered", "std::unordered_map<${1:Key}, ${2:Value}> ${3:m}${4: = {${5}}};",
         "Unordered map declaration"},
        {"set", "std::set<${1:Type}> ${2:s}${3: = {${4}}};", "Set declaration"},
        {"pair", "std::pair<${1:First}, ${2:Second}> ${3:p}${4: = {${5}, ${6}}};",
         "Pair declaration"},

        // STL Algorithms
        {"find", "std::find(${1:begin}, ${2:end}, ${3:value})", "Find algorithm"},
        {"sort", "std::sort(${1:begin}, ${2:end});", "Sort algorithm"},
        {"transform", "std::transform(${1:begin}, ${2:end}, ${3:out}, ${4:func});",
         "Transform algorithm"},

        // Error Handling
        {"try",
         "try {\n    ${1:// code}\n} catch (${2:exception}& ${3:e}) {\n    ${4:// handle}\n}",
         "Try-catch block"},
        {"throw", "throw ${1:exception}(${2:message});", "Throw exception"},

        // Templates
        {"template", "template<typename ${1:T}>\n${2}", "Template definition"},
        {"templatem",
         "template<typename ${1:T}>\n${2:return_type} ${3:function}(${4:params}) {\n    ${5:// "
         "code}\n}",
         "Template function"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},

        // I/O
        {"cout", "std::cout << ${1:value} << std::endl;", "Console output"},
        {"cin", "std::cin >> ${1:variable};", "Console input"},

        // Assertions
        {"assert", "assert(${1:condition});", "Assert statement"},
        {"static", "static_assert(${1:condition}, \"${2:message}\");", "Static assert"},

        // Pointers and Memory
        {"ifn", "if (${1:ptr} == nullptr) {\n    ${2:// handle null}\n}", "Null pointer check"},
        {"ifnn", "if (${1:ptr} != nullptr) {\n    ${2:// use pointer}\n}",
         "Not-null pointer check"},
        {"new", "${1:Type}* ${2:ptr} = new ${1:Type}(${3});", "Raw pointer allocation"},
        {"del", "delete ${1:ptr};\n${1:ptr} = nullptr;", "Safe delete pointer"},

        // Constructors / Destructors
        {"ctor",
         "${1:ClassName}::${1}( ${2:/* params */} )\n    : ${3:/* member init list */} {\n    "
         "${4:// body}\n}",
         "Constructor definition"},
        {"dtor", "${1:ClassName}::~${1}() {\n    ${2:// cleanup}\n}", "Destructor definition"},

        // RAII and Mutex
        {"mutex", "std::mutex ${1:mtx};", "Mutex declaration"},
        {"lock", "std::lock_guard<std::mutex> ${1:lock}(${2:mtx});", "Lock guard"},

        // Threading
        {"thread", "std::thread ${1:t}([&]() {\n    ${2:// code}\n});", "Thread creation"},
        {"join", "if (${1:t}.joinable()) {\n    ${1:t}.join();\n}", "Thread join"},

        // Logging / Debug
        {"dbg", "std::cout << \"${1:msg}: \" << ${2:value} << std::endl;", "Debug print"},

        // Ranged for with index
        {"foriv",
         "for (size_t ${1:i} = 0; ${1:i} < ${2:container}.size(); ++${1:i}) {\n    auto& ${3:item} "
         "= ${2:container}[${1:i}];\n    ${4:// code}\n}",
         "Index + value loop"},

        // constexpr / auto
        {"constexpr", "constexpr ${1:auto} ${2:name} = ${3:value};", "Constexpr variable"},
        {"auto", "auto ${1:name} = ${2:expression};", "Auto-deduced variable"},

        // Namespace alias
        {"nsalias", "namespace ${1:alias} = ${2:full::namespace};", "Namespace alias"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
