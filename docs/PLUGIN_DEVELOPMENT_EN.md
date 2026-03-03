# pnana Plugin Development Guide

> [中文](PLUGIN_DEVELOPMENT.md) | English

This document is based on the `src/plugins` implementation and describes how to develop Lua plugins for pnana. Lua support must be enabled with `-DBUILD_LUA=ON` to use the plugin system.

## Table of Contents

- [Plugin Structure](#plugin-structure)
- [Plugin Metadata](#plugin-metadata)
- [API Overview](#api-overview)
- [Command Registration](#command-registration)
- [Events (autocmd)](#events-autocmd)
- [Keymaps](#keymaps)
- [Theme API](#theme-api)
- [Editor API](#editor-api)
- [File API](#file-api)
- [Sandbox and Restrictions](#sandbox-and-restrictions)
- [Example Plugins](#example-plugins)

---

## Plugin Structure

A plugin is a directory containing `init.lua`, placed under a plugin search directory.

### Plugin Directory Search Order

1. `~/.config/pnana/plugins`
2. `./plugins`
3. `./lua`
4. `./.pnana/plugins`

### Directory Layout

```
your-plugin/
├── init.lua          # Main entry (required)
├── plugin.lua        # Optional, also used for metadata
└── lua/              # Optional, all .lua files under this dir are loaded
    └── ...
```

Load order: `init.lua` first, then `plugin.lua`, or all `.lua` files under `lua/`.

---

## Plugin Metadata

Define global variables in `init.lua` or `plugin.lua`:

```lua
plugin_name = "your-plugin"
plugin_version = "1.0.0"
plugin_description = "Your plugin description"
plugin_author = "Your Name"

-- Optional: call from PluginUnload handler for cleanup when plugin is disabled
function on_disable()
    -- Revert changes made by the plugin
end
```

---

## API Overview

Plugins access APIs through the `vim` global table, in a Neovim-like style.

| Namespace | Description |
|-----------|-------------|
| `vim.api` | Core APIs: editor, theme, file, etc. |
| `vim.fn` | Utility functions (readfile, writefile) |
| `vim.cmd` | Register commands (convenience alias) |
| `vim.autocmd` | Register event callbacks |
| `vim.keymap` | Register keymaps |

---

## Command Registration

### Convenience: `vim.cmd(name, callback)`

```lua
vim.cmd("MyCommand", function()
    vim.api.set_status_message("Command executed!")
end)
```

### Standard: `vim.api.create_user_command(name, callback, opts?)`

```lua
vim.api.create_user_command("InsertDate", function(opts)
    local date = os.date("%Y-%m-%d")
    local pos = vim.api.get_cursor_pos()
    vim.api.insert_text(pos.row, pos.col, date)
end, { desc = "Insert current date" })
```

### Remove Command: `vim.api.del_user_command(name)`

```lua
vim.api.del_user_command("MyCommand")
```

---

## Events (autocmd)

### Convenience: `vim.autocmd(event, callback)`

```lua
vim.autocmd("FileOpened", function(args)
    local path = (type(args) == "table" and args.file) or "unknown"
    vim.api.set_status_message("File opened: " .. path)
end)

vim.autocmd("FileSaved", function(args)
    local path = (type(args) == "table" and args.file) or "unknown"
    vim.api.set_status_message("File saved: " .. path)
end)

vim.autocmd("PluginUnload", function(args)
    -- Cleanup when plugin is unloaded (args.file is the plugin name)
    if args and args.file == "your-plugin" then
        -- Cleanup logic
    end
end)
```

### Standard: `vim.api.create_autocmd(event, opts?, callback)`

```lua
vim.api.create_autocmd("FileOpened", {
    pattern = "*.py",  -- Optional: file pattern
    once = false,       -- Optional: run only once
}, function(args)
    vim.api.set_status_message("Python file: " .. (args.file or ""))
end)
```

### Supported Events

| Event | Description |
|-------|-------------|
| `FileOpened` | File opened (`args.file` = path) |
| `BufEnter` | Buffer switched (`args.file` = path) |
| `FileSaved` | File saved (`args.file` = path) |
| `BufWrite` | Buffer written (`args.file` = path) |
| `PluginUnload` | Plugin unloaded (`args.file` = plugin name) |

### Clear autocmds: `vim.api.clear_autocmds(opts)`

```lua
vim.api.clear_autocmds({ event = "FileOpened", pattern = "*.py" })
```

---

## Keymaps

### Convenience: `vim.keymap(mode, lhs, callback)`

```lua
vim.keymap("n", "<F2>", function()
    local filepath = vim.api.get_filepath()
    vim.api.set_status_message("File: " .. (filepath or "none"))
end)
```

### Standard: `vim.keymap.set(mode, lhs, rhs, opts?)`

```lua
-- rhs as function
vim.keymap.set("n", "<F2>", function()
    vim.api.set_status_message("F2 pressed")
end, { desc = "Show file path" })

-- rhs as command string
vim.keymap.set("n", "<F3>", ":FileInfo<CR>")
```

### Remove Keymap: `vim.keymap.del(mode, lhs)`

```lua
vim.keymap.del("n", "<F2>")
```

### Mode Notes

- `n`: Normal mode (default in code editor area)
- Other modes depend on editor support

---

## Theme API

```lua
-- Get current theme
local current = vim.api.get_current_theme()

-- Set theme
vim.api.set_theme("monokai")

-- Add custom theme
vim.api.add_theme("my_theme", {
    background = {30, 30, 46},
    foreground = {202, 211, 245},
    keyword = {137, 180, 250},
    string = {166, 227, 161},
    comment = {128, 128, 128},
    -- Other fields: selection, line_number, statusbar_bg/fg, menubar_*, helpbar_*,
    -- function, type, number, operator_color, error, warning, info, success
})

-- Remove theme
vim.api.remove_theme("my_theme")
```

### Status Bar Beautify

```lua
vim.api.set_statusbar_beautify({
    enabled = true,
    bg_color = {50, 60, 80},
    fg_color = {240, 245, 255},
    icon_enhancement = { file_icons = {...}, region_icons = {...} },
    color_enhancement = { elements = {...} },
    layout_enhancement = { spacing = 1, padding = 1 }
})
```

---

## Editor API

| API | Description |
|-----|-------------|
| `vim.api.get_current_line()` | Get current line content |
| `vim.api.get_line(row)` | Get line at row |
| `vim.api.set_line(row, text)` | Set line at row |
| `vim.api.get_line_count()` | Get total line count |
| `vim.api.get_cursor_pos()` | Get cursor position `{row, col}` (0-based) |
| `vim.api.set_cursor_pos({row, col})` | Set cursor position |
| `vim.api.insert_text(row, col, text)` | Insert text at position |
| `vim.api.delete_line(row)` | Delete line at row |
| `vim.api.set_status_message(msg)` | Set status bar message |

---

## File API

| API | Description |
|-----|-------------|
| `vim.api.get_filepath()` | Current file path |
| `vim.api.open_file(path)` | Open file |
| `vim.api.save_file()` | Save current file |

### File Read/Write: `vim.fn.readfile` / `vim.fn.writefile`

```lua
local lines = vim.fn.readfile("/path/to/file")
if lines then
    for i, line in ipairs(lines) do
        print(i, line)
    end
end

vim.fn.writefile("/path/to/file", {"line1", "line2", "line3"})
```

**Note:** File access is sandboxed and limited to whitelisted paths (e.g., `~/.config/pnana/`, plugin dirs).

---

## Sandbox and Restrictions

- **`vim.fn.system(command)`**: Disabled; returns `nil, "System command execution is disabled in sandbox mode"`
- **File access**: `vim.fn.readfile` / `vim.fn.writefile` only allow whitelisted paths (`~/.config/pnana/`, plugin dirs, etc.)

---

## Example Plugins

See the `plugins/` directory:

- **example-plugin**: Commands, events, keymaps
- **theme-pro-plugin**: Theme registration and unload cleanup
- **statusbar-beautify-plugin**: Status bar beautify

### Minimal Example

```lua
plugin_name = "hello-plugin"
plugin_version = "1.0.0"
plugin_description = "Minimal example"
plugin_author = "You"

vim.cmd("Hello", function()
    vim.api.set_status_message("Hello from " .. plugin_name)
end)
```

---

## Enabling and Configuration

1. Enable Lua at build time: `cmake -DBUILD_LUA=ON ..`
2. Enable plugins in `config.json`:

```json
{
  "plugins": {
    "enabled_plugins": ["example-plugin", "statusbar-beautify-plugin"]
  }
}
```

3. Use `Alt+P` to open the plugin manager and enable/disable plugins

---

*This document is aligned with the `src/plugins` implementation; refer to the source code for authoritative details.*
