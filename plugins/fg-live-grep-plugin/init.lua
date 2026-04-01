-- FG Live Grep Plugin - 优化版本
-- 使用新的 UI DSL 和 API，架构优化

plugin_name = "fg-live-grep-plugin"
plugin_version = "2.0.0"
plugin_description = "FG live grep with optimized DSL architecture"
plugin_author = "pnana"

-- ============================================================================
-- 事件解析 API 使用示例
-- ============================================================================
-- 新的 Lua 侧事件解析 API 允许用户自定义事件解析逻辑
-- 使用方式：
--   local event_data = event_parser.parse(event_type, character)
--   local is_arrow = event_parser.is_arrow_key(event_type)
--   local is_nav = event_parser.is_navigation_key(event_type)
--   local is_func = event_parser.is_function_key(event_type)

-- ============================================================================
-- 配置管理
-- ============================================================================
local CONFIG = {
    debounce_ms = 0,  -- 禁用防抖，实现零延迟响应（依赖 C++ 侧节流）
    max_results = 10000,  -- 优化：支持最多 10000 条结果（之前是 120）
    timeout_ms = 1800,
    max_output_bytes = 1024 * 1024,
    results_width = 66,  -- 结果列表内容宽度
    preview_width = 74,  -- 预览面板内容宽度
    list_height = 15,    -- 列表显示高度（可见区域）
    window_width = 150,  -- 优化：减小弹窗总宽度，从 178 降至 150
    window_height = 26,  -- 弹窗高度
    preview_context = 9,
    debug = true, -- 启用调试日志
    min_search_interval_ms = 100,  -- 两次搜索之间的最小间隔，防止阻塞 UI
    virtual_scroll_enabled = true,  -- 启用虚拟滚动优化
}

-- ============================================================================
-- 工具函数
-- ============================================================================

--- 获取当前时间戳 (ms)
local function now_ms()
    if vim.fn and vim.fn.hrtime then
        return math.floor(vim.fn.hrtime() / 1000000)
    end
    if not _G._pnana_timer_counter then
        _G._pnana_timer_counter = 0
    end
    _G._pnana_timer_counter = _G._pnana_timer_counter + 1
    return _G._pnana_timer_counter * 1000
end

-- ============================================================================
-- 文件缓存系统
-- ============================================================================
local file_cache = {
    data = {},      -- 缓存数据：{ [file_path] = { lines = {...}, mtime = timestamp } }
    lru = {},       -- LRU 顺序：{ file_path1, file_path2, ... }
    max_files = 50, -- 最多缓存 50 个文件
    max_lines = 10000, -- 单个文件最多缓存 10000 行
}

--- 从缓存获取文件行
local function get_cached_lines(file_path)
    local cached = file_cache.data[file_path]
    if cached then
        -- 更新 LRU 顺序
        for i, path in ipairs(file_cache.lru) do
            if path == file_path then
                table.remove(file_cache.lru, i)
                table.insert(file_cache.lru, file_path)
                break
            end
        end
        return cached.lines
    end
    return nil
end

--- 缓存文件行
local function cache_file_lines(file_path, lines)
    -- 检查是否超过最大文件数
    while #file_cache.lru >= file_cache.max_files do
        -- 移除最旧的文件
        local oldest = table.remove(file_cache.lru, 1)
        if oldest then
            file_cache.data[oldest] = nil
        end
    end
    
    -- 限制缓存的行数
    local cached_lines = lines
    if #lines > file_cache.max_lines then
        cached_lines = {}
        for i = 1, file_cache.max_lines do
            table.insert(cached_lines, lines[i])
        end
    end
    
    -- 更新 LRU 和缓存数据
    for i, path in ipairs(file_cache.lru) do
        if path == file_path then
            table.remove(file_cache.lru, i)
            break
        end
    end
    table.insert(file_cache.lru, file_path)
    file_cache.data[file_path] = {
        lines = cached_lines,
        mtime = now_ms(),
    }
end

--- 清除缓存
local function clear_cache()
    file_cache.data = {}
    file_cache.lru = {}
