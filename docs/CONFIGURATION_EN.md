# pnana Configuration

> [中文](CONFIGURATION.md) | English

This document describes pnana's configuration system and usage.

---

## Table of Contents

- [Configuration File Location](#configuration-file-location)
- [Configuration Options](#configuration-options)
- [Configuration Examples](#configuration-examples)
- [Configuration File Format](#configuration-file-format)
- [Command-Line Arguments](#command-line-arguments)

---

## Configuration File Location

The configuration file is located at:

```
~/.config/pnana/config.json
```

On first run, if the configuration file does not exist, pnana will create it with default values.

---

## Configuration Options

The configuration uses a **nested JSON structure** with sections: `editor`, `display`, `files`, `search`, `themes`, and `plugins`.

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

### files

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `encoding` | string | `"UTF-8"` | Encoding: `UTF-8`, `GBK`, `GB2312`, `ASCII` |
| `line_ending` | string | `"LF"` | Line ending: `LF` (Unix), `CRLF` (Windows), `CR` (Mac) |
| `trim_trailing_whitespace` | boolean | `true` | Trim trailing whitespace on save |
| `insert_final_newline` | boolean | `true` | Insert newline at end of file on save |
| `auto_save` | boolean | `false` | Enable auto save |
| `auto_save_interval` | number | `60` | Auto save interval (seconds) |

### search

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `case_sensitive` | boolean | `false` | Case sensitive search |
| `whole_word` | boolean | `false` | Whole word match |
| `regex` | boolean | `false` | Use regular expressions |
| `wrap_around` | boolean | `true` | Wrap around when searching |

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

## Command-Line Arguments

Command-line arguments override configuration file settings:

### Basic Usage

```bash
# Use default configuration
pnana

# Open file
pnana file.txt

# Open multiple files
pnana file1.txt file2.cpp file3.py
```

### Configuration Arguments

```bash
# Specify config file
pnana --config ~/.config/pnana/custom.json

# Use specific theme
pnana --theme dracula file.txt

# Read-only mode
pnana --readonly file.txt
```

### Argument Reference

| Argument | Description | Example |
|----------|-------------|---------|
| `--config <path>` | Config file path | `--config ~/.config/pnana/custom.json` |
| `--theme <name>` | Theme name | `--theme dracula` |
| `--readonly` | Open file read-only | `--readonly file.txt` |
| `--help` | Show help | `--help` |
| `--version` | Show version | `--version` |

---

## Configuration Priority

Priority (highest to lowest):

1. **Command-line arguments**
2. **User config** (`~/.config/pnana/config.json`)
3. **Default configuration**

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

A: Use `--config` to specify a different file, but only one is active at a time.

### Q: Can I add comments in the configuration file?

A: Standard JSON does not support comments. Use external documentation for notes.

### Q: How do I backup my configuration?

A: Copy `~/.config/pnana/config.json`.

---

## Changelog

- **v0.0.5**: Initial configuration system
- JSON configuration file support
- Command-line argument override

---

**Note**: This document reflects the current configuration system. Refer to the latest source for updates.
