<div align="center">

<img src="resources/logo.png" alt="pnana logo" width="200">

#  pnana - 现代化终端文本编辑器

![pnana](https://img.shields.io/badge/pnana-v0.0.3-brightgreen)
![C++](https://img.shields.io/badge/C++-17-blue)
![FTXUI](https://img.shields.io/badge/FTXUI-Terminal%20UI-orange)
![License](https://img.shields.io/badge/license-MIT-green)

**中文** | [English](README_EN.md)

**pnana**是一个基于FTXUI构建的现代化终端文本编辑器，灵感来自Nano、Micro和Sublime Text。提供友好的用户界面、直观的快捷键和强大的编辑功能。

</div>

## 📸 演示

<div align="center">

<img src="resources/demo.gif" alt="pnana demo" width="800">

</div>

## ✨ 核心特性

### 🎨 美观友好的界面
- **28种精美主题**: Monokai（默认）、Dracula、Solarized Dark/Light、OneDark、Nord...
- **三栏布局**: 顶部菜单栏、中间编辑区、底部帮助栏
- **智能状态栏**: 显示文件信息、光标位置、编码、修改状态
- **行号显示**: 绝对行号和相对行号切换
- **当前行高亮**: 清晰标识编辑位置

### ⌨️ 现代化快捷键
摒弃传统Vim的学习曲线，采用现代编辑器的直观快捷键。使用熟悉的 `Ctrl+S` 保存、`Ctrl+Z` 撤销等标准快捷键，零学习成本。

**详细快捷键列表**：请查看 [快捷键文档](docs/KEYBINDINGS.md)

### 📝 强大的编辑功能

#### 多文件支持
- **标签页系统**: 同时打开多个文件
- **分屏编辑**: 水平/垂直分屏
- **快速切换**: 键盘和鼠标快速切换文件

#### 智能编辑
- **自动缩进**: 根据文件类型智能缩进
- **括号匹配**: 自动补全括号、引号
- **多光标编辑**: 同时编辑多个位置（计划中）
- **列选择**: 按住Alt进行列选择
- **智能撤销/重做**: 无限次撤销重做

#### 搜索和替换
- **正则表达式**: 支持正则搜索
- **大小写敏感**: 可选大小写匹配
- **批量替换**: 一次替换所有匹配
- **即时预览**: 搜索结果实时高亮

#### 语法高亮
支持多种编程语言：C/C++, Python, JavaScript/TypeScript, Java, Go, Rust, Ruby, PHP, HTML/CSS, JSON, XML, Markdown, Shell, SQL, YAML, TOML

#### LSP 支持（Language Server Protocol）
- **代码补全**：智能代码补全，支持多种编程语言
- **实时诊断**：语法错误和警告实时显示
- **代码导航**：跳转到定义、查找引用
- **符号搜索**：快速查找函数、类、变量等符号
- **自动配置**：自动检测并配置 LSP 服务器

**详细 LSP 说明**：请查看 [插件文档](docs/PLUGINS.md)

#### Lua 插件系统
- **强大的扩展能力**：使用 Lua 编写插件，轻松扩展编辑器功能
ls l- **丰富的 API**：提供完整的编辑器 API，支持文件操作、光标控制、事件监听等
- **简单易用**：参考 Neovim 的设计，插件开发简单直观
- **自动加载**：插件自动发现和加载，无需手动配置

**详细插件开发指南**：请查看 [插件文档](docs/PLUGINS.md)

### 🔧 配置系统
使用简单的 JSON 配置文件，支持主题、字体、缩进等各项设置。

**详细配置说明**：请查看 [配置文档](docs/CONFIGURATION.md)

## 🚀 快速开始

### 编译要求
**[依赖文档](docs/DEPENDENCIES.md)** - 项目依赖说明和安装指南

### 编译安装

```bash
# 克隆仓库
cd /path/to/pnana
chmod +x ./build.sh

# 编译项目
./build.sh

# 运行pnana
./build/pnana/pnana

# 或安装到系统
cd build
sudo make install
pnana filename.txt
```

### 使用示例

```bash
# 启动空白编辑器
pnana

# 打开单个文件
pnana file.txt

# 打开多个文件
pnana file1.txt file2.cpp file3.py

# 指定配置文件
pnana --config ~/.config/pnana/custom.json

# 使用特定主题
pnana --theme dracula file.txt

# 只读模式打开
pnana --readonly file.txt
```

## 📖 文档

详细的文档和指南请查看 [docs](docs/) 文件夹：

- **[快捷键参考](docs/KEYBINDINGS.md)** - 完整的快捷键列表和使用说明
- **[配置文档](docs/CONFIGURATION.md)** - 详细的配置选项和示例
- **[插件开发指南](docs/PLUGINS.md)** - Lua 插件系统开发文档和 LSP 使用说明
- **[依赖文档](docs/DEPENDENCIES.md)** - 项目依赖说明和安装指南
- **[菜单功能](docs/MENU.md)** - 菜单栏功能详细说明
- **[主题文档](docs/THEMES.md)** - 所有主题的介绍和预览
- **[开发路线图](docs/ROADMAP.md)** - 版本计划和功能路线图
- **[产品对比](docs/COMPARISON.md)** - 与同类产品的详细对比
- **[快速入门](docs/QUICKSTART.md)** - 5分钟快速上手指南


## 💡 为什么选择pnana？

1. **零学习成本**: 使用熟悉的Ctrl快捷键，无需记忆复杂命令
2. **开箱即用**: 无需配置即可获得出色的编辑体验
3. **现代化设计**: 精美的UI和舒适的配色方案
4. **轻量高效**: 基于终端，资源占用少，启动迅速
5. **功能完整**: 不输于GUI编辑器的功能集合

## 🤝 对比同类产品

| 功能 | pnana | Nano | Micro | Vim/Neovim |
|-----|-------|------|-------|------------|
| 学习曲线 | 低 | 低 | 低 | 高 |
| 现代UI | ✅ | ❌ | ✅ | 需配置 |
| 鼠标支持 | ✅ | ⚠️ | ✅ | 需配置 |
| 语法高亮 | ✅ | ⚠️ | ✅ | ✅ |
| 多文件 | ✅ | ❌ | ✅ | ✅ |
| 插件系统 | ✅ | ❌ | ✅ | ✅ |
| LSP支持 | ✅ | ❌ | ✅ | ✅ |
| 配置简单 | ✅ | ✅ | ✅ | ❌ |

**详细对比**：请查看 [产品对比文档](docs/COMPARISON.md)

## 📚 参考和灵感

本项目受以下优秀项目启发：
- [Nano](https://www.nano-editor.org/) - 简单易用的终端编辑器
- [Micro](https://micro-editor.github.io/) - 现代化的终端编辑器
- [Sublime Text](https://www.sublimetext.com/) - 经典的文本编辑器
- [VS Code](https://code.visualstudio.com/) - 现代IDE
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) - 强大的终端UI库

## 📝 许可证

本项目采用MIT许可证 - 详见LICENSE文件。

## 🌟 Star History

[![Star History Chart](https://api.star-history.com/svg?repos=Cyxuan0311/PNANA&type=Date)](https://star-history.com/#Cyxuan0311/PNANA&Date)

---
