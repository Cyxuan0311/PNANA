#ifdef BUILD_LUA_SUPPORT

#include "plugins/lua_ui_runtime.h"
#include "utils/logger.h"
#include <lua.hpp>

namespace pnana {
namespace plugins {

namespace {
constexpr const char* kLuaFtxuiDsl = R"LUA(
vim = vim or {}
vim.ftxui = vim.ftxui or {}

local M = vim.ftxui

-- 基础节点构造函数
local function node(kind, attrs, children)
  attrs = attrs or {}
  attrs.type = kind
  if children ~= nil then
    attrs.children = children
  end
  return attrs
end

-- 装饰器元表（支持链式调用）
local Decorator = {}
Decorator.__index = Decorator

function Decorator:new(decorators)
  local t = setmetatable({}, Decorator)
  t.decorators = decorators or {}
  return t
end

function Decorator:apply(element)
  if type(element) ~= "table" then
    element = { type = "text", label = tostring(element) }
  end
  element.decorators = element.decorators or {}
  for _, dec in ipairs(self.decorators) do
    table.insert(element.decorators, dec)
  end
  return element
end

-- 装饰器组合操作（模拟 | 操作符）
function Decorator.__add(self, other)
  if getmetatable(other) == Decorator then
    local combined = {}
    for _, d in ipairs(self.decorators) do table.insert(combined, d) end
    for _, d in ipairs(other.decorators) do table.insert(combined, d) end
    return Decorator:new(combined)
  end
  return self
end

-- 创建装饰器的辅助函数
local function make_decorator(name, value)
  return Decorator:new({{name = name, value = value}})
end

-- ========== 基础组件 ==========
function M.text(label, attrs) 
  attrs = attrs or {}
  attrs.label = label or ""
  return node("text", attrs) 
end

function M.paragraph(label, attrs) 
  attrs = attrs or {}
  attrs.label = label or ""
  return node("paragraph", attrs) 
end

function M.separator(attrs) 
  return node("separator", attrs or {}) 
end

function M.bullet(attrs)
  return node("bullet", attrs or {})
end

function M.link(url, label, attrs)
  attrs = attrs or {}
  attrs.label = label or url
  attrs.value = url
  return node("link", attrs)
end

-- ========== 交互组件 ==========
function M.button(label, attrs) 
  attrs = attrs or {}
  attrs.label = label or "Button"
  return node("button", attrs) 
end

function M.input(label, attrs) 
  attrs = attrs or {}
  attrs.label = label or ""
  attrs.value = attrs.value or ""
  return node("input", attrs) 
end

function M.textarea(label, attrs)
  attrs = attrs or {}
  attrs.label = label or ""
  attrs.value = attrs.value or ""
  return node("textarea", attrs)
end

function M.checkbox(label, attrs)
  attrs = attrs or {}
  attrs.label = label or ""
  attrs.value = attrs.value or "false"
  return node("checkbox", attrs)
end

function M.radiobox(items, attrs)
  attrs = attrs or {}
  attrs.items = items or {}
  return node("radiobox", attrs)
end

function M.toggle(label, attrs)
  attrs = attrs or {}
  attrs.label = label or ""
  attrs.value = attrs.value or "false"
  return node("toggle", attrs)
end

function M.slider(label, min, max, value, attrs)
  attrs = attrs or {}
  attrs.label = label or ""
  attrs.min = min or 0
  attrs.max = max or 100
  attrs.value = value or min or 0
  return node("slider", attrs)
end

function M.dropdown(items, attrs)
  attrs = attrs or {}
  attrs.items = items or {}
  return node("dropdown", attrs)
end

function M.menu(items, attrs)
  attrs = attrs or {}
  attrs.items = items or {}
  return node("menu", attrs)
end

function M.gauge(value, attrs)
  attrs = attrs or {}
  attrs.value = value or "0"
  return node("gauge", attrs)
end

function M.spinner(attrs)
  return node("spinner", attrs or {})
end

-- ========== 容器组件 ==========
function M.hbox(children, attrs) 
  return node("hbox", attrs or {}, children or {}) 
end

function M.vbox(children, attrs) 
  return node("vbox", attrs or {}, children or {}) 
end

function M.dbox(children, attrs)
  return node("dbox", attrs or {}, children or {})
end

function M.split(children, attrs)
  return node("split", attrs or {}, children or {})
end

function M.resizable_split(children, attrs)
  return node("resizable_split", attrs or {}, children or {})
end

function M.tabs(items, attrs)
  attrs = attrs or {}
  attrs.items = items or {}
  return node("tabs", attrs)
end

function M.grid(rows, attrs)
  attrs = attrs or {}
  attrs.items = rows or {}
  return node("grid", attrs or {}, rows or {})
end

function M.frame(child, attrs)
  attrs = attrs or {}
  local children = child and {child} or {}
  return node("frame", attrs, children)
end

function M.yframe(child, attrs)
  attrs = attrs or {}
  local children = child and {child} or {}
  return node("yframe", attrs, children)
end

function M.xframe(child, attrs)
  attrs = attrs or {}
  local children = child and {child} or {}
  return node("xframe", attrs, children)
end

function M.vscroll(child, attrs)
  attrs = attrs or {}
  local children = child and {child} or {}
  return node("vscroll", attrs, children)
end

function M.hscroll(child, attrs)
  attrs = attrs or {}
  local children = child and {child} or {}
  return node("hscroll", attrs, children)
end

function M.window(title, children, attrs)
  attrs = attrs or {}
  attrs.label = title or "Window"
  return node("window", attrs, children or {})
end

function M.container(children, attrs)
  return node("container", attrs or {}, children or {})
end

function M.group(children, attrs)
  return node("group", attrs or {}, children or {})
end

-- ========== 弹窗组件 ==========
function M.modal(content, attrs)
  attrs = attrs or {}
  local children = content and {content} or {}
  return node("modal", attrs, children)
end

function M.popup(content, attrs)
  attrs = attrs or {}
  local children = content and {content} or {}
  return node("popup", attrs, children)
end

function M.notification(message, attrs)
  attrs = attrs or {}
  attrs.label = message or ""
  return node("notification", attrs)
end

-- ========== 装饰器（样式/颜色/布局）==========
M.decorator = {}

-- 颜色装饰器
function M.decorator.color(c)
  return make_decorator("color", c)
end

function M.decorator.bgcolor(c)
  return make_decorator("bgcolor", c)
end

-- 文本样式
function M.decorator.bold()
  return make_decorator("bold", true)
end

function M.decorator.italic()
  return make_decorator("italic", true)
end

function M.decorator.underlined()
  return make_decorator("underlined", true)
end

function M.decorator.dim()
  return make_decorator("dim", true)
end

function M.decorator.inverted()
  return make_decorator("inverted", true)
end

-- 边框装饰器
function M.decorator.border(style)
  return make_decorator("border", style or "single")
end

function M.decorator.borderRounded()
  return make_decorator("border", "rounded")
end

function M.decorator.borderDouble()
  return make_decorator("border", "double")
end

-- 布局装饰器
function M.decorator.flex(grow)
  return make_decorator("flex", grow or 1)
end

function M.decorator.size(width, height)
  return make_decorator("size", {width = width, height = height})
end

function M.decorator.padding(p)
  return make_decorator("padding", p)
end

function M.decorator.margin(m)
  return make_decorator("margin", m)
end

-- 对齐装饰器
function M.decorator.align_left()
  return make_decorator("align", "left")
end

function M.decorator.align_center()
  return make_decorator("align", "center")
end

function M.decorator.align_right()
  return make_decorator("align", "right")
end

-- 便捷方法：元素 | 装饰器（通过方法链模拟）
function M.styled(element, ...)
  local decorators = {...}
  for i = 1, #decorators do
    if type(decorators[i]) == "table" and getmetatable(decorators[i]) == Decorator then
      decorators[i]:apply(element)
    end
  end
  return element
end

-- ========== 事件处理 ==========
function M.on(element, event_name, callback)
  if type(element) ~= "table" then
    element = { type = "text", label = tostring(element) }
  end
  element.events = element.events or {}
  element.events[event_name] = callback
  return element
end

-- ========== 高级组件 ==========
function M.canvas(width, height, attrs)
  attrs = attrs or {}
  attrs.width = width or 40
  attrs.height = height or 20
  return node("canvas", attrs)
end

function M.image(path, attrs)
  attrs = attrs or {}
  attrs.value = path
  return node("image", attrs)
end

function M.animation(frames, attrs)
  attrs = attrs or {}
  attrs.items = frames or {}
  return node("animation", attrs)
end

function M.color_picker(value, attrs)
  attrs = attrs or {}
  attrs.value = value or "#FFFFFF"
  return node("color_picker", attrs)
end

function M.file_picker(path, attrs)
  attrs = attrs or {}
  attrs.value = path or ""
  return node("file_picker", attrs)
end

-- ========== 便捷构造函数（类似 FTXUI C++ 风格）==========
function M.txt(label) return M.text(label) end
function M.p(label) return M.paragraph(label) end
function M.sep() return M.separator() end
function M.btn(label) return M.button(label) end
function M.inp(label) return M.input(label) end

-- 快速样式方法（直接返回带样式的元素）
function M.bold_text(label)
  return M.styled(M.text(label), M.decorator.bold())
end

function M.color_text(label, color)
  return M.styled(M.text(label), M.decorator.color(color))
end

function M.framed(title, child)
  return M.window(title, {child}, {border = "rounded"})
end

-- ========== 布局辅助函数 ==========
function M.center(child)
  return M.styled(child, M.decorator.align_center())
end

function M.spacer()
  return M.text("")
end

function M.filler()
  return { type = "text", label = "", flex = 1 }
end

local function flatten(node, out, indent)
  indent = indent or ""
  if type(node) ~= "table" then
    return
  end

