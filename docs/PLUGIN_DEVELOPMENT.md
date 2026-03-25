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
| `vim.ui` | UI 交互 API（notify/input/select/popup/open_window/update_window/close_window，内核 PopupManager 驱动） |
| `vim.log` | 日志 API（info/warn/error/debug） |
| `vim.secure_io` | 安全 IO API（受沙盒约束） |
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

### 安全 IO API：`vim.secure_io`

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

---

## UI / 日志 API

### 基础 UI API

```lua
vim.ui.notify("Build finished", "info")
vim.ui.popup("Plugin Panel", {"line 1", "line 2"})

vim.ui.input({ prompt = "Project name:", default = "pnana" }, function(value)
  vim.log.info("input value = " .. tostring(value))
end)

-- 新的窗口句柄 API（由 PopupManager 统一管理）
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

### 进度条 API

```lua
-- 显示进度条
local progress_id = vim.ui.progress({
  title = "文件下载",
  message = "正在下载...",
  percent = 0,           -- 0-100
  indeterminate = false  -- 是否不确定进度
})

-- 更新进度
vim.ui.update_progress(progress_id, {
  message = "下载中...",
  percent = 50
})

-- 关闭进度条
vim.ui.close_progress(progress_id)
```

### 多选列表 API

```lua
vim.ui.multiselect({
  { text = "选项 1", selected = false },
  { text = "选项 2", selected = true },
  { text = "选项 3", selected = false },
}, {
  title = "多选",
  prompt = "请选择："
}, function(selected_items)
  if selected_items then
    for _, item in ipairs(selected_items) do
      print("选中: " .. item)
    end
  end
end)
```

### 悬浮提示 API

```lua
-- 显示悬浮提示
local hover_id = vim.ui.hover({
  content = {"函数: print", "参数: ...", "返回: nil"},
  row = 10,        -- 行位置
  col = 20,        -- 列位置
  anchor = "NW"    -- 锚点: NW, NE, SW, SE
})

-- 关闭悬浮提示
vim.ui.close_hover(hover_id)
```

### 窗口管理增强 API

```lua
-- 列出所有窗口
local windows = vim.ui.list_windows()
-- 返回: { {id=1, title="..."}, {id=2, title="..."} }

-- 检查窗口是否有效
local valid = vim.ui.window_is_valid(win_id)

-- 获取窗口信息
local info = vim.ui.get_window_info(win_id)
-- 返回: { id=..., type="window", valid=true }

-- 聚焦窗口（置顶）
vim.ui.focus_window(win_id)
```

### 高级布局 API (v2.2)

**注意**：完整布局渲染需要内核 Widget 系统支持，当前为基础实现。

#### 支持的组件类型

**基础显示组件**：
- `text` - 普通文本
- `paragraph` - 自动换行段落
- `separator` - 分隔线
- `canvas` - 2D 绘图画布
- `spinner` - 加载动画
- `image` - 终端图片
- `animation` - 自定义动画
- `bullet` - 列表小圆点
- `link` - 超链接样式文本

**基础交互组件**：
- `input` - 文本输入框
- `textarea` - 多行文本输入域
- `button` - 普通按钮
- `checkbox` - 复选框
- `radiobox` - 单选框
- `toggle` - 开关按钮
- `slider` - 滑动条
- `dropdown` - 下拉选择框
- `menu` - 列表菜单
- `color_picker` - 颜色选择器
- `file_picker` - 文件选择器
- `gauge` - 进度条/仪表盘
- `list` - 列表选择

**容器组件**：
- `window` - 带标题边框的窗口
- `container` - 通用容器
- `group` - 分组框（简洁边框）
- `hbox` - 水平布局盒子
- `vbox` - 垂直布局盒子
- `dbox` - 深度层叠容器
- `split` - 分割布局
- `resizable_split` - 可拖动分割面板
- `tabs` - 标签页容器
- `grid` - 网格布局
- `frame` - 自动适配/填充父容器
- `yframe` - 纵向填充容器
- `xframe` - 横向填充容器
- `vscroll` - 带垂直滚动条的容器
- `hscroll` - 带水平滚动条的容器

**弹窗/模态组件**：
- `modal` - 模态弹窗
- `popup` - 悬浮弹出层
- `notification` - 通知提示

```lua
-- 创建布局规范
local layout = vim.ui.create_layout({
  type = "hbox",           -- 布局类型: hbox(水平), vbox(垂直), container(容器)
  direction = "horizontal", -- 布局方向: horizontal, vertical
  align = "start",          -- 对齐: start, center, end, stretch
  spacing = 2,              -- 子元素间距
  padding = 1,              -- 内边距
  flex = 1,                 -- 弹性系数 (0=固定大小)
  min_width = 20,           -- 最小宽度
  min_height = 10,          -- 最小高度
  border = "single",        -- 边框: none, single, double, rounded
  children = {
    { type = "text", text = "标签" },
    { type = "input", id = "field1", value = "" },
    { type = "button", label = "确定", id = "btn_ok" },
    { type = "list", items = {"选项1", "选项2"} },
  }
})

