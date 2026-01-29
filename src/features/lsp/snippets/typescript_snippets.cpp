#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getTypeScriptSnippets() {
    return {
        // Functions
        {"function",
         "function ${1:functionName}(${2:parameters}): ${3:returnType} {\n    ${4:// code}\n}",
         "Function declaration"},
        {"arrow",
         "const ${1:functionName} = (${2:parameters}): ${3:returnType} => {\n    ${4:// code}\n};",
         "Arrow function"},
        {"async",
         "async function ${1:functionName}(${2:parameters}): Promise<${3:returnType}> {\n    "
         "${4:// code}\n}",
         "Async function"},
        {"asynca",
         "const ${1:functionName} = async (${2:parameters}): Promise<${3:returnType}> => {\n    "
         "${4:// code}\n};",
         "Async arrow function"},

        // Types and Interfaces
        {"interface", "interface ${1:InterfaceName} {\n    ${2:property}: ${3:type};\n}",
         "Interface definition"},
        {"type", "type ${1:TypeName} = ${2:type};", "Type alias"},
        {"enum", "enum ${1:EnumName} {\n    ${2:Key} = ${3:value}\n}", "Enum definition"},
        {"class",
         "class ${1:ClassName} {\n    ${2:property}: ${3:type};\n    \n    "
         "constructor(${4:parameters}) {\n        ${5:// code}\n    }\n}",
         "Class definition"},
        {"classe",
         "class ${1:ClassName} extends ${2:BaseClass} implements ${3:Interface} {\n    ${4:// "
         "code}\n}",
         "Class with inheritance"},

        // Generics
        {"generic", "<${1:T}>", "Generic type parameter"},
        {"genericf",
         "function ${1:functionName}<${2:T}>(${3:params}): ${4:returnType} {\n    ${5:// code}\n}",
         "Generic function"},
        {"genericc", "class ${1:ClassName}<${2:T}> {\n    ${3:// code}\n}", "Generic class"},

        // Type Guards and Assertions
        {"typeguard",
         "function is${1:Type}(obj: any): obj is ${1:Type} {\n    return ${2:condition};\n}",
         "Type guard"},
        {"as", "${1:value} as ${2:type}", "Type assertion"},
        {"asany", "${1:value} as any", "Type assertion to any"},

        // Utility Types
        {"partial", "Partial<${1:Type}>", "Partial utility type"},
        {"required", "Required<${1:Type}>", "Required utility type"},
        {"pick", "Pick<${1:Type}, '${2:key}'>", "Pick utility type"},
        {"omit", "Omit<${1:Type}, '${2:key}'>", "Omit utility type"},
        {"record", "Record<${1:Key}, ${2:Value}>", "Record utility type"},

        // Conditionals and Loops
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"ternary", "${1:condition} ? ${2:trueValue} : ${3:falseValue}", "Ternary operator"},
        {"for", "for (let ${1:i} = 0; ${1:i} < ${2:array}.length; ${1:i}++) {\n    ${3:// code}\n}",
         "For loop"},
        {"forof", "for (const ${1:item} of ${2:iterable}) {\n    ${3:// code}\n}", "For-of loop"},
        {"foreach", "${1:array}.forEach((${2:item}) => {\n    ${3:// code}\n});", "ForEach loop"},

        // Array Methods
        {"map", "${1:array}.map((${2:item}) => ${3:item});", "Array map"},
        {"filter", "${1:array}.filter((${2:item}) => ${3:condition});", "Array filter"},
        {"reduce", "${1:array}.reduce((${2:acc}, ${3:item}) => ${4:acc + item}, ${5:0});",
         "Array reduce"},

        // Promises
        {"promise", "Promise<${1:Type}>", "Promise type"},
        {"await", "await ${1:promise}", "Await expression"},
        {"try", "try {\n    ${1:// code}\n} catch (${2:error}) {\n    ${3:// handle}\n}",
         "Try-catch block"},

        // Modules
        {"import", "import ${1:name} from '${2:module}';", "ES6 import"},
        {"importa", "import {${1:name}} from '${2:module}';", "ES6 named import"},
        {"importt", "import type {${1:Type}} from '${2:module}';", "Type-only import"},
        {"export", "export ${1:const} ${2:name}: ${3:type} = ${4:value};", "ES6 export"},
        {"exportd", "export default ${1:name};", "ES6 default export"},

        // Decorators
        {"decorator", "@${1:decorator}\n${2:target}", "Decorator"},
        {"decoratorp", "@${1:Decorator}(${2:params})\n${3:target}", "Decorator with parameters"},

        // Common Patterns
        {"const", "const ${1:name}: ${2:type} = ${3:value};", "Const declaration"},
        {"let", "let ${1:name}: ${2:type} = ${3:value};", "Let declaration"},
        {"return", "return ${1:value};", "Return statement"},
        {"null", "null", "Null value"},
        {"undefined", "undefined", "Undefined value"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
