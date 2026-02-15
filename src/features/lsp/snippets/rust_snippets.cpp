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

        // Testing
        {"test", "#[test]\nfn test_${1:name}() {\n    ${2:// test code}\n}",
         "Test function"},
        {"testmod", "#[cfg(test)]\nmod tests {\n    use super::*;\n    ${1:// tests}\n}",
         "Test module"},
        {"shouldpanic", "#[should_panic]", "Should panic attribute"},
        {"ignore", "#[ignore]", "Ignore test attribute"},

        // Async/Await
        {"await", "${1:future}.await", "Await future"},
        {"tokio", "#[tokio::main]\nasync fn main() {\n    ${1:// code}\n}",
         "Tokio main"},
        {"spawn", "tokio::spawn(async {\n    ${1:// code}\n});",
         "Spawn async task"},
        {"join", "tokio::join!(${1:future1}, ${2:future2})",
         "Join futures"},
        {"select", "tokio::select! {\n    ${1:result} = ${2:future} => ${3:// handle},\n}",
         "Select from futures"},

        // Error Handling Advanced
        {"map_err", "${1:result}.map_err(|${2:e}| ${3:error})",
         "Map error"},
        {"and_then", "${1:result}.and_then(|${2:x}| ${3:Ok(x)})",
         "And then"},
        {"or_else", "${1:result}.or_else(|${2:e}| ${3:Ok(default)})",
         "Or else"},
        {"map", "${1:option}.map(|${2:x}| ${3:x})",
         "Map option"},
        {"ok_or", "${1:option}.ok_or(${2:error})",
         "Ok or error"},
        {"ok_or_else", "${1:option}.ok_or_else(|| ${2:error})",
         "Ok or else"},

        // Smart Pointers
        {"refcell", "RefCell::new(${1:value})",
         "RefCell"},
        {"mutex", "Mutex::new(${1:value})",
         "Mutex"},
        {"rwlock", "RwLock::new(${1:value})",
         "RwLock"},
        {"cell", "Cell::new(${1:value})",
         "Cell"},

        // Collections Advanced
        {"btree", "BTreeMap::new()",
         "BTreeMap"},
        {"btreeset", "BTreeSet::new()",
         "BTreeSet"},
        {"vecdeque", "VecDeque::new()",
         "VecDeque"},
        {"linkedlist", "LinkedList::new()",
         "LinkedList"},
        {"binaryheap", "BinaryHeap::new()",
         "BinaryHeap"},

        // Iterator Advanced
        {"fold", "${1:iter}.fold(${2:init}, |${3:acc}, ${4:x}| ${5:acc + x})",
         "Fold iterator"},
        {"reduce", "${1:iter}.reduce(|${2:acc}, ${3:x}| ${4:acc + x})",
         "Reduce iterator"},
        {"find", "${1:iter}.find(|${2:x}| ${3:condition})",
         "Find in iterator"},
        {"any", "${1:iter}.any(|${2:x}| ${3:condition})",
         "Any in iterator"},
        {"all", "${1:iter}.all(|${2:x}| ${3:condition})",
         "All in iterator"},
        {"enumerate", "${1:iter}.enumerate()",
         "Enumerate iterator"},
        {"zip", "${1:iter1}.zip(${2:iter2})",
         "Zip iterators"},
        {"chain", "${1:iter1}.chain(${2:iter2})",
         "Chain iterators"},
        {"take", "${1:iter}.take(${2:n})",
         "Take n items"},
        {"skip", "${1:iter}.skip(${2:n})",
         "Skip n items"},
        {"rev", "${1:iter}.rev()",
         "Reverse iterator"},

        // String Operations
        {"to_string", "${1:value}.to_string()",
         "Convert to string"},
        {"as_str", "${1:string}.as_str()",
         "Get string slice"},
        {"push_str", "${1:string}.push_str(\"${2:text}\")",
         "Push string"},
        {"push", "${1:string}.push('${2:c}')",
         "Push char"},
        {"split", "${1:string}.split(\"${2:delimiter}\")",
         "Split string"},
        {"trim", "${1:string}.trim()",
         "Trim string"},
        {"contains", "${1:string}.contains(\"${2:substring}\")",
         "Check contains"},
        {"starts_with", "${1:string}.starts_with(\"${2:prefix}\")",
         "Check starts with"},
        {"ends_with", "${1:string}.ends_with(\"${2:suffix}\")",
         "Check ends with"},
        {"replace", "${1:string}.replace(\"${2:from}\", \"${3:to}\")",
         "Replace string"},
        {"parse", "${1:string}.parse::<${2:Type}>()",
         "Parse string"},

        // File I/O
        {"read", "std::fs::read_to_string(\"${1:file.txt}\")",
         "Read file to string"},
        {"write", "std::fs::write(\"${1:file.txt}\", ${2:contents})",
         "Write file"},
        {"read_lines", "std::fs::read_to_string(\"${1:file.txt}\")?.lines()",
         "Read lines"},
        {"open", "std::fs::File::open(\"${1:file.txt}\")?",
         "Open file"},
        {"create", "std::fs::File::create(\"${1:file.txt}\")?",
         "Create file"},

        // Serialization
        {"serde", "#[derive(Serialize, Deserialize)]",
         "Serde derive"},
        {"serde_json", "serde_json::to_string(&${1:value})?",
         "Serialize to JSON"},
        {"serde_json_parse", "serde_json::from_str::<${1:Type}>(${2:json})?",
         "Deserialize from JSON"},
        {"serde_json_value", "serde_json::json!(${1:value})",
         "JSON value macro"},

        // Concurrency
        {"thread", "std::thread::spawn(|| {\n    ${1:// code}\n})",
         "Spawn thread"},
        {"thread_join", "${1:handle}.join()?",
         "Join thread"},
        {"channel", "let (${1:tx}, ${2:rx}) = mpsc::channel();",
         "Create channel"},
        {"send", "${1:sender}.send(${2:value})?",
         "Send to channel"},
        {"recv", "${1:receiver}.recv()?",
         "Receive from channel"},
        {"try_recv", "${1:receiver}.try_recv()",
         "Try receive from channel"},

        // Path Operations
        {"path", "Path::new(\"${1:path}\")",
         "Create path"},
        {"pathbuf", "PathBuf::from(\"${1:path}\")",
         "Create path buffer"},
        {"exists", "${1:path}.exists()",
         "Check path exists"},
        {"is_file", "${1:path}.is_file()",
         "Check is file"},
        {"is_dir", "${1:path}.is_dir()",
         "Check is directory"},

        // Time
        {"duration", "Duration::from_secs(${1:seconds})",
         "Create duration"},
        {"instant", "Instant::now()",
         "Get current instant"},
        {"elapsed", "${1:instant}.elapsed()",
         "Get elapsed time"},
        {"sleep", "std::thread::sleep(Duration::from_secs(${1:1}))",
         "Sleep thread"},

        // Logging
        {"log", "log::info!(\"${1:message}\");",
         "Log info"},
        {"log_debug", "log::debug!(\"${1:message}\");",
         "Log debug"},
        {"log_error", "log::error!(\"${1:message}\");",
         "Log error"},
        {"log_warn", "log::warn!(\"${1:message}\");",
         "Log warning"},

        // HTTP (using reqwest)
        {"reqwest_get", "reqwest::get(\"${1:url}\").await?",
         "HTTP GET request"},
        {"reqwest_post", "reqwest::Client::new().post(\"${1:url}\").json(&${2:data}).send().await?",
         "HTTP POST request"},

        // Database (using sqlx)
        {"sqlx_query", "sqlx::query(\"${1:SELECT * FROM table}\").fetch_all(&${2:pool}).await?",
         "SQLx query"},
        {"sqlx_query_as", "sqlx::query_as::<_, ${1:Type}>(\"${2:SELECT * FROM table}\").fetch_all(&${3:pool}).await?",
         "SQLx query as type"},

        // Common Patterns
        {"new", "impl ${1:Type} {\n    pub fn new() -> Self {\n        Self {\n            ${2:// fields}\n        }\n    }\n}",
         "New function"},
        {"default", "impl Default for ${1:Type} {\n    fn default() -> Self {\n        Self {\n            ${2:// fields}\n        }\n    }\n}",
         "Default trait"},
        {"display", "impl Display for ${1:Type} {\n    fn fmt(&self, f: &mut Formatter) -> Result {\n        write!(f, \"${2:{}}\", ${3:self.field})\n    }\n}",
         "Display trait"},
        {"debug", "#[derive(Debug)]",
         "Debug derive"},
        {"clone", "#[derive(Clone)]",
         "Clone derive"},
        {"copy", "#[derive(Copy, Clone)]",
         "Copy derive"},
        {"eq", "#[derive(PartialEq, Eq)]",
         "Equality derive"},
        {"ord", "#[derive(PartialOrd, Ord)]",
         "Ordering derive"},
        {"hash", "#[derive(Hash)]",
         "Hash derive"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
