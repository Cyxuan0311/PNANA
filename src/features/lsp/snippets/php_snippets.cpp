#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getPhpSnippets() {
    return {
        // Functions
        {"function", "function ${1:functionName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Function definition"},
        {"functionr",
         "function ${1:functionName}(${2:parameters}): ${3:returnType} {\n    ${4:// code}\n}",
         "Function with return type"},
        {"closure",
         "${1:\\$closure} = function(${2:params}) use (${3:vars}) {\n    ${4:// code}\n};",
         "Closure"},
        {"arrow", "fn(${1:params}) => ${2:expression}", "Arrow function"},

        // Classes
        {"class", "class ${1:ClassName} {\n    ${2:// code}\n}", "Class definition"},
        {"classe", "class ${1:ClassName} extends ${2:ParentClass} {\n    ${3:// code}\n}",
         "Class with inheritance"},
        {"classi", "class ${1:ClassName} implements ${2:Interface} {\n    ${3:// code}\n}",
         "Class implementing interface"},
        {"trait", "trait ${1:TraitName} {\n    ${2:// code}\n}", "Trait definition"},
        {"interface", "interface ${1:InterfaceName} {\n    ${2:// code}\n}",
         "Interface definition"},

        // Methods
        {"method", "public function ${1:methodName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Public method"},
        {"methodp", "private function ${1:methodName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Private method"},
        {"methodpr", "protected function ${1:methodName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Protected method"},
        {"methodst",
         "public static function ${1:methodName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Static method"},
        {"construct", "public function __construct(${1:parameters}) {\n    ${2:// code}\n}",
         "Constructor"},
        {"destruct", "public function __destruct() {\n    ${1:// code}\n}", "Destructor"},

        // Conditionals
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"else", "else {\n    ${1:// code}\n}", "Else statement"},
        {"elseif", "elseif (${1:condition}) {\n    ${2:// code}\n}", "Else-if statement"},
        {"ternary", "${1:condition} ? ${2:trueValue} : ${3:falseValue}", "Ternary operator"},
        {"switch",
         "switch (${1:expression}) {\n    case ${2:value}:\n        ${3:// code}\n        break;\n "
         "   default:\n        ${4:// code}\n}",
         "Switch statement"},
        {"case", "case ${1:value}:\n    ${2:// code}\n    break;", "Case statement"},

        // Loops
        {"for", "for (${1:\\$i} = 0; ${1:\\$i} < ${2:n}; ${1:\\$i}++) {\n    ${3:// code}\n}",
         "For loop"},
        {"foreach",
         "foreach (${1:\\$array} as ${2:\\$key} => ${3:\\$value}) {\n    ${4:// code}\n}",
         "Foreach loop"},
        {"foreachv", "foreach (${1:\\$array} as ${2:\\$value}) {\n    ${3:// code}\n}",
         "Foreach loop (values only)"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do {\n    ${1:// code}\n} while (${2:condition});", "Do-while loop"},

        // Arrays
        {"array", "${1:\\$array} = [${2}];", "Array declaration"},
        {"arraya", "${1:\\$array} = array(${2});", "Array declaration (old syntax)"},
        {"arraya", "${1:\\$array} = [\n    ${2:'key'} => ${3:'value'}\n];", "Associative array"},

        // Variables
        {"var", "${1:\\$variable} = ${2:value};", "Variable assignment"},
        {"const", "const ${1:CONSTANT} = ${2:value};", "Constant definition"},
        {"define", "define('${1:CONSTANT}', ${2:value});", "Define constant"},

        // Error Handling
        {"try",
         "try {\n    ${1:// code}\n} catch (${2:Exception} ${3:\\$e}) {\n    ${4:// handle}\n}",
         "Try-catch block"},
        {"throw", "throw new ${1:Exception}(${2:message});", "Throw exception"},

        // Common Patterns
        {"echo", "echo ${1:value};", "Echo statement"},
        {"print", "print ${1:value};", "Print statement"},
        {"return", "return ${1:value};", "Return statement"},
        {"returnn", "return null;", "Return null"},
        {"null", "null", "Null value"},

        // String Functions
        {"strlen", "strlen(${1:\\$string})", "String length"},
        {"substr", "substr(${1:\\$string}, ${2:start}, ${3:length})", "Substring"},
        {"strpos", "strpos(${1:\\$haystack}, ${2:\\$needle})", "String position"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"doc",
         "/**\n * ${1:description}\n * @param ${2:type} ${3:\\$param} ${4:description}\n * @return "
         "${5:type} ${6:description}\n */",
         "PHPDoc comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
