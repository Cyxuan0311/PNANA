-- 状态栏美化插件：为 pnana 添加精美的状态栏效果
plugin_name = "statusbar-beautify-plugin"
plugin_version = "1.0.0"
plugin_description = "Beautiful statusbar effects for pnana with enhanced colors and icons"
plugin_author = "pnana"

-- 状态栏美化配置 - 完全在 Lua 中定义，启用后有微妙的视觉增强
local beautify_config = {
    enabled = true,
    bg_color = {50, 60, 80},        -- 深蓝色背景 - 比默认更鲜艳但不突兀
    fg_color = {240, 245, 255},     -- 浅蓝色文字 - 更亮的文字

    -- 图标优化配置
    icon_enhancement = {
        -- 文件类型图标映射优化
        file_icons = {
            cpp = "",           -- C++ 文件
            c = "",             -- C 文件
            python = "",       -- Python 文件
            javascript = "",   -- JavaScript 文件
            typescript = "",   -- TypeScript 文件
            java = "",         -- Java 文件
            go = "",           -- Go 文件
            rust = "",         -- Rust 文件
            lua = "",          -- Lua 文件
            markdown = "",     -- Markdown 文件
            json = "",         -- JSON 文件
            html = "",         -- HTML 文件
            css = "",          -- CSS 文件
            shell = "",        -- Shell 文件
            git = "",          -- Git 相关
            default = ""       -- 默认文件图标
        },

        -- 区域图标映射
        region_icons = {
            edit = "",         -- 编辑模式
            terminal = "",     -- 终端
            files = "",        -- 文件浏览器
            tabs = "ﱡ",         -- 标签页
            help = "",         -- 帮助
            info = ""          -- 信息
        },

        -- 状态图标映射
        status_icons = {
            modified = "●",     -- 已修改
            readonly = "",     -- 只读
            git_branch = "",   -- Git 分支
            git_modified = "", -- Git 修改
            selection = "",    -- 选择模式
            position = ""      -- 位置信息
        }
    },

    -- 颜色增强配置
    color_enhancement = {
        -- 不同元素使用不同的颜色
        elements = {
            filename = {255, 255, 200},     -- 文件名：浅黄
            mode_indicator = {100, 200, 255}, -- 模式指示器：蓝
            git_info = {150, 255, 150},     -- Git 信息：绿
            ssh_info = {255, 150, 200},     -- SSH 信息：粉
            position = {255, 200, 100},     -- 位置：橙
            progress = {200, 150, 255},     -- 进度：紫
            encoding = {150, 200, 255},     -- 编码：浅蓝
            line_ending = {200, 200, 150},  -- 行尾：浅黄
            file_type = {255, 150, 150},    -- 文件类型：浅红
            selection = {255, 180, 100}     -- 选择：橙
        }
    },

    -- 布局增强配置
    layout_enhancement = {
        spacing = 1,            -- 元素间距
        padding = 1,            -- 内边距
        border_radius = 0,      -- 边框圆角
        shadow = false,          -- 阴影效果
        gradient = false         -- 渐变效果
    }
}

-- 应用状态栏美化配置
print("[DEBUG] Applying statusbar beautify config...")
local result = vim.api.set_statusbar_beautify(beautify_config)

if result then
    vim.api.set_status_message("✓ Statusbar Beautify Plugin loaded! Enhanced statusbar with beautiful colors and optimized icons!")
    print("[DEBUG] ✓ Statusbar beautify config applied successfully")
    print(string.format("[DEBUG] BG Color: RGB(%d, %d, %d)", beautify_config.bg_color[1], beautify_config.bg_color[2], beautify_config.bg_color[3]))
    print(string.format("[DEBUG] FG Color: RGB(%d, %d, %d)", beautify_config.fg_color[1], beautify_config.fg_color[2], beautify_config.fg_color[3]))
    print("[DEBUG] Plugin should now be active - check statusbar appearance")
else
    vim.api.set_status_message("✗ Statusbar Beautify Plugin failed to load!")
    print("[DEBUG] ✗ Statusbar beautify config failed to apply")
end

-- 插件禁用时的清理函数
function on_disable()
    print("[DEBUG] Statusbar beautify plugin disabled - resetting config")
    local reset_config = {
        enabled = false,
        bg_color = {45, 45, 45},  -- 默认背景色
        fg_color = {248, 248, 242} -- 默认前景色
    }
    vim.api.set_statusbar_beautify(reset_config)
    vim.api.set_status_message("Statusbar Beautify Plugin disabled - reverted to default appearance")
end

-- 插件初始化完成
-- 所有配置都在 Lua 中定义，包括颜色、图标和布局
print("[DEBUG] Statusbar beautify plugin init.lua loaded")
