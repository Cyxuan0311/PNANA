#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getGoSnippets() {
    return {
        // Functions
        {"func", "func ${1:functionName}(${2:parameters}) ${3:returnType} {\n    ${4:// code}\n}",
         "Function definition"},
        {"funcm",
         "func (${1:receiver} *${2:Type}) ${3:methodName}(${4:params}) ${5:returnType} {\n    "
         "${6:// code}\n}",
         "Method definition"},
        {"main", "func main() {\n    ${1:// code}\n}", "Main function"},
        {"init", "func init() {\n    ${1:// code}\n}", "Init function"},

        // Types and Structs
        {"type", "type ${1:TypeName} ${2:underlying}", "Type definition"},
        {"struct", "type ${1:StructName} struct {\n    ${2:Field} ${3:Type}\n}",
         "Struct definition"},
        {"interface", "type ${1:InterfaceName} interface {\n    ${2:Method}()\n}",
         "Interface definition"},

        // Variables and Constants
        {"var", "var ${1:name} ${2:type} = ${3:value}", "Variable declaration"},
        {"const", "const ${1:name} = ${2:value}", "Constant declaration"},
        {"short", "${1:name} := ${2:value}", "Short variable declaration"},

        // Conditionals
        {"if", "if ${1:condition} {\n    ${2:// code}\n}", "If statement"},
        {"ifelse", "if ${1:condition} {\n    ${2:// code}\n} else {\n    ${3:// code}\n}",
         "If-else statement"},
        {"iferr", "if ${1:err} != nil {\n    ${2:return ${1:err}}\n}", "Error check"},
        {"switch",
         "switch ${1:value} {\ncase ${2:case}:\n    ${3:// code}\ndefault:\n    ${4:// code}\n}",
         "Switch statement"},
        {"select", "select {\ncase ${1:case}:\n    ${2:// code}\ndefault:\n    ${3:// code}\n}",
         "Select statement"},

        // Loops
        {"for", "for ${1:i} := ${2:0}; ${1:i} < ${3:n}; ${1:i}++ {\n    ${4:// code}\n}",
         "For loop"},
        {"forr", "for ${1:key}, ${2:value} := range ${3:collection} {\n    ${4:// code}\n}",
         "Range loop"},
        {"fori", "for ${1:condition} {\n    ${2:// code}\n}", "For loop (while-style)"},

        // Error Handling
        {"err", "if err != nil {\n    ${1:return err}\n}", "Error handling"},
        {"errc", "if err := ${1:function}(); err != nil {\n    ${2:return err}\n}",
         "Error check inline"},
        {"panic", "panic(\"${1:message}\")", "Panic"},
        {"recover",
         "defer func() {\n    if r := recover(); r != nil {\n        ${1:// handle}\n    }\n}()",
         "Recover from panic"},

        // Defer
        {"defer", "defer ${1:function}()", "Defer statement"},
        {"deferc", "defer func() {\n    ${1:// cleanup}\n}()", "Defer closure"},

        // Goroutines and Channels
        {"go", "go ${1:function}()", "Goroutine"},
        {"chan", "make(chan ${1:type})", "Channel"},
        {"chanb", "make(chan ${1:type}, ${2:buffer})", "Buffered channel"},
        {"close", "close(${1:channel})", "Close channel"},

        // Slices and Maps
        {"slice", "${1:name} := []${2:type}{${3}}", "Slice declaration"},
        {"make", "make(${1:type}, ${2:length})", "Make function"},
        {"append", "append(${1:slice}, ${2:element})", "Append to slice"},
        {"map", "${1:name} := make(map[${2:key}]${3:value})", "Map declaration"},
        {"mapl", "${1:name} := map[${2:key}]${3:value}{${4}}", "Map literal"},

        // Pointers
        {"ptr", "&${1:value}", "Address of"},
        {"deref", "*${1:pointer}", "Dereference"},
        {"new", "new(${1:type})", "New pointer"},

        // Testing
        {"test", "func Test${1:Name}(t *testing.T) {\n    ${2:// test code}\n}", "Test function"},
        {"bench", "func Benchmark${1:Name}(b *testing.B) {\n    ${2:// benchmark code}\n}",
         "Benchmark function"},
        {"t", "t.Run(\"${1:name}\", func(t *testing.T) {\n    ${2:// test}\n})", "Subtest"},

        // Common Patterns
        {"return", "return ${1:value}", "Return statement"},
        {"returnn", "return nil", "Return nil"},
        {"returnv", "return", "Return void"},
        {"fmt", "fmt.Printf(\"${1:format}\", ${2:args})", "Printf"},
        {"fmtp", "fmt.Println(${1:value})", "Println"},

        // Comments
        {"cmt", "// ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
        {"doc", "// ${1:Name} ${2:description}", "Documentation comment"},

        // Context
        {"ctx", "ctx, cancel := context.WithCancel(${1:context.Background()})\ndefer cancel()",
         "Context with cancel"},
        {"ctxto",
         "ctx, cancel := context.WithTimeout(${1:context.Background()}, ${2:time.Second})\ndefer "
         "cancel()",
         "Context with timeout"},

        // HTTP Server
        {"httph",
         "http.HandleFunc(\"/${1:path}\", func(w http.ResponseWriter, r *http.Request) {\n    "
         "${2:// code}\n})",
         "HTTP handler function"},
        {"httpget",
         "resp, err := http.Get(${1:url})\nif err != nil {\n    ${2:return err}\n}\ndefer "
         "resp.Body.Close()",
         "HTTP GET request"},

        // Logging
        {"logp", "log.Printf(\"${1:format}\", ${2:args})", "log.Printf"},

        // Map lookup
        {"mapok", "if ${1:val}, ok := ${2:m}[${3:key}]; ok {\n    ${4:// code}\n}",
         "Map lookup with ok"},

        // Struct with JSON tag
        {"structj", "type ${1:Name} struct {\n    ${2:Field} ${3:Type} `json:\"${4:field}\"`\n}",
         "Struct with json tag"},

        // Worker goroutine
        {"worker",
         "go func() {\n    for ${1:item} := range ${2:ch} {\n        ${3:// work}\n    }\n}()",
         "Worker goroutine consuming channel"},

        // HTTP Methods
        {"httppost",
         "resp, err := http.Post(${1:url}, \"${2:application/json}\", "
         "bytes.NewBuffer(${3:data}))\nif err != nil {\n    ${4:return err}\n}\ndefer "
         "resp.Body.Close()",
         "HTTP POST request"},
        {"httpput",
         "req, err := http.NewRequest(\"PUT\", ${1:url}, bytes.NewBuffer(${2:data}))\nif err != "
         "nil {\n    ${3:return err}\n}\nreq.Header.Set(\"Content-Type\", "
         "\"${4:application/json}\")\nresp, err := http.DefaultClient.Do(req)",
         "HTTP PUT request"},
        {"httpdel",
         "req, err := http.NewRequest(\"DELETE\", ${1:url}, nil)\nif err != nil {\n    ${2:return "
         "err}\n}\nresp, err := http.DefaultClient.Do(req)",
         "HTTP DELETE request"},
        {"httpserver", "http.ListenAndServe(\"${1::8080}\", ${2:nil})", "HTTP server"},
        {"httpmux",
         "mux := http.NewServeMux()\nmux.HandleFunc(\"${1:/}\", "
         "${2:handler})\nhttp.ListenAndServe(\"${3::8080}\", mux)",
         "HTTP server with mux"},
        {"middleware",
         "func ${1:middlewareName}(next http.Handler) http.Handler {\n    return "
         "http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {\n        ${2:// "
         "middleware logic}\n        next.ServeHTTP(w, r)\n    })\n}",
         "HTTP middleware"},

        // Concurrency Patterns
        {"wg",
         "var wg sync.WaitGroup\nwg.Add(${1:1})\ngo func() {\n    defer wg.Done()\n    ${2:// "
         "code}\n}()\nwg.Wait()",
         "WaitGroup pattern"},
        {"mutex", "var mu sync.Mutex\nmu.Lock()\ndefer mu.Unlock()\n${1:// code}", "Mutex lock"},
        {"rwmutex", "var mu sync.RWMutex\nmu.RLock()\ndefer mu.RUnlock()\n${1:// code}",
         "RWMutex read lock"},
        {"once", "var once sync.Once\nonce.Do(func() {\n    ${1:// code}\n})", "sync.Once pattern"},
        {"pool",
         "var pool = sync.Pool{\n    New: func() interface{} {\n        return ${1:&Object{}}\n    "
         "},\n}",
         "sync.Pool pattern"},

        // Error Handling
        {"errnew", "errors.New(\"${1:error message}\")", "Create new error"},
        {"errf", "fmt.Errorf(\"${1:format}: %w\", ${2:err})", "Formatted error with wrapping"},
        {"erris", "if errors.Is(${1:err}, ${2:target}) {\n    ${3:// code}\n}", "Error Is check"},
        {"erras",
         "var ${1:target} ${2:ErrorType}\nif errors.As(${3:err}, &${1:target}) {\n    ${4:// "
         "code}\n}",
         "Error As check"},
        {"errwrap", "errors.Wrap(${1:err}, \"${2:context}\")", "Wrap error with context"},

        // JSON
        {"jsonm",
         "${1:data}, err := json.Marshal(${2:obj})\nif err != nil {\n    ${3:return err}\n}",
         "JSON marshal"},
        {"jsonu",
         "var ${1:obj} ${2:Type}\nerr := json.Unmarshal(${3:data}, &${1:obj})\nif err != nil {\n   "
         " ${4:return err}\n}",
         "JSON unmarshal"},
        {"jsonenc", "json.NewEncoder(${1:w}).Encode(${2:obj})", "JSON encoder"},
        {"jsondec", "json.NewDecoder(${1:r}).Decode(&${2:obj})", "JSON decoder"},

        // Time
        {"time", "time.Now()", "Current time"},
        {"timet", "time.Parse(\"${1:2006-01-02}\", ${2:timeStr})", "Parse time"},
        {"timef", "time.Now().Format(\"${1:2006-01-02 15:04:05}\")", "Format time"},
        {"ticker",
         "ticker := time.NewTicker(${1:time.Second})\ndefer ticker.Stop()\nfor t := range ticker.C "
         "{\n    ${2:// code}\n}",
         "Ticker"},
        {"timer", "timer := time.NewTimer(${1:time.Second})\ndefer timer.Stop()\n<-timer.C",
         "Timer"},
        {"sleep", "time.Sleep(${1:time.Second})", "Sleep"},

        // File Operations
        {"readfile",
         "data, err := os.ReadFile(\"${1:file.txt}\")\nif err != nil {\n    ${2:return err}\n}",
         "Read file"},
        {"writefile",
         "err := os.WriteFile(\"${1:file.txt}\", ${2:data}, ${3:0644})\nif err != nil {\n    "
         "${4:return err}\n}",
         "Write file"},
        {"openfile",
         "file, err := os.Open(\"${1:file.txt}\")\nif err != nil {\n    ${2:return err}\n}\ndefer "
         "file.Close()",
         "Open file"},
        {"createfile",
         "file, err := os.Create(\"${1:file.txt}\")\nif err != nil {\n    ${2:return "
         "err}\n}\ndefer file.Close()",
         "Create file"},
        {"readall", "data, err := io.ReadAll(${1:reader})\nif err != nil {\n    ${2:return err}\n}",
         "Read all from reader"},

        // String Operations
        {"split", "strings.Split(${1:str}, \"${2:,\"})", "Split string"},
        {"join", "strings.Join(${1:slice}, \"${2:,\"})", "Join strings"},
        {"trim", "strings.Trim(${1:str}, \"${2: }\")", "Trim string"},
        {"contains", "strings.Contains(${1:str}, \"${2:substr}\")", "Check if string contains"},
        {"hasprefix", "strings.HasPrefix(${1:str}, \"${2:prefix}\")", "Check prefix"},
        {"hassuffix", "strings.HasSuffix(${1:str}, \"${2:suffix}\")", "Check suffix"},
        {"replace", "strings.Replace(${1:str}, \"${2:old}\", \"${3:new}\", ${4:-1})",
         "Replace string"},
        {"replaceall", "strings.ReplaceAll(${1:str}, \"${2:old}\", \"${3:new}\")", "Replace all"},

        // Bytes Operations
        {"bytesbuf", "var buf bytes.Buffer\nbuf.WriteString(\"${1:text}\")", "Bytes buffer"},
        {"bytesread", "bytes.NewReader(${1:data})", "Bytes reader"},
        {"byteswrite", "var buf bytes.Buffer\nbuf.Write(${1:data})", "Write to buffer"},

        // Testing Patterns
        {"testtbl",
         "tests := []struct {\n    name string\n    ${2:// fields}\n}{\n    {\n        name: "
         "\"${3:test case}\",\n        ${4:// values}\n    },\n}\nfor _, tt := range tests {\n    "
         "t.Run(tt.name, func(t *testing.T) {\n        ${5:// test}\n    })\n}",
         "Table-driven test"},
        {"testmock",
         "type ${1:MockType} struct {\n    ${2:// mock fields}\n}\nfunc (m *${1:MockType}) "
         "${3:Method}() {\n    ${4:// mock implementation}\n}",
         "Mock type"},
        {"testcleanup", "t.Cleanup(func() {\n    ${1:// cleanup}\n})", "Test cleanup"},
        {"testhelper", "t.Helper()", "Mark test helper"},
        {"testskip", "t.Skip(\"${1:reason}\")", "Skip test"},
        {"testfatal", "t.Fatal(\"${1:message}\")", "Fatal test error"},
        {"testerror", "t.Error(\"${1:message}\")", "Test error"},

        // Common Patterns
        {"builder",
         "type ${1:Builder} struct {\n    ${2:// fields}\n}\nfunc New${1:Builder}() *${1:Builder} "
         "{\n    return &${1:Builder}{}\n}\nfunc (b *${1:Builder}) ${3:Method}(${4:param}) "
         "*${1:Builder} {\n    ${5:// set}\n    return b\n}\nfunc (b *${1:Builder}) Build() "
         "${6:Type} {\n    return ${6:Type}{${7:// fields}}\n}",
         "Builder pattern"},
        {"options",
         "type ${1:Option} func(*${2:Config})\nfunc ${3:WithOption}(${4:value}) ${1:Option} {\n    "
         "return func(c *${2:Config}) {\n        c.${5:field} = ${4:value}\n    }\n}",
         "Options pattern"},
        {"validator",
         "if ${1:value} == ${2:zero} {\n    return fmt.Errorf(\"${3:field} is required\")\n}",
         "Validation check"},

        // Sorting
        {"sort",
         "sort.Slice(${1:slice}, func(i, j int) bool {\n    return ${1:slice}[i] ${2:<} "
         "${1:slice}[j]\n})",
         "Sort slice"},
        {"sortint", "sort.Ints(${1:slice})", "Sort ints"},
        {"sortstr", "sort.Strings(${1:slice})", "Sort strings"},

        // Environment Variables
        {"env", "os.Getenv(\"${1:VAR}\")", "Get environment variable"},
        {"envset", "os.Setenv(\"${1:VAR}\", \"${2:value}\")", "Set environment variable"},
        {"envlookup", "${1:value}, ${2:ok} := os.LookupEnv(\"${3:VAR}\")",
         "Lookup environment variable"},

        // Command Execution
        {"exec",
         "cmd := exec.Command(\"${1:command}\", ${2:args}...)\ncmd.Stdout = "
         "${3:os.Stdout}\ncmd.Stderr = ${4:os.Stderr}\nerr := cmd.Run()",
         "Execute command"},
        {"execout",
         "out, err := exec.Command(\"${1:command}\", ${2:args}...).Output()\nif err != nil {\n    "
         "${3:return err}\n}",
         "Execute command and get output"},

        // Reflection
        {"reflect", "reflect.TypeOf(${1:value})", "Get type"},
        {"reflectv", "reflect.ValueOf(${1:value})", "Get value"},

        // Regex
        {"regex", "regexp.MustCompile(\"${1:pattern}\")", "Compile regex"},
        {"regexm", "${1:re}.MatchString(${2:str})", "Match regex"},
        {"regexf", "${1:re}.FindString(${2:str})", "Find regex match"},

        // URL
        {"urlp",
         "u, err := url.Parse(\"${1:http://example.com}\")\nif err != nil {\n    ${2:return "
         "err}\n}",
         "Parse URL"},
        {"urlq", "u.Query().Get(\"${1:key}\")", "Get URL query parameter"},

        // Path Operations
        {"pathjoin", "filepath.Join(${1:\"path\"}, \"${2:to\"}, \"${3:file}\")", "Join path"},
        {"pathbase", "filepath.Base(${1:path})", "Get base path"},
        {"pathdir", "filepath.Dir(${1:path})", "Get directory path"},
        {"pathext", "filepath.Ext(${1:path})", "Get file extension"},

        // Logging
        {"logf", "log.Fatalf(\"${1:format}\", ${2:args})", "log.Fatalf"},
        {"logp", "log.Panicf(\"${1:format}\", ${2:args})", "log.Panicf"},

        // Channel Patterns
        {"chansend", "${1:ch} <- ${2:value}", "Send to channel"},
        {"chanrecv", "${1:value} := <-${2:ch}", "Receive from channel"},
        {"chanclose", "close(${1:ch})", "Close channel"},
        {"chanselect",
         "select {\ncase ${1:msg} := <-${2:ch}:\n    ${3:// handle}\ncase <-${4:ctx}.Done():\n    "
         "${5:return ${4:ctx}.Err()}\n}",
         "Select with context"},

        // Context Patterns
        {"ctxval", "${1:ctx}.Value(\"${2:key}\")", "Get context value"},
        {"ctxwith", "context.WithValue(${1:ctx}, \"${2:key}\", ${3:value})", "Context with value"},
        {"ctxdeadline",
         "ctx, cancel := context.WithDeadline(${1:context.Background()}, ${2:deadline})\ndefer "
         "cancel()",
         "Context with deadline"},

        // Database Patterns (common patterns)
        {"dbquery",
         "rows, err := ${1:db}.Query(\"${2:SELECT * FROM table}\")\nif err != nil {\n    "
         "${3:return err}\n}\ndefer rows.Close()",
         "Database query"},
        {"dbexec",
         "_, err := ${1:db}.Exec(\"${2:INSERT INTO table}\", ${3:args}...)\nif err != nil {\n    "
         "${4:return err}\n}",
         "Database exec"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
