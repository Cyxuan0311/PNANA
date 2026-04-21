# 智能缩进功能文档

## 概述

pnana 的智能缩进模块基于 **Tree-sitter** 语法解析器实现，能够根据代码的抽象语法树（AST）自动计算正确的缩进级别。该功能参考了 [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) 的设计理念，为终端编辑器提供现代化的智能缩进体验。

## 功能特性

- **语言特定配置**：每种编程语言可以自定义缩进大小、是否使用空格等参数
- **文件后缀匹配**：通过 `file_extensions` 列表自动识别文件类型并应用对应语言的缩进规则
- **Tree-sitter 驱动**：基于 AST 语法树进行精确的缩进计算
- **智能回退**：如果 Tree-sitter 不可用或解析失败，会自动回退到基于括号的简单缩进逻辑
- **性能优化**：使用范围限制查询，确保即使在大型文件中也能快速响应
- **配置灵活**：支持通过 JSON 配置文件自定义每种语言的缩进参数

## 工作原理

### 1. Tree-sitter 解析

当按下回车键时，系统会使用 Tree-sitter 解析当前文件的抽象语法树（AST）。Tree-sitter 是一个增量解析器，能够高效地构建和维护代码的语法树结构。

### 2. 查询匹配

根据语言的 `indents.scm` 查询文件（如果存在）或硬编码逻辑，识别代码块结构。查询文件定义了哪些语法节点会影响缩进，例如：
- 代码块的开始（`{`, `(`, `[`）
- 控制流语句（`if`, `for`, `while`, `function`）
- 类定义和函数定义

### 3. 缩进计算

向上遍历 AST 节点，统计需要缩进的层级，计算最终缩进级别：
1. 从当前光标位置开始
2. 向上遍历父节点
3. 统计每个影响缩进的节点
4. 根据配置的缩进大小计算最终缩进

### 4. 应用缩进

将计算出的缩进应用到新行，确保代码结构清晰可读。

## 配置说明

### 配置文件位置

`~/.config/pnana/config.json`

### 配置格式

```json
{
  "language_indent": {
    "python": {
      "indent_size": 4,
      "insert_spaces": true,
      "smart_indent": true,
      "file_extensions": [".py", ".pyw", ".pyi", ".python"]
    },
    "cpp": {
      "indent_size": 4,
      "insert_spaces": true,
      "smart_indent": true,
      "file_extensions": [".cpp", ".cxx", ".cc", ".c++", ".hpp", ".hxx", ".hh", ".h"]
    },
    "javascript": {
      "indent_size": 2,
      "insert_spaces": true,
      "smart_indent": true,
      "file_extensions": [".js", ".jsx", ".mjs", ".cjs", ".javascript"]
    }
  }
}
```

### 配置项说明

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `indent_size` | number | `4` | 缩进空格数 |
| `insert_spaces` | boolean | `true` | 是否使用空格代替 Tab |
| `smart_indent` | boolean | `true` | 是否启用 Tree-sitter 智能缩进 |
| `file_extensions` | array\<string> | `[]` | 该语言支持的文件后缀列表 |

## 支持的语言

pnana 内置支持以下语言的智能缩进（需要编译时启用 Tree-sitter 支持）：

| 语言 | 默认缩进 | 使用空格 | 常见后缀 |
|------|----------|----------|----------|
| Python | 4 | 是 | `.py`, `.pyw`, `.pyi` |
| C/C++ | 4 | 是 | `.cpp`, `.c`, `.h`, `.hpp` |
| JavaScript | 2 | 是 | `.js`, `.jsx`, `.mjs` |
| TypeScript | 4 | 是 | `.ts`, `.tsx` |
| Go | 4 | 是 | `.go` |
| Rust | 4 | 是 | `.rs` |
| Java | 4 | 是 | `.java` |
| Ruby | 2 | 是 | `.rb` |
| PHP | 4 | 是 | `.php` |
| Lua | 2 | 是 | `.lua` |
| Swift | 4 | 是 | `.swift` |
| Kotlin | 4 | 是 | `.kt`, `.kts` |
| C# | 4 | 是 | `.cs` |
| Zig | 4 | 是 | `.zig` |
| SQL | 4 | 是 | `.sql` |
| YAML | 2 | 是 | `.yaml`, `.yml` |
| JSON | 2 | 是 | `.json`, `.jsonc` |
| HTML | 2 | 是 | `.html`, `.htm` |
| CSS | 2 | 是 | `.css`, `.scss` |
| Bash/Shell | 4 | 是 | `.sh`, `.bash`, `.zsh` |

