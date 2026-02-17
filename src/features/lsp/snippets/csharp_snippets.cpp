#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getCSharpSnippets() {
    return {
        // Classes and Types
        {"class", "class ${1:ClassName}\n{\n    ${2:// code}\n}", "Class definition"},
        {"interface", "interface ${1:InterfaceName}\n{\n    ${2:// methods}\n}",
         "Interface definition"},
        {"struct", "struct ${1:StructName}\n{\n    ${2:// fields}\n}", "Struct definition"},
        {"enum", "enum ${1:EnumName}\n{\n    ${2:Value}\n}", "Enum definition"},
        {"record", "record ${1:RecordName}(${2:properties});", "Record definition"},
        {"namespace", "namespace ${1:Namespace}\n{\n    ${2:// code}\n}", "Namespace definition"},

        // Methods
        {"method", "${1:public} ${2:void} ${3:MethodName}(${4:parameters})\n{\n    ${5:// code}\n}",
         "Method definition"},
        {"methoda",
         "${1:public} async Task${2:<T>} ${3:MethodName}(${4:parameters})\n{\n    ${5:// code}\n}",
         "Async method"},
        {"ctor", "public ${1:ClassName}(${2:parameters})\n{\n    ${3:// code}\n}", "Constructor"},
        {"dtor", "~${1:ClassName}()\n{\n    ${2:// cleanup}\n}", "Destructor"},
        {"prop", "public ${1:Type} ${2:PropertyName} { get; set; }", "Auto property"},
        {"propfull",
         "private ${1:Type} ${2:field};\npublic ${1:Type} ${3:PropertyName}\n{\n    get => "
         "${2:field};\n    set => ${2:field} = value;\n}",
         "Full property"},
        {"propread", "public ${1:Type} ${2:PropertyName} { get; }", "Read-only property"},
        {"propinit", "public ${1:Type} ${2:PropertyName} { get; init; }", "Init-only property"},

        // Access Modifiers
        {"public", "public ${1:member}", "Public modifier"},
        {"private", "private ${1:member}", "Private modifier"},
        {"protected", "protected ${1:member}", "Protected modifier"},
        {"internal", "internal ${1:member}", "Internal modifier"},
        {"static", "static ${1:member}", "Static modifier"},
        {"const", "const ${1:Type} ${2:NAME} = ${3:value};", "Constant"},
        {"readonly", "readonly ${1:Type} ${2:field};", "Readonly field"},

        // Control Flow
        {"if", "if (${1:condition})\n{\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition})\n{\n    ${2:// code}\n}\nelse\n{\n    ${3:// code}\n}",
         "If-else statement"},
        {"elseif", "else if (${1:condition})\n{\n    ${2:// code}\n}", "Else-if statement"},
        {"switch",
         "switch (${1:value})\n{\n    case ${2:value}:\n        ${3:// code}\n        break;\n    "
         "default:\n        ${4:// code}\n        break;\n}",
         "Switch statement"},
        {"switch_expr",
         "var ${1:result} = ${2:value} switch\n{\n    ${3:pattern} => ${4:expression},\n    _ => "
         "${5:default}\n};",
         "Switch expression"},
        {"case", "case ${1:value}:\n    ${2:// code}\n    break;", "Case statement"},
        {"for", "for (int ${1:i} = ${2:0}; ${1:i} < ${3:length}; ${1:i}++)\n{\n    ${4:// code}\n}",
         "For loop"},
        {"foreach", "foreach (var ${1:item} in ${2:collection})\n{\n    ${3:// code}\n}",
         "Foreach loop"},
        {"while", "while (${1:condition})\n{\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do\n{\n    ${1:// code}\n} while (${2:condition});", "Do-while loop"},
        {"break", "break;", "Break statement"},
        {"continue", "continue;", "Continue statement"},

        // Exception Handling
        {"try", "try\n{\n    ${1:// code}\n}\ncatch (${2:Exception} ex)\n{\n    ${3:// handle}\n}",
         "Try-catch"},
        {"tryf", "try\n{\n    ${1:// code}\n}\nfinally\n{\n    ${2:// cleanup}\n}", "Try-finally"},
        {"trycf",
         "try\n{\n    ${1:// code}\n}\ncatch (${2:Exception} ex)\n{\n    ${3:// "
         "handle}\n}\nfinally\n{\n    ${4:// cleanup}\n}",
         "Try-catch-finally"},
        {"throw", "throw new ${1:Exception}(\"${2:message}\");", "Throw exception"},
        {"throw_if_null", "ArgumentNullException.ThrowIfNull(${1:argument});", "Throw if null"},

        // LINQ
        {"where", "${1:collection}.Where(${2:x} => ${3:condition})", "Where clause"},
        {"select", "${1:collection}.Select(${2:x} => ${3:x})", "Select clause"},
        {"selectmany", "${1:collection}.SelectMany(${2:x} => ${3:x})", "SelectMany clause"},
        {"orderby", "${1:collection}.OrderBy(${2:x} => ${3:x})", "OrderBy clause"},
        {"orderbydesc", "${1:collection}.OrderByDescending(${2:x} => ${3:x})",
         "OrderByDescending clause"},
        {"groupby", "${1:collection}.GroupBy(${2:x} => ${3:x})", "GroupBy clause"},
        {"join",
         "${1:collection1}.Join(${2:collection2}, ${3:x} => ${4:x}, ${5:y} => ${6:y}, (${3:x}, "
         "${5:y}) => ${7:result})",
         "Join clause"},
        {"first", "${1:collection}.First()", "First element"},
        {"firstordefault", "${1:collection}.FirstOrDefault()", "First or default"},
        {"single", "${1:collection}.Single()", "Single element"},
        {"any", "${1:collection}.Any(${2:x} => ${3:condition})", "Any clause"},
        {"all", "${1:collection}.All(${2:x} => ${3:condition})", "All clause"},
        {"count", "${1:collection}.Count(${2:x} => ${3:condition})", "Count clause"},
        {"sum", "${1:collection}.Sum(${2:x} => ${3:x})", "Sum clause"},
        {"average", "${1:collection}.Average(${2:x} => ${3:x})", "Average clause"},
        {"max", "${1:collection}.Max(${2:x} => ${3:x})", "Max clause"},
        {"min", "${1:collection}.Min(${2:x} => ${3:x})", "Min clause"},
        {"tolist", "${1:collection}.ToList()", "To list"},
        {"toarray", "${1:collection}.ToArray()", "To array"},
        {"todict", "${1:collection}.ToDictionary(${2:x} => ${3:key}, ${4:x} => ${5:value})",
         "To dictionary"},

        // Collections
        {"list", "var ${1:list} = new List<${2:Type}>();", "List"},
        {"dict", "var ${1:dict} = new Dictionary<${2:Key}, ${3:Value}>();", "Dictionary"},
        {"hashset", "var ${1:set} = new HashSet<${2:Type}>();", "HashSet"},
        {"queue", "var ${1:queue} = new Queue<${2:Type}>();", "Queue"},
        {"stack", "var ${1:stack} = new Stack<${2:Type}>();", "Stack"},
        {"array", "${1:Type}[] ${2:array} = new ${1:Type}[${3:length}];", "Array"},

        // Async/Await
        {"async", "async Task${1:<T>} ${2:MethodName}(${3:parameters})\n{\n    ${4:// code}\n}",
         "Async method"},
        {"await", "await ${1:task};", "Await"},
        {"task", "Task.Run(() =>\n{\n    ${1:// code}\n});", "Task.Run"},
        {"task_delay", "await Task.Delay(${1:1000});", "Task delay"},
        {"task_whenall", "await Task.WhenAll(${1:task1}, ${2:task2});", "Task WhenAll"},
        {"task_whenany", "await Task.WhenAny(${1:task1}, ${2:task2});", "Task WhenAny"},

        // Attributes
        {"attr", "[${1:Attribute}]", "Attribute"},
        {"obsolete", "[Obsolete(\"${1:message}\")]", "Obsolete attribute"},
        {"serializable", "[Serializable]", "Serializable attribute"},
        {"dllimport", "[DllImport(\"${1:library.dll}\")]", "DllImport attribute"},

        // Generics
        {"generic", "${1:Type}<${2:T}>", "Generic type"},
        {"where", "where ${1:T} : ${2:constraint}", "Generic constraint"},
        {"where_class", "where ${1:T} : class", "Class constraint"},
        {"where_struct", "where ${1:T} : struct", "Struct constraint"},
        {"where_new", "where ${1:T} : new()", "New constraint"},

        // Delegates and Events
        {"delegate", "delegate ${1:ReturnType} ${2:DelegateName}(${3:parameters});", "Delegate"},
        {"action", "Action<${1:T}> ${2:action} = ${3:method};", "Action delegate"},
        {"func", "Func<${1:T}, ${2:ReturnType}> ${3:func} = ${4:method};", "Func delegate"},
        {"event", "event ${1:EventHandler} ${2:EventName};", "Event"},
        {"event_handler", "public event EventHandler ${1:EventName};", "Event handler"},

        // Nullable
        {"nullable", "${1:Type}? ${2:variable} = ${3:null};", "Nullable type"},
        {"null_coalescing", "${1:value} ?? ${2:default}", "Null coalescing operator"},
        {"null_conditional", "${1:object}?.${2:Member}", "Null conditional operator"},
        {"null_forgiving", "${1:value}!", "Null forgiving operator"},

        // Pattern Matching
        {"is", "if (${1:value} is ${2:Type} ${3:var})\n{\n    ${4:// code}\n}", "Is pattern"},
        {"switch_pattern",
         "switch (${1:value})\n{\n    case ${2:Type} ${3:var}:\n        ${4:// code}\n        "
         "break;\n}",
         "Switch pattern matching"},

        // String Operations
        {"string_format", "string.Format(\"${1:format}\", ${2:args})", "String format"},
        {"string_interp", "$\"${1:text} {${2:variable}}\"", "String interpolation"},
        {"string_verbatim", "@\"${1:text}\"", "Verbatim string"},
        {"stringbuilder", "var ${1:sb} = new StringBuilder();\n${1:sb}.Append(${2:text});",
         "StringBuilder"},

        // File I/O
        {"readalltext", "File.ReadAllText(\"${1:file.txt}\")", "Read all text"},
        {"writealltext", "File.WriteAllText(\"${1:file.txt}\", ${2:content});", "Write all text"},
        {"readalllines", "File.ReadAllLines(\"${1:file.txt}\")", "Read all lines"},
        {"writealllines", "File.WriteAllLines(\"${1:file.txt}\", ${2:lines});", "Write all lines"},
        {"streamreader", "using var ${1:reader} = new StreamReader(\"${1:file.txt}\");",
         "StreamReader"},
        {"streamwriter", "using var ${1:writer} = new StreamWriter(\"${1:file.txt}\");",
         "StreamWriter"},

        // JSON
        {"json_serialize", "JsonSerializer.Serialize(${1:object})", "Serialize to JSON"},
        {"json_deserialize", "JsonSerializer.Deserialize<${1:Type}>(${2:json})",
         "Deserialize from JSON"},
        {"json_convert", "JsonConvert.SerializeObject(${1:object})", "Newtonsoft JSON serialize"},
        {"json_deserialize_object", "JsonConvert.DeserializeObject<${1:Type}>(${2:json})",
         "Newtonsoft JSON deserialize"},

        // HTTP Client
        {"httpclient",
         "using var ${1:client} = new HttpClient();\nvar ${2:response} = await "
         "${1:client}.GetAsync(\"${3:url}\");",
         "HttpClient GET"},
        {"httpclient_post",
         "using var ${1:client} = new HttpClient();\nvar ${2:content} = new "
         "StringContent(${3:json}, Encoding.UTF8, \"application/json\");\nvar ${4:response} = "
         "await ${1:client}.PostAsync(\"${5:url}\", ${2:content});",
         "HttpClient POST"},

        // Entity Framework
        {"dbcontext",
         "public class ${1:DbContextName} : DbContext\n{\n    public DbSet<${2:Entity}> "
         "${3:Entities} { get; set; }\n}",
         "DbContext"},
        {"dbset", "public DbSet<${1:Entity}> ${2:Entities} { get; set; }", "DbSet"},

        // Testing
        {"test", "[Fact]\npublic void ${1:TestName}()\n{\n    ${2:// test code}\n}", "xUnit test"},
        {"testmstest", "[TestMethod]\npublic void ${1:TestName}()\n{\n    ${2:// test code}\n}",
         "MSTest test"},
        {"testnunit", "[Test]\npublic void ${1:TestName}()\n{\n    ${2:// test code}\n}",
         "NUnit test"},
        {"assert", "Assert.${1:Equal}(${2:expected}, ${3:actual});", "Assert statement"},
        {"assert_true", "Assert.True(${1:condition});", "Assert true"},
        {"assert_false", "Assert.False(${1:condition});", "Assert false"},
        {"assert_null", "Assert.Null(${1:value});", "Assert null"},
        {"assert_notnull", "Assert.NotNull(${1:value});", "Assert not null"},

        // Common Patterns
        {"using", "using (var ${1:resource} = new ${2:Type}())\n{\n    ${3:// code}\n}",
         "Using statement"},
        {"using_static", "using static ${1:Namespace.Class};", "Using static"},
        {"lock", "lock (${1:lockObject})\n{\n    ${2:// code}\n}", "Lock statement"},
        {"checked", "checked\n{\n    ${1:// code}\n}", "Checked block"},
        {"unchecked", "unchecked\n{\n    ${1:// code}\n}", "Unchecked block"},
        {"fixed", "fixed (${1:Type}* ${2:ptr} = &${3:value})\n{\n    ${4:// code}\n}",
         "Fixed statement"},
        {"unsafe", "unsafe\n{\n    ${1:// code}\n}", "Unsafe block"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"doc", "/// <summary>\n/// ${1:description}\n/// </summary>", "XML documentation comment"},
        {"doc_param", "/// <param name=\"${1:param}\">${2:description}</param>",
         "XML param documentation"},
        {"doc_returns", "/// <returns>${1:description}</returns>", "XML returns documentation"},
        {"doc_exception", "/// <exception cref=\"${1:Exception}\">${2:description}</exception>",
         "XML exception documentation"},

        // Main Entry Point
        {"main", "static void Main(string[] args)\n{\n    ${1:// code}\n}", "Main method"},
        {"main_async", "static async Task Main(string[] args)\n{\n    ${1:// code}\n}",
         "Async Main method"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
