#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getJavaScriptSnippets() {
    return {
        // Functions
        {"function", "function ${1:functionName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Function declaration"},
        {"arrow", "const ${1:functionName} = (${2:parameters}) => {\n    ${3:// code}\n};",
         "Arrow function"},
        {"arrowi", "const ${1:functionName} = (${2:parameters}) => ${3:expression};",
         "Arrow function (implicit return)"},
        {"async", "async function ${1:functionName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Async function"},
        {"asynca", "const ${1:functionName} = async (${2:parameters}) => {\n    ${3:// code}\n};",
         "Async arrow function"},
        {"iife", "(function(${1:params}) {\n    ${2:// code}\n})(${3:args});",
         "IIFE (Immediately Invoked Function Expression)"},

        // Classes
        {"class",
         "class ${1:ClassName} {\n    constructor(${2:parameters}) {\n        ${3:// code}\n    "
         "}\n}",
         "Class definition"},
        {"classe",
         "class ${1:ClassName} extends ${2:BaseClass} {\n    constructor(${3:parameters}) {\n      "
         "  super(${4:args});\n        ${5:// code}\n    }\n}",
         "Class with inheritance"},
        {"method", "${1:methodName}(${2:parameters}) {\n    ${3:// code}\n}", "Class method"},
        {"getter", "get ${1:property}() {\n    return ${2:this._${1:property}};\n}",
         "Getter method"},
        {"setter", "set ${1:property}(${2:value}) {\n    this._${1:property} = ${2:value};\n}",
         "Setter method"},
        {"static", "static ${1:methodName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Static method"},

        // Conditionals
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"else", "else {\n    ${1:// code}\n}", "Else statement"},
        {"elseif", "else if (${1:condition}) {\n    ${2:// code}\n}", "Else-if statement"},
        {"ternary", "${1:condition} ? ${2:trueValue} : ${3:falseValue}", "Ternary operator"},
        {"switch",
         "switch (${1:expression}) {\n    case ${2:value}:\n        ${3:// code}\n        break;\n "
         "   default:\n        ${4:// code}\n}",
         "Switch statement"},
        {"case", "case ${1:value}:\n    ${2:// code}\n    break;", "Case statement"},

        // Loops
        {"for", "for (let ${1:i} = 0; ${1:i} < ${2:array}.length; ${1:i}++) {\n    ${3:// code}\n}",
         "For loop"},
        {"forin", "for (let ${1:key} in ${2:object}) {\n    ${3:// code}\n}", "For-in loop"},
        {"forof", "for (let ${1:item} of ${2:iterable}) {\n    ${3:// code}\n}", "For-of loop"},
        {"foreach", "${1:array}.forEach((${2:item}, ${3:index}) => {\n    ${4:// code}\n});",
         "ForEach loop"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do {\n    ${1:// code}\n} while (${2:condition});", "Do-while loop"},

        // Array Methods
        {"map", "${1:array}.map((${2:item}) => ${3:item});", "Array map"},
        {"filter", "${1:array}.filter((${2:item}) => ${3:condition});", "Array filter"},
        {"reduce", "${1:array}.reduce((${2:acc}, ${3:item}) => ${4:acc + item}, ${5:0});",
         "Array reduce"},
        {"find", "${1:array}.find((${2:item}) => ${3:condition});", "Array find"},
        {"some", "${1:array}.some((${2:item}) => ${3:condition});", "Array some"},
        {"every", "${1:array}.every((${2:item}) => ${3:condition});", "Array every"},

        // Objects
        {"object", "const ${1:obj} = {\n    ${2:key}: ${3:value}\n};", "Object literal"},
        {"destruct", "const {${1:prop1}, ${2:prop2}} = ${3:object};", "Object destructuring"},
        {"destructa", "const [${1:item1}, ${2:item2}] = ${3:array};", "Array destructuring"},
        {"spread", "...${1:object}", "Spread operator"},
        {"rest", "...${1:rest}", "Rest parameter"},

        // Promises and Async
        {"promise", "new Promise((resolve, reject) => {\n    ${1:// code}\n});",
         "Promise constructor"},
        {"then", ".then((${1:result}) => {\n    ${2:// code}\n})", "Promise then"},
        {"catch", ".catch((${1:error}) => {\n    ${2:// code}\n})", "Promise catch"},
        {"await", "await ${1:promise}", "Await expression"},
        {"try", "try {\n    ${1:// code}\n} catch (${2:error}) {\n    ${3:// handle}\n}",
         "Try-catch block"},

        // Modules
        {"import", "import ${1:name} from '${2:module}';", "ES6 import"},
        {"importa", "import {${1:name}} from '${2:module}';", "ES6 named import"},
        {"importd", "import * as ${1:alias} from '${2:module}';", "ES6 import all"},
        {"export", "export ${1:const} ${2:name} = ${3:value};", "ES6 export"},
        {"exportd", "export default ${1:name};", "ES6 default export"},
        {"require", "const ${1:module} = require('${2:module}');", "CommonJS require"},
        {"module", "module.exports = ${1:value};", "CommonJS export"},

        // Console
        {"console", "console.log(${1:message});", "Console log"},
        {"consolee", "console.error(${1:message});", "Console error"},
        {"consolew", "console.warn(${1:message});", "Console warn"},
        {"consoled", "console.debug(${1:message});", "Console debug"},
        {"consolet", "console.table(${1:data});", "Console table"},

        // DOM (if applicable)
        {"query", "document.querySelector('${1:selector}');", "Query selector"},
        {"querya", "document.querySelectorAll('${1:selector}');", "Query selector all"},
        {"addevent",
         "${1:element}.addEventListener('${2:event}', (${3:e}) => {\n    ${4:// code}\n});",
         "Add event listener"},

        // Common Patterns
        {"usestrict", "\"use strict\";", "Use strict"},
        {"return", "return ${1:value};", "Return statement"},
        {"const", "const ${1:name} = ${2:value};", "Const declaration"},
        {"let", "let ${1:name} = ${2:value};", "Let declaration"},
        {"var", "var ${1:name} = ${2:value};", "Var declaration"},
        {"null", "null", "Null value"},
        {"undefined", "undefined", "Undefined value"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"jsdoc",
         "/**\n * ${1:description}\n * @param {${2:type}} ${3:param}\n * @returns {${4:type}}\n */",
         "JSDoc comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