end

-- ============================================================================
-- 状态管理
-- ============================================================================
local state = {
    win_id = nil,
    query = "",
    cwd = ".",
    loading = false,
    selected = 1,
    total_matches = 0,
    visible_results = {},  -- 实际显示的子集（虚拟滚动）
    all_results = {},      -- 存储所有结果（支持大量数据）
    scroll_offset = 0,     -- 虚拟滚动偏移量
    request_seq = 0,
    last_applied_request = 0,
    debounce_timer = nil,
}

-- ============================================================================
-- 工具函数
-- ============================================================================

--- 获取目录路径
local function dirname(path)
    if not path or path == "" then return "." end
    local normalized = path:gsub("\\", "/")
    local i = normalized:match("^.*()/")
    if not i then return "." end
    return i == 1 and "/" or normalized:sub(1, i - 1)
end

--- 右填充字符串
local function pad_right(s, width)
    s = tostring(s or "")
    if #s >= width then return s:sub(1, width) end
    return s .. string.rep(" ", width - #s)
end

--- 读取文件预览内容（带缓存）
local function read_preview(file_path, line_1based, context)
    local t1 = now_ms()
    
    -- 将相对路径转换为绝对路径
    local abs_path = file_path
    if not file_path:match("^/") and not file_path:match("^%./") then
        -- 如果不是以 / 或 ./ 开头，添加 cwd
        abs_path = state.cwd .. "/" .. file_path
    elseif file_path:match("^%./") then
        -- 如果是 ./ 开头，替换为 cwd
        abs_path = state.cwd .. file_path:sub(2)
    end
    
    -- 规范化路径（移除多余的 /）
    abs_path = abs_path:gsub("/+", "/")
    
    -- 尝试从缓存获取
    local lines = get_cached_lines(abs_path)
    local cache_hit = lines ~= nil
    
    if not lines then
        -- 缓存未命中，读取文件
        local ok, result = pcall(vim.fn.readfile, abs_path)
        if not ok or not result or #result == 0 then
            return { "(preview unavailable)" }
        end
        lines = result
        -- 缓存文件内容
        cache_file_lines(abs_path, lines)
    end
    
    local t2 = now_ms()
    
    local center = tonumber(line_1based) or 1
    local start_line = math.max(1, center - context)
    local end_line = math.min(#lines, center + context)

    local preview = {}
    for i = start_line, end_line do
        -- 使用 Unicode 字符作为标记
        local marker
        if i == center then
            marker = "►"  -- U+25B6: 黑色右指三角形，突出显示当前行
        else
            marker = " "   -- 空白占位
        end
        
        -- 使用 Unicode 制表符作为分隔线
        local separator = "│"  -- U+2502: 盒制图表垂直线
        local gutter = string.format("%5d", i)  -- 右对齐行号
        
        -- 构建预览行：标记 + 空格 + 行号 + 分隔符 + 内容
        table.insert(preview, string.format("%s %s %s %s", marker, gutter, separator, lines[i] or ""))
    end
    
    local t3 = now_ms()
    
    return preview
end

-- ============================================================================
-- 虚拟滚动优化
-- ============================================================================

--- 更新可见结果子集（虚拟滚动核心）
local function update_visible_results()
    local total = #state.all_results
    local visible_count = CONFIG.list_height
    
    -- 计算滚动偏移量
    local start_offset = 0
    if state.selected > visible_count then
        start_offset = state.selected - visible_count
    end
    
    -- 确保不超过总数
    if start_offset + visible_count > total then
        start_offset = math.max(0, total - visible_count)
    end
    
    state.scroll_offset = start_offset
    
    -- 提取可见子集
    state.visible_results = {}
    local end_offset = math.min(start_offset + visible_count, total)
    for i = start_offset + 1, end_offset do
        table.insert(state.visible_results, state.all_results[i])
    end
    
    -- 调整选中索引在可见范围内的相对位置
    state.selected_in_view = state.selected - start_offset
end

--- 处理大量结果的智能截断
local function process_results(parsed_results)
    local total = #parsed_results
    
    -- 如果启用虚拟滚动，存储所有结果
    if CONFIG.virtual_scroll_enabled and total <= CONFIG.max_results then
        state.all_results = parsed_results
        update_visible_results()
        return
    end
    
    -- 超过最大限制时截断
    if total > CONFIG.max_results then
        state.all_results = {}
        for i = 1, CONFIG.max_results do
            state.all_results[i] = parsed_results[i]
        end
        state.total_matches = CONFIG.max_results
        update_visible_results()
        vim.api.set_status_message(string.format("fg-live-grep: Showing first %d of %d results", 
                                                  CONFIG.max_results, total))
        return
    end
    
    -- 正常情况
    state.all_results = parsed_results
    update_visible_results()
end

-- ============================================================================
-- 数据解析
-- ============================================================================

--- 解析 rg vimgrep 输出
local function parse_rg_vimgrep(lines)
    if not lines then 
        return {} 
    end

    local items = {}
    local parsed_count = 0
    local failed_count = 0
    
    for i, line in ipairs(lines) do
        local fpath, l, c, text = line:match("^(.-):(%d+):(%d+):(.*)$")
        if fpath and l then
            table.insert(items, {
                file = fpath,
                line = tonumber(l),
                col = tonumber(c),
                text = text or "",
            })
            parsed_count = parsed_count + 1
        else
            failed_count = failed_count + 1
        end
    end
    
    return items
end

-- ============================================================================
-- UI 构建 (使用新 DSL)
-- ============================================================================

-- ============================================================================
-- 字符串池优化
-- ============================================================================
local string_pool = {
    cache = {},  -- 字符串缓存
    hits = 0,    -- 命中次数
    misses = 0,  -- 未命中次数
}

--- 从字符串池获取或创建字符串
local function pool_string(s)
    if string_pool.cache[s] then
        string_pool.hits = string_pool.hits + 1
        return string_pool.cache[s]
    end
    string_pool.misses = string_pool.misses + 1
    string_pool.cache[s] = s
    return s
end

--- 获取字符串池统计
local function get_pool_stats()
    return string_pool.hits .. "/" .. (string_pool.hits + string_pool.misses)
end

--- 构建组件视图数据（优化版）
local function build_view_data()
    local t_start = now_ms()
    
    local function get_icon(name)
        if vim.icon and vim.icon.get then
            local icon = vim.icon.get(name)
            return icon or ""
        end
        return ""
    end
    
    -- 获取各种图标
    local search_icon = get_icon("SEARCH")
    local file_icon = get_icon("FILE_TEXT")
    local preview_icon = get_icon("CODE")
    
    local t_preview_start = now_ms()
    -- 使用可见结果中的选中项
    local selected_item = state.visible_results[state.selected_in_view]
    local preview_lines = selected_item and read_preview(selected_item.file, selected_item.line, CONFIG.preview_context) or {"(no selection)"}
    local t_preview_end = now_ms()

    -- 虚拟滚动已经处理好 visible_results，直接使用
    local t_calc_start = now_ms()
    local t_calc_end = now_ms()

    -- 预分配 table 大小
    local t_left_start = now_ms()
    local left_lines = {}
    local left_line_colors = {}
    local cached_spaces = string.rep(" ", CONFIG.results_width)
    
    for i = 1, CONFIG.list_height do
        local item = state.visible_results[i]
        if item then
            -- 使用相对索引
            local marker = (i == state.selected_in_view) and ">>" or "  "
            local line_str = string.format("%s %s:%d:%d %s", marker, item.file, item.line, item.col, item.text)
            table.insert(left_lines, pad_right(line_str, CONFIG.results_width))
            
            -- 为每行添加颜色配置
            if i == state.selected_in_view then
                -- 选中行：白色前景 + 深色背景 + 粗体
                table.insert(left_line_colors, {
                    fg = "#F8F8F2",      -- 白色前景
                    bg = "#44475A",      -- 选中背景（深色）
                    bold = true,
                })
            else
                -- 普通行：蓝色前景
                table.insert(left_line_colors, {
                    fg = "#6272A4",      -- 蓝色前景
                    dim = true,
                })
            end
        else
            table.insert(left_lines, cached_spaces)
            table.insert(left_line_colors, {})
        end
    end
    local t_left_end = now_ms()

    -- 预分配 table 大小
    local t_right_start = now_ms()
    local right_lines = {}
    local right_line_colors = {}
    local max_lines = math.max(CONFIG.list_height, #preview_lines)
    
    for i = 1, max_lines do
        local line = preview_lines[i] or ""
        table.insert(right_lines, pad_right(line, CONFIG.preview_width))
        
        -- 为预览行添加颜色配置
        if line:find("►") then
            -- 当前行标记：绿色前景 + 粗体
            table.insert(right_line_colors, {
                fg = "#50FA7B",      -- 绿色
                bold = true,
            })
        else
            -- 普通预览行：默认前景 + 淡化
            table.insert(right_line_colors, {
                fg = "#F8F8F2",      -- 默认前景
                dim = true,
            })
        end
    end
    local t_right_end = now_ms()
    
    local t_total = now_ms() - t_start
    
    -- 使用获取到的图标构建标题
    local results_title = file_icon .. " Results"
    if state.total_matches > 0 then
        results_title = string.format("%s %d/%d", results_title, state.selected, state.total_matches)
    end
    local preview_title = preview_icon .. " Preview"
    
    return {
        input_line = "❯ " .. state.query .. "█",  -- 使用 U+276F: 装饰性右指括号
        left_title = results_title,
        right_title = preview_title,
        left_lines = left_lines,
        right_lines = right_lines,
        left_line_colors = left_line_colors,      -- 新增：左侧列表颜色配置
        right_line_colors = right_line_colors,    -- 新增：右侧预览颜色配置
        help_lines = {
            "↑/↓: move  PgUp/PgDn: scroll  Enter: open  Type: query  Esc: close",
            string.format("cwd: %s  status: %s  total: %d  showing: %d  cache: %d", 
                state.cwd, state.loading and "searching..." or "idle", 
                state.total_matches, #state.visible_results, #file_cache.lru),
        },
    }
end

--- 使用 DSL 构建窗口组件
local function build_window_spec()
    local view = build_view_data()
    
    local function get_icon(name)
        if vim.icon and vim.icon.get then
            local icon = vim.icon.get(name)
            return icon or ""
        end
        return ""
    end
    local search_icon = get_icon("SEARCH")

    local spec = {
        type = "window",
        window_title = search_icon .. " FG Live Grep",
        window_title_decorators = {
            bold = true,
            inverted = true,  -- 反白效果
            color = "cyan",   -- 青色文字
        },
        min_width = CONFIG.window_width,
        min_height = CONFIG.window_height,
        component_input_line = view.input_line,
        component_left_title = view.left_title,
        component_right_title = view.right_title,
        component_left_lines = view.left_lines,
        component_right_lines = view.right_lines,
        component_left_line_colors = view.left_line_colors,      -- 新增：传递左侧颜色配置
        component_right_line_colors = view.right_line_colors,    -- 新增：传递右侧颜色配置
        component_help_lines = view.help_lines,
        decorators = {
            {name = "border", value = "double"},
            {name = "border_color", value = "blue"},
        },
    }
    
    return spec
end

-- ============================================================================
-- 搜索逻辑
-- ============================================================================

--- 更新窗口显示
local function update_window(reason)
    local t_start = now_ms()
    
    if not state.win_id then 
        return 
    end
    
    local t_build_start = now_ms()
    local spec = build_window_spec()
    local t_build_end = now_ms()
    
    local t_update_start = now_ms()
    local ok = vim.ui.update_component_window(state.win_id, spec)
    local t_update_end = now_ms()
    
    local t_total = now_ms() - t_start
end

--- 执行搜索（异步版本）
local function run_search(request_id, query)
    local t_start = now_ms()
    
    if not query or query == "" then
        state.visible_results = {}
        state.total_matches = 0
        state.selected = 1
        state.loading = false
        update_window("empty_query")
        return
    end

    state.loading = true
    local t_update1 = now_ms()
    update_window("search_loading")
    local t_update2 = now_ms()

    local argv = {
        "rg", "--vimgrep", "--smart-case", "--no-heading",
        "--line-number", "--column", "--color", "never",
        query, ".",
    }

    local t_exec_start = now_ms()
    
    -- 使用异步 API
    local request_id_async = vim.fn.systemlist_async(argv, {
        cwd = state.cwd,
        timeout_ms = CONFIG.timeout_ms,
        max_output_bytes = CONFIG.max_output_bytes,
    }, function(lines, err)
        -- 异步回调
        local t_callback_start = now_ms()
        
        -- 检查是否是过期的请求
        if request_id < state.last_applied_request then
            return
        end

        state.last_applied_request = request_id
        state.loading = false

        if not lines then
            state.all_results = {}
            state.visible_results = {}
            state.total_matches = 0
            state.scroll_offset = 0
            vim.api.set_status_message("fg-live-grep error: " .. tostring(err or "unknown"))
            update_window("search_error")
            return
        end

        local t_parse_start = now_ms()
        local parsed = parse_rg_vimgrep(lines)
        local t_parse_end = now_ms()
        
        -- 使用新的虚拟滚动处理函数
        process_results(parsed)
        
        state.total_matches = #state.all_results
        state.selected = math.max(1, math.min(state.selected, math.max(1, #state.all_results)))
        
        local t_update_start = now_ms()
        update_window("search_done")
        local t_update_end = now_ms()
        
        local t_total = now_ms() - t_callback_start
        
        -- Display result statistics
        if CONFIG.debug then
            vim.api.set_status_message(string.format("fg-live-grep: %d results (showing %d)", 
                                                      state.total_matches, #state.visible_results))
        end
    end)
    
    
    -- 立即返回，不阻塞主线程
    local t_total = now_ms() - t_start
end

--- 延迟搜索 (防抖) - 当 debounce_ms=0 时立即执行
local function schedule_search_debounced(query)
    local t_start = now_ms()
    
    state.query = query or ""
    state.request_seq = state.request_seq + 1
    local request_id = state.request_seq
    local t_seq_end = now_ms()

    if state.debounce_timer then
        local t_cancel_start = now_ms()
        vim.defer_cancel(state.debounce_timer)
        state.debounce_timer = nil
        local t_cancel_end = now_ms()
    end

    -- 当 debounce_ms=0 时，立即执行搜索
    if CONFIG.debounce_ms == 0 then
        run_search(request_id, state.query)
    else
        local t_defer_start = now_ms()
        state.debounce_timer = vim.defer_fn(function()
            local t2 = now_ms()
            run_search(request_id, state.query)
        end, CONFIG.debounce_ms)
        local t_defer_end = now_ms()
    end
    
    local t_total = now_ms() - t_start
end

-- ============================================================================
-- 事件处理
-- ============================================================================

--- 打开选中的结果
local function open_selected_result()
    local item = state.visible_results[state.selected]
    if not item then
        vim.api.set_status_message("fg-live-grep: no selection")
        return
    end

    if not vim.api.open_file(item.file) then
        vim.api.set_status_message("fg-live-grep: open_file failed")
        return
    end

    vim.api.set_cursor_pos({ row = math.max(0, item.line - 1), col = math.max(0, item.col - 1) })
    vim.api.set_status_message(string.format("fg-live-grep: %s:%d:%d", item.file, item.line, item.col))
end

--- 窗口事件处理
local function on_window_event(event_name, payload)
    local t1 = now_ms()
    
    if event_name == "arrow_down" then
        if #state.all_results > 0 then
            state.selected = math.min(#state.all_results, state.selected + 1)
            -- 重新计算可见子集
            update_visible_results()
            update_window("arrow_down")
        end
        return true
    end

    if event_name == "arrow_up" then
        if #state.all_results > 0 then
            state.selected = math.max(1, state.selected - 1)
            -- 重新计算可见子集
            update_visible_results()
            update_window("arrow_up")
        end
        return true
    end

    if event_name == "pagedown" then
        if #state.all_results > 0 then
            local old_selected = state.selected
            -- 向下翻页：一次翻 list_height 行（约一屏）
            state.selected = math.min(#state.all_results, state.selected + CONFIG.list_height)
            if state.selected ~= old_selected then
                update_visible_results()
                update_window("pagedown")
                vim.api.set_status_message(string.format("fg-live-grep: %d/%d", state.selected, state.total_matches))
            end
        end
        return true
    end

    if event_name == "pageup" then
        if #state.all_results > 0 then
            local old_selected = state.selected
            -- 向上翻页：一次翻 list_height 行（约一屏）
            state.selected = math.max(1, state.selected - CONFIG.list_height)
            if state.selected ~= old_selected then
                update_visible_results()
                update_window("pageup")
                vim.api.set_status_message(string.format("fg-live-grep: %d/%d", state.selected, state.total_matches))
            end
        end
        return true
    end

    if event_name == "enter" then
        local item = state.all_results[state.selected]
        if item then
            open_selected_result()
        else
            vim.api.set_status_message("fg-live-grep: no selection")
        end
        return true
    end

    if event_name == "backspace" then
        if #state.query > 0 then
            local old_query = state.query
            state.query = state.query:sub(1, -2)
            schedule_search_debounced(state.query)
            local t2 = now_ms()
        end
        return true
    end

    if event_name == "char" then
        if payload and payload ~= "" then
            local t_char_start = now_ms()
            local old_query = state.query
            state.query = state.query .. payload
            local t_query_update = now_ms()
            
            local t_schedule_start = now_ms()
            schedule_search_debounced(state.query)
            local t_schedule_end = now_ms()
            
            local t_total = now_ms() - t1
        end
        return true
    end

    if event_name == "escape" then
        if state.win_id then
            vim.ui.close_window(state.win_id)
            state.win_id = nil
            local t2 = now_ms()
        end
        return true
    end

    return false
end

-- ============================================================================
-- 窗口管理
-- ============================================================================

--- 更新 CWD
local function update_cwd_from_current_file()
    local fp = vim.api.get_filepath()
    state.cwd = fp and dirname(fp) or "."
end

--- 确保窗口存在
local function ensure_window()
    if state.win_id then 
        return 
    end

    local t1 = now_ms()
    local spec = build_window_spec()
    local t2 = now_ms()
    
    spec.on_event = on_window_event
    
    state.win_id = vim.ui.open_component_window(spec)
    local t3 = now_ms()
    
end

-- ============================================================================
-- 命令注册
-- ============================================================================

--- FgLiveGrep 命令
vim.api.create_user_command("FgLiveGrep", function(opts)
    update_cwd_from_current_file()
    ensure_window()

    local q = (opts and opts.args and opts.args ~= "") and opts.args or state.query
    schedule_search_debounced(q)
end, {
    nargs = "*",
    desc = "Open fg live grep window and search",
    force = true,
})

--- FgLiveGrepClose 命令
vim.api.create_user_command("FgLiveGrepClose", function()
    if state.win_id then
        vim.ui.close_window(state.win_id)
        state.win_id = nil
    end
end, { desc = "Close fg live grep window", force = true })

-- ============================================================================
-- 快捷键和调色板
-- ============================================================================

--- 快捷键
vim.keymap.set("n", "<A-g>", ":FgLiveGrep<CR>", { desc = "Open FG live grep" })

--- 调色板命令
vim.api.register_palette_command({
    id = "fg.live_grep",
    name = "FG: Live Grep",
    desc = "Open fg live grep panel",
    keywords = { "grep", "search", "rg", "live" },
    force = true,
}, function()
    update_cwd_from_current_file()
    ensure_window()
    schedule_search_debounced(state.query)
end)
