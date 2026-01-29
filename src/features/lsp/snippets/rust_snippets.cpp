#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getRustSnippets() {
    return {
        // Functions
        {"fn", "fn ${1:function_name}(${2:parameters}) -> ${3:return_type} {\n    ${4:// code}\n}",
         "Function definition"},
        {"fnm",
         "fn ${1:function_name}(&mut self${2:, params}) -> ${3:return_type} {\n    ${4:// code}\n}",
         "Mutable method"},
        {"fns",
         "fn ${1:function_name}(&self${2:, params}) -> ${3:return_type} {\n    ${4:// code}\n}",
         "Immutable method"},
        {"async",
         "async fn ${1:function_name}(${2:parameters}) -> ${3:return_type} {\n    ${4:// code}\n}",
         "Async function"},
        {"main", "fn main() {\n    ${1:// code}\n}", "Main function"},

        // Structs and Enums
        {"struct", "struct ${1:StructName} {\n    ${2:field}: ${3:type},\n}", "Struct definition"},
        {"enum", "enum ${1:EnumName} {\n    ${2:Variant},\n}", "Enum definition"},
        {"impl", "impl ${1:Type} {\n    ${2:// methods}\n}", "Implementation block"},
        {"trait", "trait ${1:TraitName} {\n    fn ${2:method}(${3:&self});\n}", "Trait definition"},
        {"implt", "impl ${1:TraitName} for ${2:Type} {\n    ${3:// methods}\n}",
         "Trait implementation"},

        // Ownership and Borrowing
        {"ref", "&${1:value}", "Borrow"},
        {"refm", "&mut ${1:value}", "Mutable borrow"},
        {"box", "Box::new(${1:value})", "Box allocation"},
        {"rc", "Rc::new(${1:value})", "Reference counted"},
        {"arc", "Arc::new(${1:value})", "Atomically reference counted"},

        // Pattern Matching
        {"match",
         "match ${1:value} {\n    ${2:pattern} => ${3:expression},\n    _ => ${4:default},\n}",
         "Match expression"},
        {"iflet", "if let ${1:pattern} = ${2:value} {\n    ${3:// code}\n}", "If-let pattern"},
        {"whilelet", "while let ${1:pattern} = ${2:value} {\n    ${3:// code}\n}",
         "While-let pattern"},
        {"let", "let ${1:variable} = ${2:value};", "Let binding"},
        {"letm", "let mut ${1:variable} = ${2:value};", "Mutable let binding"},

        // Error Handling
        {"result", "Result<${1:Ok}, ${2:Err}>", "Result type"},
        {"option", "Option<${1:Type}>", "Option type"},
        {"ok", "Ok(${1:value})", "Ok variant"},
        {"err", "Err(${1:error})", "Err variant"},
        {"some", "Some(${1:value})", "Some variant"},
        {"none", "None", "None variant"},
        {"unwrap", "${1:result}.unwrap()", "Unwrap result"},
        {"expect", "${1:result}.expect(\"${2:message}\")", "Expect with message"},
        {"unwrapor", "${1:option}.unwrap_or(${2:default})", "Unwrap or default"},
        {"?", "${1:result}?", "Question mark operator"},

        // Collections
        {"vec", "Vec::new()", "Vector"},
        {"vecm", "vec![${1}]", "Vector macro"},
        {"hashmap", "HashMap::new()", "HashMap"},
        {"hashset", "HashSet::new()", "HashSet"},
        {"string", "String::new()", "String"},
        {"str", "\"${1:text}\"", "String literal"},

        // Iterators
        {"iter", "${1:collection}.iter()", "Iterator"},
        {"iterm", "${1:collection}.iter_mut()", "Mutable iterator"},
        {"into", "${1:collection}.into_iter()", "Into iterator"},
        {"map", "${1:iter}.map(|${2:x}| ${3:x})", "Map iterator"},
        {"filter", "${1:iter}.filter(|${2:x}| ${3:condition})", "Filter iterator"},
        {"collect", "${1:iter}.collect::<${2:Vec<_>>()", "Collect iterator"},
        {"for", "for ${1:item} in ${2:iterable} {\n    ${3:// code}\n}", "For loop"},

        // Conditionals and Loops
        {"if", "if ${1:condition} {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if ${1:condition} {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"while", "while ${1:condition} {\n    ${2:// code}\n}", "While loop"},
        {"loop", "loop {\n    ${1:// code}\n}", "Infinite loop"},
        {"break", "break;", "Break statement"},
        {"continue", "continue;", "Continue statement"},

        // Closures
        {"closure", "|${1:params}| ${2:expression}", "Closure"},
        {"closureb", "|${1:params}| {\n    ${2:// code}\n}", "Closure with block"},
        {"move", "move |${1:params}| ${2:expression}", "Move closure"},

        // Macros
        {"println", "println!(\"${1:text}\");", "Print line macro"},
        {"print", "print!(\"${1:text}\");", "Print macro"},
        {"format", "format!(\"${1:text}\", ${2:args})", "Format macro"},
        {"panic", "panic!(\"${1:message}\");", "Panic macro"},
        {"assert", "assert!(${1:condition});", "Assert macro"},
        {"assert_eq", "assert_eq!(${1:left}, ${2:right});", "Assert equal macro"},

        // Modules
        {"mod", "mod ${1:module} {\n    ${2:// code}\n}", "Module definition"},
        {"use", "use ${1:path};", "Use statement"},
        {"pub", "pub ${1:item}", "Public visibility"},
        {"pubc", "pub const ${1:NAME}: ${2:type} = ${3:value};", "Public constant"},

        // Common Patterns
        {"return", "return ${1:value};", "Return statement"},
        {"self", "self", "Self reference"},
        {"selfm", "&mut self", "Mutable self reference"},
        {"selfs", "&self", "Immutable self reference"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"doc", "/// ${1:documentation}", "Documentation comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
