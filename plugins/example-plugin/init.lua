-- 示例插件：展示 pnana 插件系统的基本功能
plugin_name = "example-plugin"
plugin_version = "1.0.0"
plugin_description = "pnana plugin system example"
plugin_author = "pnana"

-- 插件加载时显示消息（已禁用，避免干扰用户）
-- vim.api.set_status_message("Example plugin loaded!")

-- 使用新API注册自定义命令：显示插件信息
vim.api.create_user_command("PluginInfo", function(opts)
    local info = string.format(
        "Plugin: %s v%s\nDescription: %s\nAuthor: %s",
        plugin_name,
        plugin_version,
        plugin_description,
        plugin_author
    )
    vim.api.set_status_message(info)
end, { desc = "Show plugin information" })

-- 使用新API注册自定义命令：显示当前文件信息
vim.api.create_user_command("FileInfo", function(opts)
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
end, { desc = "Show current file information" })

-- 使用新API注册自定义命令：插入当前时间
vim.api.create_user_command("InsertTime", function(opts)
    local time = os.date("%Y-%m-%d %H:%M:%S")
    local pos = vim.api.get_cursor_pos()
    vim.api.insert_text(pos.row, pos.col, time)
end, { desc = "Insert current timestamp" })

-- 使用新API注册自定义命令：插入当前日期
vim.api.create_user_command("InsertDate", function(opts)
    local date = os.date("%Y-%m-%d")
    local pos = vim.api.get_cursor_pos()
    vim.api.insert_text(pos.row, pos.col, date)
end, { desc = "Insert current date" })

-- 使用新API监听文件打开事件
vim.api.create_autocmd("FileOpened", {
    pattern = "*",
}, function(args)
    local path = (type(args) == "table" and args.file) or "unknown"
    vim.api.set_status_message("File opened: " .. path)
end)

-- 使用新API监听文件保存事件
vim.api.create_autocmd("FileSaved", {
    pattern = "*",
}, function(args)
    local path = (type(args) == "table" and args.file) or "unknown"
    vim.api.set_status_message("File saved: " .. path)
end)

-- 使用新API注册键位映射：F2 显示文件信息
vim.keymap.set("n", "<F2>", function()
    local filepath = vim.api.get_filepath()
    if filepath then
        vim.api.set_status_message("File: " .. filepath)
    end
end, { desc = "Show file path" })

-- 插件初始化完成
-- vim.log.info("Example plugin initialized successfully!")
