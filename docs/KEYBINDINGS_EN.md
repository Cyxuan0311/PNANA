# pnana Keybindings Reference

This document matches the in-editor help (F1), based on `src/ui/help.cpp` and `src/input/key_binding_manager.cpp`.

## Table of Contents

- [File Operations](#file-operations)
- [Editing](#editing)
- [Navigation](#navigation)
- [View & Tools](#view--tools)
- [Help Window](#help-window)
- [Modes & Dialogs](#modes--dialogs)

---

## File Operations

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New file |
| `Ctrl+O` | Toggle file browser |
| `Ctrl+S` | Save file |
| `Alt+A` | Save as |
| `Ctrl+W` | Close tab |
| `Alt+F` | Create folder |
| `Alt+M` | Open file picker |
| `Ctrl+Q` | Quit editor |
| `Alt+Tab` / `Ctrl+PageDown` | Next tab |
| `Alt+Shift+Tab` / `Ctrl+PageUp` | Previous tab |

### In File Browser

| Action | Function |
|--------|----------|
| `↑` `↓` | Navigate file list |
| `Enter` | Open file/folder |
| `Backspace` | Go to parent folder |
| `Tab` | Toggle type filter |
| `F6` | Move file/folder |
| `Ctrl+L` | Split view with selected file (select a file first) |

---

## Editing

### Undo / Redo

| Shortcut | Action |
|----------|--------|
| `Ctrl+Z` | Undo |
| `Ctrl+Y` / `Ctrl+Shift+Z` | Redo |

### Clipboard & Selection

| Shortcut | Action |
|----------|--------|
| `Ctrl+X` | Cut |
| `Ctrl+P` | Copy |
| `Ctrl+V` | Paste |
| `Ctrl+A` | Select all |
| `Alt+D` | Select word |
| `Ctrl+→` / `Ctrl+←` | Free select (move by word and select) |
| `Shift+↑` `↓` `←` `→` | Select text |
| `Alt+Shift+↑` `↓` `←` `→` | Extend selection |

### Line Operations

| Shortcut | Action |
|----------|--------|
| `Ctrl+D` | Duplicate line |
| `Ctrl+Shift+K` | Delete line |
| `Ctrl+Backspace` | Delete word before cursor |
| `Tab` | Indent line |
| `Shift+Tab` | Unindent line |
| `Ctrl+/` | Toggle comment |
| `Alt+↑` | Move line up |
| `Alt+↓` | Move line down |

> **Note**: `Ctrl+L` opens split view when a file is selected in the file browser; or shows the close-split dialog when split view exists in the code area.

### Search

| Shortcut | Action |
|----------|--------|
| `Ctrl+F` | Search |
| `Ctrl+H` | Replace |
| `Ctrl+F3` | Find next |
| `Ctrl+Shift+F3` | Find previous |

### LSP (when LSP support is enabled)

| Shortcut | Action |
|----------|--------|
| `Ctrl+Space` | Trigger code completion |
| `Ctrl+U` | Toggle fold |
| `Ctrl+Shift+U` | Fold all |
| `Ctrl+Alt+U` | Unfold all |

---

## Navigation

| Shortcut | Action |
|----------|--------|
| `↑` `↓` `←` `→` | Move cursor |
| `Home` | Line start |
| `End` | Line end |
| `Ctrl+Home` | File start |
| `Ctrl+End` | File end |
| `Ctrl+G` | Go to line number |
| `Page Up` / `Page Down` | Page scroll |

---

## View & Tools

| Shortcut | Action |
|----------|--------|
| `Ctrl+T` | Theme menu |
| `Ctrl+B` | Symbol navigation (LSP document symbols) |
| `Alt+E` | Error diagnostics |
| `F1` | Show/hide help |
| `Ctrl+Shift+L` | Toggle line numbers |
| `F3` | Command palette |
| `F4` | SSH remote file editor |
| `Ctrl+L` | Split view dialog (when file selected) |
| `Ctrl+←` `→` `↑` `↓` | Navigate between split regions |
| `Ctrl+Shift+A` | AI assistant |
| `Alt+W` | Toggle Markdown preview |

### Image Preview & Terminal

- **Select an image filename** in the code area to auto-preview
- Choose "terminal" from the command palette to open the integrated terminal
- Use `+` / `-` to adjust terminal height

### Plugins (when Lua support is enabled)

| Shortcut | Action |
|----------|--------|
| `Alt+P` | Open plugin manager |

---

## Help Window

| Action | Function |
|--------|----------|
| `Tab` / `←` `→` | Switch category tabs |
| `↑` `↓` / `j` `k` | Scroll content |
| `Page Up` / `Page Down` | Page navigation |
| `Home` / `End` | Jump to top/bottom |
| `Esc` | Close help |

---

## Modes & Dialogs

### Theme Menu

- `↑` `↓` or `k` `j`: Select theme
- `Enter`: Apply
- `Esc`: Cancel

### Search / Replace

- `Enter`: Execute
- `Esc`: Cancel

### Go to Line

- Enter line number, then `Enter` to jump

### Save As / Create Folder

- `Enter`: Confirm
- `Esc`: Cancel

---

## Basic Editing Keys

| Key | Action |
|-----|--------|
| `Backspace` | Delete previous character |
| `Delete` | Delete next character |
| `Enter` | Insert newline |

---

*Document synced with in-editor F1 help. Actual behavior may vary by build configuration.*
