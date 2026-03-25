-- 主题增强插件：为 pnana 添加专业主题
plugin_name = "theme-pro-plugin"
plugin_version = "1.0.0"
plugin_description = "Professional theme pack for pnana with 4 additional themes"
plugin_author = "pnana"

-- 插件加载时显示消息
vim.api.set_status_message("Theme Pro Plugin loaded! Added 4 professional themes.")

-- 定义4个专业主题
local themes = {
    -- Cyberpunk 主题 - 霓虹科技风格
    {
        name = "cyberpunk",
        config = {
            background = {20, 20, 30},           -- #14141e
            foreground = {255, 255, 255},        -- #ffffff
            current_line = {40, 40, 60},         -- #28283c
            selection = {60, 60, 90},            -- #3c3c5a
            line_number = {100, 100, 130},       -- #646482
            line_number_current = {255, 255, 255}, -- #ffffff
            statusbar_bg = {30, 30, 45},         -- #1e1e2d
            statusbar_fg = {255, 255, 255},      -- #ffffff
            menubar_bg = {20, 20, 30},           -- #14141e
            menubar_fg = {255, 255, 255},        -- #ffffff
            helpbar_bg = {30, 30, 45},           -- #1e1e2d
            helpbar_fg = {180, 180, 200},        -- #b4b4c8
            helpbar_key = {0, 255, 255},         -- #00ffff (cyan)
            keyword = {255, 20, 147},            -- #ff1493 (deep pink)
            string = {255, 165, 0},              -- #ffa500 (orange)
            comment = {100, 100, 130},           -- #646482
            number = {0, 255, 255},              -- #00ffff (cyan)
            ["function"] = {255, 0, 255},        -- #ff00ff (magenta)
            ["type"] = {0, 255, 0},               -- #00ff00 (lime)
            operator_color = {255, 20, 147},     -- #ff1493 (deep pink)
            error = {255, 0, 0},                 -- #ff0000 (red)
            warning = {255, 165, 0},             -- #ffa500 (orange)
            info = {0, 255, 255},                -- #00ffff (cyan)
            success = {0, 255, 0}                -- #00ff00 (lime)
        }
    },

    -- Forest 主题 - 森林自然风格
    {
        name = "forest",
        config = {
            background = {20, 35, 25},           -- #142319
            foreground = {220, 235, 200},        -- #dcebca
            current_line = {35, 55, 40},         -- #233728
            selection = {50, 75, 55},            -- #324b37
            line_number = {120, 140, 110},       -- #788c6e
            line_number_current = {220, 235, 200}, -- #dcebca
            statusbar_bg = {30, 45, 35},         -- #1e2d23
            statusbar_fg = {220, 235, 200},      -- #dcebca
            menubar_bg = {20, 35, 25},           -- #142319
            menubar_fg = {220, 235, 200},        -- #dcebca
            helpbar_bg = {30, 45, 35},           -- #1e2d23
            helpbar_fg = {160, 180, 150},        -- #a0b496
            helpbar_key = {152, 251, 152},       -- #98fb98 (pale green)
            keyword = {34, 139, 34},             -- #228b22 (forest green)
            string = {50, 205, 50},              -- #32cd32 (lime green)
            comment = {107, 142, 35},            -- #6b8e23 (olive drab)
            number = {154, 205, 50},             -- #9acd32 (yellow green)
            ["function"] = {60, 179, 113},       -- #3cb371 (medium sea green)
            ["type"] = {25, 25, 112},            -- #191970 (midnight blue)
            operator_color = {34, 139, 34},      -- #228b22 (forest green)
            error = {220, 20, 60},               -- #dc143c (crimson)
            warning = {255, 140, 0},             -- #ff8c00 (dark orange)
            info = {60, 179, 113},               -- #3cb371 (medium sea green)
            success = {50, 205, 50}              -- #32cd32 (lime green)
        }
    },

    -- Sunset 主题 - 日落温暖风格
    {
        name = "sunset",
        config = {
            background = {45, 25, 15},           -- #2d190f
            foreground = {255, 235, 205},        -- #ffebcd
            current_line = {70, 40, 25},         -- #462819
            selection = {95, 55, 35},            -- #5f3723
            line_number = {160, 120, 90},        -- #a0785a
            line_number_current = {255, 235, 205}, -- #ffebcd
            statusbar_bg = {55, 30, 20},         -- #371e14
            statusbar_fg = {255, 235, 205},      -- #ffebcd
            menubar_bg = {45, 25, 15},           -- #2d190f
            menubar_fg = {255, 235, 205},        -- #ffebcd
            helpbar_bg = {55, 30, 20},           -- #371e14
            helpbar_fg = {200, 160, 130},        -- #c8a082
            helpbar_key = {255, 218, 185},       -- #ffdab9 (peach puff)
            keyword = {205, 92, 92},             -- #cd5c5c (indian red)
            string = {255, 160, 122},            -- #ffa07a (light salmon)
            comment = {160, 120, 90},            -- #a0785a
            number = {255, 165, 0},              -- #ffa500 (orange)
            ["function"] = {255, 140, 0},        -- #ff8c00 (dark orange)
            ["type"] = {139, 69, 19},            -- #8b4513 (saddle brown)
            operator_color = {205, 92, 92},      -- #cd5c5c (indian red)
            error = {255, 0, 0},                 -- #ff0000 (red)
            warning = {255, 140, 0},             -- #ff8c00 (dark orange)
            info = {255, 165, 0},                -- #ffa500 (orange)
            success = {255, 215, 0}              -- #ffd700 (gold)
        }
    },

    -- Ice 主题 - 冰雪清凉风格
    {
        name = "ice",
        config = {
            background = {15, 25, 35},           -- #0f1923
            foreground = {220, 235, 255},        -- #dcebff
            current_line = {25, 40, 55},         -- #192837
            selection = {35, 55, 75},            -- #23374b
            line_number = {100, 130, 160},       -- #6482a0
            line_number_current = {220, 235, 255}, -- #dcebff
            statusbar_bg = {20, 30, 45},         -- #141e2d
            statusbar_fg = {220, 235, 255},      -- #dcebff
            menubar_bg = {15, 25, 35},           -- #0f1923
            menubar_fg = {220, 235, 255},        -- #dcebff
            helpbar_bg = {20, 30, 45},           -- #141e2d
            helpbar_fg = {150, 180, 210},        -- #96b4d2
            helpbar_key = {173, 216, 230},       -- #add8e6 (light blue)
            keyword = {70, 130, 180},            -- #4682b4 (steel blue)
            string = {135, 206, 235},            -- #87ceeb (sky blue)
            comment = {100, 130, 160},           -- #6482a0
            number = {176, 196, 222},            -- #b0c4de (light steel blue)
            ["function"] = {100, 149, 237},      -- #6495ed (cornflower blue)
            ["type"] = {25, 25, 112},            -- #191970 (midnight blue)
            operator_color = {70, 130, 180},     -- #4682b4 (steel blue)
            error = {255, 99, 71},               -- #ff6347 (tomato)
            warning = {255, 165, 0},             -- #ffa500 (orange)
            info = {100, 149, 237},              -- #6495ed (cornflower blue)
            success = {135, 206, 235}            -- #87ceeb (sky blue)
        }
    }
}

