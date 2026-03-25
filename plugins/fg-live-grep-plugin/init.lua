plugin_name = "fg-live-grep-plugin"
plugin_version = "1.0.1"
plugin_description = "fg live grep with 3-column window + debounce refresh"
plugin_author = "pnana"

local state = {
    win_id = nil,
    query = "",
    cwd = ".",
    loading = false,
    selected = 1,
    total_matches = 0,
    visible_results = {},
    max_results = 60,
    debounce_timer = nil,
    request_seq = 0,
    last_applied_request = 0,
}

local function split(s, sep)
    local out = {}
    if not s or s == "" then
        return out
    end
    for part in string.gmatch(s, "([^" .. sep .. "]+)") do
        table.insert(out, part)
    end
    return out
end

local function dirname(path)
    if not path or path == "" then
        return "."
    end
    local normalized = string.gsub(path, "\\", "/")
    local i = normalized:match("^.*()/")
    if not i then
        return "."
    end
    if i == 1 then
        return "/"
    end
    return normalized:sub(1, i - 1)
end

local function pad_right(s, width)
    s = tostring(s or "")
    local n = #s
    if n >= width then
        return s:sub(1, width)
    end
    return s .. string.rep(" ", width - n)
end

local function read_preview(file_path, line_1_based, context)
    local lines = vim.fn.readfile(file_path)
    if not lines or #lines == 0 then
        return {"(preview unavailable)"}
    end

    local center = tonumber(line_1_based) or 1
    local start_line = math.max(1, center - context)
    local end_line = math.min(#lines, center + context)

    local preview = {}
    for i = start_line, end_line do
        local marker = (i == center) and ">" or " "
        local num = tostring(i)
        local gutter = string.rep(" ", math.max(0, 5 - #num)) .. num
        local text = lines[i] or ""
        table.insert(preview, marker .. " " .. gutter .. " | " .. text)
    end

    return preview
end

local function parse_rg_vimgrep(lines)
    local items = {}
    if not lines then
        return items
    end

    for _, line in ipairs(lines) do
        local f, l, c, text = string.match(line, "^(.-):(%d+):(%d+):(.*)$")
        if f and l and c then
            table.insert(items, {
                file = f,
                line = tonumber(l),
                col = tonumber(c),
                text = text or "",
            })
        end
    end

    return items
end

local function render_window_lines()
    local results_w = 58
    local preview_w = 70
    local right_w = 42

    local left_header = "Results"
    local mid_header = "Preview"
    local right_header = "Input + Count"

    local out = {}
    table.insert(out, pad_right(left_header, results_w) .. " | " .. pad_right(mid_header, preview_w) .. " | " .. pad_right(right_header, right_w))
    table.insert(out, string.rep("-", results_w) .. "-+" .. string.rep("-", preview_w) .. "-+" .. string.rep("-", right_w))

    local selected_item = state.visible_results[state.selected]
    local preview_lines = {}
    if selected_item then
        preview_lines = read_preview(selected_item.file, selected_item.line, 5)
    else
        preview_lines = {"(no selection)"}
    end

    local right_lines = {
        "query: " .. state.query,
        "cwd: " .. state.cwd,
        "status: " .. (state.loading and "searching..." or "idle"),
        "matches: " .. tostring(state.total_matches),
        "shown: " .. tostring(#state.visible_results),
        "selected: " .. tostring(state.selected) .. "/" .. tostring(math.max(1, #state.visible_results)),
        "",
        "Commands:",
        ":FgLiveGrep <q>",
        ":FgLiveGrepSet <q>",
        ":FgLiveGrepNext",
        ":FgLiveGrepPrev",
        ":FgLiveGrepOpen",
        ":FgLiveGrepClose",
    }

    local row_count = math.max(#state.visible_results, #preview_lines, #right_lines)
    if row_count < 16 then
        row_count = 16
    end

    for i = 1, row_count do
        local left = ""
        local item = state.visible_results[i]
        if item then
            local marker = (i == state.selected) and ">" or " "
            local left_text = string.format("%s %s:%d:%d %s", marker, item.file, item.line, item.col, item.text)
            left = pad_right(left_text, results_w)
        else
            left = string.rep(" ", results_w)
        end

        local mid = pad_right(preview_lines[i] or "", preview_w)
        local right = pad_right(right_lines[i] or "", right_w)

        table.insert(out, left .. " | " .. mid .. " | " .. right)
    end

    return out
end

local function ensure_window()
    if state.win_id then
        return
    end

    -- 新 API: open_window 返回窗口 ID
    state.win_id = vim.ui.open_window({
        title = "FG Live Grep",
        lines = render_window_lines(),
    })
end

local function refresh_window()
    ensure_window()
    -- 新 API: update_window 需要窗口 ID 作为第一个参数
    vim.ui.update_window(state.win_id, {
        title = "FG Live Grep",
        lines = render_window_lines(),
    })
end

local function run_search(request_id, query)
    if not query or query == "" then
        state.visible_results = {}
        state.total_matches = 0
        state.selected = 1
        state.loading = false
        refresh_window()
        return
    end

    state.loading = true
    refresh_window()

    local argv = {
        "rg",
        "--vimgrep",
        "--smart-case",
        "--no-heading",
        "--line-number",
        "--column",
        "--color",
        "never",
        query,
        ".",
    }

    local lines, err = vim.fn.systemlist(argv, {
        cwd = state.cwd,
        timeout_ms = 1800,
        max_output_bytes = 1024 * 1024,
    })

    if request_id < state.last_applied_request then
        return
    end

    state.last_applied_request = request_id
    state.loading = false

    if not lines then
        state.visible_results = {}
        state.total_matches = 0
        vim.api.set_status_message("fg-live-grep error: " .. tostring(err or "unknown"))
        refresh_window()
        return
    end

    local parsed = parse_rg_vimgrep(lines)
    state.total_matches = #parsed

    if #parsed > state.max_results then
        local trimmed = {}
        for i = 1, state.max_results do
            trimmed[i] = parsed[i]
        end
        state.visible_results = trimmed
    else
        state.visible_results = parsed
    end

    if state.selected < 1 then
        state.selected = 1
    end
    if state.selected > #state.visible_results then
        state.selected = math.max(1, #state.visible_results)
    end

    refresh_window()
end

local function schedule_search_debounced(query)
    state.query = query or ""
    state.request_seq = state.request_seq + 1
    local request_id = state.request_seq

    if state.debounce_timer then
        vim.defer_cancel(state.debounce_timer)
        state.debounce_timer = nil
    end

    state.debounce_timer = vim.defer_fn(function()
        run_search(request_id, state.query)
    end, 180)

    refresh_window()
end

local function open_selected_result()
    local item = state.visible_results[state.selected]
    if not item then
        vim.api.set_status_message("fg-live-grep: no selection")
        return
    end

    local ok = vim.api.open_file(item.file)
    if not ok then
        vim.api.set_status_message("fg-live-grep: open_file failed")
        return
    end

    vim.api.set_cursor_pos({ row = math.max(0, item.line - 1), col = math.max(0, item.col - 1) })
    vim.api.set_status_message(string.format("fg-live-grep: %s:%d:%d", item.file, item.line, item.col))
end

local function update_cwd_from_current_file()
    local fp = vim.api.get_filepath()
    if fp then
        state.cwd = dirname(fp)
    else
        state.cwd = "."
    end
end

vim.api.create_user_command("FgLiveGrep", function(opts)
    update_cwd_from_current_file()
    ensure_window()

    local q = ""
    if opts and opts.args and opts.args ~= "" then
        q = opts.args
    else
        q = state.query
    end

    schedule_search_debounced(q)
end, {
    nargs = "*",
    desc = "Open fg live grep window and search",
    force = true,
})

vim.api.create_user_command("FgLiveGrepSet", function(opts)
    local q = (opts and opts.args) or ""
    schedule_search_debounced(q)
end, {
    nargs = "*",
    desc = "Set query and debounce refresh",
    force = true,
})

vim.api.create_user_command("FgLiveGrepNext", function()
    if #state.visible_results == 0 then
        return
    end
    state.selected = math.min(#state.visible_results, state.selected + 1)
    refresh_window()
end, { desc = "Move selection down", force = true })

vim.api.create_user_command("FgLiveGrepPrev", function()
    if #state.visible_results == 0 then
        return
    end
    state.selected = math.max(1, state.selected - 1)
    refresh_window()
end, { desc = "Move selection up", force = true })

vim.api.create_user_command("FgLiveGrepOpen", function()
    open_selected_result()
end, { desc = "Open selected grep result", force = true })

vim.api.create_user_command("FgLiveGrepClose", function()
    if state.win_id then
        -- 新 API: close_window 需要窗口 ID 作为参数
        vim.ui.close_window(state.win_id)
        state.win_id = nil
    end
end, { desc = "Close fg live grep window", force = true })

vim.keymap.set("n", "<A-g>", ":FgLiveGrep<CR>", { desc = "Open FG live grep" })
vim.keymap.set("n", "<A-j>", ":FgLiveGrepNext<CR>", { desc = "FG grep next" })
vim.keymap.set("n", "<A-k>", ":FgLiveGrepPrev<CR>", { desc = "FG grep prev" })
vim.keymap.set("n", "<A-o>", ":FgLiveGrepOpen<CR>", { desc = "FG grep open" })

vim.api.register_palette_command({
    id = "fg.live_grep",
    name = "FG: Live Grep",
    desc = "Open fg live grep panel",
    keywords = { "grep", "search", "rg", "live" },
    force = true,
}, function()
    update_cwd_from_current_file()
    ensure_window()

    vim.ui.input({
        title = "FG Live Grep",
        prompt = "Query:",
        default = state.query,
    }, function(value)
        if value == nil then
            return
        end
        schedule_search_debounced(value)
    end)
end)
