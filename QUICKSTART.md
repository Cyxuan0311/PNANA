# PNANA 快速开始指南 / Quick Start Guide

快速入门和安装PNANA终端文本编辑器的指南 / Quick guide to get started with and install PNANA terminal text editor

## ⚒️ 前置条件 / Prerequisites

请在本地安装FTXUI: / Please install FTXUI locally:

```bash
# Debian/Ubuntu
sudo apt install libftxui-dev

# Arch Linux
yay -S ftxui
```

## 🚀 快速安装 / Quick Installation

### Ubuntu/Debian 软件包安装 / Ubuntu/Debian Package Installation

```bash
# deb软件包（请安装对应的deb包）
# deb package (please install the corresponding deb package)
sudo dpkg -i pnana-0.0.4-amd64.deb
```

### 其他发行版 (Arch | SUSE | CentOS ...) / Other Distributions (Arch | SUSE | CentOS ...)

可用软件包: / Available packages:
- pnana-0.0.4.tar.bz2
- pnana-0.0.4.tar.gz
- pnana-0.0.4.tar.xz

本地下载软件包（以pnana-0.0.4.tar.bz2为例）: / Download the package locally (using pnana-0.0.4.tar.bz2 as an example):
```bash
# 解压 / Extract
tar -xjvf pnana-0.0.4.tar.bz2

# 安装 / Install
cd ./pnana-0.0.4

chmod +x ./install.sh

./install.sh
```

## 🏗️ 基本编译 / Basic Compilation

```bash
# 获取源代码 / Get source code
git clone https://github.com/Cyxuan0311/PNANA.git
cd pnana

# 创建构建目录 / Create build directory
mkdir build && cd build

# 配置和编译 / Configure and compile
cmake ..
make -j$(nproc)

# 可选：安装到系统 / Optional: Install to system
sudo make install

# 安装用户配置和插件到 ~/.config/pnana/
# Install user configuration and plugins to ~/.config/pnana/
# 此脚本将默认配置和插件复制到您的用户目录
# This script copies the default config and plugins to your user directory
chmod +x ./install_user_config.sh

# 运行安装脚本 / Run the installation script
./install_user_config.sh
```

## ⚡ 启用高级功能 / Enable Advanced Features

从发布的预构建软件包或源代码编译后，某些功能（代码提示、Lua插件、SSH连接等）可能受限。在安装相应依赖后重新安装可以增强功能。/ After compiling from the released pre-built packages or source code, some features (code hints, Lua plugins, SSH connections, etc.) may be limited. Reinstalling after installing the corresponding dependencies can enhance functionality.

### 图片预览 / Image Preview
```bash
# 安装依赖 / Install dependencies
sudo apt install -y libavformat-dev libavcodec-dev libswscale-dev libavutil-dev

# 编译时启用 / Enable during compilation
cmake -DBUILD_IMAGE_PREVIEW=ON ..
make -j$(nproc)
```

### 语法高亮 (Tree-sitter) / Syntax Highlighting (Tree-sitter)
```bash
# 安装依赖 / Install dependencies
sudo apt install -y libtree-sitter-dev

# 还需要安装特定的tree-sitter语言包，例如tree-sitter-cpp
# Also need to install specific tree-sitter language packages, e.g., tree-sitter-cpp

# 克隆语言包源码（替换为对应的语言仓库）
# Clone language package source (replace with corresponding language repository)
git clone https://github.com/tree-sitter/tree-sitter-cpp.git
cd tree-sitter-cpp

# 编译为共享库(.so)，符合CMake命名规则(libtree-sitter-cpp.so)
# Compile as shared library (.so), matching CMake naming rules (libtree-sitter-cpp.so)
# -fPIC: 生成位置无关代码（共享库必需）
# -fPIC: Generate position-independent code (required for shared libraries)
# -shared: 编译为共享库
# -shared: Compile as shared library
# -O2: 优化编译
# -O2: Optimize compilation
gcc -shared -fPIC -O2 src/parser.c src/scanner.c -o libtree-sitter-cpp.so

# 安装到系统目录（优先/usr/local/lib，CMake首先查找此处）
# Install to system directory (prefer /usr/local/lib, CMake looks here first)
sudo cp libtree-sitter-cpp.so /usr/local/lib/

# 安装头文件（可选，一些项目需要parser.h）
# Install header files (optional, some projects need parser.h)
sudo mkdir -p /usr/local/include/tree-sitter/cpp
sudo cp src/parser.h /usr/local/include/tree-sitter/cpp/

# 更新系统库缓存（让系统识别新安装的库）
# Update system library cache (let system recognize newly installed library)
sudo ldconfig

# 验证安装（符合CMake的find_library逻辑）
# Verify installation (matches CMake's find_library logic)
ls /usr/local/lib/libtree-sitter-cpp.so

# 编译时启用 / Enable during compilation
cmake -DBUILD_TREE_SITTER=ON ..
make -j$(nproc)
```

### 插件系统 (Lua) / Plugin System (Lua)
```bash
# 安装依赖 / Install dependencies
sudo apt install -y liblua5.4-dev

# 编译时启用 / Enable during compilation
cmake -DBUILD_LUA=ON ..
make -j$(nproc)
```

### SSH支持 (Go) / SSH Support (Go)
```bash
# 安装依赖 / Install dependencies
sudo apt install -y golang-go

# 编译时启用 / Enable during compilation
cmake -DBUILD_GO=ON ..
make -j$(nproc)
```

## 🎯 完整功能编译 / Full Feature Compilation

```bash
# 启用所有功能 / Enable all features
cmake \
  -DBUILD_IMAGE_PREVIEW=ON \
  -DBUILD_TREE_SITTER=ON \
  -DBUILD_LUA=ON \
  -DBUILD_GO=ON \
  ..

# 编译 / Compile
make -j$(nproc)
```

## 📦 运行PNANA / Running PNANA

```bash
# 从构建目录运行 / Run from build directory
./build/pnana

# 或者安装后运行 / Or run after installation
pnana

# 打开文件 / Open a file
pnana filename.txt

# 查看帮助 / View help
pnana --help
```

## 🛠️ 故障排除 / Troubleshooting

### 构建失败 / Build Failure
```bash
# 清理并重新构建 / Clean and rebuild
rm -rf build/
mkdir build && cd build
cmake ..
make clean && make -j$(nproc)
```

### 依赖问题 / Dependency Issues
```bash
# Ubuntu/Debian
sudo apt update && sudo apt upgrade

# Fedora/RHEL
sudo dnf update
```

### 内存不足 / Insufficient Memory
```bash
# 使用更少的并行作业 / Use fewer parallel jobs
make -j2
```

---

*PNANA - 一个现代化的终端文本编辑器 / A Modern Terminal Text Editor*
