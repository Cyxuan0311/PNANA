# pnana Dependencies

This document describes pnana's build dependencies and optional features, based on `CMakeLists.txt` and `build.sh`.

## Table of Contents

- [Build Tools](#build-tools)
- [Required Dependencies](#required-dependencies)
- [Optional Features & Dependencies](#optional-features--dependencies)
- [Bundled Third-Party Libraries](#bundled-third-party-libraries)
- [Building & Enabling Features](#building--enabling-features)
- [Dependency Version Table](#dependency-version-table)
- [FAQ](#faq)

---

## Build Tools

| Tool | Version | Description |
|------|---------|-------------|
| CMake | ≥ 3.10 | Build system |
| C++ compiler | C++17 | GCC 7+ / Clang 5+ / MSVC 2017+ |

### Installation

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

## Required Dependencies

### FTXUI

FTXUI (Functional Terminal User Interface) is required for the terminal UI.

**Ubuntu/Debian** `sudo apt install libftxui-dev`  
**Fedora/RHEL** `sudo dnf install ftxui-devel`  
**macOS** `brew install ftxui`

**Verify**: `pkg-config --modversion ftxui`

---

## Optional Features & Dependencies

All optional features are **disabled by default** and must be enabled via CMake or `build.sh`.

### LSP Support (auto-detected)

**Dependencies**: `third-party/nlohmann/json.hpp`, `third-party/JSON-RPC-CXX`

LSP is enabled automatically when these libraries are present in the project. No system packages needed.

```bash
git submodule update --init --recursive  # Initialize submodules
```

### Image Preview (-DBUILD_IMAGE_PREVIEW=ON)

**Dependencies**: FFmpeg dev libraries (libavformat, libavcodec, libswscale, libavutil)

**Ubuntu/Debian** `sudo apt install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev`  
**Fedora/RHEL** `sudo dnf install ffmpeg-devel`  
**macOS** `brew install ffmpeg`

### Tree-sitter Syntax Highlighting (-DBUILD_TREE_SITTER=ON)

**Dependencies**: Tree-sitter dev library

**Ubuntu/Debian** `sudo apt install libtree-sitter-dev`  
**macOS** `brew install tree-sitter`

Falls back to built-in highlighter when disabled.

### Lua Plugin System (-DBUILD_LUA=ON)

**Dependencies**: Lua 5.3 or 5.4

**Ubuntu/Debian** `sudo apt install liblua5.4-dev`  
**Fedora/RHEL** `sudo dnf install lua-devel`  
**macOS** `brew install lua`

### Go SSH Module (-DBUILD_GO=ON)

**Dependencies**: Go compiler

**Ubuntu/Debian** `sudo apt install golang-go`  
**Fedora/RHEL** `sudo dnf install golang`  
**macOS** `brew install go`

Falls back to system SSH commands when disabled.

### AI Client (-DBUILD_AI_CLIENT=ON)

**Dependencies**: libcurl

**Ubuntu/Debian** `sudo apt install libcurl4-openssl-dev`  
**Fedora/RHEL** `sudo dnf install libcurl-devel`  
**macOS** `brew install curl`

### iconv (auto-detected, optional)

Used for encoding conversion. Falls back to built-in implementation if not found. Usually included in Linux glibc, no extra install needed.

---

## Bundled Third-Party Libraries

These are included in the source tree; no separate installation:

| Library | Path | Purpose |
|---------|------|---------|
| nlohmann/json | `third-party/nlohmann/json.hpp` | JSON, LSP & AI config |
| jsonrpccxx | `third-party/JSON-RPC-CXX` | JSON-RPC, LSP |
| md4c | `third-party/md4c/` | Markdown parsing |
| stb | `third-party/dsa/stb_image.h` | Image handling |

---

## Building & Enabling Features

### Using build.sh

```bash
./build.sh                                    # Basic build (LSP auto-detected)
./build.sh BUILD_IMAGE_PREVIEW=ON             # Image preview
./build.sh BUILD_TREE_SITTER=ON               # Tree-sitter
./build.sh BUILD_LUA=ON                       # Lua plugins
./build.sh BUILD_GO=ON                        # Go SSH
./build.sh BUILD_AI_CLIENT=ON                 # AI client
./build.sh --clean BUILD_LUA=ON               # Clean then build
./build.sh --clean --install BUILD_AI_CLIENT=ON  # Build and install
```

### Using CMake

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Enable optional features
cmake .. -DBUILD_IMAGE_PREVIEW=ON \
        -DBUILD_TREE_SITTER=ON \
        -DBUILD_LUA=ON \
        -DBUILD_GO=ON \
        -DBUILD_AI_CLIENT=ON

make -j$(nproc)
```

### CMake Options Summary

| Option | Default | Depends On | Feature |
|--------|---------|------------|---------|
| `BUILD_IMAGE_PREVIEW` | OFF | FFmpeg | Image preview |
| `BUILD_TREE_SITTER` | OFF | Tree-sitter | Syntax highlighting |
| `BUILD_LUA` | OFF | Lua 5.3/5.4 | Lua plugins |
| `BUILD_GO` | OFF | Go | SSH module |
| `BUILD_AI_CLIENT` | OFF | libcurl | AI client |

LSP is determined by bundled nlohmann/json and jsonrpccxx; there is no separate option.

---

## Dependency Version Table

| Dependency | Min Version | Required/Optional |
|-------------|-------------|-------------------|
| CMake | 3.10 | Required |
| C++ compiler | C++17 | Required |
| GCC | 7.0 | Required |
| Clang | 5.0 | Required |
| FTXUI | Latest | Required |
| Tree-sitter | 0.20+ | Optional (-DBUILD_TREE_SITTER=ON) |
| FFmpeg | 4.0 | Optional (-DBUILD_IMAGE_PREVIEW=ON) |
| Lua | 5.3 / 5.4 | Optional (-DBUILD_LUA=ON) |
| Go | 1.21+ | Optional (-DBUILD_GO=ON) |
| libcurl | Latest | Optional (-DBUILD_AI_CLIENT=ON) |
| iconv | - | Optional (auto-detected) |
| nlohmann/json | 3.x | Bundled (third-party) |
| jsonrpccxx | Latest | Bundled (third-party) |

---

## FAQ

### Q: How do I verify which features are enabled?

**A**: Check the CMake configuration output. Example:
```
✓ FFmpeg found - image preview enabled
✓ Tree-sitter found - syntax highlighting enabled
✓ Lua found - plugin system enabled
LSP support enabled (using local third-party libraries)
✓ iconv found - enhanced encoding conversion enabled
```

### Q: Do I need to install LSP support separately?

**A**: No. The project includes `third-party/nlohmann/json.hpp` and `third-party/JSON-RPC-CXX`. Ensure submodules are initialized: `git submodule update --init --recursive`.

### Q: CMake can't find FTXUI?

**A**: After installing the dev package, try:
```bash
cmake .. -DCMAKE_PREFIX_PATH=/usr/local
```

### Q: What about Windows?

**A**: Requires VS 2017+, CMake 3.10+, FTXUI (vcpkg or source). WSL2 or Linux/macOS is recommended for building.

### Q: How do I fully reconfigure?

**A**:
```bash
rm -rf build && mkdir build && cd build
cmake .. [options]
```

---

*This document aligns with CMakeLists.txt. Refer to the source for the latest behavior.*