  local t = node.type or "text"
  if t == "text" or t == "paragraph" then
    table.insert(out, indent .. tostring(node.label or ""))
  elseif t == "separator" then
    table.insert(out, indent .. string.rep("-", 60))
  elseif t == "button" then
    local marker = node.__selected and "►" or " "
    table.insert(out, indent .. marker .. " [ " .. tostring(node.label or "Button") .. " ]")
  elseif t == "input" then
    table.insert(out, indent .. "> " .. tostring(node.value or ""))
  else
    if node.label and node.label ~= "" and t ~= "window" then
      table.insert(out, indent .. tostring(node.label))
    end
  end

  local children = node.children or {}
  for i = 1, #children do
    flatten(children[i], out, indent)
  end
end

function M.render_lines(tree)
  local out = {}
  flatten(tree, out, "")
  return out
end

function M.ContainerVertical(children)
  return M.vbox(children or {}, { id = "container_vertical" })
end

function M.ContainerHorizontal(children)
  return M.hbox(children or {}, { id = "container_horizontal" })
end

function M.Renderer(component, fn)
  return {
    __kind = "renderer",
    component = component,
    render_fn = fn,
  }
end

function M.CatchEvent(component, fn)
  component.__event_handler = fn
  return component
end

function M.Event(name)
  return name
end

function M.open_component_window(opts)
  opts = opts or {}
  local c = opts.component
  local tree = c
  if type(c) == "table" and c.__kind == "renderer" and type(c.render_fn) == "function" then
    tree = c.render_fn()
  end

