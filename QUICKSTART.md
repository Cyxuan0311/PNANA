# PNANA Quick Start

Quick guide to get started with and install PNANA terminal text editor

## ⚒️ Prerequisites

Please install FTXUI locally:

```bash

## Debian/Ubuntu
sudo apt install libftxui-dev

## Arch Linux
yay -S ftxui

```

## 🚀 Quick Installation

### Ubuntu/Debian Package Installation

```bash
# deb package (please install the corresponding deb package)
sudo dpkg -i pnana-0.0.4-amd64.deb
```

### Other Distributions (Arch | SUSE | CentOS ...)

Available packages:
- pnana-0.0.4.tar.bz2
- pnana-0.0.4.tar.gz
- pnana-0.0.4.tar.xz

Download the package locally (using pnana-0.0.4.tar.bz2 as an example):
```bash
## Extract
tar -xjvf pnana-0.0.4.tar.bz2

## Install
cd ./pnana-0.0.4

chmod +x ./install.sh

./install.sh
```

## 🏗️ Basic Compilation

```bash
# Get source code
git clone https://github.com/Cyxuan0311/PNANA.git
cd pnana

# Create build directory
mkdir build && cd build

# Configure and compile
cmake ..
make -j$(nproc)

# Optional: Install to system
sudo make install
```

## ⚡ Enable Advanced Features

After compiling from the released pre-built packages or source code, some features (code hints, Lua plugins, SSH connections, etc.) may be limited. Reinstalling after installing the corresponding dependencies can enhance functionality.

### Image Preview
```bash
# Install dependencies
sudo apt install -y libavformat-dev libavcodec-dev libswscale-dev libavutil-dev

# Enable during compilation
cmake -DBUILD_IMAGE_PREVIEW=ON ..
make -j$(nproc)
```

### Syntax Highlighting (Tree-sitter)
```bash
# Install dependencies
sudo apt install -y libtree-sitter-dev

# Also need to install specific tree-sitter language packages, e.g., tree-sitter-cpp

# Clone language package source (replace with corresponding language repository)
git clone https://github.com/tree-sitter/tree-sitter-cpp.git
cd tree-sitter-cpp

# Compile as shared library (.so), matching CMake naming rules (libtree-sitter-cpp.so)
# -fPIC: Generate position-independent code (required for shared libraries)
# -shared: Compile as shared library
# -O2: Optimize compilation
gcc -shared -fPIC -O2 src/parser.c src/scanner.c -o libtree-sitter-cpp.so

# Install to system directory (prefer /usr/local/lib, CMake looks here first)
sudo cp libtree-sitter-cpp.so /usr/local/lib/

# Install header files (optional, some projects need parser.h)
sudo mkdir -p /usr/local/include/tree-sitter/cpp
sudo cp src/parser.h /usr/local/include/tree-sitter/cpp/

# Update system library cache (let system recognize newly installed library)
sudo ldconfig

# Verify installation (matches CMake's find_library logic)
ls /usr/local/lib/libtree-sitter-cpp.so

# Enable during compilation
cmake -DBUILD_TREE_SITTER=ON ..
make -j$(nproc)
```

### Plugin System (Lua)
```bash
# Install dependencies
sudo apt install -y liblua5.4-dev

# Enable during compilation
cmake -DBUILD_LUA=ON ..
make -j$(nproc)
```

### SSH Support (Go)
```bash
# Install dependencies
sudo apt install -y golang-go

# Enable during compilation
cmake -DBUILD_GO=ON ..
make -j$(nproc)
```

## 🎯 Full Feature Compilation

```bash
# Enable all features
cmake \
  -DBUILD_IMAGE_PREVIEW=ON \
  -DBUILD_TREE_SITTER=ON \
  -DBUILD_LUA=ON \
  -DBUILD_GO=ON \
  ..

# Compile
make -j$(nproc)
```

## 📦 Running PNANA

```bash
# Run from build directory
./build/pnana

# Or run after installation
pnana

# Open a file
pnana filename.txt

# View help
pnana --help
```

## 🛠️ Troubleshooting

### Build Failure
```bash
# Clean and rebuild
rm -rf build/
mkdir build && cd build
cmake ..
make clean && make -j$(nproc)
```

### Dependency Issues
```bash
# Ubuntu/Debian
sudo apt update && sudo apt upgrade

# Fedora/RHEL
sudo dnf update
```

### Insufficient Memory
```bash
# Use fewer parallel jobs
make -j2
```

*PNANA - A Modern Terminal Text Editor*
