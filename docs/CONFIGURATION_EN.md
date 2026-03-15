# pnana Configuration

> [中文](CONFIGURATION.md) | English

This document describes pnana's configuration system and usage.

---

## Table of Contents

- [Configuration File Location](#configuration-file-location)
- [Configuration Options](#configuration-options)
- [LSP Configuration](#lsp-configuration)
- [Configuration Examples](#configuration-examples)
- [Configuration File Format](#configuration-file-format)

---

## Configuration File Location

The configuration file is located at:

```
~/.config/pnana/config.json
```

On first run, if the configuration file does not exist, pnana will create it with default values.

---

## Configuration Options

The configuration uses a **nested JSON structure** with sections: `editor`, `display`, `files`, `search`, `themes`, `plugins`, and `lsp`.

### editor

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `theme` | string | `"monokai"` | Theme; options include `monokai`, `dracula`, `solarized-dark`, `solarized-light`, `onedark`, `nord`, `gruvbox`, `tokyo-night`, `catppuccin`, `cyberpunk`, `hacker`, etc. |
| `font_size` | number | `12` | Font size (pixels) |
| `tab_size` | number | `4` | Number of spaces for Tab indent |
| `insert_spaces` | boolean | `true` | Use spaces instead of Tab character |
| `word_wrap` | boolean | `false` | Enable word wrap |
| `auto_indent` | boolean | `true` | Enable auto indent |

### display

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `show_line_numbers` | boolean | `true` | Show line numbers |
| `relative_line_numbers` | boolean | `false` | Use relative line numbers |
| `highlight_current_line` | boolean | `true` | Highlight current line |
| `show_whitespace` | boolean | `false` | Show whitespace characters |
| `cursor_style` | string | `"block"` | Cursor style: `block`, `underline`, `bar`, `hollow` |
| `cursor_color` | string | `"255,255,255"` | Cursor color (RGB, comma-separated) |
| `cursor_blink_rate` | number | `500` | Cursor blink interval (ms); 0 for no blink |
| `cursor_smooth` | boolean | `false` | Smooth cursor effect |
| `show_helpbar` | boolean | `true` | showing buttom help bar (true or false) |
| `logo_gradient` | boolean | `true` | Use gradient colors for welcome screen Logo |
| `file_browser_side` | string | `"left"` | Position of file list panel relative to code area: `"left"` or `"right"` |
| `ai_panel_side` | string | `"right"` | Position of AI assistant side panel relative to code area: `"left"` or `"right"` |
| `terminal_side` | string | `"bottom"` | Position of integrated terminal relative to code area: `"bottom"` (default) or `"top"` |
| `logo_style` | string | `"default"` | Logo style: `"default"`, `"ascii"`, `"big-ascii"` |
| `statusbar_style` | string | `"default"` | Statusbar style: `"default"`, `"neovim"`, `"vscode"`, `"minimal"`, `"classic"`, `"highlight"` |

### themes

The `themes` section controls the current theme, optional theme list, and user-defined custom themes.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `current` | string | `"monokai"` | Name of the active theme at startup. Must match a built‑in theme, a plugin theme, or a custom theme name. |
| `available` | array\<string> | `[]` (auto) | Optional list of theme names to show in the theme menu. If empty or omitted, all built‑in themes are shown; custom themes are appended automatically. |
| `custom` | object | `{}` | Optional map of custom theme definitions keyed by theme name. Each value is a color object; see below. |

Each entry under `themes.custom` is an object with RGB arrays (0–255) for various UI elements. All fields are optional but these are recommended:

```json
"themes": {
  "current": "my-dark",
  "available": ["monokai", "dracula", "my-dark"],
  "custom": {
    "my-dark": {
      "background": [10, 10, 10],
      "foreground": [230, 230, 230],
      "current_line": [25, 25, 25],
      "selection": [40, 40, 40],
      "line_number": [120, 120, 120],
      "line_number_current": [230, 230, 230],
      "statusbar_bg": [20, 20, 20],
      "statusbar_fg": [230, 230, 230],
      "menubar_bg": [20, 20, 20],
      "menubar_fg": [230, 230, 230],
      "helpbar_bg": [20, 20, 20],
      "helpbar_fg": [150, 150, 150],
      "helpbar_key": [166, 226, 46],
      "keyword": [249, 38, 114],
      "string": [230, 219, 116],
      "comment": [117, 113, 94],
      "number": [174, 129, 255],
      "function": [166, 226, 46],
      "type": [102, 217, 239],
      "operator": [249, 38, 114],
      "error": [249, 38, 114],
      "warning": [253, 151, 31],
      "info": [102, 217, 239],
      "success": [166, 226, 46]
    }
  }
}
```

Any field you omit falls back to the internal default colors. Once defined, custom themes appear in the theme menu alongside built‑in themes and can be previewed/selected normally.

### files

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `encoding` | string | `"UTF-8"` | Encoding: `UTF-8`, `GBK`, `GB2312`, `ASCII` |
| `line_ending` | string | `"LF"` | Line ending: `LF` (Unix), `CRLF` (Windows), `CR` (Mac) |
| `trim_trailing_whitespace` | boolean | `true` | Trim trailing whitespace on save |
| `insert_final_newline` | boolean | `true` | Insert newline at end of file on save |
| `auto_save` | boolean | `false` | Enable auto save |
| `auto_save_interval` | number | `60` | Auto save interval (seconds) |
| `max_file_size_before_prompt_mb` | number | `50` | Maximum file size before prompt when opening (MB) |

### search

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `case_sensitive` | boolean | `false` | Case sensitive search |
| `whole_word` | boolean | `false` | Whole word match |
| `regex` | boolean | `false` | Use regular expressions |
| `wrap_around` | boolean | `true` | Wrap around when searching |

---

## LSP Configuration

pnana supports customizing LSP (Language Server Protocol) servers via the configuration file. **When a `servers` entry has the same `language_id` as a built-in language, the config file entry takes precedence.** If a field is left empty or invalid, the built-in value for that field is used as fallback. Entries with a `language_id` not present in the built-in list are appended as new languages.

### lsp (Language Servers)

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enabled` | boolean | `true` | Enable or disable LSP. Set to `false` to disable all language servers |
| `completion_popup_enabled` | boolean | `true` | Show code completion popup when typing. Set to `false` to disable the completion popup |
| `servers` | array | `[]` | Server configs. Same language_id as built-in overrides built-in; empty fields fall back to built-in; new language_id appends |

### Server Entry Format

Each element in the `servers` array is an object with these fields:

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | yes | Server name (for identification) |
| `command` | string | yes | Command to start the server (e.g. `clangd`, `python3`) |
| `language_id` | string | yes | LSP language ID (e.g. `cpp`, `python`, `go`) |
| `extensions` | array | yes | Supported file extensions, e.g. `[".cpp", ".c", ".h"]` |
| `args` | array | no | Command-line arguments, e.g. `["-m", "pylsp"]` |
| `env` | object | no | Environment variables. If empty, defaults like `XDG_CACHE_HOME` and `TMPDIR` are used |

### Built-in Default Servers (overridden by same language_id in config; unset fields use below)

| Language | Command | Extensions |
|----------|---------|------------|
| C/C++ | `clangd` | `.cpp`, `.c`, `.h`, `.hpp`, `.cc`, `.cxx`, etc. |
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

### LSP Configuration Examples

**Use built-in defaults (recommended for first-time setup):**

```json
"lsp": {
  "enabled": true,
  "servers": []
}
```

**Override Python command (e.g. use virtual environment):**

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

**Enable only specific language servers:**

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

**Disable LSP entirely:**

```json
"lsp": {
  "enabled": false,
  "servers": []
}
```

**Add a custom language server (e.g. Lua):**

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

> **Note**: When using custom `servers`, you must install the corresponding language servers (e.g. `clangd`, `pylsp`, `gopls`). Reload the configuration or restart pnana for changes to take effect.

---

## Configuration Examples

### Basic Configuration

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
    "cursor_smooth": false,
    "show_helpbar": true,
    "logo_gradient": true,
    "logo_style": "default",
    "file_browser_side": "left",
    "ai_panel_side": "right",
    "terminal_side": "bottom",
    "statusbar_style": "default"
  },
  "files": {
    "encoding": "UTF-8",
    "line_ending": "LF",
    "trim_trailing_whitespace": true,
    "insert_final_newline": true,
    "auto_save": false,
    "auto_save_interval": 60,
    "max_file_size_before_prompt_mb": 50
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

### Developer Configuration

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

### Writing Configuration

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

## Configuration File Format

The configuration file uses JSON format with these requirements:

1. **Encoding**: UTF-8
2. **Format**: Standard JSON
3. **Comments**: JSON does not support comments; use external notes if needed

### Validation

On startup, pnana validates the configuration:

- Invalid format: falls back to defaults and may warn the user
- Missing options: uses default values
- Invalid values: uses defaults and may warn

---

## Configuration Priority

Priority (highest to lowest):

1. **User config** (`~/.config/pnana/config.json`)
2. **Default configuration**

---

## Hot Reload

The current version does not support hot reload. Changes to the configuration file require restarting pnana.

Planned features:

- Config file change detection
- Automatic reload
- Real-time updates for some options

---

## FAQ

### Q: Where is the configuration file?

A: `~/.config/pnana/config.json`. pnana creates it on first run if it does not exist.

### Q: How do I reset to default configuration?

A: Delete or rename the configuration file. pnana will recreate it on next start.

### Q: Can I use multiple configuration files?

A: Only one configuration file is active at a time.

### Q: Can I add comments in the configuration file?

A: Standard JSON does not support comments. Use external documentation for notes.

### Q: How do I backup my configuration?

A: Copy `~/.config/pnana/config.json`.

### Q: LSP is not working. What should I check?

A: 1) Ensure `lsp.enabled` is `true`; 2) If using custom `servers`, install the corresponding language servers (e.g. `clangd`, `pylsp`); 3) When `servers` is empty, built-in defaults are used—ensure the default commands are in your system PATH.

---

## Changelog

- **v0.0.5**: Initial configuration system
- JSON configuration file support
- **LSP configuration**: Support for `lsp` section to configure language servers (command, extensions, args, env)

---

**Note**: This document reflects the current configuration system. Refer to the latest source for updates.
