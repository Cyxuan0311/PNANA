# Pnana 插件开发指南

Pnana 提供了强大的插件系统，支持 Lua 脚本插件和 C++ 插件。本文档将指导您如何开发自己的插件。

## 架构概述

Pnana 的插件系统采用了分层架构：

1. **插件接口层** - 定义标准插件接口
2. **插件加载器** - 负责加载不同类型的插件
3. **插件注册表** - 管理已加载的插件
4. **服务总线** - 插件间通信
5. **生命周期管理器** - 管理插件生命周期

## Lua 插件开发

### 基本结构

每个 Lua 插件都应该是一个包含 `init.lua` 文件的目录。插件的基本结构如下：

```
your-plugin/
├── init.lua          # 主插件文件
├── plugin.lua        # 可选：插件配置
└── assets/           # 可选：资源文件
```

### 插件对象

插件应该定义一个包含以下方法的表：

```lua
local plugin = {
    name = "your-plugin-name",
    version = "1.0.0",
    description = "Your plugin description",
    author = "Your Name"
}
```

### 生命周期方法

```lua
-- 插件初始化
function plugin.initialize()
    -- 初始化代码
    return true  -- 返回 true 表示初始化成功
end

-- 插件启动
function plugin.start()
    -- 启动代码
    return true
end

-- 插件停止
function plugin.stop()
    -- 停止代码
    return true
end

-- 插件卸载
function plugin.unload()
    -- 清理代码
    return true
end
```

### 配置管理

```lua
-- 加载配置
function plugin.load_config(config)
    -- 处理配置
    return true
end

-- 获取配置
function plugin.get_config()
    return {
        setting1 = "value1",
        setting2 = "value2"
    }
end
```

### API 使用

#### 主题管理

```lua
-- 添加主题
local theme_config = {
    background = {r = 30, g = 30, b = 46},
    foreground = {r = 202, g = 211, b = 245},
    keyword = {r = 137, g = 180, b = 250},
    string = {r = 166, g = 227, b = 161},
    comment = {r = 128, g = 128, b = 128},
    selection = {r = 68, g = 71, b = 90}
}
vim.api.add_theme("your_theme", theme_config)

-- 移除主题
vim.api.remove_theme("your_theme")
```

#### 命令注册

```lua
-- 注册命令
vim.api.register_command("YourCommand", function(args)
    -- 命令实现
    vim.api.set_status_message("Command executed!")
end)

-- 注销命令
vim.api.unregister_command("YourCommand")
```

#### 键位映射

```lua
-- 注册键位映射
vim.api.register_keymap("normal", "<leader>yc", ":YourCommand<CR>")

-- 注销键位映射
vim.api.unregister_keymap("normal", "<leader>yc")
```

#### 事件系统

```lua
-- 订阅事件
vim.api.subscribe_event("BufferOpened", function(args)
    vim.api.set_status_message("Buffer opened: " .. args[1])
end)

-- 发布事件
vim.api.publish_event("CustomEvent", {"arg1", "arg2"})
```

#### 服务系统

```lua
-- 注册服务
vim.api.register_service("YourService", {
    get_data = function()
        return "service data"
    end
})

-- 获取服务
local service = vim.api.get_service("YourService")
if service then
    local data = service.get_data()
end
```

### 状态消息

```lua
-- 显示状态消息
vim.api.set_status_message("Your plugin message")
```

### 注册插件

最后，使用以下代码注册插件：

```lua
vim.api.register_plugin(plugin)
return plugin
```

## C++ 插件开发

### 基本结构

C++ 插件需要实现 `PluginInterface`：

```cpp
#include "plugins/plugin_interface.h"

class YourPlugin : public pnana::plugins::PluginInterface {
public:
    const PluginMetadata& getMetadata() const override {
        static PluginMetadata metadata = {
            "your-plugin",
            "1.0.0",
            "Your C++ plugin description",
            "Your Name"
        };
        return metadata;
    }

    PluginState getState() const override {
        return state_;
    }

    bool initialize(std::shared_ptr<PluginContext> context) override {
        context_ = context;
        // 初始化代码
        return true;
    }

    bool start() override {
        // 启动代码
        return true;
    }

    bool stop() override {
        // 停止代码
        return true;
    }

    bool unload() override {
        // 清理代码
        return true;
    }

    // 实现其他必要方法...
};

// 导出函数
extern "C" {
    pnana::plugins::PluginInterface* CreatePlugin() {
        return new YourPlugin();
    }
}
```

### 编译和安装

1. 编译插件为动态库（.so/.dll/.dylib）
2. 将动态库文件放在插件目录中
3. 确保导出 `CreatePlugin` 函数

## 插件配置

插件可以通过 `plugin.lua` 文件提供配置：

```lua
plugin_name = "your-plugin"
plugin_version = "1.0.0"
plugin_description = "Your plugin description"
plugin_author = "Your Name"

-- 其他配置...
```

## 最佳实践

1. **错误处理** - 始终检查操作的返回值
2. **资源清理** - 在 `unload` 方法中清理所有资源
3. **向后兼容** - 保持 API 稳定
4. **文档化** - 为您的插件编写清晰的文档
5. **测试** -  thoroughly 测试您的插件

## 示例插件

查看 `plugins/example-plugin/` 目录中的示例插件，了解完整的实现。

## 插件加载顺序

插件按照以下优先级加载：

1. 核心插件
2. 用户自定义插件
3. 依赖关系解析

插件加载器会自动解析依赖关系并按正确顺序加载插件。
