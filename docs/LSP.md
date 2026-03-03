# pnana LSP 支持文档

LSP (Language Server Protocol) 是由 Microsoft 开发的开放标准协议，允许编辑器与语言服务器通信，提供代码补全、语法检查、代码导航等智能功能。

pnana 内置了完整的 LSP 客户端支持，可以自动检测并连接各种语言的 LSP 服务器。

## 目录

- [支持的 LSP 服务器](#支持的-lsp-服务器)
- [LSP 功能](#lsp-功能)
- [配置 LSP](#配置-lsp)
- [使用技巧](#使用技巧)

---

## 支持的 LSP 服务器

pnana 支持所有符合 LSP 标准的语言服务器，包括但不限于：

- **C/C++**: clangd
- **Python**: pyright, pylsp, jedi-language-server
- **JavaScript/TypeScript**: typescript-language-server
- **Java**: eclipse.jdt.ls
- **Go**: gopls
- **Rust**: rust-analyzer
- **HTML/CSS**: vscode-html-languageserver, vscode-css-languageserver
- **JSON**: vscode-json-languageserver
- **YAML**: yaml-language-server
- **Markdown**: markdown-language-server

---

## LSP 功能

pnana 的 LSP 支持提供以下功能：

#### 1. 代码补全 (Code Completion)
- 智能代码补全，根据上下文提供建议
- 支持函数、变量、类、方法等符号补全
- 实时响应，输入时自动触发
- 支持使用上下键浏览补全项

#### 2. 实时诊断 (Diagnostics)
- 语法错误实时显示
- 警告信息提示
- 错误位置高亮
- 悬停显示错误详情

#### 3. 代码导航 (Code Navigation)
- **跳转到定义** (Go to Definition): 快速跳转到符号定义位置
- **查找引用** (Find References): 查找符号的所有引用
- **符号搜索** (Symbol Search): 快速查找文件中的符号

#### 4. 悬停信息 (Hover)
- 鼠标悬停显示符号信息
- 显示函数签名、类型信息、文档等

---

## 配置 LSP

#### 自动配置

pnana 会自动检测系统中已安装的 LSP 服务器，无需手动配置即可使用。

#### 安装 LSP 服务器

**Python (pylsp)**:
```bash
pip install python-lsp-server
```

**C/C++ (clangd)**:
```bash
# Ubuntu/Debian
sudo apt install clangd

# macOS
brew install llvm
```

**JavaScript/TypeScript**:
```bash
npm install -g typescript-language-server
```

**Go (gopls)**:
```bash
go install golang.org/x/tools/gopls@latest
```

**Rust (rust-analyzer)**:
```bash
# 通过 rustup 安装
rustup component add rust-analyzer
```

---

## 使用技巧

1. **代码补全触发**：输入 2-3 个字符后自动触发，或手动按 `Ctrl+Space`
2. **浏览补全项**：使用上下键在补全列表中导航
3. **接受补全**：按 `Enter` 或 `Tab` 接受当前补全项
4. **跳转到定义**：将光标放在符号上，按 `F12` 或右键菜单选择
5. **查看错误**：错误会在状态栏显示，悬停查看详情