## 编译要求

要启用 Tree-sitter 智能缩进功能，需要在编译时启用相关选项：

```bash
cmake -DBUILD_TREE_SITTER_SUPPORT=ON ..
make
```

或者使用构建脚本：

```bash
./build.sh --enable-tree-sitter
```

## 技术架构

### 核心组件

1. **AutoIndentEngine** (`auto_indent_engine.h/cpp`)
   - 智能缩进引擎主类
   - 管理 Tree-sitter 解析器
   - 提供缩进计算接口

2. **IndentQuery** (`indent_query.h/cpp`)
   - Tree-sitter 查询封装
   - 加载和管理语言的缩进查询
   - 执行 AST 查询获取缩进信息

3. **LanguageIndentConfig** (`config_manager.h`)
   - 语言缩进配置结构
   - 包含缩进大小、空格使用、智能缩进开关等

4. **ConfigManager** (`config_manager.h/cpp`)
   - 配置管理
   - 解析和保存 `language_indent` 配置段

### 缩进计算流程

```
用户按下回车
    ↓
检查 auto_indent 配置
    ↓
获取当前文件类型
    ↓
查找对应的语言配置
    ↓
Tree-sitter 是否可用？
    ├─ 是 → 使用 AST 计算缩进
    └─ 否 → 使用括号匹配回退逻辑
    ↓
应用缩进到新行
```

## 安装查询文件和动态库

### 获取 indents.scm 查询文件

pnana 使用 `.scm` 查询文件来定义每种语言的缩进规则。这些查询文件可以从 [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) 项目中获取。

#### 步骤 1：克隆 nvim-treesitter 仓库

```bash
git clone https://github.com/nvim-treesitter/nvim-treesitter.git
cd nvim-treesitter
```

#### 步骤 2：找到对应语言的查询文件

查询文件位于 `queries/<language>/` 目录下，例如：
- Python: `queries/python/indents.scm`
- C++: `queries/cpp/indents.scm`
- JavaScript: `queries/javascript/indents.scm`
- TypeScript: `queries/typescript/indents.scm`

#### 步骤 3：安装查询文件到本地

将查询文件复制到 pnana 的查询目录中：

```bash
# 创建查询目录
mkdir -p ~/.config/pnana/queries/<language>/

# 复制查询文件
cp /path/to/nvim-treesitter/queries/<language>/indents.scm ~/.config/pnana/queries/<language>/
```

**示例 - 安装 Python 查询文件：**

```bash
mkdir -p ~/.config/pnana/queries/python/
cp /path/to/nvim-treesitter/queries/python/indents.scm ~/.config/pnana/queries/python/
```

**示例 - 安装 C++ 查询文件：**

```bash
mkdir -p ~/.config/pnana/queries/cpp/
cp /path/to/nvim-treesitter/queries/cpp/indents.scm ~/.config/pnana/queries/cpp/
```

### 安装 Tree-sitter 动态库

除了查询文件，还需要安装对应语言的 Tree-sitter 解析器动态库。

#### 方法 1：使用包管理器安装（推荐）

大多数 Linux 发行版提供 Tree-sitter 解析器库：

```bash
# Ubuntu/Debian
sudo apt install libtree-sitter-dev

# Arch Linux
sudo pacman -S tree-sitter

# Fedora
sudo dnf install tree-sitter-devel
```

#### 方法 2：从源码编译

