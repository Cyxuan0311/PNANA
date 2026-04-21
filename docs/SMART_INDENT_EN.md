# Smart Indent Feature Documentation

## Overview

pnana's smart indent module is implemented using the **Tree-sitter** syntax parser, which automatically calculates the correct indentation level based on the code's Abstract Syntax Tree (AST). This feature draws inspiration from the design philosophy of [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter), providing a modern smart indent experience for terminal editors.

## Features

- **Language-Specific Configuration**: Each programming language can customize indent size, space usage, and other parameters
- **File Extension Matching**: Automatically identifies file types via the `file_extensions` list and applies corresponding language indent rules
- **Tree-sitter Driven**: Precise indent calculation based on AST syntax trees
- **Smart Fallback**: If Tree-sitter is unavailable or parsing fails, automatically falls back to simple bracket-based indent logic
- **Performance Optimized**: Uses range-limited queries to ensure fast response even in large files
- **Flexible Configuration**: Supports customizing indent parameters for each language via JSON configuration files

## How It Works

### 1. Tree-sitter Parsing

When you press Enter, the system uses Tree-sitter to parse the current file's Abstract Syntax Tree (AST). Tree-sitter is an incremental parser that efficiently builds and maintains the code's syntax tree structure.

### 2. Query Matching

Based on the language's `indents.scm` query file (if available) or hard-coded logic, it identifies code block structures. Query files define which syntax nodes affect indentation, such as:
- Code block starts (`{`, `(`, `[`)
- Control flow statements (`if`, `for`, `while`, `function`)
- Class and function definitions

### 3. Indent Calculation

Traverses AST nodes upward, counts indentation levels, and calculates the final indent level:
1. Start from the current cursor position
2. Traverse parent nodes upward
3. Count each node that affects indentation
4. Calculate the final indent based on the configured indent size

### 4. Apply Indent

Apply the calculated indentation to the new line, ensuring clear and readable code structure.

## Configuration

### Configuration File Location

`~/.config/pnana/config.json`

### Configuration Format

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

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `indent_size` | number | `4` | Number of spaces for indentation |
| `insert_spaces` | boolean | `true` | Use spaces instead of Tab characters |
| `smart_indent` | boolean | `true` | Enable Tree-sitter smart indent |
| `file_extensions` | array\<string> | `[]` | List of file extensions supported by this language |

## Supported Languages

pnana has built-in support for smart indentation in the following languages (requires Tree-sitter support enabled at compile time):

| Language | Default Indent | Use Spaces | Common Extensions |
|----------|----------------|------------|-------------------|
| Python | 4 | Yes | `.py`, `.pyw`, `.pyi` |
| C/C++ | 4 | Yes | `.cpp`, `.c`, `.h`, `.hpp` |
| JavaScript | 2 | Yes | `.js`, `.jsx`, `.mjs` |
| TypeScript | 4 | Yes | `.ts`, `.tsx` |
| Go | 4 | Yes | `.go` |
| Rust | 4 | Yes | `.rs` |
| Java | 4 | Yes | `.java` |
| Ruby | 2 | Yes | `.rb` |
| PHP | 4 | Yes | `.php` |
| Lua | 2 | Yes | `.lua` |
| Swift | 4 | Yes | `.swift` |
| Kotlin | 4 | Yes | `.kt`, `.kts` |
| C# | 4 | Yes | `.cs` |
| Zig | 4 | Yes | `.zig` |
| SQL | 4 | Yes | `.sql` |
| YAML | 2 | Yes | `.yaml`, `.yml` |
| JSON | 2 | Yes | `.json`, `.jsonc` |
| HTML | 2 | Yes | `.html`, `.htm` |
| CSS | 2 | Yes | `.css`, `.scss` |
| Bash/Shell | 4 | Yes | `.sh`, `.bash`, `.zsh` |

## Build Requirements

To enable Tree-sitter smart indent functionality, you need to enable the relevant option during compilation:

```bash
cmake -DBUILD_TREE_SITTER_SUPPORT=ON ..
make
```

Or use the build script:

```bash
./build.sh --enable-tree-sitter
```

## Technical Architecture

### Core Components

1. **AutoIndentEngine** (`auto_indent_engine.h/cpp`)
   - Main smart indent engine class
   - Manages Tree-sitter parsers
   - Provides indent calculation interfaces

2. **IndentQuery** (`indent_query.h/cpp`)
   - Tree-sitter query wrapper
   - Loads and manages language indent queries
   - Executes AST queries to obtain indent information

3. **LanguageIndentConfig** (`config_manager.h`)
   - Language indent configuration structure
   - Contains indent size, space usage, smart indent toggle, etc.

