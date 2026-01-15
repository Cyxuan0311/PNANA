# 语法高亮测试文件

这个目录包含了多种编程语言的示例文件，用于测试 pnana 编辑器的语法高亮功能。

## 支持的语言

### 编译型语言
- **C** (`hello.c`) - 系统编程语言的基础
- **C++** (`hello.cpp`) - 面向对象的系统编程语言
- **Go** (`hello.go`) - Google开发的现代系统编程语言
- **Rust** (`hello.rs`) - 安全且高性能的系统编程语言
- **Swift** (`hello.swift`) - Apple开发的现代编程语言
- **Java** (`hello.java`) - 企业级面向对象编程语言
- **Scala** (`hello.scala`) - 函数式编程与面向对象编程的融合
- **Kotlin** (`hello.kt`) - JVM上的现代编程语言
- **Haskell** (`hello.hs`) - 纯函数式编程语言

### 脚本语言
- **Python** (`hello.py`) - 简洁易读的动态语言
- **JavaScript** (`hello.js`) - Web开发的核心语言
- **TypeScript** (`hello.ts`) - JavaScript的类型化超集
- **Ruby** (`hello.rb`) - 面向对象的脚本语言
- **PHP** (`hello.php`) - Web后端开发语言
- **Perl** (`hello.pl`) - 强大的文本处理语言
- **Lua** (`hello.lua`) - 轻量级嵌入式脚本语言

### 函数式语言
- **Clojure** (`hello.clj`) - JVM上的Lisp方言
- **Elixir** (`hello.ex`) - Erlang VM上的现代函数式语言

### 配置和标记语言
- **YAML** (`config.yaml`) - 人类可读的数据序列化格式
- **JSON** (`package.json`) - JavaScript对象表示法
- **Markdown** (`README.md`) - 轻量级标记语言

### 其他
- **SQL** (`schema.sql`) - 数据库查询语言
- **Shell** (`build.sh`) - Unix shell脚本
- **Makefile** (`Makefile`) - 构建系统描述文件
- **Dockerfile** (`Dockerfile`) - Docker容器构建文件

## 功能展示

每个语言文件都包含了该语言的核心特性示例：

- 基本数据类型
- 控制结构（循环、条件判断）
- 函数定义和调用
- 类和对象（面向对象语言）
- 集合和数据结构操作
- 错误处理
- 文件操作
- 并发/并行编程
- 语言特定的高级特性

## 测试语法高亮

要测试语法高亮功能：

1. 启动 pnana 编辑器
2. 打开任意一个语言文件
3. 观察代码的颜色高亮和语法结构

每个文件都设计为展示该语言的典型用法和语法结构，帮助验证语法高亮器的正确性。

## 扩展支持

如果需要添加更多语言的示例文件，可以：

1. 按照现有文件的命名约定创建新文件
2. 实现该语言的核心语法特性
3. 更新相应的语法高亮器配置
4. 测试高亮效果

## 注意事项

- 某些语言可能需要特定的编译器或运行时环境
- 一些高级特性可能需要特定的库或框架支持
- 所有示例都是为了展示语法而设计的教学性代码

这个目录包含了各种编程语言的示例文件，用于测试语法高亮功能。

## 支持的语言

### 编程语言
- **C** (`hello.c`) - C语言基础语法示例
- **C++** (`hello.cpp`) - C++现代特性示例，包括类、模板、智能指针等
- **Python** (`hello.py`) - Python语法特性，包括装饰器、生成器、类型提示等
- **JavaScript** (`hello.js`) - ES6+ JavaScript特性，包括async/await、模板字符串等
- **TypeScript** (`hello.ts`) - 虽然文件不存在，但高亮器支持
- **Java** (`hello.java`) - 虽然文件不存在，但高亮器支持
- **Go** (`hello.go`) - 虽然文件不存在，但高亮器支持
- **Rust** (`hello.rs`) - 虽然文件不存在，但高亮器支持
- **Ruby** (`hello.rb`) - Ruby面向对象编程示例
- **PHP** (`hello.php`) - PHP 8+ 特性示例，包括枚举、特征等
- **Swift** (`hello.swift`) - Swift现代语法特性示例
- **Kotlin** (`hello.kt`) - 虽然文件不存在，但高亮器支持
- **Scala** (`hello.scala`) - 虽然文件不存在，但高亮器支持
- **R** (`hello.r`) - 虽然文件不存在，但高亮器支持
- **Perl** (`hello.pl`) - 虽然文件不存在，但高亮器支持

### 脚本语言
- **Shell/Bash** (`build.sh`) - Bash脚本示例，包含函数、条件判断、循环等

### 标记语言
- **YAML** (`config.yaml`) - YAML配置文件示例
- **Markdown** (`README.md`) - 本文件就是Markdown示例
- **XML** (`*.xml`) - 虽然文件不存在，但高亮器支持
- **HTML** (`*.html`) - 虽然文件不存在，但高亮器支持
- **CSS** (`*.css`) - 虽然文件不存在，但高亮器支持