-- 使用布局打开窗口
local win_id = vim.ui.open_layout_window({
  title = "自定义布局窗口",
  width = 80,
  height = 24,
  layout = layout,
})

-- 更新窗口布局
vim.ui.update_layout(win_id, new_layout)
```

#### 布局示例：左右分栏

```lua
local sidebar = {
  type = "vbox",
  flex = 1,
  border = "single",
  children = {
    { type = "text", text = "文件浏览器" },
    { type = "list", items = vim.fn.readdir(".") },
  }
}

local editor = {
  type = "vbox",
  flex = 3,
  border = "single",
  children = {
    { type = "text", text = "编辑器" },
    { type = "input", id = "content", value = "..." },
  }
}

local win_id = vim.ui.open_layout_window({
  title = "IDE 布局",
  width = 100,
  height = 30,
  layout = {
    type = "hbox",
    spacing = 1,
    children = { sidebar, editor }
  },
})
```

#### 布局示例：表单

```lua
local form_layout = {
  type = "vbox",
  spacing = 1,
  padding = 2,
  children = {
    {
      type = "hbox",
      children = {
        { type = "text", text = "用户名:", min_width = 10 },
        { type = "input", id = "username", flex = 1 },
      }
    },
    {
      type = "hbox",
      children = {
        { type = "text", text = "密码:", min_width = 10 },
        { type = "input", id = "password", flex = 1 },
      }
    },
    {
      type = "hbox",
      align = "end",
      children = {
        { type = "button", label = "取消", id = "cancel" },
        { type = "button", label = "确定", id = "ok" },
      }
    },
  }
}
```

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

## UI API 版本说明

- 当前 UI API 版本：`v2.2`（内核 `PopupManager` 驱动）
- 新增 API（v2.2）：
  - `vim.ui.create_layout/open_layout_window/update_layout` - 高级布局系统
  - 支持 hbox/vbox/container 布局容器
  - 支持 flex 弹性布局、对齐、间距等属性
- 新增 API（v2.1）：
  - `vim.ui.progress/update_progress/close_progress` - 进度条
  - `vim.ui.multiselect` - 多选列表
  - `vim.ui.hover/close_hover` - 悬浮提示
  - `vim.ui.list_windows/window_is_valid/get_window_info/focus_window` - 窗口管理
- 兼容策略：
  - `vim.ui.input/select/dialog/popup` 保持兼容
  - `vim.ui.open_window/update_window/close_window` 现在返回并操作真实弹窗句柄
- 后续版本将继续保证 `v2` 兼容，并在新增能力时采用向后兼容扩展

## 布局演示插件

参考 `plugins/layout-demo-plugin/init.lua` 查看完整的布局演示，包括：
- 左右分栏布局
- 上下分栏布局
- 表单布局
- 三栏布局
- 组合布局

运行 `:LayoutDemo` 命令或按 `F11` 查看演示菜单。

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