4. **ConfigManager** (`config_manager.h/cpp`)
   - Configuration management
   - Parses and saves `language_indent` configuration section

### Indent Calculation Flow

```
User presses Enter
    ↓
Check auto_indent configuration
    ↓
Get current file type
    ↓
Find corresponding language configuration
    ↓
Is Tree-sitter available?
    ├─ Yes → Calculate indent using AST
    └─ No → Use bracket matching fallback logic
    ↓
Apply indent to new line
```

## Installing Query Files and Dynamic Libraries

### Getting indents.scm Query Files

pnana uses `.scm` query files to define indentation rules for each language. These query files can be obtained from the [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) project.

#### Step 1: Clone the nvim-treesitter Repository

```bash
git clone https://github.com/nvim-treesitter/nvim-treesitter.git
cd nvim-treesitter
```

#### Step 2: Find Query Files for Your Language

Query files are located in the `queries/<language>/` directory, for example:
- Python: `queries/python/indents.scm`
- C++: `queries/cpp/indents.scm`
- JavaScript: `queries/javascript/indents.scm`
- TypeScript: `queries/typescript/indents.scm`

#### Step 3: Install Query Files Locally

Copy the query files to pnana's query directory:

```bash
# Create query directory
mkdir -p ~/.config/pnana/queries/<language>/

# Copy query file
cp /path/to/nvim-treesitter/queries/<language>/indents.scm ~/.config/pnana/queries/<language>/
```

**Example - Installing Python Query Files:**

```bash
mkdir -p ~/.config/pnana/queries/python/
cp /path/to/nvim-treesitter/queries/python/indents.scm ~/.config/pnana/queries/python/
```

**Example - Installing C++ Query Files:**

```bash
mkdir -p ~/.config/pnana/queries/cpp/
cp /path/to/nvim-treesitter/queries/cpp/indents.scm ~/.config/pnana/queries/cpp/
```

### Installing Tree-sitter Dynamic Libraries

In addition to query files, you also need to install the Tree-sitter parser dynamic libraries for the corresponding languages.

#### Method 1: Install via Package Manager (Recommended)

Most Linux distributions provide Tree-sitter parser libraries:

```bash
# Ubuntu/Debian
sudo apt install libtree-sitter-dev

# Arch Linux
sudo pacman -S tree-sitter

# Fedora
sudo dnf install tree-sitter-devel
```

#### Method 2: Build from Source

If you need parsers for specific languages, you can build them from source:

```bash
# Clone the language parser repository (Python example)
git clone https://github.com/tree-sitter/tree-sitter-python.git
cd tree-sitter-python

# Build the dynamic library
make

# Install to system library directory
sudo cp build/*.so /usr/local/lib/
sudo ldconfig
```

#### Method 3: Using tree-sitter CLI

```bash
# Install tree-sitter CLI
npm install -g tree-sitter-cli

# Get and compile language parsers
tree-sitter init-config
tree-sitter build-wasm  # or use tree-sitter generate
```

### Verifying Installation

After installation, check that the following directories contain the corresponding files:

```bash
# Check query files
ls ~/.config/pnana/queries/python/indents.scm

# Check dynamic libraries
ls /usr/local/lib/libtree-sitter-python.so
```

### Supported Language Parser Repositories

Here are the Tree-sitter parser repository URLs for commonly used languages:

| Language | Repository URL |
|----------|----------------|
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

## Relationship with nvim-treesitter

pnana's smart indent feature draws inspiration from [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter):

1. **Query-Driven**: Uses `.scm` query files to define indent rules
2. **Incremental Parsing**: Tree-sitter's incremental parsing ensures efficient performance
3. **Language-Agnostic**: Supports multiple languages through a unified query interface
4. **Configurability**: Allows users to customize indent parameters for each language

## Frequently Asked Questions

### Q: Why isn't indentation working?

A: Check the following:
1. Confirm Tree-sitter support was enabled during compilation
2. Confirm `smart_indent` is set to `true` in the configuration file
3. Confirm the file extension is in the corresponding language's `file_extensions` list

### Q: How do I add indent support for a custom language?

A: Add a new language configuration in the configuration file:

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

### Q: What if indent calculation is slow?

A: Tree-sitter uses range-limited queries to optimize performance. If it still feels slow:
1. Check if the file is too large (recommended < 10MB)
2. Try disabling `smart_indent` to use simple fallback logic
3. Ensure the Tree-sitter library is up to date

## Related Documentation

- [Configuration Documentation](CONFIGURATION_EN.md) - Detailed configuration options
- [Dependencies Documentation](DEPENDENCIES_EN.md) - Tree-sitter dependency installation guide
- [nvim-treesitter](https://github.com/nvim-treesitter/nvim-treesitter) - Reference project