### 查询语言
- **SQL** (`schema.sql`) - SQL数据库模式定义示例

### 配置文件
- **JSON** (`package.json`) - Node.js项目配置文件

### 构建工具
- **CMake** (`*.cmake`) - 虽然文件不存在，但高亮器支持
- **Makefile** (`*.mk`) - 虽然文件不存在，但高亮器支持
- **Dockerfile** (`*.dockerfile`) - 虽然文件不存在，但高亮器支持

## 文件类型检测

编辑器根据文件扩展名自动检测语言类型：

| 扩展名 | 语言 | 说明 |
|--------|------|------|
| `.c` | C | C语言源文件 |
| `.cpp`, `.cc`, `.cxx`, `.c++` | C++ | C++源文件 |
| `.h`, `.hpp`, `.hxx`, `.hh` | C/C++ | C/C++头文件 |
| `.py`, `.pyw`, `.pyi` | Python | Python脚本和类型存根 |
| `.js`, `.jsx`, `.mjs` | JavaScript | JavaScript和JSX文件 |
| `.ts`, `.tsx` | TypeScript | TypeScript和TSX文件 |
| `.java` | Java | Java源文件 |
| `.go` | Go | Go源文件 |
| `.rs` | Rust | Rust源文件 |
| `.rb`, `.rake`, `.gemspec` | Ruby | Ruby脚本和配置文件 |
| `.php`, `.phtml`, `.php3`, `.php4`, `.php5`, `.php7` | PHP | PHP脚本文件 |
| `.swift` | Swift | Swift源文件 |
| `.kt`, `.kotlin`, `.kts` | Kotlin | Kotlin源文件 |
| `.scala`, `.sc` | Scala | Scala源文件 |
| `.r`, `.rmd`, `.rscript` | R | R语言脚本 |
| `.pl`, `.pm`, `.perl`, `.pod` | Perl | Perl脚本和模块 |
| `.sh`, `.bash`, `.zsh`, `.shell` | Shell | Shell脚本 |
| `.yaml`, `.yml` | YAML | YAML配置文件 |
| `.json`, `.jsonc` | JSON | JSON数据文件 |
| `.xml`, `.svg`, `.xhtml` | XML | XML和SVG文件 |
| `.html`, `.htm` | HTML | HTML文件 |
| `.css`, `.scss`, `.sass`, `.less` | CSS | CSS样式文件 |
| `.sql`, `.mysql`, `.postgresql`, `.sqlite` | SQL | SQL数据库脚本 |
| `.cmake` | CMake | CMake构建脚本 |
| `.tcl`, `.tk` | TCL | TCL脚本 |
| `.fortran`, `.f`, `.f90`, `.f95`, `.f03`, `.f08`, `.for`, `.ftn`, `.fpp` | Fortran | Fortran源文件 |
| `.hs`, `.lhs`, `.haskell` | Haskell | Haskell源文件 |
| `.lua` | Lua | Lua脚本 |
| `.vim`, `.vimrc` | Vim | Vim脚本 |
| `.powershell`, `.ps1`, `.psm1`, `.psd1` | PowerShell | PowerShell脚本 |
| `Makefile`, `makefile`, `*.mk` | Makefile | Make构建文件 |
| `Dockerfile`, `*.dockerfile`, `containerfile` | Dockerfile | Docker构建文件 |

## 特殊文件名检测

除了扩展名检测，编辑器还支持特殊文件名检测：

- `CMakeLists.txt`, `cmake.in`, `cmake.in.in` → CMake
- `Portfile` → TCL (MacPorts)
- `Dockerfile`, `containerfile` → Dockerfile
- `Makefile`, `makefile` → Makefile
- `.vimrc` → Vim

## 语法高亮特性

每个语言文件都包含了该语言的主要语法特性：

- **关键字**：语言保留字
- **类型**：内置类型和自定义类型
- **字符串**：各种字符串字面量
- **注释**：单行和多行注释
- **数字**：整数、浮点数、科学计数法等
- **操作符**：算术、逻辑、比较操作符
- **函数**：函数定义和调用
- **类和对象**：面向对象编程特性
- **控制结构**：条件判断、循环等

## 测试方法

1. 在编辑器中打开任意示例文件
2. 查看语法高亮是否正确显示
3. 验证不同语法元素的颜色区分
4. 测试Tree-sitter（如果已安装）和原生语法高亮器的差异

## 扩展支持

要添加新的语言支持：

1. 在`syntax_highlighter.cpp`中添加新的tokenize函数
2. 在`syntax_highlighter.h`中声明函数
3. 在`editor.cpp`的`getFileType()`函数中添加文件类型检测
4. 更新CMakeLists.txt以支持Tree-sitter库链接
5. 在`statusbar.cpp`中添加文件类型图标

## 注意事项

- Tree-sitter提供更准确的语法分析，但需要安装相应的语言库
- 原生语法高亮器提供基础支持，适用于所有语言
- 某些高级语言特性可能在原生高亮器中无法完美识别
- 建议优先使用Tree-sitter以获得最佳语法高亮体验
