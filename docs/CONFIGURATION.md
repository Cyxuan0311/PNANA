# pnana 配置文档

> [English](CONFIGURATION_EN.md) | 中文

本文档详细说明 pnana 的配置系统和使用方法。

## 📋 目录

- [配置文件位置](#配置文件位置)
- [配置选项说明](#配置选项说明)
- [配置示例](#配置示例)
- [配置文件格式](#配置文件格式)
- [命令行参数](#命令行参数)

---

## 配置文件位置

pnana 的配置文件位于：

```
~/.config/pnana/config.json
```

首次运行时，如果配置文件不存在，pnana 会自动创建默认配置文件。

---

## 配置选项说明

配置文件采用**嵌套 JSON 结构**，分为 `editor`、`display`、`files`、`search`、`themes`、`plugins` 等节。

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
  "plugins": { "enabled_plugins": [] }
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

## 命令行参数

pnana 支持通过命令行参数覆盖配置文件设置：

### 基本用法

```bash
# 使用默认配置
pnana

# 打开文件
pnana file.txt

# 打开多个文件
pnana file1.txt file2.cpp file3.py
```

### 配置相关参数

```bash
# 指定配置文件
pnana --config ~/.config/pnana/custom.json

# 使用特定主题
pnana --theme dracula file.txt

# 只读模式打开
pnana --readonly file.txt
```

### 参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| `--config <path>` | 指定配置文件路径 | `--config ~/.config/pnana/custom.json` |
| `--theme <name>` | 指定主题名称 | `--theme dracula` |
| `--readonly` | 以只读模式打开文件 | `--readonly file.txt` |
| `--help` | 显示帮助信息 | `--help` |
| `--version` | 显示版本信息 | `--version` |

---

## 配置优先级

配置的优先级从高到低：

1. **命令行参数** - 最高优先级
2. **用户配置文件** (`~/.config/pnana/config.json`)
3. **默认配置** - 最低优先级

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

A: 可以通过 `--config` 参数指定不同的配置文件，但每次只能使用一个。

### Q: 配置文件中可以添加注释吗？

A: 标准 JSON 格式不支持注释。如果需要注释，请使用外部文档记录。

### Q: 如何备份配置？

A: 直接复制 `~/.config/pnana/config.json` 文件即可。

---

## 更新日志

- **v0.0.5**：初始配置系统
- 支持 JSON 格式配置文件
- 支持命令行参数覆盖

---

**注意**：本文档基于当前版本的配置系统。如有更新，请参考最新代码。

