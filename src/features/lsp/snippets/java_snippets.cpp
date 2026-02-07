#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getJavaSnippets() {
    return {
        // Classes
        {"class", "public class ${1:ClassName} {\n    ${2}\n}", "Class definition"},
        {"classa", "public abstract class ${1:ClassName} {\n    ${2}\n}", "Abstract class"},
        {"interface", "public interface ${1:InterfaceName} {\n    ${2}\n}", "Interface definition"},
        {"enum", "public enum ${1:EnumName} {\n    ${2:VALUE}\n}", "Enum definition"},

        // Methods
        {"method", "public ${1:returnType} ${2:methodName}(${3:parameters}) {\n    ${4:// code}\n}",
         "Public method"},
        {"methodp",
         "private ${1:returnType} ${2:methodName}(${3:parameters}) {\n    ${4:// code}\n}",
         "Private method"},
        {"methodst",
         "public static ${1:returnType} ${2:methodName}(${3:parameters}) {\n    ${4:// code}\n}",
         "Static method"},
        {"main", "public static void main(String[] args) {\n    ${1:// code}\n}", "Main method"},

        // Constructors
        {"ctor", "public ${1:ClassName}(${2:parameters}) {\n    ${3:// code}\n}", "Constructor"},
        {"ctorp", "private ${1:ClassName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Private constructor"},

        // Conditionals
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"else", "else {\n    ${1:// code}\n}", "Else statement"},
        {"elseif", "else if (${1:condition}) {\n    ${2:// code}\n}", "Else-if statement"},
        {"switch",
         "switch (${1:expression}) {\n    case ${2:value}:\n        ${3:// code}\n        break;\n "
         "   default:\n        ${4:// code}\n}",
         "Switch statement"},
        {"case", "case ${1:value}:\n    ${2:// code}\n    break;", "Case statement"},
        {"ternary", "${1:condition} ? ${2:trueValue} : ${3:falseValue}", "Ternary operator"},

        // Loops
        {"for", "for (int ${1:i} = 0; ${1:i} < ${2:n}; ${1:i}++) {\n    ${3:// code}\n}",
         "For loop"},
        {"fore", "for (${1:type} ${2:item} : ${3:collection}) {\n    ${4:// code}\n}",
         "Enhanced for loop"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do {\n    ${1:// code}\n} while (${2:condition});", "Do-while loop"},

        // Exception Handling
        {"try", "try {\n    ${1:// code}\n} catch (${2:Exception} ${3:e}) {\n    ${4:// handle}\n}",
         "Try-catch block"},
        {"tryf",
         "try {\n    ${1:// code}\n} catch (${2:Exception} ${3:e}) {\n    ${4:// handle}\n} "
         "finally {\n    ${5:// cleanup}\n}",
         "Try-catch-finally block"},
        {"throw", "throw new ${1:Exception}(${2:message});", "Throw exception"},
        {"throws", "throws ${1:Exception}", "Throws clause"},

        // Collections
        {"list", "List<${1:Type}> ${2:list} = new ArrayList<>();", "List declaration"},
        {"map", "Map<${1:Key}, ${2:Value}> ${3:map} = new HashMap<>();", "Map declaration"},
        {"set", "Set<${1:Type}> ${2:set} = new HashSet<>();", "Set declaration"},
        {"array", "${1:Type}[] ${2:array} = new ${1:Type}[${3:size}];", "Array declaration"},
        {"arrayl", "${1:Type}[] ${2:array} = {${3}};", "Array literal"},

        // Annotations
        {"override", "@Override\n${1:method}", "Override annotation"},
        {"deprecated", "@Deprecated\n${1:item}", "Deprecated annotation"},
        {"suppress", "@SuppressWarnings(\"${1:warning}\")\n${2:item}", "Suppress warnings"},

        // Common Patterns
        {"sysout", "System.out.println(${1:value});", "System.out.println"},
        {"syserr", "System.err.println(${1:value});", "System.err.println"},
        {"return", "return ${1:value};", "Return statement"},
        {"returnv", "return;", "Return void"},
        {"null", "null", "Null value"},

        // Access Modifiers
        {"public", "public ${1:item}", "Public modifier"},
        {"private", "private ${1:item}", "Private modifier"},
        {"protected", "protected ${1:item}", "Protected modifier"},
        {"static", "static ${1:item}", "Static modifier"},
        {"final", "final ${1:item}", "Final modifier"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"javadoc",
         "/**\n * ${1:description}\n * @param ${2:param} ${3:description}\n * @return "
         "${4:description}\n */",
         "JavaDoc comment"},

        // Logging
        {"logger",
         "private static final Logger LOGGER = LoggerFactory.getLogger(${1:ClassName}.class);",
         "SLF4J logger field"},

        // Try-with-resources
        {"tryw",
         "try (${1:ResourceType} ${2:resource} = ${3:expression}) {\n    ${4:// code}\n} catch "
         "(${5:Exception} ${6:e}) {\n    ${7:// handle}\n}",
         "Try-with-resources block"},

        // Synchronized method
        {"syncm",
         "public synchronized ${1:void} ${2:methodName}(${3:parameters}) {\n    ${4:// code}\n}",
         "Synchronized method"},

        // Thread with lambda
        {"thread", "new Thread(() -> {\n    ${1:// code}\n}).start();", "Thread using lambda"},

        // Optional
        {"optional", "Optional<${1:Type}> ${2:opt} = Optional.ofNullable(${3:value});",
         "Optional.ofNullable"},

        // Stream API
        {"stream",
         "${1:list}.stream()\n    .filter(${2:item} -> ${3:condition})\n    .map(${4:item} -> "
         "${5:expression})\n    .collect(Collectors.toList());",
         "Stream pipeline"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
