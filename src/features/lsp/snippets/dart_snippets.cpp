#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getDartSnippets() {
    return {
        // Functions
        {"void", "void ${1:functionName}(${2:parameters}) {\n    ${3:// code}\n}", "Void function"},
        {"func",
         "${1:ReturnType} ${2:functionName}(${3:parameters}) {\n    ${4:return ${5:value};}\n}",
         "Function with return"},
        {"funca", "${1:ReturnType} ${2:functionName}(${3:parameters}) => ${4:expression};",
         "Arrow function"},
        {"async",
         "Future<${1:ReturnType}> ${2:functionName}(${3:parameters}) async {\n    ${4:// code}\n}",
         "Async function"},
        {"main", "void main() {\n    ${1:// code}\n}", "Main function"},
        {"runapp", "void main() {\n    runApp(${1:MyApp}());\n}", "Flutter main"},

        // Classes
        {"class", "class ${1:ClassName} {\n    ${2:// code}\n}", "Class definition"},
        {"classc", "class ${1:ClassName} {\n    ${1:ClassName}(${2:parameters});\n}",
         "Class with constructor"},
        {"classn", "class ${1:ClassName} {\n    ${1:ClassName}.${2:named}(${3:parameters});\n}",
         "Class with named constructor"},
        {"abstract", "abstract class ${1:ClassName} {\n    ${2:// code}\n}", "Abstract class"},
        {"mixin", "mixin ${1:MixinName} {\n    ${2:// code}\n}", "Mixin"},
        {"extension", "extension ${1:ExtensionName} on ${2:Type} {\n    ${3:// code}\n}",
         "Extension"},
        {"enum", "enum ${1:EnumName} {\n    ${2:value}\n}", "Enum definition"},

        // Flutter Widgets
        {"stateless",
         "class ${1:WidgetName} extends StatelessWidget {\n    const ${1:WidgetName}({Key? key}) : "
         "super(key: key);\n\n    @override\n    Widget build(BuildContext context) {\n        "
         "return ${2:Container}();\n    }\n}",
         "StatelessWidget"},
        {"stateful",
         "class ${1:WidgetName} extends StatefulWidget {\n    const ${1:WidgetName}({Key? key}) : "
         "super(key: key);\n\n    @override\n    State<${1:WidgetName}> createState() => "
         "_${1:WidgetName}State();\n}\n\nclass _${1:WidgetName}State extends "
         "State<${1:WidgetName}> {\n    @override\n    Widget build(BuildContext context) {\n      "
         "  return ${2:Container}();\n    }\n}",
         "StatefulWidget"},
        {"scaffold",
         "Scaffold(\n    appBar: AppBar(\n        title: Text('${1:Title}'),\n    ),\n    body: "
         "${2:// body},\n)",
         "Scaffold"},
        {"container",
         "Container(\n    width: ${1:double.infinity},\n    height: ${2:double.infinity},\n    "
         "child: ${3:// child},\n)",
         "Container"},
        {"column", "Column(\n    children: [\n        ${1:// children},\n    ],\n)", "Column"},
        {"row", "Row(\n    children: [\n        ${1:// children},\n    ],\n)", "Row"},
        {"listview", "ListView(\n    children: [\n        ${1:// children},\n    ],\n)",
         "ListView"},
        {"text", "Text('${1:text}')", "Text widget"},
        {"elevatedbutton",
         "ElevatedButton(\n    onPressed: () {\n        ${1:// code}\n    },\n    child: "
         "Text('${2:Button}'),\n)",
         "ElevatedButton"},
        {"textfield",
         "TextField(\n    decoration: InputDecoration(\n        labelText: '${1:Label}',\n    "
         "),\n)",
         "TextField"},

        // Variables
        {"var", "var ${1:name} = ${2:value};", "Variable"},
        {"final", "final ${1:name} = ${2:value};", "Final variable"},
        {"const", "const ${1:name} = ${2:value};", "Const variable"},
        {"late", "late ${1:Type} ${2:name};", "Late variable"},

        // Control Flow
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"switch",
         "switch (${1:value}) {\n    case ${2:case}:\n        ${3:// code}\n        break;\n    "
         "default:\n        ${4:// code}\n}",
         "Switch statement"},
        {"for", "for (var ${1:i} = ${2:0}; ${1:i} < ${3:length}; ${1:i}++) {\n    ${4:// code}\n}",
         "For loop"},
        {"forin", "for (var ${1:item} in ${2:collection}) {\n    ${3:// code}\n}", "For-in loop"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do {\n    ${1:// code}\n} while (${2:condition});", "Do-while loop"},

        // Collections
        {"list", "List<${1:Type}> ${2:list} = [${3}];", "List"},
        {"map", "Map<${1:Key}, ${2:Value}> ${3:map} = {${4:key: value}};", "Map"},
        {"set", "Set<${1:Type}> ${2:set} = {${3}};", "Set"},

        // Null Safety
        {"?", "${1:value}?.${2:member}", "Null-aware operator"},
        {"??", "${1:value} ?? ${2:default}", "Null-coalescing operator"},
        {"!", "${1:value}!", "Null assertion operator"},
        {"??"
         "=",
         "${1:value} ??"
         "= ${2:default}",
         "Null-aware assignment"},

        // Streams and Futures
        {"future", "Future<${1:Type}> ${2:function}() async {\n    ${3:// code}\n}", "Future"},
        {"stream", "Stream<${1:Type}> ${2:function}() async* {\n    ${3:// code}\n}", "Stream"},
        {"await", "await ${1:future}", "Await"},
        {"then", "${1:future}.then((${2:value}) {\n    ${3:// code}\n});", "Then callback"},
        {"catch", "${1:future}.catchError((${2:error}) {\n    ${3:// handle}\n});", "Catch error"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"doc", "/// ${1:documentation}", "Documentation comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
