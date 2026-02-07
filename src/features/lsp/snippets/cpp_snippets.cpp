#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getCppSnippets() {
    return {
        // Loops
        {"for", "for (auto& ${1:item} : ${2:container}) {\n    ${3:// code}\n}",
         "Range-based for loop"},
        {"fori", "for (size_t ${1:i} = 0; ${1:i} < ${2:n}; ++${1:i}) {\n    ${3:// code}\n}",
         "Index-based for loop"},
        {"forr", "for (int ${1:i} = ${2:n} - 1; ${1:i} >= 0; --${1:i}) {\n    ${3:// code}\n}",
         "Reverse for loop"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"dowhile", "do {\n    ${1:// code}\n} while (${2:condition});", "Do-while loop"},

        // Conditionals
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if (${1:condition}) {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"else", "else {\n    ${1:// code}\n}", "Else statement"},
        {"elseif", "else if (${1:condition}) {\n    ${2:// code}\n}", "Else-if statement"},
        {"switch",
         "switch (${1:expression}) {\ncase ${2:value}:\n    ${3:// code}\n    break;\ndefault:\n   "
         " ${4:// code}\n}",
         "Switch statement"},
        {"case", "case ${1:value}:\n    ${2:// code}\n    break;", "Case statement"},

        // Classes and Structures
        {"class",
         "class ${1:ClassName} {\npublic:\n    ${1}(${2});\n    ~${1}();\n\nprivate:\n    ${3}\n};",
         "Class definition"},
        {"struct", "struct ${1:StructName} {\n    ${2}\n};", "Struct definition"},
        {"namespace", "namespace ${1:name} {\n    ${2}\n}", "Namespace definition"},
        {"enum", "enum class ${1:EnumName} {\n    ${2}\n};", "Enum class definition"},

        // Functions
        {"func",
         "${1:return_type} ${2:function_name}(${3:parameters}) {\n    ${4:// code}\n    return "
         "${5:value};\n}",
         "Function definition"},
        {"lambda",
         "[${1:capture}](${2:params}) ${3:mutable} -> ${4:return_type} {\n    ${5:// code}\n}",
         "Lambda function"},
        {"main", "int main(int argc, char* argv[]) {\n    ${1:// code}\n    return 0;\n}",
         "Main function"},
        {"mainv", "int main() {\n    ${1:// code}\n    return 0;\n}", "Main function (no args)"},

        // Includes and Headers
        {"include", "#include \"${1:header.h}\"", "Include header"},
        {"includei", "#include <${1:header}>", "Include system header"},
        {"guard", "#ifndef ${1:HEADER_H}\n#define ${1:HEADER_H}\n\n${2}\n\n#endif // ${1:HEADER_H}",
         "Header guard"},
        {"pragma", "#pragma once\n\n${1}", "Pragma once"},

        // Smart Pointers
        {"unique", "std::unique_ptr<${1:Type}> ${2:ptr} = std::make_unique<${1:Type}>(${3});",
         "Unique pointer"},
        {"shared", "std::shared_ptr<${1:Type}> ${2:ptr} = std::make_shared<${1:Type}>(${3});",
         "Shared pointer"},
        {"weak", "std::weak_ptr<${1:Type}> ${2:ptr} = ${3:shared_ptr};", "Weak pointer"},

        // Containers
        {"vector", "std::vector<${1:Type}> ${2:vec}${3: = {${4}}};", "Vector declaration"},
        {"map", "std::map<${1:Key}, ${2:Value}> ${3:m}${4: = {${5}}};", "Map declaration"},
        {"unordered", "std::unordered_map<${1:Key}, ${2:Value}> ${3:m}${4: = {${5}}};",
         "Unordered map declaration"},
        {"set", "std::set<${1:Type}> ${2:s}${3: = {${4}}};", "Set declaration"},
        {"pair", "std::pair<${1:First}, ${2:Second}> ${3:p}${4: = {${5}, ${6}}};",
         "Pair declaration"},

        // STL Algorithms
        {"find", "std::find(${1:begin}, ${2:end}, ${3:value})", "Find algorithm"},
        {"sort", "std::sort(${1:begin}, ${2:end});", "Sort algorithm"},
        {"transform", "std::transform(${1:begin}, ${2:end}, ${3:out}, ${4:func});",
         "Transform algorithm"},

        // Error Handling
        {"try",
         "try {\n    ${1:// code}\n} catch (${2:exception}& ${3:e}) {\n    ${4:// handle}\n}",
         "Try-catch block"},
        {"throw", "throw ${1:exception}(${2:message});", "Throw exception"},

        // Templates
        {"template", "template<typename ${1:T}>\n${2}", "Template definition"},
        {"templatem",
         "template<typename ${1:T}>\n${2:return_type} ${3:function}(${4:params}) {\n    ${5:// "
         "code}\n}",
         "Template function"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},

        // I/O
        {"cout", "std::cout << ${1:value} << std::endl;", "Console output"},
        {"cin", "std::cin >> ${1:variable};", "Console input"},

        // Assertions
        {"assert", "assert(${1:condition});", "Assert statement"},
        {"static", "static_assert(${1:condition}, \"${2:message}\");", "Static assert"},

        // Pointers and Memory
        {"ifn", "if (${1:ptr} == nullptr) {\n    ${2:// handle null}\n}", "Null pointer check"},
        {"ifnn", "if (${1:ptr} != nullptr) {\n    ${2:// use pointer}\n}",
         "Not-null pointer check"},
        {"new", "${1:Type}* ${2:ptr} = new ${1:Type}(${3});", "Raw pointer allocation"},
        {"del", "delete ${1:ptr};\n${1:ptr} = nullptr;", "Safe delete pointer"},

        // Constructors / Destructors
        {"ctor",
         "${1:ClassName}::${1}( ${2:/* params */} )\n    : ${3:/* member init list */} {\n    "
         "${4:// body}\n}",
         "Constructor definition"},
        {"dtor", "${1:ClassName}::~${1}() {\n    ${2:// cleanup}\n}", "Destructor definition"},

        // RAII and Mutex
        {"mutex", "std::mutex ${1:mtx};", "Mutex declaration"},
        {"lock", "std::lock_guard<std::mutex> ${1:lock}(${2:mtx});", "Lock guard"},

        // Threading
        {"thread", "std::thread ${1:t}([&]() {\n    ${2:// code}\n});", "Thread creation"},
        {"join", "if (${1:t}.joinable()) {\n    ${1:t}.join();\n}", "Thread join"},

        // Logging / Debug
        {"dbg", "std::cout << \"${1:msg}: \" << ${2:value} << std::endl;", "Debug print"},

        // Ranged for with index
        {"foriv",
         "for (size_t ${1:i} = 0; ${1:i} < ${2:container}.size(); ++${1:i}) {\n    auto& ${3:item} "
         "= ${2:container}[${1:i}];\n    ${4:// code}\n}",
         "Index + value loop"},

        // constexpr / auto
        {"constexpr", "constexpr ${1:auto} ${2:name} = ${3:value};", "Constexpr variable"},
        {"auto", "auto ${1:name} = ${2:expression};", "Auto-deduced variable"},

        // Namespace alias
        {"nsalias", "namespace ${1:alias} = ${2:full::namespace};", "Namespace alias"},

        // C Language specific snippets
        {"malloc", "${1:Type}* ${2:ptr} = (${1:Type}*)malloc(sizeof(${1:Type}) * ${3:count});",
         "Malloc memory allocation"},
        {"calloc", "${1:Type}* ${2:ptr} = (${1:Type}*)calloc(${3:count}, sizeof(${1:Type}));",
         "Calloc memory allocation"},
        {"realloc", "${1:ptr} = (${1:Type}*)realloc(${1:ptr}, sizeof(${1:Type}) * ${2:new_size});",
         "Realloc memory reallocation"},
        {"free", "free(${1:ptr});\n${1:ptr} = NULL;", "Free memory"},
        {"typedef", "typedef ${1:original} ${2:alias};", "Type definition"},
        {"typedefs", "typedef struct ${1:StructName} {\n    ${2}\n} ${1:StructName};",
         "Typedef struct"},
        {"fopen", "FILE* ${1:fp} = fopen(\"${2:filename}\", \"${3:r}\");", "File open"},
        {"fclose", "fclose(${1:fp});", "File close"},
        {"fread", "fread(${1:buffer}, sizeof(${2:Type}), ${3:count}, ${4:fp});", "File read"},
        {"fwrite", "fwrite(${1:buffer}, sizeof(${2:Type}), ${3:count}, ${4:fp});", "File write"},
        {"fgets", "fgets(${1:buffer}, ${2:size}, ${3:fp});", "File gets string"},
        {"fprintf", "fprintf(${1:fp}, \"${2:format}\", ${3:args});", "File printf"},
        {"strlen", "strlen(${1:str})", "String length"},
        {"strcpy", "strcpy(${1:dest}, ${2:src});", "String copy"},
        {"strncpy", "strncpy(${1:dest}, ${2:src}, ${3:size});", "String copy with size"},
        {"strcmp", "strcmp(${1:str1}, ${2:str2})", "String compare"},
        {"strncmp", "strncmp(${1:str1}, ${2:str2}, ${3:size})", "String compare with size"},
        {"strcat", "strcat(${1:dest}, ${2:src});", "String concatenate"},
        {"sprintf", "sprintf(${1:buffer}, \"${2:format}\", ${3:args});", "String printf"},
        {"snprintf", "snprintf(${1:buffer}, ${2:size}, \"${3:format}\", ${4:args});",
         "String printf with size"},

        // More STL Containers
        {"deque", "std::deque<${1:Type}> ${2:dq}${3: = {${4}}};", "Deque declaration"},
        {"list", "std::list<${1:Type}> ${2:lst}${3: = {${4}}};", "List declaration"},
        {"queue", "std::queue<${1:Type}> ${2:q};", "Queue declaration"},
        {"stack", "std::stack<${1:Type}> ${2:stk};", "Stack declaration"},
        {"priority", "std::priority_queue<${1:Type}> ${2:pq};", "Priority queue declaration"},
        {"unordered_set", "std::unordered_set<${1:Type}> ${2:s}${3: = {${4}}};",
         "Unordered set declaration"},
        {"array", "std::array<${1:Type}, ${2:size}> ${3:arr}${4: = {${5}}};", "Array declaration"},
        {"tuple", "std::tuple<${1:Types}> ${2:t}${3: = {${4}}};", "Tuple declaration"},
        {"optional", "std::optional<${1:Type}> ${2:opt}${3: = ${4:value}};",
         "Optional declaration"},
        {"variant", "std::variant<${1:Types}> ${2:v}${3: = ${4:value}};", "Variant declaration"},

        // More STL Algorithms
        {"count", "std::count(${1:begin}, ${2:end}, ${3:value})", "Count algorithm"},
        {"count_if", "std::count_if(${1:begin}, ${2:end}, ${3:predicate})", "Count if algorithm"},
        {"find_if", "std::find_if(${1:begin}, ${2:end}, ${3:predicate})", "Find if algorithm"},
        {"for_each", "std::for_each(${1:begin}, ${2:end}, ${3:func});", "For each algorithm"},
        {"reverse", "std::reverse(${1:begin}, ${2:end});", "Reverse algorithm"},
        {"copy", "std::copy(${1:begin}, ${2:end}, ${3:out});", "Copy algorithm"},
        {"fill", "std::fill(${1:begin}, ${2:end}, ${3:value});", "Fill algorithm"},
        {"accumulate", "std::accumulate(${1:begin}, ${2:end}, ${3:init})", "Accumulate algorithm"},
        {"max_element", "std::max_element(${1:begin}, ${2:end})", "Max element"},
        {"min_element", "std::min_element(${1:begin}, ${2:end})", "Min element"},
        {"binary_search", "std::binary_search(${1:begin}, ${2:end}, ${3:value})", "Binary search"},
        {"lower_bound", "std::lower_bound(${1:begin}, ${2:end}, ${3:value})", "Lower bound"},
        {"upper_bound", "std::upper_bound(${1:begin}, ${2:end}, ${3:value})", "Upper bound"},
        {"remove", "std::remove(${1:begin}, ${2:end}, ${3:value})", "Remove algorithm"},
        {"remove_if", "std::remove_if(${1:begin}, ${2:end}, ${3:predicate})",
         "Remove if algorithm"},
        {"unique", "std::unique(${1:begin}, ${2:end})", "Unique algorithm"},
        {"partition", "std::partition(${1:begin}, ${2:end}, ${3:predicate})",
         "Partition algorithm"},

        // Iterators
        {"begin", "${1:container}.begin()", "Begin iterator"},
        {"end", "${1:container}.end()", "End iterator"},
        {"rbegin", "${1:container}.rbegin()", "Reverse begin iterator"},
        {"rend", "${1:container}.rend()", "Reverse end iterator"},
        {"cbegin", "${1:container}.cbegin()", "Const begin iterator"},
        {"cend", "${1:container}.cend()", "Const end iterator"},
        {"autoit", "auto ${1:it} = ${2:container}.begin();", "Auto iterator"},

        // C++11/14/17 Features
        {"move", "std::move(${1:value})", "Move semantics"},
        {"forward", "std::forward<${1:T}>(${2:value})", "Perfect forwarding"},
        {"make_pair", "std::make_pair(${1:first}, ${2:second})", "Make pair"},
        {"make_tuple", "std::make_tuple(${1:args})", "Make tuple"},
        {"tie", "std::tie(${1:vars})", "Tie variables"},
        {"emplace", "${1:container}.emplace(${2:args});", "Emplace element"},
        {"emplace_back", "${1:container}.emplace_back(${2:args});", "Emplace back"},
        {"emplace_front", "${1:container}.emplace_front(${2:args});", "Emplace front"},

        // Type Traits and SFINAE
        {"enable_if", "typename std::enable_if<${1:condition}, ${2:Type}>::type",
         "Enable if type trait"},
        {"is_same", "std::is_same<${1:T1}, ${2:T2}>::value", "Is same type trait"},
        {"is_integral", "std::is_integral<${1:T}>::value", "Is integral type trait"},
        {"is_pointer", "std::is_pointer<${1:T}>::value", "Is pointer type trait"},
        {"decay", "std::decay<${1:T}>::type", "Decay type trait"},

        // Type Casting
        {"static_cast", "static_cast<${1:Type}>(${2:value})", "Static cast"},
        {"dynamic_cast", "dynamic_cast<${1:Type}>(${2:value})", "Dynamic cast"},
        {"const_cast", "const_cast<${1:Type}>(${2:value})", "Const cast"},
        {"reinterpret_cast", "reinterpret_cast<${1:Type}>(${2:value})", "Reinterpret cast"},

        // Function Objects and Lambdas
        {"function", "std::function<${1:return_type}(${2:params})> ${3:func};", "Function wrapper"},
        {"bind", "std::bind(${1:func}, ${2:args});", "Bind function"},
        {"lambdac", "[&](${1:params}) { ${2:// code} }", "Lambda with capture by reference"},
        {"lambdav", "[=](${1:params}) { ${2:// code} }", "Lambda with capture by value"},
        {"lambdam", "[${1:vars}](${2:params}) { ${3:// code} }", "Lambda with specific capture"},

        // More Class Features
        {"operator",
         "${1:return_type} operator${2:op}(${3:params}) ${4:const} {\n    ${5:// code}\n}",
         "Operator overload"},
        {"friend", "friend ${1:declaration};", "Friend declaration"},
        {"virtual", "virtual ${1:return_type} ${2:function}(${3:params}) ${4:const} = 0;",
         "Pure virtual function"},
        {"override", "${1:return_type} ${2:function}(${3:params}) ${4:const} override;",
         "Override function"},
        {"final", "${1:return_type} ${2:function}(${3:params}) ${4:const} final;",
         "Final function"},
        {"explicit", "explicit ${1:ClassName}(${2:params});", "Explicit constructor"},
        {"delete", "${1:function} = delete;", "Deleted function"},
        {"default", "${1:function} = default;", "Default function"},

        // Move Semantics
        {"move_ctor",
         "${1:ClassName}(${1}&& ${2:other}) noexcept\n    : ${3:/* move members */} {\n    ${4:// "
         "body}\n}",
         "Move constructor"},
        {"move_assign",
         "${1:ClassName}& operator=(${1}&& ${2:other}) noexcept {\n    ${3:// move assignment}\n   "
         " return *this;\n}",
         "Move assignment operator"},

        // More Loops and Control Flow
        {"break", "break;", "Break statement"},
        {"continue", "continue;", "Continue statement"},
        {"return", "return ${1:value};", "Return statement"},
        {"returnv", "return;", "Return void"},
        {"goto", "goto ${1:label};", "Goto statement"},
        {"label", "${1:label}:", "Label"},

        // Preprocessor
        {"define", "#define ${1:MACRO} ${2:value}", "Define macro"},
        {"undef", "#undef ${1:MACRO}", "Undefine macro"},
        {"ifdef", "#ifdef ${1:MACRO}\n${2:// code}\n#endif", "If defined"},
        {"ifndef", "#ifndef ${1:MACRO}\n${2:// code}\n#endif", "If not defined"},
        {"ifc", "#if ${1:condition}\n${2:// code}\n#endif", "If condition"},
        {"elif", "#elif ${1:condition}\n${2:// code}", "Else if"},
        {"elsep", "#else\n${1:// code}", "Else preprocessor"},
        {"error", "#error \"${1:message}\"", "Error directive"},
        {"warning", "#warning \"${1:message}\"", "Warning directive"},
        {"line", "#line ${1:number} \"${2:file}\"", "Line directive"},

        // Attributes (C++11/17)
        {"nodiscard", "[[nodiscard]] ${1:return_type} ${2:function}(${3:params});",
         "Nodiscard attribute"},
        {"maybe_unused", "[[maybe_unused]] ${1:variable};", "Maybe unused attribute"},
        {"deprecated", "[[deprecated]] ${1:declaration};", "Deprecated attribute"},
        {"fallthrough", "[[fallthrough]];", "Fallthrough attribute"},

        // More Error Handling
        {"noexcept", "${1:function} noexcept;", "Noexcept specification"},
        {"noexcepte", "${1:function} noexcept(${2:condition});", "Noexcept with condition"},
        {"catch_all", "catch (...) {\n    ${1:// handle any exception}\n}", "Catch all exceptions"},
        {"catch_const", "catch (const ${1:exception}& ${2:e}) {\n    ${3:// handle}\n}",
         "Catch const reference"},

        // More Memory Management
        {"new_arr", "${1:Type}* ${2:ptr} = new ${1:Type}[${3:size}];", "New array"},
        {"delete_arr", "delete[] ${1:ptr};\n${1:ptr} = nullptr;", "Delete array"},
        {"make_unique_arr", "auto ${1:ptr} = std::make_unique<${2:Type}[]>(${3:size});",
         "Make unique array"},
        {"make_shared_arr", "auto ${1:ptr} = std::make_shared<${2:Type}[]>(${3:size});",
         "Make shared array"},

        // More String Operations
        {"string", "std::string ${1:str}${2: = \"${3}\"};", "String declaration"},
        {"wstring", "std::wstring ${1:str}${2: = L\"${3}\"};", "Wide string declaration"},
        {"u8string", "std::u8string ${1:str}${2: = u8\"${3}\"};", "UTF-8 string declaration"},
        {"u16string", "std::u16string ${1:str}${2: = u\"${3}\"};", "UTF-16 string declaration"},
        {"u32string", "std::u32string ${1:str}${2: = U\"${3}\"};", "UTF-32 string declaration"},
        {"stringstream", "std::stringstream ${1:ss};", "String stream"},
        {"ostringstream", "std::ostringstream ${1:oss};", "Output string stream"},
        {"istringstream", "std::istringstream ${1:iss}(${2:str});", "Input string stream"},

        // More I/O
        {"cerr", "std::cerr << ${1:value} << std::endl;", "Error output"},
        {"clog", "std::clog << ${1:value} << std::endl;", "Log output"},
        {"getline", "std::getline(${1:stream}, ${2:str});", "Get line"},
        {"printf", "printf(\"${1:format}\", ${2:args});", "Printf"},
        {"scanf", "scanf(\"${1:format}\", ${2:&args});", "Scanf"},

        // More Threading
        {"async", "auto ${1:fut} = std::async(std::launch::async, ${2:func}, ${3:args});",
         "Async execution"},
        {"future", "std::future<${1:Type}> ${2:fut};", "Future declaration"},
        {"promise", "std::promise<${1:Type}> ${2:prom};", "Promise declaration"},
        {"packaged_task", "std::packaged_task<${1:Type}()> ${2:task}(${3:func});", "Packaged task"},
        {"condition", "std::condition_variable ${1:cv};", "Condition variable"},
        {"wait", "${1:cv}.wait(${2:lock}, ${3:predicate});", "Wait on condition"},
        {"notify_one", "${1:cv}.notify_one();", "Notify one"},
        {"notify_all", "${1:cv}.notify_all();", "Notify all"},
        {"atomic", "std::atomic<${1:Type}> ${2:var}${3: = ${4:value}};", "Atomic variable"},
        {"lock_guard", "std::lock_guard<std::mutex> ${1:lock}(${2:mtx});", "Lock guard"},
        {"unique_lock", "std::unique_lock<std::mutex> ${1:lock}(${2:mtx});", "Unique lock"},
        {"scoped_lock", "std::scoped_lock ${1:lock}(${2:mtx1}, ${3:mtx2});", "Scoped lock"},

        // Chrono
        {"sleep_for", "std::this_thread::sleep_for(std::chrono::${1:seconds}(${2:1}));",
         "Sleep for duration"},
        {"sleep_until", "std::this_thread::sleep_until(${1:time_point});", "Sleep until time"},
        {"now", "std::chrono::steady_clock::now()", "Current time point"},
        {"duration", "std::chrono::${1:seconds} ${2:d}(${3:1});", "Duration"},

        // Filesystem (C++17)
        {"path", "std::filesystem::path ${1:p}${2: = \"${3}\"};", "Filesystem path"},
        {"exists", "std::filesystem::exists(${1:path})", "Check if path exists"},
        {"is_directory", "std::filesystem::is_directory(${1:path})", "Check if directory"},
        {"is_regular_file", "std::filesystem::is_regular_file(${1:path})", "Check if regular file"},

        // More C Language Features
        {"enumc", "enum ${1:EnumName} {\n    ${2}\n};", "C-style enum"},
        {"union", "union ${1:UnionName} {\n    ${2}\n};", "Union definition"},
        {"volatile", "volatile ${1:type} ${2:var};", "Volatile variable"},
        {"register", "register ${1:type} ${2:var};", "Register variable"},
        {"extern", "extern ${1:type} ${2:var};", "External declaration"},
        {"staticc", "static ${1:type} ${2:var};", "Static variable"},
        {"constc", "const ${1:type} ${2:var} = ${3:value};", "Const variable"},
        {"restrict", "${1:type}* restrict ${2:ptr};", "Restrict pointer"},
        {"inlinec", "inline ${1:return_type} ${2:function}(${3:params});", "Inline function"},

        // More C String Functions
        {"memset", "memset(${1:ptr}, ${2:value}, ${3:size});", "Memory set"},
        {"memcpy", "memcpy(${1:dest}, ${2:src}, ${3:size});", "Memory copy"},
        {"memmove", "memmove(${1:dest}, ${2:src}, ${3:size});", "Memory move"},
        {"memcmp", "memcmp(${1:ptr1}, ${2:ptr2}, ${3:size})", "Memory compare"},
        {"strstr", "strstr(${1:haystack}, ${2:needle})", "String search"},
        {"strchr", "strchr(${1:str}, ${2:ch})", "String character search"},
        {"strrchr", "strrchr(${1:str}, ${2:ch})", "String reverse character search"},
        {"strtok", "strtok(${1:str}, ${2:delim})", "String tokenize"},
        {"strdup", "strdup(${1:str})", "String duplicate"},
        {"strncat", "strncat(${1:dest}, ${2:src}, ${3:size});", "String concatenate with size"},

        // More C File Operations
        {"fscanf", "fscanf(${1:fp}, \"${2:format}\", ${3:&args});", "File scanf"},
        {"fseek", "fseek(${1:fp}, ${2:offset}, ${3:SEEK_SET});", "File seek"},
        {"ftell", "ftell(${1:fp})", "File tell"},
        {"rewind", "rewind(${1:fp});", "Rewind file"},
        {"feof", "feof(${1:fp})", "File end of file"},
        {"ferror", "ferror(${1:fp})", "File error"},
        {"fflush", "fflush(${1:fp});", "File flush"},
        {"remove", "remove(\"${1:filename}\")", "Remove file"},
        {"rename", "rename(\"${1:old}\", \"${2:new}\")", "Rename file"},

        // Utility
        {"size", "sizeof(${1:type})", "Size of type"},
        {"alignof", "alignof(${1:type})", "Alignment of type"},
        {"offsetof", "offsetof(${1:type}, ${2:member})", "Offset of member"},
        {"typeid", "typeid(${1:value})", "Type ID"},
        {"decltype", "decltype(${1:expression})", "Decltype"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