  local lines = opts.lines or M.render_lines(tree or M.text(""))
  return vim.ui.open_component_window({
    title = opts.title or "Component",
    width = opts.width or 90,
    height = opts.height or 24,
    lines = lines,
    input_line = opts.input_line,
    left_title = opts.left_title,
    right_title = opts.right_title,
    left_lines = opts.left_lines,
    right_lines = opts.right_lines,
    help_lines = opts.help_lines,
    on_event = opts.on_event,
  })
end

function M.update_component_window(win_id, opts)
  opts = opts or {}
  local c = opts.component
  local tree = c
  if type(c) == "table" and c.__kind == "renderer" and type(c.render_fn) == "function" then
    tree = c.render_fn()
  end

  local lines = opts.lines or M.render_lines(tree or M.text(""))
  return vim.ui.update_component_window(win_id, {
    title = opts.title,
    lines = lines,
    input_line = opts.input_line,
    left_title = opts.left_title,
    right_title = opts.right_title,
    left_lines = opts.left_lines,
    right_lines = opts.right_lines,
    help_lines = opts.help_lines,
  })
end

function M.popup(opts)
  opts = opts or {}
  local layout = opts.layout or M.window(opts.title or "Popup", opts.children or {})
  return vim.ui.open_layout_window({
    title = opts.title or "Popup",
    width = opts.width or 56,
    height = opts.height or 12,
    layout = layout,
  })
end

-- ========== 高阶模板（保持兼容：仅新增，不改变既有行为）==========
M.templates = M.templates or {}

-- dialog 模板：统一 header/body/footer 布局，减少插件样板代码
-- opts:
--   title: string
--   body: string | string[]
--   buttons: { {label=string, primary=bool}? }  -- 仅用于渲染（事件处理仍由调用方决定）
--   width/height: number (用于 open_* convenience)
--   style: { border="rounded"|"normal", padding=number, spacing=number }
function M.templates.dialog(opts)
  opts = opts or {}
  local title = opts.title or "Dialog"
  local style = opts.style or {}

