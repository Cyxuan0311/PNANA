# pnana 依赖文档

本文档基于 `CMakeLists.txt` 和 `build.sh`，说明 pnana 的构建依赖与可选功能。

## 目录

- [构建工具](#构建工具)
- [必需依赖](#必需依赖)
- [可选功能与依赖](#可选功能与依赖)
- [内置第三方库](#内置第三方库)
- [构建与启用](#构建与启用)
- [依赖版本表](#依赖版本表)
- [常见问题](#常见问题)

---

## 构建工具

| 工具 | 版本 | 说明 |
|------|------|------|
| CMake | ≥ 3.10 | 构建系统 |
| C++ 编译器 | C++17 | GCC 7+ / Clang 5+ / MSVC 2017+ |

### 安装

**Ubuntu/Debian**
```bash
sudo apt update
sudo apt install cmake build-essential g++
```

**Fedora/RHEL**
```bash
sudo dnf install cmake gcc-c++
```

**macOS**
```bash
brew install cmake
xcode-select --install
```

---

## 必需依赖

### FTXUI

FTXUI (Functional Terminal User Interface) 为必需库，用于终端 UI。

**Ubuntu/Debian** `sudo apt install libftxui-dev`  
**Fedora/RHEL** `sudo dnf install ftxui-devel`  
**macOS** `brew install ftxui`

**验证**：`pkg-config --modversion ftxui`

---

## 可选功能与依赖

以下功能默认**关闭**，需通过 CMake 或 `build.sh` 显式启用。

### LSP 支持（自动检测）

**依赖**：`third-party/nlohmann/json.hpp`、`third-party/JSON-RPC-CXX`

若项目内存在上述库，LSP 会自动启用，无需系统包。否则 LSP 功能不可用。

```bash
git submodule update --init --recursive  # 初始化子模块
```

### 图片预览（-DBUILD_IMAGE_PREVIEW=ON）

**依赖**：Chafa 开发库

**Ubuntu/Debian** `sudo apt install libchafa-dev`  
**Fedora/RHEL** `sudo dnf install chafa-devel`  
**macOS** `brew install chafa`

### Tree-sitter 语法高亮（-DBUILD_TREE_SITTER=ON）

**依赖**：Tree-sitter 开发库

**Ubuntu/Debian** `sudo apt install libtree-sitter-dev`  
**macOS** `brew install tree-sitter`

未启用时使用内置语法高亮器。

### Lua 插件系统（-DBUILD_LUA=ON）

**依赖**：Lua 5.3 或 5.4

**Ubuntu/Debian** `sudo apt install liblua5.4-dev`  
**Fedora/RHEL** `sudo dnf install lua-devel`  
**macOS** `brew install lua`

### Go SSH 模块（-DBUILD_SSH_MODE=GO）

**依赖**：Go 编译器

**Ubuntu/Debian** `sudo apt install golang-go`  
**Fedora/RHEL** `sudo dnf install golang`  
**macOS** `brew install go`

### C++ SSH 模块（-DBUILD_SSH_MODE=CPP）

**依赖**：libssh2 开发库

**Ubuntu/Debian** `sudo apt install libssh2-1-dev`  
**Fedora/RHEL** `sudo dnf install libssh2-devel`  
**macOS** `brew install libssh2`

### 不启用 SSH（-DBUILD_SSH_MODE=NONE，默认）

无需额外依赖，使用系统 SSH 命令作为后备。

### libvterm 终端模拟（-DBUILD_LIBVTERM=ON）

**依赖**：libvterm 开发库

**Ubuntu/Debian** `sudo apt install libvterm-dev`  
**Fedora/RHEL** `sudo dnf install libvterm-devel`  
**macOS** `brew install libvterm`

用于完整的终端模拟功能。

### AI 客户端（-DBUILD_AI_CLIENT=ON）

**依赖**：libcurl

**Ubuntu/Debian** `sudo apt install libcurl4-openssl-dev`  
**Fedora/RHEL** `sudo dnf install libcurl-devel`  
**macOS** `brew install curl`

### iconv（自动检测，可选）

用于编码转换。若未找到，使用内置实现。多数 Linux 发行版已包含，无需单独安装。

---

## 内置第三方库

以下库随源码提供，无需单独安装：

| 库 | 路径 | 用途 |
|----|------|------|
| nlohmann/json | `third-party/nlohmann/json.hpp` | JSON，LSP 与 AI 配置 |
| jsonrpccxx | `third-party/JSON-RPC-CXX` | JSON-RPC，LSP |
| md4c | `third-party/md4c/` | Markdown 解析 |
| stb | `third-party/dsa/stb_image.h` | 图像处理 |
| ftxui | `third-party/ftxui/` | 终端 UI 框架（备用） |

---

## 构建与启用

### 使用 build.sh

```bash
./build.sh                                    # 基础编译（LSP 自动检测）
./build.sh BUILD_IMAGE_PREVIEW=ON             # 图片预览（Chafa）
./build.sh BUILD_TREE_SITTER=ON               # Tree-sitter
./build.sh BUILD_LUA=ON                       # Lua 插件
./build.sh BUILD_SSH_MODE=GO                  # Go SSH 模块
./build.sh BUILD_SSH_MODE=CPP                 # C++ SSH 模块（基于 libssh2）
./build.sh BUILD_SSH_MODE=NONE                # 不启用 SSH（默认）
./build.sh BUILD_LIBVTERM=ON                  # libvterm 终端模拟
./build.sh BUILD_AI_CLIENT=ON                 # AI 客户端
./build.sh --clean BUILD_LUA=ON               # 清理后编译
./build.sh --clean --install BUILD_AI_CLIENT=ON  # 编译并安装
```

### 使用 CMake

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# 启用可选功能
cmake .. -DBUILD_IMAGE_PREVIEW=ON \
        -DBUILD_TREE_SITTER=ON \
        -DBUILD_LUA=ON \
        -DBUILD_SSH_MODE=GO \
        -DBUILD_LIBVTERM=ON \
        -DBUILD_AI_CLIENT=ON

make -j$(nproc)
```

### CMake 选项一览

| 选项 | 默认 | 依赖 | 功能 |
|------|------|------|------|
| `BUILD_IMAGE_PREVIEW` | OFF | Chafa | 图片预览 |
| `BUILD_TREE_SITTER` | OFF | Tree-sitter | 语法高亮 |
| `BUILD_LUA` | OFF | Lua 5.3/5.4 | Lua 插件 |
| `BUILD_SSH_MODE` | NONE | Go/libssh2 | SSH 模块（GO/CPP/NONE） |
| `BUILD_LIBVTERM` | OFF | libvterm | 终端模拟 |
| `BUILD_AI_CLIENT` | OFF | libcurl | AI 客户端 |

LSP 由内置 nlohmann/json 与 jsonrpccxx 决定，无单独选项。

---

## 依赖版本表

| 依赖 | 最低版本 | 必需/可选 |
|------|----------|-----------|
| CMake | 3.10 | 必需 |
| C++ 编译器 | C++17 | 必需 |
| GCC | 7.0 | 必需 |
| Clang | 5.0 | 必需 |
| FTXUI | 最新 | 必需 |
| Chafa | 1.12+ | 可选（-DBUILD_IMAGE_PREVIEW=ON） |
| Tree-sitter | 0.20+ | 可选（-DBUILD_TREE_SITTER=ON） |
| Lua | 5.3 / 5.4 | 可选（-DBUILD_LUA=ON） |
| Go | 1.21+ | 可选（-DBUILD_SSH_MODE=GO） |
| libssh2 | 1.9+ | 可选（-DBUILD_SSH_MODE=CPP） |
| libvterm | 0.3+ | 可选（-DBUILD_LIBVTERM=ON） |
| libcurl | 最新 | 可选（-DBUILD_AI_CLIENT=ON） |
| iconv | - | 可选（自动检测） |
| nlohmann/json | 3.x | 内置（third-party） |
| jsonrpccxx | 最新 | 内置（third-party） |

---

## 常见问题

### Q: 如何确认各功能是否启用？

**A**: 配置阶段会输出状态，例如：
```
✓ FFmpeg found - image preview enabled
✓ Tree-sitter found - syntax highlighting enabled
✓ Lua found - plugin system enabled
LSP support enabled (using local third-party libraries)
✓ iconv found - enhanced encoding conversion enabled
```

### Q: LSP 支持需要单独安装吗？

**A**: 不需要。项目内含 `third-party/nlohmann/json.hpp` 和 `third-party/JSON-RPC-CXX`。确保子模块已初始化：`git submodule update --init --recursive`。

### Q: 编译时找不到 FTXUI？

**A**: 安装开发包后若仍失败，可指定查找路径：
```bash
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
```

### Q: Windows 支持如何？

**A**: 需要 VS 2017+、CMake 3.10+、FTXUI（vcpkg 或源码）。推荐在 WSL2 或 Linux/macOS 下构建。

### Q: 如何完全重新配置？

**A**: 
```bash
rm -rf build && mkdir build && cd build
cmake .. [选项]
```

---

*文档与 CMakeLists.txt 一致，如有更新以代码为准。*
