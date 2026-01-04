# 构建指南 (Build Guide)

本文档介绍了如何编译和构建 pnana 文本编辑器。

## 目录

- [系统要求](#系统要求)
- [快速开始](#快速开始)
- [编译选项](#编译选项)
- [依赖项](#依赖项)
- [故障排除](#故障排除)

## 系统要求

### 最低要求
- **操作系统**: Linux, macOS, 或 Windows (WSL)
- **编译器**: GCC 7+ 或 Clang 6+
- **CMake**: 3.10+
- **内存**: 至少 2GB RAM
- **磁盘空间**: 至少 500MB

### 推荐配置
- **操作系统**: Ubuntu 18.04+ 或 macOS 10.14+
- **编译器**: GCC 9+ 或 Clang 8+
- **CMake**: 3.16+
- **内存**: 4GB+ RAM
- **磁盘空间**: 1GB+

## 快速开始

### 1. 克隆仓库
```bash
git clone https://github.com/yourusername/pnana.git
cd pnana
```

### 2. 创建构建目录
```bash
mkdir build
cd build
```

### 3. 配置构建
```bash
cmake ..
```

### 4. 编译
```bash
make -j$(nproc)
```

### 5. 运行
```bash
./pnana
```

## 编译选项

pnana 支持多种编译选项来自定义构建。使用以下语法启用/禁用功能：

```bash
cmake -DOPTION_NAME=ON/OFF ..
```

### 核心功能选项

#### 图片预览支持 (Image Preview Support)
- **选项**: `BUILD_IMAGE_PREVIEW`
- **默认**: `OFF`
- **描述**: 启用图片预览功能，需要FFmpeg库
- **启用**: `cmake -DBUILD_IMAGE_PREVIEW=ON ..`
- **依赖**: FFmpeg 开发库

#### LSP 支持 (Language Server Protocol)
- **选项**: 自动检测
- **描述**: 启用代码补全和语言服务
- **依赖**: nlohmann/json 和 jsonrpccxx

#### Lua 插件系统 (Lua Plugin System)
- **选项**: 自动检测
- **描述**: 启用Lua脚本插件支持
- **依赖**: Lua 5.3+ 开发库

#### SSH 远程文件编辑 (SSH Remote File Editing)
- **选项**: 自动检测
- **描述**: 启用SSH远程文件编辑功能
- **依赖**: Go 编译器 (用于构建SSH模块)

#### Tree-sitter 语法高亮 (Tree-sitter Syntax Highlighting)
- **选项**: 自动检测
- **描述**: 启用高级语法高亮
- **依赖**: Tree-sitter 库

### 调试和开发选项

#### 构建类型 (Build Type)
```bash
# 调试版本 (默认)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 发布版本
cmake -DCMAKE_BUILD_TYPE=Release ..

# 最小发布版本
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..

# 性能优化版本
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

#### 编译器标志 (Compiler Flags)
```bash
# 自定义C++编译器
cmake -DCMAKE_CXX_COMPILER=clang++ ..

# 自定义C编译器
cmake -DCMAKE_C_COMPILER=clang ..
```

## 依赖项

### 必需依赖

#### FTXUI (终端UI库)
- **版本**: 3.0+
- **安装** (Ubuntu/Debian):
  ```bash
  sudo apt install libftxui-dev
  ```
- **安装** (macOS):
  ```bash
  brew install ftxui
  ```
- **源码构建**:
  ```bash
  git clone https://github.com/ArthurSonzogni/FTXUI.git
  cd FTXUI
  mkdir build && cd build
  cmake ..
  make -j$(nproc)
  sudo make install
  ```

### 可选依赖

#### FFmpeg (图片预览)
- **版本**: 4.0+
- **安装** (Ubuntu/Debian):
  ```bash
  sudo apt install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev
  ```
- **启用**: `cmake -DBUILD_IMAGE_PREVIEW=ON ..`

#### nlohmann/json (LSP支持)
- **版本**: 3.9+
- **安装** (Ubuntu/Debian):
  ```bash
  sudo apt install nlohmann-json3-dev
  ```

#### Lua (插件系统)
- **版本**: 5.3+
- **安装** (Ubuntu/Debian):
  ```bash
  sudo apt install liblua5.4-dev
  # 或
  sudo apt install liblua5.3-dev
  ```

#### Go (SSH模块)
- **版本**: 1.16+
- **安装** (Ubuntu/Debian):
  ```bash
  sudo apt install golang-go
  ```

### 构建依赖

#### 构建工具
```bash
# Ubuntu/Debian
sudo apt install cmake build-essential git

# macOS
brew install cmake git

# Windows (MSYS2)
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc git
```

## 完整构建示例

### 启用所有功能的构建
```bash
# 安装所有依赖
sudo apt install libftxui-dev nlohmann-json3-dev liblua5.4-dev golang-go
sudo apt install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev

# 构建
mkdir build && cd build
cmake -DBUILD_IMAGE_PREVIEW=ON ..
make -j$(nproc)
```

### 最小化构建
```bash
# 只安装必需依赖
sudo apt install libftxui-dev cmake build-essential git

# 构建
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 查看构建信息
```bash
# 查看CMake配置摘要
cd build
cmake .. 2>&1 | grep -E "(enabled|disabled|found|✓|✗)"

# 查看编译定义
make help 2>&1 | head -20
```

### 高级编译选项

#### 静态链接
```bash
# 静态链接所有库（生成较大的单文件二进制）
cmake -DCMAKE_EXE_LINKER_FLAGS="-static" ..
make -j$(nproc)
```

#### 链接时优化 (LTO)
```bash
# 启用链接时优化，提高性能
cmake -DCMAKE_CXX_FLAGS="-flto" -DCMAKE_EXE_LINKER_FLAGS="-flto" ..
make -j$(nproc)
```

#### 自定义安装路径
```bash
# 自定义安装目录
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc)
make install
```

#### 交叉编译示例
```bash
# 为ARM架构交叉编译
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/arm-linux-gnueabihf.cmake ..
make -j$(nproc)
```

#### 启用所有警告
```bash
# 严格的编译警告检查
cmake -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror" ..
make -j$(nproc)
```

## 故障排除

### 常见问题

#### 1. FTXUI 未找到
```
CMake Error: Could not find FTXUI
```
**解决方案**:
```bash
# 安装FTXUI
sudo apt install libftxui-dev
# 或从源码构建
```

#### 2. FFmpeg 相关错误
```
FFmpeg not found but BUILD_IMAGE_PREVIEW is enabled
```
**解决方案**:
```bash
# 安装FFmpeg
sudo apt install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev
# 或禁用图片预览
cmake -DBUILD_IMAGE_PREVIEW=OFF ..
```

#### 3. Lua 相关错误
```
Lua not found
```
**解决方案**:
```bash
# 安装Lua
sudo apt install liblua5.4-dev
```

#### 4. 编译错误
```
undefined reference to...
```
**解决方案**:
- 确保所有依赖都正确安装
- 清理构建目录重新编译：
  ```bash
  rm -rf build
  mkdir build && cd build
  cmake ..
  make -j$(nproc)
  ```

#### 5. Go 模块构建失败
```
Go compiler not found
```
**解决方案**:
```bash
# 安装Go
sudo apt install golang-go
# 或禁用SSH功能（将影响SSH远程编辑）
```

### 调试构建

启用详细输出：
```bash
# 启用CMake调试
cmake -DCMAKE_VERBOSE_MAKEFILE=ON ..

# 查看编译命令
make VERBOSE=1
```

### 性能优化

```bash
# 启用链接时优化 (LTO)
cmake -DCMAKE_CXX_FLAGS="-flto" ..

# 使用Clang而不是GCC
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ..

# 启用更多警告
cmake -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic" ..
```