  local padding = style.padding
  if padding == nil then padding = 1 end
  local spacing = style.spacing
  if spacing == nil then spacing = 1 end
  local border = style.border or "rounded"

  local body_nodes = {}
  local body = opts.body
  if type(body) == "string" then
    body_nodes = { M.paragraph(body) }
  elseif type(body) == "table" then
    for _, line in ipairs(body) do
      table.insert(body_nodes, M.text(tostring(line)))
    end
  else
    body_nodes = { M.text("") }
  end

  local button_nodes = {}
  if type(opts.buttons) == "table" and #opts.buttons > 0 then
    for _, b in ipairs(opts.buttons) do
      local label = (type(b) == "table" and b.label) and tostring(b.label) or tostring(b)
      local btn = M.button(label)
      if type(b) == "table" and b.primary then
        btn.decorators = btn.decorators or {}
        table.insert(btn.decorators, { name = "bold", value = true })
      end
      table.insert(button_nodes, btn)
    end
  end

  local footer = nil
  if #button_nodes > 0 then
    footer = M.hbox(button_nodes, { spacing = 2, align = "center" })
  end

  local children = {}
  -- body
  table.insert(children, M.vbox(body_nodes, { spacing = spacing, align = "start" }))
  -- footer
  if footer then
    table.insert(children, M.separator())
    table.insert(children, footer)
  end

  return M.window(title, children, {
    padding = padding,
    spacing = spacing,
    border = border,
    window_title_decorators = { bold = true },
  })
end

-- 直接打开 dialog（使用现有 open_layout_window，保持兼容）
function M.open_dialog_window(opts)
  opts = opts or {}
  local layout = opts.layout or M.templates.dialog(opts)
  return vim.ui.open_layout_window({
    title = opts.title or "Dialog",
    width = opts.width or 60,
    height = opts.height or 14,
    layout = layout,
  })
end
)LUA";
} // namespace

LuaUIRuntime::LuaUIRuntime(core::Editor* editor, LuaEngine* engine)
    : editor_(editor), engine_(engine) {}

bool LuaUIRuntime::initialize() {
    (void)editor_;
    if (!engine_) {
        LOG_ERROR("LuaUIRuntime initialize failed: engine is null");
        return false;
    }

    lua_State* L = engine_->getState();
    if (!L) {
        LOG_ERROR("LuaUIRuntime initialize failed: lua state is null");
        return false;
    }

    luaL_dostring(L, "vim = vim or {}\nvim.ftxui = vim.ftxui or {}");

    lua_getglobal(L, "vim");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "ftxui");
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, "ftxui");
        }

        lua_pushstring(L, "_runtime");
        lua_pushstring(L, "lua_native");
        lua_settable(L, -3);
        lua_pushstring(L, "version");
        lua_pushstring(L, "0.1.0");
        lua_settable(L, -3);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (!engine_->executeString(kLuaFtxuiDsl)) {
        LOG_ERROR("LuaUIRuntime initialize failed: DSL bootstrap execution error");
        return false;
    }

    return true;
}

std::string LuaUIRuntime::normalizeWidgetType(const std::string& type) {
    return type;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
