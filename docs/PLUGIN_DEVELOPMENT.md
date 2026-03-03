# pnana 插件开发指南

> 中文 | [English](PLUGIN_DEVELOPMENT_EN.md)

本文档基于 `src/plugins` 实现，介绍如何为 pnana 开发 Lua 插件。需启用 Lua 支持（`-DBUILD_LUA=ON`）才能使用插件系统。

## 目录

- [插件结构](#插件结构)
- [插件元信息](#插件元信息)
- [API 概览](#api-概览)
- [命令注册](#命令注册)
- [事件（autocmd）](#事件autocmd)
- [键位映射](#键位映射)
- [主题 API](#主题-api)
- [编辑器 API](#编辑器-api)
- [文件 API](#文件-api)
- [沙盒与限制](#沙盒与限制)
- [示例插件](#示例插件)

---

## 插件结构

插件为包含 `init.lua` 的目录，放置在插件目录下。

### 插件目录查找顺序

1. `~/.config/pnana/plugins`
2. `./plugins`
3. `./lua`
4. `./.pnana/plugins`

### 目录结构

```
your-plugin/
├── init.lua          # 主入口（必需）
├── plugin.lua        # 可选，也用于元信息
└── lua/              # 可选，自动加载该目录下所有 .lua 文件
    └── ...
```

加载顺序：优先 `init.lua`，否则 `plugin.lua`，或 `lua/` 下所有 `.lua`。

---

## 插件元信息

在 `init.lua` 或 `plugin.lua` 中定义全局变量：

```lua
plugin_name = "your-plugin"
plugin_version = "1.0.0"
plugin_description = "Your plugin description"
plugin_author = "Your Name"

-- 可选：在 PluginUnload 事件中自行调用，用于插件禁用时的清理
function on_disable()
    -- 撤销插件所做的修改
end
```

---

## API 概览

插件通过 `vim` 全局表访问 API，风格类似 Neovim。

| 命名空间 | 说明 |
|----------|------|
| `vim.api` | 编辑器、主题、文件等核心 API |
| `vim.fn` | 工具函数（如 readfile、writefile） |
| `vim.cmd` | 注册命令（便捷别名） |
| `vim.autocmd` | 注册事件回调 |
| `vim.keymap` | 注册键位映射 |

---

## 命令注册

### 便捷方式：`vim.cmd(name, callback)`

```lua
vim.cmd("MyCommand", function()
    vim.api.set_status_message("Command executed!")
end)
```

### 标准方式：`vim.api.create_user_command(name, callback, opts?)`

```lua
vim.api.create_user_command("InsertDate", function(opts)
    local date = os.date("%Y-%m-%d")
    local pos = vim.api.get_cursor_pos()
    vim.api.insert_text(pos.row, pos.col, date)
end, { desc = "Insert current date" })
```

### 删除命令：`vim.api.del_user_command(name)`

```lua
vim.api.del_user_command("MyCommand")
```

---

## 事件（autocmd）

### 便捷方式：`vim.autocmd(event, callback)`

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
    -- 插件卸载时清理（args.file 为插件名）
    if args and args.file == "your-plugin" then
        -- 清理逻辑
    end
end)
```

### 标准方式：`vim.api.create_autocmd(event, opts?, callback)`

```lua
vim.api.create_autocmd("FileOpened", {
    pattern = "*.py",  -- 可选：文件匹配
    once = false,       -- 可选：是否只执行一次
}, function(args)
    vim.api.set_status_message("Python file: " .. (args.file or ""))
end)
```

### 支持的事件

| 事件 | 说明 |
|------|------|
| `FileOpened` | 文件打开（`args.file` 为路径） |
| `BufEnter` | 缓冲区切换（`args.file` 为路径） |
| `FileSaved` | 文件保存（`args.file` 为路径） |
| `BufWrite` | 缓冲区写入（`args.file` 为路径） |
| `PluginUnload` | 插件卸载（`args.file` 为插件名） |

### 清除 autocmd：`vim.api.clear_autocmds(opts)`

```lua
vim.api.clear_autocmds({ event = "FileOpened", pattern = "*.py" })
```

---

## 键位映射

### 便捷方式：`vim.keymap(mode, lhs, callback)`

```lua
vim.keymap("n", "<F2>", function()
    local filepath = vim.api.get_filepath()
    vim.api.set_status_message("File: " .. (filepath or "none"))
end)
```

### 标准方式：`vim.keymap.set(mode, lhs, rhs, opts?)`

```lua
-- rhs 为函数
vim.keymap.set("n", "<F2>", function()
    vim.api.set_status_message("F2 pressed")
end, { desc = "Show file path" })

-- rhs 为命令字符串
vim.keymap.set("n", "<F3>", ":FileInfo<CR>")
```

### 删除映射：`vim.keymap.del(mode, lhs)`

```lua
vim.keymap.del("n", "<F2>")
```

### 模式说明

- `n`：普通模式（代码编辑区默认）
- 其他模式由编辑器支持情况决定

---

## 主题 API

```lua
-- 获取当前主题
local current = vim.api.get_current_theme()

-- 设置主题
vim.api.set_theme("monokai")

-- 添加自定义主题
vim.api.add_theme("my_theme", {
    background = {30, 30, 46},
    foreground = {202, 211, 245},
    keyword = {137, 180, 250},
    string = {166, 227, 161},
    comment = {128, 128, 128},
    -- 其他字段：selection, line_number, statusbar_bg/fg, menubar_*, helpbar_*,
    -- function, type, number, operator_color, error, warning, info, success
})

-- 移除主题
vim.api.remove_theme("my_theme")
```

### 状态栏美化

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

## 编辑器 API

| API | 说明 |
|-----|------|
| `vim.api.get_current_line()` | 获取当前行内容 |
| `vim.api.get_line(row)` | 获取指定行 |
| `vim.api.set_line(row, text)` | 设置指定行 |
| `vim.api.get_line_count()` | 获取行数 |
| `vim.api.get_cursor_pos()` | 获取光标位置 `{row, col}`（0-based） |
| `vim.api.set_cursor_pos({row, col})` | 设置光标位置 |
| `vim.api.insert_text(row, col, text)` | 在指定位置插入文本 |
| `vim.api.delete_line(row)` | 删除指定行 |
| `vim.api.set_status_message(msg)` | 设置状态栏消息 |

---

## 文件 API

| API | 说明 |
|-----|------|
| `vim.api.get_filepath()` | 当前文件路径 |
| `vim.api.open_file(path)` | 打开文件 |
| `vim.api.save_file()` | 保存当前文件 |

### 文件读写：`vim.fn.readfile` / `vim.fn.writefile`

```lua
local lines = vim.fn.readfile("/path/to/file")
if lines then
    for i, line in ipairs(lines) do
        print(i, line)
    end
end

vim.fn.writefile("/path/to/file", {"line1", "line2", "line3"})
```

**注意**：文件读写受沙盒限制，仅允许访问配置和插件相关路径。

---

## 沙盒与限制

- **`vim.fn.system(command)`**：被禁用，返回 `nil, "System command execution is disabled in sandbox mode"`
- **文件访问**：`vim.fn.readfile` / `vim.fn.writefile` 仅允许访问白名单路径（如 `~/.config/pnana/`、插件目录等）

---

## 示例插件

参考 `plugins/` 目录：

- **example-plugin**：命令、事件、键位映射
- **theme-pro-plugin**：主题添加与卸载清理
- **statusbar-beautify-plugin**：状态栏美化

### 最小示例

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

## 启用与配置

1. 编译时启用 Lua：`cmake -DBUILD_LUA=ON ..`
2. 在编辑器配置 `config.json` 中启用插件：

```json
{
  "plugins": {
    "enabled_plugins": ["example-plugin", "statusbar-beautify-plugin"]
  }
}
```

3. 通过 `Alt+P` 打开插件管理器，启用/禁用插件

---

*文档与 `src/plugins` 实现同步，如有更新以代码为准。*