-- 注册所有主题
for _, theme in ipairs(themes) do
    vim.api.add_theme(theme.name, theme.config)
end

-- 插件卸载清理函数
local function cleanup_themes()
    vim.api.set_status_message("Theme Pro Plugin unloaded - removing themes...")
    local themes_to_remove = {"cyberpunk", "forest", "sunset", "ice"}
    local current_theme = vim.api.get_current_theme()

    for _, theme_name in ipairs(themes_to_remove) do
        local success = vim.api.remove_theme(theme_name)
        if success then
            vim.api.set_status_message("Theme '" .. theme_name .. "' removed successfully")
            -- 如果当前主题是这个插件提供的主题，重置为默认主题
            if current_theme == theme_name then
                vim.api.set_theme("monokai")
                vim.api.set_status_message("Reset theme to default (monokai) since '" .. theme_name .. "' was removed")
            end
        else
            vim.api.set_status_message("Failed to remove theme '" .. theme_name .. "'")
        end
    end
    vim.api.set_status_message("Theme Pro Plugin cleanup completed.")
end

-- 使用新API注册插件卸载事件监听器
vim.api.create_autocmd("PluginUnload", {
    pattern = "*",
}, function(args)
    if args and args.file == "theme-pro-plugin" then
        cleanup_themes()
    end
end)

-- 使用新API注册命令：快速切换主题
vim.api.create_user_command("ThemeSetCyberpunk", function(opts)
    vim.api.set_theme("cyberpunk")
    vim.api.set_status_message("Theme set to Cyberpunk")
end, { desc = "Set theme to Cyberpunk" })

vim.api.create_user_command("ThemeSetForest", function(opts)
    vim.api.set_theme("forest")
    vim.api.set_status_message("Theme set to Forest")
end, { desc = "Set theme to Forest" })

vim.api.create_user_command("ThemeSetSunset", function(opts)
    vim.api.set_theme("sunset")
    vim.api.set_status_message("Theme set to Sunset")
end, { desc = "Set theme to Sunset" })

vim.api.create_user_command("ThemeSetIce", function(opts)
    vim.api.set_theme("ice")
    vim.api.set_status_message("Theme set to Ice")
end, { desc = "Set theme to Ice" })

-- 使用新API注册命令：显示当前主题
vim.api.create_user_command("ThemeCurrent", function(opts)
    local current = vim.api.get_current_theme()
    vim.api.set_status_message("Current theme: " .. current)
end, { desc = "Show current theme" })

-- 使用新API注册键位映射：快速切换主题
vim.keymap.set("n", "<F9>", function()
    local theme_list = {"cyberpunk", "forest", "sunset", "ice"}
    local current = vim.api.get_current_theme()
    local next_index = 1

    for i, name in ipairs(theme_list) do
        if name == current then
            next_index = (i % #theme_list) + 1
            break
        end
    end

    vim.api.set_theme(theme_list[next_index])
    vim.api.set_status_message("Theme: " .. theme_list[next_index])
end, { desc = "Cycle through Theme Pro themes" })

-- 插件初始化完成
-- vim.log.info("Theme Pro Plugin: 4 professional themes loaded!")
