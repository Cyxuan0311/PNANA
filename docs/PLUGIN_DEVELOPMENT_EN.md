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
- [Event Parser API](#event-parser-api)
- [Theme API](#theme-api)
- [Editor API](#editor-api)
- [File API](#file-api)
- [UI / Logging API](#ui--logging-api)
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
| `vim.fn` | Utility functions (readfile, writefile, systemlist_async, hrtime) |
| `vim.ui` | UI interaction APIs (notify/input/select/popup/open_window/update_window/close_window, driven by PopupManager) |
| `vim.log` | Logging APIs (info/warn/error/debug) |
| `vim.secure_io` | Sandboxed secure I/O APIs |
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

## Event Parser API

The Event Parser API (`event_parser`) allows Lua scripts to customize event parsing logic on the Lua side, without hardcoding event conversions in C++. This provides greater flexibility, especially when handling complex events or adding new event types.

### API Overview

| API | Description | Returns |
|-----|-------------|---------|
| `event_parser.parse(event_type, character?)` | Parse event into structured data | `{event_type, modifiers, character}` |
| `event_parser.to_string(event_type, character?)` | Convert event to string | `"event_type"` or `"event_type:char"` |
| `event_parser.get_modifiers()` | Get modifier key info | `{ctrl, alt, shift, meta}` |
| `event_parser.is_arrow_key(event_type)` | Check if arrow key | `true/false` |
| `event_parser.is_function_key(event_type)` | Check if function key | `true/false` |
| `event_parser.is_navigation_key(event_type)` | Check if navigation key | `true/false` |

### Usage Examples

#### 1. Parse Event into Structured Data

```lua
-- In window event callback
local function on_window_event(event_name, payload)
    -- Parse event
    local event_data = event_parser.parse(event_name, payload)
    
    print("Event type: " .. event_data.event_type)
    print("Character: " .. tostring(event_data.character))
    print("Modifiers: " .. vim.inspect(event_data.modifiers))
    
    -- Handle based on event type
    if event_parser.is_navigation_key(event_name) then
        -- Handle navigation keys (PageUp, PageDown, Home, End, etc.)
        if event_name == "pageup" then
            print("Page up")
        elseif event_name == "pagedown" then
            print("Page down")
        elseif event_name == "home" then
            print("Go to beginning")
        elseif event_name == "end" then
            print("Go to end")
        end
    elseif event_parser.is_arrow_key(event_name) then
        -- Handle arrow keys
        if event_name == "arrow_up" then
            print("Move up")
        elseif event_name == "arrow_down" then
            print("Move down")
        end
    elseif event_parser.is_function_key(event_name) then
        -- Handle function keys (F1-F12)
        print("Function key: " .. event_name)
    end
    
    return true
end
```

#### 2. Check Event Type

```lua
-- Check if arrow key
if event_parser.is_arrow_key(event_name) then
    print("This is an arrow key")
end

-- Check if navigation key
if event_parser.is_navigation_key(event_name) then
    print("This is a navigation key")
end

-- Check if function key
if event_parser.is_function_key(event_name) then
    print("This is a function key")
end
```

#### 3. Get Modifier Information

```lua
local mods = event_parser.get_modifiers()
print("Ctrl: " .. tostring(mods.ctrl))
print("Alt: " .. tostring(mods.alt))
print("Shift: " .. tostring(mods.shift))
print("Meta: " .. tostring(mods.meta))
```

#### 4. Event String Conversion

```lua
-- Simple event
local event_str = event_parser.to_string("pageup")
print(event_str)  -- Output: "pageup"

-- Event with character
local event_str = event_parser.to_string("char", "a")
print(event_str)  -- Output: "char:a"
```

### Supported Event Types

| Event Type | Lua Event Name | Description |
|------------|----------------|-------------|
| Arrow Keys | `arrow_up`, `arrow_down`, `arrow_left`, `arrow_right` | Up/Down/Left/Right arrows |
| Navigation Keys | `pageup`, `pagedown`, `home`, `end`, `insert`, `delete` | Page navigation |
| Function Keys | `f1` - `f12` | Function keys F1 to F12 |
| Action Keys | `enter`, `escape`, `backspace`, `tab` | Common actions |
| Character Keys | `char` | Regular character input (payload = character) |
| Other Events | `raw` | Undefined events (payload = raw string) |

### Architecture Advantages

#### Before (C++ Hardcoded)

```
User Key Press → C++ Hardcoded Conversion → Lua Receives Fixed Event Name
                  ↓
            Only predefined events supported
            Adding new events requires C++ code changes
```

#### After (Lua Side Parsing)

```
User Key Press → C++ Generic Pass-through → Lua Side Flexible Parsing
                  ↓                          ↓
            All events passed         User-defined parsing logic
                                      New events don't require C++ changes
```

### Debug Example

```lua
-- Debug all events in plugin
local function on_window_event(event_name, payload)
    -- Print all event information
    local event_data = event_parser.parse(event_name, payload)
    
    print("=== Event Debug Info ===")
    print("Event Type: " .. event_data.event_type)
    print("Payload: " .. tostring(payload))
    print("Is Arrow Key: " .. tostring(event_parser.is_arrow_key(event_name)))
    print("Is Navigation Key: " .. tostring(event_parser.is_navigation_key(event_name)))
    print("Is Function Key: " .. tostring(event_parser.is_function_key(event_name)))
    print("========================")
    
    -- ... handling logic
    return true
end
```

### Real-World Application

#### FG Live Grep Plugin Example

```lua
-- Handle page navigation in FG Live Grep plugin
local function on_window_event(event_name, payload)
    -- Use event parser API to check event type
    if event_parser.is_navigation_key(event_name) then
        if event_name == "pageup" then
            -- Page up
            state.selected = math.max(1, state.selected - CONFIG.list_height)
            update_visible_results()
            update_window("pageup")
        elseif event_name == "pagedown" then
            -- Page down
            state.selected = math.min(#state.all_results, state.selected + CONFIG.list_height)
            update_visible_results()
            update_window("pagedown")
        end
        return true
    end
    
    -- ... other event handling
    return false
end
```

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

### Secure I/O API: `vim.secure_io`

```lua
local content, err = vim.secure_io.read_text("./plugins/example-plugin/init.lua")
if not content then
  vim.log.error("read failed: " .. (err or "unknown"))
  return
end

local ok, werr = vim.secure_io.write_text("./plugins/example-plugin/tmp.txt", "hello")
if not ok then
  vim.log.warn("write failed: " .. (werr or "unknown"))
end

vim.secure_io.append_text("./plugins/example-plugin/tmp.txt", "\nworld")
local exists = vim.secure_io.exists("./plugins/example-plugin/tmp.txt")
```

### Async Command Execution: `vim.fn.systemlist_async`

**Note**: `vim.fn.system` is disabled, `vim.fn.systemlist` is synchronous and blocking, async version is recommended.

```lua
-- Execute command asynchronously (non-blocking)
local request_id = vim.fn.systemlist_async(argv, opts, callback)

-- Parameters:
--   argv: Command line arguments array, e.g., {"rg", "--vimgrep", "pattern"}
--   opts: Options table (optional)
--     - cwd: Working directory (default: ".")
--     - timeout_ms: Timeout in milliseconds (default: 800)
--     - max_output_bytes: Max output bytes (default: 1048576)
--   callback: Callback function function(lines, err)
--     - lines: Output lines array
--     - err: Error message (if any)

-- Example: Async ripgrep search
local request_id = vim.fn.systemlist_async(
  {"rg", "--vimgrep", "--max-count=100", "pattern"},
  {
    cwd = "/path/to/search",
    timeout_ms = 1800,
    max_output_bytes = 1024 * 1024,
  },
  function(lines, err)
    if err then
      vim.log.error("Search failed: " .. err)
      return
    end
    
    if lines then
      vim.log.info("Found " .. #lines .. " results")
      -- Process results...
    end
  end
)

-- Return value:
--   request_id: Request ID (integer) for tracking
```

**Security Restrictions**:
- Only `rg` or `ripgrep` executables are allowed
- Other executables will be blocked and return an error

### High-Resolution Time: `vim.fn.hrtime`

```lua
-- Get high-resolution time in nanoseconds
local nanoseconds = vim.fn.hrtime()

-- Example: Measure execution time
local start = vim.fn.hrtime()
-- ... execute some code ...
local elapsed_ms = (vim.fn.hrtime() - start) / 1000000
print("Elapsed: " .. elapsed_ms .. "ms")
```

**Note**: Returns time in nanoseconds (ns), divide by 1,000,000 to convert to milliseconds (ms).

---

## UI / Logging API

### Basic UI API

```lua
vim.ui.notify("Build finished", "info")
vim.ui.popup("Plugin Panel", {"line 1", "line 2"})

vim.ui.input({ prompt = "Project name:", default = "pnana" }, function(value)
  vim.log.info("input value = " .. tostring(value))
end)

-- New window handle API (managed by PopupManager)
local win = vim.ui.open_window({
  title = "Build Output",
  lines = {"Compiling...", "Done"}
})
vim.ui.update_window(win, { title = "Build Output ✅", lines = {"Done in 1.2s"} })
vim.ui.close_window(win)

vim.ui.select({"A", "B", "C"}, { prompt = "Pick one" }, function(item, idx)
  vim.log.debug("selected: " .. tostring(item) .. " #" .. tostring(idx))
end)

vim.log.info("hello")
vim.log.warn("warn msg")
vim.log.error("error msg")
vim.log.debug("debug msg")
```

### Progress Bar API

```lua
-- Show progress bar
local progress_id = vim.ui.progress({
  title = "File Download",
  message = "Downloading...",
  percent = 0,           -- 0-100
  indeterminate = false  -- Indeterminate progress
})

-- Update progress
vim.ui.update_progress(progress_id, {
  message = "Downloading...",
  percent = 50
})

-- Close progress bar
vim.ui.close_progress(progress_id)
```

### Multi-select List API

```lua
vim.ui.multiselect({
  { text = "Option 1", selected = false },
  { text = "Option 2", selected = true },
  { text = "Option 3", selected = false },
}, {
  title = "Multi-select",
  prompt = "Please select:"
}, function(selected_items)
  if selected_items then
    for _, item in ipairs(selected_items) do
      print("Selected: " .. item)
    end
  end
end)
```

### Hover Tooltip API

```lua
-- Show hover tooltip
local hover_id = vim.ui.hover({
  content = {"Function: print", "Args: ...", "Returns: nil"},
  row = 10,        -- Row position
  col = 20,        -- Column position
  anchor = "NW"    -- Anchor: NW, NE, SW, SE
})

-- Close hover tooltip
vim.ui.close_hover(hover_id)
```

### Window Management Enhanced API

```lua
-- List all windows
local windows = vim.ui.list_windows()
-- Returns: { {id=1, title="..."}, {id=2, title="..."} }

-- Check if window is valid
local valid = vim.ui.window_is_valid(win_id)

-- Get window info
local info = vim.ui.get_window_info(win_id)
-- Returns: { id=..., type="window", valid=true }

-- Focus window (bring to front)
vim.ui.focus_window(win_id)
```

### Advanced Layout API (v2.2)

**Note**: Full layout rendering requires kernel Widget system support. Current implementation is basic.

#### Supported Widget Types

**Basic Display Widgets**:
- `text` - Plain text
- `paragraph` - Auto-wrapping paragraph
- `separator` - Separator line
- `canvas` - 2D drawing canvas
- `spinner` - Loading animation
- `image` - Terminal image
- `animation` - Custom animation
- `bullet` - List bullet point
- `link` - Hyperlink styled text

**Basic Interactive Widgets**:
- `input` - Text input field
- `textarea` - Multi-line text area
- `button` - Regular button
- `checkbox` - Checkbox
- `radiobox` - Radio button
- `toggle` - Toggle switch
- `slider` - Slider
- `dropdown` - Dropdown select
- `menu` - List menu
- `color_picker` - Color picker
- `file_picker` - File picker
- `gauge` - Progress bar/gauge
- `list` - List selection

**Container Widgets**:
- `window` - Window with title border
- `container` - Generic container
- `group` - Group box (simple border)
- `hbox` - Horizontal layout box
- `vbox` - Vertical layout box
- `dbox` - Depth stacking container
- `split` - Split layout
- `resizable_split` - Resizable split panel
- `tabs` - Tab container
- `grid` - Grid layout
- `frame` - Auto-fit/fill parent
- `yframe` - Vertical fill container
- `xframe` - Horizontal fill container
- `vscroll` - Container with vertical scrollbar
- `hscroll` - Container with horizontal scrollbar

**Popup/Modal Widgets**:
- `modal` - Modal dialog
- `popup` - Floating popup layer
- `notification` - Notification toast

```lua
-- Create layout specification
local layout = vim.ui.create_layout({
  type = "hbox",           -- Layout type: hbox (horizontal), vbox (vertical), container
  direction = "horizontal", -- Layout direction: horizontal, vertical
  align = "start",          -- Alignment: start, center, end, stretch
  spacing = 2,              -- Spacing between children
  padding = 1,              -- Padding
  flex = 1,                 -- Flex grow (0=fixed size)
  min_width = 20,           -- Minimum width
  min_height = 10,          -- Minimum height
  border = "single",        -- Border: none, single, double, rounded
  children = {
    { type = "text", text = "Label" },
    { type = "input", id = "field1", value = "" },
    { type = "button", label = "OK", id = "btn_ok" },
    { type = "list", items = {"Option1", "Option2"} },
  }
})

-- Open window with layout
local win_id = vim.ui.open_layout_window({
  title = "Custom Layout Window",
  width = 80,
  height = 24,
  layout = layout,
})

-- Update window layout
vim.ui.update_layout(win_id, new_layout)
```

#### Layout Example: Horizontal Split

```lua
local sidebar = {
  type = "vbox",
  flex = 1,
  border = "single",
  children = {
    { type = "text", text = "File Browser" },
    { type = "list", items = vim.fn.readdir(".") },
  }
}

local editor = {
  type = "vbox",
  flex = 3,
  border = "single",
  children = {
    { type = "text", text = "Editor" },
    { type = "input", id = "content", value = "..." },
  }
}

local win_id = vim.ui.open_layout_window({
  title = "IDE Layout",
  width = 100,
  height = 30,
  layout = {
    type = "hbox",
    spacing = 1,
    children = { sidebar, editor }
  },
})
```

#### Layout Example: Form

```lua
local form_layout = {
  type = "vbox",
  spacing = 1,
  padding = 2,
  children = {
    {
      type = "hbox",
      children = {
        { type = "text", text = "Username:", min_width = 10 },
        { type = "input", id = "username", flex = 1 },
      }
    },
    {
      type = "hbox",
      children = {
        { type = "text", text = "Password:", min_width = 10 },
        { type = "input", id = "password", flex = 1 },
      }
    },
    {
      type = "hbox",
      align = "end",
      children = {
        { type = "button", label = "Cancel", id = "cancel" },
        { type = "button", label = "OK", id = "ok" },
      }
    },
  }
}
```

---

## Sandbox and Restrictions

- **`vim.fn.system(command)`**: Disabled; returns `nil, "System command execution is disabled in sandbox mode"`
- **`vim.fn.systemlist(argv)`**: Synchronous blocking version, blocks event loop, async version recommended
- **`vim.fn.systemlist_async(argv, opts, callback)`**: Async non-blocking version (recommended)
  - Only `rg` or `ripgrep` executables are allowed
  - Other executables will be blocked
- **File access**: `vim.fn.readfile` / `vim.fn.writefile` only allow whitelisted paths (`~/.config/pnana/`, plugin dirs, etc.)
- **High-resolution time**: `vim.fn.hrtime()` is unrestricted, returns nanosecond timestamps

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

## UI API Version Notes

- Current UI API version: `v2.2` (driven by PopupManager)
- New APIs (v2.2):
  - `vim.ui.create_layout/open_layout_window/update_layout` - Advanced layout system
  - Support for hbox/vbox/container layout containers
  - Support for flex layout, alignment, spacing, etc.
- New APIs (v2.1):
  - `vim.ui.progress/update_progress/close_progress` - Progress bar
  - `vim.ui.multiselect` - Multi-select list
  - `vim.ui.hover/close_hover` - Hover tooltip
  - `vim.ui.list_windows/window_is_valid/get_window_info/focus_window` - Window management
- Compatibility policy:
  - `vim.ui.input/select/dialog/popup` remain compatible
  - `vim.ui.open_window/update_window/close_window` now return and operate on real popup handles
- Future versions will maintain v2 compatibility and use backward-compatible extensions for new features

## Layout Demo Plugin

See `plugins/layout-demo-plugin/init.lua` for complete layout demonstrations, including:
- Horizontal split layout
- Vertical split layout
- Form layout
- Three-column layout
- Composite layout

Run `:LayoutDemo` command or press `F11` to see the demo menu.

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