如果需要特定语言的解析器，可以从源码编译：

```bash
# 克隆语言解析器仓库（以 Python 为例）
git clone https://github.com/tree-sitter/tree-sitter-python.git
cd tree-sitter-python

# 编译动态库
make

# 安装到系统库目录
sudo cp build/*.so /usr/local/lib/
sudo ldconfig
```

#### 方法 3：使用 tree-sitter CLI

```bash
# 安装 tree-sitter CLI
npm install -g tree-sitter-cli

# 获取并编译语言解析器
tree-sitter init-config
tree-sitter build-wasm  # 或者使用 tree-sitter generate
```

### 验证安装

安装完成后，检查以下目录是否包含相应文件：

```bash
# 检查查询文件
ls ~/.config/pnana/queries/python/indents.scm

# 检查动态库
ls /usr/local/lib/libtree-sitter-python.so
```

### 支持的语言解析器仓库

以下是常用语言的 Tree-sitter 解析器仓库地址：

| 语言 | 仓库地址 |
|------|----------|
| Python | https://github.com/tree-sitter/tree-sitter-python |
| C/C++ | https://github.com/tree-sitter/tree-sitter-cpp |
| JavaScript | https://github.com/tree-sitter/tree-sitter-javascript |
| TypeScript | https://github.com/tree-sitter/tree-sitter-typescript |
| Go | https://github.com/tree-sitter/tree-sitter-go |
| Rust | https://github.com/tree-sitter/tree-sitter-rust |
| Java | https://github.com/tree-sitter/tree-sitter-java |
| Ruby | https://github.com/tree-sitter/tree-sitter-ruby |
| PHP | https://github.com/tree-sitter/tree-sitter-php |
| Lua | https://github.com/Azganoth/tree-sitter-lua |
| Swift | https://github.com/alex-pinkus/tree-sitter-swift |
| Kotlin | https://github.com/fwcd/tree-sitter-kotlin |
| C# | https://github.com/tree-sitter/tree-sitter-c-sharp |
| Zig | https://github.com/maxxnino/tree-sitter-zig |
| SQL | https://github.com/DerekStride/tree-sitter-sql |
| YAML | https://github.com/ikatyang/tree-sitter-yaml |
| HTML | https://github.com/tree-sitter/tree-sitter-html |
| CSS | https://github.com/tree-sitter/tree-sitter-css |
| Bash | https://github.com/tree-sitter/tree-sitter-bash |

## 与 nvim-treesitter 的关系

pnana 的智能缩进功能参考了 [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) 的设计理念：

1. **查询驱动**：使用 `.scm` 查询文件定义缩进规则
2. **增量解析**：Tree-sitter 的增量解析确保高效性能
3. **语言无关**：通过统一的查询接口支持多种语言
4. **可配置性**：允许用户自定义每种语言的缩进参数

## 常见问题

### Q: 为什么缩进没有生效？

A: 检查以下几点：
1. 确认编译时启用了 Tree-sitter 支持
2. 确认配置文件中 `smart_indent` 设置为 `true`
3. 确认文件后缀在对应语言的 `file_extensions` 列表中

### Q: 如何为自定义语言添加缩进支持？

A: 在配置文件中添加新的语言配置：

```json
{
  "language_indent": {
    "mylanguage": {
      "indent_size": 2,
      "insert_spaces": true,
      "smart_indent": true,
      "file_extensions": [".ml", ".mylang"]
    }
  }
}
```

### Q: 缩进计算很慢怎么办？

A: Tree-sitter 使用范围限制查询来优化性能。如果仍然感觉慢，可以：
1. 检查文件是否过大（建议 < 10MB）
2. 尝试关闭 `smart_indent` 使用简单回退逻辑
3. 确保 Tree-sitter 库是最新版本

## 相关文档

- [配置文档](CONFIGURATION.md) - 详细的配置选项说明
- [依赖文档](DEPENDENCIES.md) - Tree-sitter 依赖安装指南
- [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) - 参考项目
