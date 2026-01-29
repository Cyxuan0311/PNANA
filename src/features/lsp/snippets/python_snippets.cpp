#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getPythonSnippets() {
    return {
        // Functions
        {"def", "def ${1:function_name}(${2:parameters}):\n    ${3:pass}", "Function definition"},
        {"defa", "def ${1:function_name}(${2:parameters}) -> ${3:return_type}:\n    ${4:pass}",
         "Function with type annotation"},
        {"lambda", "lambda ${1:params}: ${2:expression}", "Lambda function"},
        {"async", "async def ${1:function_name}(${2:parameters}):\n    ${3:pass}",
         "Async function definition"},
        {"main", "if __name__ == \"__main__\":\n    ${1:main()}", "Main guard"},

        // Classes
        {"class",
         "class ${1:ClassName}:\n    def __init__(self${2:, parameters}):\n        ${3:pass}",
         "Class definition"},
        {"classi",
         "class ${1:ClassName}(${2:BaseClass}):\n    def __init__(self${3:, parameters}):\n        "
         "super().__init__()\n        ${4:pass}",
         "Class with inheritance"},
        {"init", "def __init__(self${1:, parameters}):\n    ${2:pass}", "__init__ method"},
        {"str", "def __str__(self):\n    return ${1:f\"${2}\"}", "__str__ method"},
        {"repr", "def __repr__(self):\n    return ${1:f\"${2}\"}", "__repr__ method"},
        {"property", "@property\ndef ${1:name}(self):\n    return self._${1:name}",
         "Property decorator"},

        // Conditionals
        {"if", "if ${1:condition}:\n    ${2:pass}", "If statement"},
        {"elif", "elif ${1:condition}:\n    ${2:pass}", "Elif statement"},
        {"else", "else:\n    ${1:pass}", "Else statement"},
        {"ifelse", "if ${1:condition}:\n    ${2:pass}\nelse:\n    ${3:pass}", "If-else statement"},
        {"ternary", "${1:value} if ${2:condition} else ${3:other}", "Ternary operator"},

        // Loops
        {"for", "for ${1:item} in ${2:iterable}:\n    ${3:pass}", "For loop"},
        {"fore", "for ${1:item} in enumerate(${2:iterable}):\n    ${3:pass}",
         "For loop with enumerate"},
        {"forr", "for ${1:item} in range(${2:start}, ${3:end}):\n    ${4:pass}",
         "For loop with range"},
        {"forz", "for ${1:item1}, ${2:item2} in zip(${3:iter1}, ${4:iter2}):\n    ${5:pass}",
         "For loop with zip"},
        {"while", "while ${1:condition}:\n    ${2:pass}", "While loop"},
        {"break", "break", "Break statement"},
        {"continue", "continue", "Continue statement"},

        // Error Handling
        {"try", "try:\n    ${1:pass}\nexcept ${2:Exception} as ${3:e}:\n    ${4:pass}",
         "Try-except block"},
        {"tryf",
         "try:\n    ${1:pass}\nexcept ${2:Exception} as ${3:e}:\n    ${4:pass}\nfinally:\n    "
         "${5:pass}",
         "Try-except-finally block"},
        {"raise", "raise ${1:Exception}(${2:message})", "Raise exception"},
        {"assert", "assert ${1:condition}, \"${2:message}\"", "Assert statement"},

        // Imports
        {"import", "import ${1:module}", "Import statement"},
        {"from", "from ${1:module} import ${2:name}", "From import statement"},
        {"froma", "from ${1:module} import ${2:name} as ${3:alias}", "From import with alias"},

        // Data Structures
        {"list", "${1:name} = [${2}]", "List declaration"},
        {"dict", "${1:name} = {${2:key}: ${3:value}}", "Dictionary declaration"},
        {"set", "${1:name} = {${2}}", "Set declaration"},
        {"tuple", "${1:name} = (${2},)", "Tuple declaration"},
        {"listc", "[${1:x} for ${2:x} in ${3:iterable}${4: if ${5:condition}}]",
         "List comprehension"},
        {"dictc", "{${1:k}: ${2:v} for ${3:k}, ${4:v} in ${5:items}${6: if ${7:condition}}}",
         "Dictionary comprehension"},

        // Decorators
        {"decorator", "@${1:decorator}\ndef ${2:function}(${3:params}):\n    ${4:pass}",
         "Function decorator"},
        {"staticmethod", "@staticmethod\ndef ${1:method}(${2:params}):\n    ${3:pass}",
         "Static method"},
        {"classmethod", "@classmethod\ndef ${1:method}(cls${2:, params}):\n    ${3:pass}",
         "Class method"},

        // Context Managers
        {"with", "with ${1:resource} as ${2:alias}:\n    ${3:pass}", "With statement"},
        {"open", "with open('${1:file}', '${2:r}') as ${3:f}:\n    ${4:content = f.read()}",
         "File open"},

        // Generators
        {"yield", "yield ${1:value}", "Yield statement"},
        {"generator", "def ${1:generator}():\n    yield ${2:value}", "Generator function"},

        // Type Hints
        {"type", "${1:var}: ${2:type} = ${3:value}", "Type annotation"},
        {"typelist", "${1:var}: list[${2:type}] = []", "List type annotation"},
        {"typedict", "${1:var}: dict[${2:key}, ${3:value}] = {}", "Dict type annotation"},

        // Testing
        {"test", "def test_${1:name}():\n    ${2:assert True}", "Test function"},
        {"pytest", "def test_${1:name}():\n    ${2:assert True}", "Pytest test function"},

        // Comments
        {"cmt", "# ${1:comment}", "Single line comment"},
        {"docstring", "\"\"\"\n${1:docstring}\n\"\"\"", "Docstring"},

        // String Formatting
        {"fstring", "f\"${1:{variable}}\"", "F-string"},
        {"format", "\"${1:text}\".format(${2:variable})", "String format"},

        // Common Patterns
        {"ifmain", "if __name__ == \"__main__\":\n    ${1:main()}", "Main guard"},
        {"print", "print(${1:value})", "Print statement"},
        {"return", "return ${1:value}", "Return statement"},
        {"pass", "pass", "Pass statement"},

        // Dataclasses and Named Tuples
        {"dataclass", "@dataclass\nclass ${1:Name}:\n    ${2:field}: ${3:type} = ${4:default}",
         "Dataclass definition"},
        {"namedtuple",
         "from collections import namedtuple\n${1:Name} = namedtuple('${1:Name}', ['${2:field1}', "
         "'${3:field2}'])",
         "namedtuple definition"},

        // Argparse
        {"argparse",
         "import argparse\n\nparser = "
         "argparse.ArgumentParser(description='${1:description}')\nparser.add_argument('${2:--flag}"
         "', help='${3:help}')\nargs = parser.parse_args()\n${4:print(args)}",
         "Argparse basic setup"},

        // Logging
        {"logging",
         "import logging\n\nlogging.basicConfig(level=logging.${1:INFO})\nlogger = "
         "logging.getLogger(__name__)\n\nlogger.${2:info}('${3:message}')",
         "Logging setup and usage"},

        // Async helpers
        {"asyncfor", "async for ${1:item} in ${2:aiterable}:\n    ${3:pass}", "Async for loop"},
        {"asyncwith", "async with ${1:resource} as ${2:alias}:\n    ${3:pass}", "Async with block"},

        // Context manager class
        {"contextmanagerclass",
         "class ${1:Context}:\n    def __enter__(self):\n        ${2:return self}\n\n    def "
         "__exit__(self, exc_type, exc, tb):\n        ${3:pass}",
         "Context manager class"},

        // List / Dict comprehension with condition
        {"listcf", "[${1:x} for ${2:x} in ${3:iterable} if ${4:condition}]",
         "List comprehension with filter"},
        {"dictcf", "{${1:k}: ${2:v} for ${3:k}, ${4:v} in ${5:items} if ${6:condition}}",
         "Dict comprehension with filter"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
