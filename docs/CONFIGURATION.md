# pnana 配置文档

> [English](CONFIGURATION_EN.md) | 中文

本文档详细说明 pnana 的配置系统和使用方法。

## 📋 目录

- [配置文件位置](#配置文件位置)
- [配置选项说明](#配置选项说明)
- [LSP 配置](#lsp-配置)
- [配置示例](#配置示例)
- [配置文件格式](#配置文件格式)

---

## 配置文件位置

pnana 的配置文件位于：

```
~/.config/pnana/config.json
```

首次运行时，如果配置文件不存在，pnana 会自动创建默认配置文件。

---

## 配置选项说明

配置文件采用**嵌套 JSON 结构**，分为 `editor`、`display`、`files`、`search`、`themes`、`plugins`、`lsp` 等节。

### editor（编辑器）

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `theme` | string | `"monokai"` | 主题，可选：`monokai`, `dracula`, `solarized-dark`, `solarized-light`, `onedark`, `nord`, `gruvbox`, `tokyo-night`, `catppuccin`, `cyberpunk`, `hacker` 等 |
| `font_size` | number | `12` | 字体大小（像素） |
| `tab_size` | number | `4` | Tab 缩进空格数 |
| `insert_spaces` | boolean | `true` | 用空格替代 Tab 字符 |
| `word_wrap` | boolean | `false` | 是否自动换行 |
| `auto_indent` | boolean | `true` | 是否自动缩进 |

### display（显示）

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `show_line_numbers` | boolean | `true` | 是否显示行号 |
| `relative_line_numbers` | boolean | `false` | 是否使用相对行号 |
| `highlight_current_line` | boolean | `true` | 是否高亮当前行 |
| `show_whitespace` | boolean | `false` | 是否显示空白字符 |
| `cursor_style` | string | `"block"` | 光标样式：`block`, `underline`, `bar`, `hollow` |
| `cursor_color` | string | `"255,255,255"` | 光标颜色（RGB，逗号分隔） |
| `cursor_blink_rate` | number | `500` | 光标闪烁间隔（毫秒），0 不闪烁 |
| `cursor_smooth` | boolean | `false` | 流动光标效果 |
| `show_helpbar` | boolean | `true` | 是否显示底部帮助栏（快捷键提示） |
| `logo_gradient` | boolean | `true` | 欢迎界面 Logo 是否使用渐变颜色 |

### files（文件）

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `encoding` | string | `"UTF-8"` | 编码：`UTF-8`, `GBK`, `GB2312`, `ASCII` |
| `line_ending` | string | `"LF"` | 行尾：`LF` (Unix), `CRLF` (Windows), `CR` (Mac) |
| `trim_trailing_whitespace` | boolean | `true` | 保存时删除行尾空白 |
| `insert_final_newline` | boolean | `true` | 保存时在文件末尾插入换行 |
| `auto_save` | boolean | `false` | 是否启用自动保存 |
| `auto_save_interval` | number | `60` | 自动保存间隔（秒） |

### search（搜索）

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `case_sensitive` | boolean | `false` | 区分大小写 |
| `whole_word` | boolean | `false` | 全词匹配 |
| `regex` | boolean | `false` | 正则表达式 |
| `wrap_around` | boolean | `true` | 循环搜索 |

---

## LSP 配置

pnana 支持通过配置文件自定义 LSP（Language Server Protocol）语言服务器。**当配置文件中某条 `servers` 的 `language_id` 与代码内置的某一语言重复时，以配置文件中的该项为准**；若配置里某项未填或不可用，会回退使用内置的对应字段。仅当 `language_id` 在内置中不存在时，该条会作为新语言追加。

### lsp（语言服务器）

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `enabled` | boolean | `true` | 是否启用 LSP。设为 `false` 可完全禁用语言服务器 |
| `completion_popup_enabled` | boolean | `true` | 是否显示代码补全提示弹窗。设为 `false` 可关闭输入时的补全弹窗 |
| `servers` | array | `[]` | 服务器配置。与内置同 language_id 时以本配置为准；空字段用内置兜底；新 language_id 会追加 |

### 单个服务器配置格式

每个 `servers` 数组元素为一个对象，包含以下字段：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `name` | string | 是 | 服务器名称（用于标识） |
| `command` | string | 是 | 启动命令（如 `clangd`、`python3`） |
| `language_id` | string | 是 | LSP 语言 ID（如 `cpp`、`python`、`go`） |
| `extensions` | array | 是 | 支持的文件扩展名，如 `[".cpp", ".c", ".h"]` |
| `args` | array | 否 | 命令行参数，如 `["-m", "pylsp"]` |
| `env` | object | 否 | 环境变量。不填时自动使用 `XDG_CACHE_HOME`、`TMPDIR` 等默认值 |

### 内置默认服务器（可被同 language_id 的配置覆盖，未配置项使用下表）

| 语言 | 命令 | 扩展名 |
|------|------|--------|
| C/C++ | `clangd` | `.cpp`, `.c`, `.h`, `.hpp`, `.cc`, `.cxx` 等 |
| Python | `python3` | `.py`, `.pyw`, `.pyi` |
| Go | `gopls` | `.go` |
| Rust | `rust-analyzer` | `.rs` |
| Java | `jdtls` | `.java` |
| TypeScript | `typescript-language-server` | `.ts`, `.tsx`, `.mts`, `.cts` |
| JavaScript | `typescript-language-server` | `.js`, `.jsx`, `.mjs`, `.cjs` |
| HTML | `html-languageserver` | `.html`, `.htm` |
| CSS | `css-languageserver` | `.css`, `.scss`, `.less`, `.sass` |
| JSON | `json-languageserver` | `.json`, `.jsonc` |
| YAML | `yaml-language-server` | `.yaml`, `.yml` |
| Markdown | `marksman` | `.md`, `.markdown` |
| Shell | `bash-language-server` | `.sh`, `.bash`, `.zsh` |

### LSP 配置示例

**使用内置默认（推荐初次使用）：**

```json
"lsp": {
  "enabled": true,
  "servers": []
}
```

**覆盖 Python 命令（如使用虚拟环境）：**

```json
"lsp": {
  "enabled": true,
  "servers": [
    {
      "name": "pylsp",
      "command": "/path/to/venv/bin/python",
      "language_id": "python",
      "extensions": [".py", ".pyw", ".pyi"],
      "args": ["-m", "pylsp"],
      "env": {}
    }
  ]
}
```

**仅启用部分语言服务器：**

```json
"lsp": {
  "enabled": true,
  "servers": [
    {
      "name": "clangd",
      "command": "clangd",
      "language_id": "cpp",
      "extensions": [".cpp", ".c", ".h", ".hpp", ".cc", ".cxx"],
      "args": [],
      "env": {}
    },
    {
      "name": "pylsp",
      "command": "python3",
      "language_id": "python",
      "extensions": [".py", ".pyw", ".pyi"],
      "args": ["-m", "pylsp"],
      "env": {}
    }
  ]
}
```

**完全禁用 LSP：**

```json
"lsp": {
  "enabled": false,
  "servers": []
}
```

**添加自定义语言服务器（如 Lua）：**

```json
"lsp": {
  "enabled": true,
  "servers": [
    {
      "name": "lua-language-server",
      "command": "lua-language-server",
      "language_id": "lua",
      "extensions": [".lua"],
      "args": [],
      "env": {}
    }
  ]
}
```

> **注意**：自定义 `servers` 时，需自行安装对应的语言服务器（如 `clangd`、`pylsp`、`gopls` 等）。修改配置后需重新加载配置或重启 pnana 生效。

---

## 配置示例

### 基础配置

```json
{
  "editor": {
    "theme": "monokai",
    "font_size": 12,
    "tab_size": 4,
    "insert_spaces": true,
    "word_wrap": false,
    "auto_indent": true
  },
  "display": {
    "show_line_numbers": true,
    "relative_line_numbers": false,
    "highlight_current_line": true,
    "show_whitespace": false,
    "cursor_style": "block",
    "cursor_color": "255,255,255",
    "cursor_blink_rate": 500,
    "cursor_smooth": false
  },
  "files": {
    "encoding": "UTF-8",
    "line_ending": "LF",
    "trim_trailing_whitespace": true,
    "insert_final_newline": true,
    "auto_save": false,
    "auto_save_interval": 60
  },
  "search": {
    "case_sensitive": false,
    "whole_word": false,
    "regex": false,
    "wrap_around": true
  },
  "themes": { "current": "monokai", "available": [] },
  "plugins": { "enabled_plugins": [] },
  "lsp": { "enabled": true, "completion_popup_enabled": true, "servers": [] }
}
```

### 开发者配置

```json
{
  "editor": {
    "theme": "dracula",
    "font_size": 14,
    "tab_size": 2,
    "insert_spaces": true,
    "word_wrap": false,
    "auto_indent": true
  },
  "display": {
    "show_line_numbers": true,
    "relative_line_numbers": true,
    "highlight_current_line": true,
    "show_whitespace": true
  },
  "files": {
    "auto_save": true,
    "auto_save_interval": 30
  }
}
```

### 写作配置

```json
{
  "editor": {
    "theme": "solarized-light",
    "font_size": 16,
    "tab_size": 2,
    "word_wrap": true,
    "auto_indent": false
  },
  "display": {
    "show_line_numbers": false,
    "highlight_current_line": false
  },
  "files": {
    "auto_save": true,
    "auto_save_interval": 60
  }
}
```

---

## 配置文件格式

配置文件使用 JSON 格式，必须符合以下要求：

1. **文件编码**：UTF-8
2. **格式**：标准 JSON 格式
3. **注释**：JSON 不支持注释，如需注释请使用外部文档

### 配置验证

pnana 在启动时会验证配置文件：
- 如果配置文件格式错误，会使用默认配置并提示用户
- 如果缺少某个配置项，会使用该配置项的默认值
- 如果配置项值无效，会使用默认值并提示用户

---

## 配置优先级

配置的优先级从高到低：

1. **用户配置文件** (`~/.config/pnana/config.json`)
2. **默认配置** - 最低优先级

---

## 配置热重载

当前版本暂不支持配置热重载，修改配置文件后需要重启 pnana 才能生效。

未来版本计划支持：
- 配置文件变更检测
- 自动重新加载配置
- 部分配置项实时生效

---

## 常见问题

### Q: 配置文件在哪里？

A: 配置文件位于 `~/.config/pnana/config.json`。如果不存在，pnana 会在首次运行时自动创建。

### Q: 如何重置为默认配置？

A: 删除或重命名配置文件，pnana 会在下次启动时重新创建默认配置。

### Q: 可以同时使用多个配置文件吗？

A: 每次只能使用一个配置文件。

### Q: 配置文件中可以添加注释吗？

A: 标准 JSON 格式不支持注释。如果需要注释，请使用外部文档记录。

### Q: 如何备份配置？

A: 直接复制 `~/.config/pnana/config.json` 文件即可。

### Q: LSP 不工作怎么办？

A: 1) 确认 `lsp.enabled` 为 `true`；2) 若使用自定义 `servers`，确保已安装对应语言服务器（如 `clangd`、`pylsp`）；3) `servers` 为空时使用内置默认，需保证系统 PATH 中能找到默认命令。

---

## 更新日志

- **v0.0.5**：初始配置系统
- 支持 JSON 格式配置文件
- **LSP 配置**：支持通过 `lsp` 节配置语言服务器，可自定义命令、扩展名、参数及环境变量

---

**注意**：本文档基于当前版本的配置系统。如有更新，请参考最新代码。

