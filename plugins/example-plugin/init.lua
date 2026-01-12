-- 示例插件：展示 pnana 插件系统的基本功能
plugin_name = "example-plugin"
plugin_version = "1.0.0"
plugin_description = "pnana plugin system example"
plugin_author = "pnana"

-- 插件加载时显示消息（已禁用，避免干扰用户）
-- vim.api.set_status_message("Example plugin loaded!")

-- 注册自定义命令：显示插件信息
vim.cmd("PluginInfo", function()
    local info = string.format(
        "Plugin: %s v%s\nDescription: %s\nAuthor: %s",
        plugin_name,
        plugin_version,
        plugin_description,
        plugin_author
    )
    vim.api.set_status_message(info)
end)

-- 注册自定义命令：显示当前文件信息
vim.cmd("FileInfo", function()
    local filepath = vim.api.get_filepath()
    local line_count = vim.api.get_line_count()
    local pos = vim.api.get_cursor_pos()
    
    if filepath then
        local info = string.format(
            "File: %s\nLines: %d\nCursor: %d:%d",
            filepath,
            line_count,
            pos.row + 1,  -- 转换为 1-based
            pos.col + 1
        )
        vim.api.set_status_message(info)
    else
        vim.api.set_status_message("No file opened")
    end
end)

-- 注册自定义命令：插入当前时间
vim.cmd("InsertTime", function()
    local time = os.date("%Y-%m-%d %H:%M:%S")
    local pos = vim.api.get_cursor_pos()
    vim.api.insert_text(pos.row, pos.col, time)
end)

-- 注册自定义命令：插入当前日期
vim.cmd("InsertDate", function()
    local date = os.date("%Y-%m-%d")
    local pos = vim.api.get_cursor_pos()
    vim.api.insert_text(pos.row, pos.col, date)
end)

-- 监听文件打开事件
vim.autocmd("FileOpened", function(filepath)
    vim.api.set_status_message("File opened: " .. (filepath or "unknown"))
end)

-- 监听文件保存事件
vim.autocmd("FileSaved", function(filepath)
    vim.api.set_status_message("File saved: " .. (filepath or "unknown"))
end)

-- 注册键位映射：F2 显示文件信息
vim.keymap("n", "<F2>", function()
    -- 调用 FileInfo 命令
    -- 注意：这里简化处理，实际应该通过命令系统调用
    local filepath = vim.api.get_filepath()
    if filepath then
        vim.api.set_status_message("File: " .. filepath)
    end
end)

-- 插件初始化完成（已禁用输出，避免干扰用户）
-- print("Example plugin initialized successfully!")
