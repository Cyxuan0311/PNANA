#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getKotlinSnippets() {
    return {
        // Functions
        {"fun", "fun ${1:functionName}(${2:parameters}): ${3:ReturnType} {\n    ${4:// code}\n}",
         "Function definition"},
        {"funp", "fun ${1:functionName}(${2:parameters}) = ${3:expression}",
         "Single expression function"},
        {"funm", "fun ${1:functionName}(${2:parameters}): ${3:ReturnType} {\n    ${4:// code}\n}",
         "Member function"},
        {"funex",
         "fun ${1:Type}.${2:functionName}(${3:parameters}): ${4:ReturnType} {\n    ${5:// code}\n}",
         "Extension function"},
        {"suspend",
         "suspend fun ${1:functionName}(${2:parameters}): ${3:ReturnType} {\n    ${4:// code}\n}",
         "Suspend function"},
        {"main", "fun main(args: Array<String>) {\n    ${1:// code}\n}", "Main function"},

        // Classes
        {"class", "class ${1:ClassName} {\n    ${2:// code}\n}", "Class definition"},
        {"classp", "class ${1:ClassName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Class with primary constructor"},
        {"data", "data class ${1:ClassName}(${2:parameters})", "Data class"},
        {"sealed", "sealed class ${1:ClassName} {\n    ${2:// subclasses}\n}", "Sealed class"},
        {"enum", "enum class ${1:EnumName} {\n    ${2:VALUE}\n}", "Enum class"},
        {"object", "object ${1:ObjectName} {\n    ${2:// code}\n}", "Object declaration"},
        {"companion", "companion object {\n    ${1:// code}\n}", "Companion object"},
        {"interface", "interface ${1:InterfaceName} {\n    ${2:// methods}\n}",
         "Interface definition"},

        // Properties
        {"val", "val ${1:name}: ${2:Type} = ${3:value}", "Immutable property"},
        {"var", "var ${1:name}: ${2:Type} = ${3:value}", "Mutable property"},
        {"lateinit", "lateinit var ${1:name}: ${2:Type}", "Lateinit property"},
        {"lazy", "val ${1:name} by lazy { ${2:value} }", "Lazy property"},

        // Control Flow
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"when", "when (${1:value}) {\n    ${2:case} -> ${3:// code}\n    else -> ${4:// code}\n}",
         "When expression"},
        {"for", "for (${1:item} in ${2:collection}) {\n    ${3:// code}\n}", "For loop"},
        {"fori", "for (${1:i} in ${2:start}..${3:end}) {\n    ${4:// code}\n}",
         "For loop with range"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do {\n    ${1:// code}\n} while (${2:condition})", "Do-while loop"},

        // Null Safety
        {"let", "${1:value}?.let { ${2:it} ->\n    ${3:// code}\n}", "Safe call with let"},
        {"elvis", "${1:value} ?: ${2:default}", "Elvis operator"},
        {"notnull", "${1:value}!!", "Not-null assertion"},
        {"safe", "${1:value}?.${2:member}", "Safe call operator"},

        // Collections
        {"list", "val ${1:list} = listOf(${2})", "List"},
        {"mutablelist", "val ${1:list} = mutableListOf<${2:Type}>()", "Mutable list"},
        {"map", "val ${1:map} = mapOf(${2:key} to ${3:value})", "Map"},
        {"mutablemap", "val ${1:map} = mutableMapOf<${2:Key}, ${3:Value}>()", "Mutable map"},
        {"set", "val ${1:set} = setOf(${2})", "Set"},
        {"mutableset", "val ${1:set} = mutableSetOf<${2:Type}>()", "Mutable set"},
        {"array", "val ${1:array} = arrayOf(${2})", "Array"},

        // Higher-Order Functions
        {"map", "${1:collection}.map { ${2:it} ->\n    ${3:it}\n}", "Map transformation"},
        {"filter", "${1:collection}.filter { ${2:it} ->\n    ${3:condition}\n}",
         "Filter collection"},
        {"fold",
         "${1:collection}.fold(${2:initial}) { ${3:acc}, ${4:item} ->\n    ${5:acc + item}\n}",
         "Fold operation"},
        {"reduce", "${1:collection}.reduce { ${2:acc}, ${3:item} ->\n    ${4:acc + item}\n}",
         "Reduce operation"},
        {"foreach", "${1:collection}.forEach { ${2:it} ->\n    ${3:// code}\n}", "ForEach"},
        {"first", "${1:collection}.first()", "First element"},
        {"firstor", "${1:collection}.firstOrNull()", "First or null"},
        {"last", "${1:collection}.last()", "Last element"},
        {"any", "${1:collection}.any { ${2:it} ->\n    ${3:condition}\n}", "Any element matches"},
        {"all", "${1:collection}.all { ${2:it} ->\n    ${3:condition}\n}", "All elements match"},
        {"find", "${1:collection}.find { ${2:it} ->\n    ${3:condition}\n}", "Find element"},
        {"groupby", "${1:collection}.groupBy { ${2:it} ->\n    ${3:key}\n}", "Group by"},

        // Scope Functions
        {"apply", "${1:object}.apply {\n    ${2:// code}\n}", "Apply scope function"},
        {"with", "with(${1:object}) {\n    ${2:// code}\n}", "With scope function"},
        {"run", "${1:object}.run {\n    ${2:// code}\n}", "Run scope function"},
        {"also", "${1:object}.also { ${2:it} ->\n    ${3:// code}\n}", "Also scope function"},

        // Coroutines
        {"launch", "launch {\n    ${1:// code}\n}", "Launch coroutine"},
        {"async", "async {\n    ${1:// code}\n}", "Async coroutine"},
        {"await", "await ${1:deferred}", "Await deferred"},
        {"coroutinescope", "coroutineScope {\n    ${1:// code}\n}", "Coroutine scope"},
        {"withcontext", "withContext(Dispatchers.${1:IO}) {\n    ${2:// code}\n}", "With context"},

        // Exception Handling
        {"try", "try {\n    ${1:// code}\n} catch (e: ${2:Exception}) {\n    ${3:// handle}\n}",
         "Try-catch"},
        {"tryf", "try {\n    ${1:// code}\n} finally {\n    ${2:// cleanup}\n}", "Try-finally"},
        {"throw", "throw ${1:Exception}(\"${2:message}\")", "Throw exception"},
        {"require", "require(${1:condition}) { \"${2:message}\" }", "Require condition"},
        {"check", "check(${1:condition}) { \"${2:message}\" }", "Check condition"},
        {"requirenotnull", "requireNotNull(${1:value}) { \"${2:message}\" }", "Require not null"},

        // String Operations
        {"string", "\"${1:text}\"", "String literal"},
        {"stringt", "\"\"\"\n${1:multiline text}\n\"\"\"", "Triple-quoted string"},
        {"stringi", "\"${1:text} ${2:$variable}\"", "String interpolation"},
        {"stringtempl", "\"${1:text} ${2:${expression}}\"", "String template"},

        // Type Checks and Casts
        {"is", "${1:value} is ${2:Type}", "Type check"},
        {"as", "${1:value} as ${2:Type}", "Unsafe cast"},
        {"assafe", "${1:value} as? ${2:Type}", "Safe cast"},

        // Delegates
        {"delegate", "var ${1:property} by ${2:delegate}", "Delegated property"},
        {"observable",
         "var ${1:property} by Delegates.observable(${2:initial}) { ${3:prop}, ${4:old}, ${5:new} "
         "->\n    ${6:// code}\n}",
         "Observable delegate"},
        {"vetoable",
         "var ${1:property} by Delegates.vetoable(${2:initial}) { ${3:prop}, ${4:old}, ${5:new} "
         "->\n    ${6:condition}\n}",
         "Vetoable delegate"},

        // Testing
        {"test", "@Test\nfun test${1:Name}() {\n    ${2:// test code}\n}", "Test function"},
        {"before", "@Before\nfun setUp() {\n    ${1:// setup}\n}", "Before test"},
        {"after", "@After\nfun tearDown() {\n    ${1:// cleanup}\n}", "After test"},
        {"assert", "assertEquals(${1:expected}, ${2:actual})", "Assert equals"},
        {"asserttrue", "assertTrue(${1:condition})", "Assert true"},
        {"assertfalse", "assertFalse(${1:condition})", "Assert false"},
        {"assertnull", "assertNull(${1:value})", "Assert null"},
        {"assertnotnull", "assertNotNull(${1:value})", "Assert not null"},

        // Common Patterns
        {"singleton", "object ${1:Singleton} {\n    ${2:// code}\n}", "Singleton object"},
        {"sealedwhen",
         "when (val ${1:value} = ${2:expression}) {\n    is ${3:Type} -> ${4:// code}\n    else -> "
         "${5:// code}\n}",
         "Sealed class when"},
        {"pair", "Pair(${1:first}, ${2:second})", "Pair"},
        {"triple", "Triple(${1:first}, ${2:second}, ${3:third})", "Triple"},
        {"destruct", "val (${1:first}, ${2:second}) = ${3:pair}", "Destructuring declaration"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"doc", "/**\n * ${1:description}\n */", "KDoc comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
