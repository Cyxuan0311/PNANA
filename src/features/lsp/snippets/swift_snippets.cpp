#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getSwiftSnippets() {
    return {
        // Functions
        {"func", "func ${1:functionName}(${2:parameters}) -> ${3:ReturnType} {\n    ${4:// code}\n}",
         "Function definition"},
        {"funcp", "func ${1:functionName}(${2:parameters}) -> ${3:ReturnType} {\n    return ${4:expression}\n}",
         "Function with return"},
        {"funcv", "func ${1:functionName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Void function"},
        {"async", "func ${1:functionName}(${2:parameters}) async -> ${3:ReturnType} {\n    ${4:// code}\n}",
         "Async function"},
        {"throws", "func ${1:functionName}(${2:parameters}) throws -> ${3:ReturnType} {\n    ${4:// code}\n}",
         "Throwing function"},
        {"main", "@main\nstruct ${1:App} {\n    static func main() {\n        ${2:// code}\n    }\n}",
         "Main entry point"},

        // Classes and Structures
        {"class", "class ${1:ClassName} {\n    ${2:// code}\n}",
         "Class definition"},
        {"struct", "struct ${1:StructName} {\n    ${2:// properties}\n}",
         "Struct definition"},
        {"enum", "enum ${1:EnumName} {\n    case ${2:case}\n}",
         "Enum definition"},
        {"enumr", "enum ${1:EnumName}: ${2:Type} {\n    case ${3:case} = ${4:value}\n}",
         "Enum with raw value"},
        {"protocol", "protocol ${1:ProtocolName} {\n    ${2:// requirements}\n}",
         "Protocol definition"},
        {"extension", "extension ${1:Type} {\n    ${2:// code}\n}",
         "Extension"},
        {"init", "init(${1:parameters}) {\n    ${2:// code}\n}",
         "Initializer"},
        {"deinit", "deinit {\n    ${1:// cleanup}\n}",
         "Deinitializer"},

        // Properties
        {"var", "var ${1:name}: ${2:Type} = ${3:value}",
         "Variable"},
        {"let", "let ${1:name}: ${2:Type} = ${3:value}",
         "Constant"},
        {"computed", "var ${1:name}: ${2:Type} {\n    get {\n        return ${3:value}\n    }\n    set {\n        ${4:value} = newValue\n    }\n}",
         "Computed property"},
        {"computedr", "var ${1:name}: ${2:Type} {\n    ${3:return value}\n}",
         "Read-only computed property"},
        {"willSet", "var ${1:name}: ${2:Type} {\n    willSet {\n        ${3:// code}\n    }\n}",
         "Property willSet"},
        {"didSet", "var ${1:name}: ${2:Type} {\n    didSet {\n        ${3:// code}\n    }\n}",
         "Property didSet"},
        {"lazy", "lazy var ${1:name}: ${2:Type} = ${3:value}",
         "Lazy property"},
        {"static", "static var ${1:name}: ${2:Type} = ${3:value}",
         "Static property"},

        // Control Flow
        {"if", "if ${1:condition} {\n    ${2:// code}\n}",
         "If statement"},
        {"ifelse", "if ${1:condition} {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"guard", "guard ${1:condition} else {\n    ${2:return}\n}",
         "Guard statement"},
        {"switch", "switch ${1:value} {\ncase ${2:case}:\n    ${3:// code}\ndefault:\n    ${4:// code}\n}",
         "Switch statement"},
        {"for", "for ${1:item} in ${2:collection} {\n    ${3:// code}\n}",
         "For-in loop"},
        {"fori", "for ${1:i} in ${2:start}..<${3:end} {\n    ${4:// code}\n}",
         "For loop with range"},
        {"while", "while ${1:condition} {\n    ${2:// code}\n}",
         "While loop"},
        {"repeat", "repeat {\n    ${1:// code}\n} while ${2:condition}",
         "Repeat-while loop"},

        // Optionals
        {"optional", "${1:Type}?",
         "Optional type"},
        {"iflet", "if let ${1:value} = ${2:optional} {\n    ${3:// code}\n}",
         "If-let unwrap"},
        {"guardlet", "guard let ${1:value} = ${2:optional} else {\n    ${3:return}\n}",
         "Guard-let unwrap"},
        {"nilcoalescing", "${1:optional} ?? ${2:default}",
         "Nil-coalescing operator"},
        {"optionalchaining", "${1:object}?.${2:property}",
         "Optional chaining"},
        {"forceunwrap", "${1:optional}!",
         "Force unwrap"},

        // Collections
        {"array", "var ${1:array}: [${2:Type}] = [${3}]",
         "Array"},
        {"dict", "var ${1:dict}: [${2:Key}: ${3:Value}] = [${4:key: value}]",
         "Dictionary"},
        {"set", "var ${1:set}: Set<${2:Type}> = [${3}]",
         "Set"},

        // Closures
        {"closure", "{ (${1:parameters}) -> ${2:ReturnType} in\n    ${3:// code}\n}",
         "Closure"},
        {"closuret", "{ ${1:parameters} in\n    ${2:// code}\n}",
         "Trailing closure"},
        {"escaping", "@escaping (${1:Type}) -> ${2:ReturnType}",
         "Escaping closure"},

        // Error Handling
        {"do", "do {\n    ${1:// code}\n} catch ${2:error} {\n    ${3:// handle}\n}",
         "Do-catch"},
        {"try", "try ${1:expression}",
         "Try expression"},
        {"try?", "try? ${1:expression}",
         "Try optional"},
        {"try!", "try! ${1:expression}",
         "Try force"},
        {"throw", "throw ${1:Error}(\"${2:message}\")",
         "Throw error"},

        // Generics
        {"generic", "${1:Type}<${2:T}>",
         "Generic type"},
        {"where", "where ${1:T}: ${2:constraint}",
         "Generic constraint"},
        {"where_protocol", "where ${1:T}: ${2:Protocol}",
         "Protocol constraint"},
        {"where_class", "where ${1:T}: ${2:Class}",
         "Class constraint"},

        // String Operations
        {"string", "\"${1:text}\"",
         "String literal"},
        {"stringm", "\"\"\"\n${1:multiline text}\n\"\"\"",
         "Multiline string"},
        {"stringi", "\"${1:text} \\(${2:variable})\"",
         "String interpolation"},
        {"stringr", "#\"${1:text}\"#",
         "Raw string"},

        // Type Casting
        {"as", "${1:value} as ${2:Type}",
         "Type cast"},
        {"as?", "${1:value} as? ${2:Type}",
         "Conditional cast"},
        {"is", "${1:value} is ${2:Type}",
         "Type check"},

        // Access Control
        {"private", "private ${1:member}",
         "Private access"},
        {"fileprivate", "fileprivate ${1:member}",
         "Fileprivate access"},
        {"internal", "internal ${1:member}",
         "Internal access"},
        {"public", "public ${1:member}",
         "Public access"},
        {"open", "open ${1:member}",
         "Open access"},

        // Property Wrappers
        {"state", "@State private var ${1:name}: ${2:Type} = ${3:value}",
         "@State property wrapper"},
        {"binding", "@Binding var ${1:name}: ${2:Type}",
         "@Binding property wrapper"},
        {"observed", "@ObservedObject var ${1:name}: ${2:Type}",
         "@ObservedObject property wrapper"},
        {"published", "@Published var ${1:name}: ${2:Type} = ${3:value}",
         "@Published property wrapper"},

        // Testing
        {"test", "func test${1:Name}() {\n    ${2:// test code}\n}",
         "Test function"},
        {"xctassert", "XCTAssert${1:Equal}(${2:expected}, ${3:actual})",
         "XCTAssert"},
        {"xctasserttrue", "XCTAssertTrue(${1:condition})",
         "XCTAssertTrue"},
        {"xctassertfalse", "XCTAssertFalse(${1:condition})",
         "XCTAssertFalse"},
        {"xctassertnil", "XCTAssertNil(${1:value})",
         "XCTAssertNil"},
        {"xctassertnotnil", "XCTAssertNotNil(${1:value})",
         "XCTAssertNotNil"},

        // Common Patterns
        {"singleton", "static let shared = ${1:ClassName}()\nprivate init() {}",
         "Singleton pattern"},
        {"builder", "func ${1:method}(${2:param}: ${3:Type}) -> Self {\n    ${4:// set}\n    return self\n}",
         "Builder pattern"},
        {"delegate", "weak var delegate: ${1:Delegate}?",
         "Weak delegate"},

        // Comments
        {"cmt", "// ${1:comment}",
         "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */",
         "Block comment"},
        {"doc", "/// ${1:description}",
         "Documentation comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana

