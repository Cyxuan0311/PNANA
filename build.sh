#!/bin/bash

# PNANA 一键编译脚本(基础编译)
# 支持清理、编译、安装等功能

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'NC='\033[0m' # No Color
NC='\033[0m' # No Color
# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    print_info "Checking dependencies..."
    
    local missing_deps=()
    
    # 检查 CMake
    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    fi
    
    # 检查 C++ 编译器
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing_deps+=("g++ or clang++")
    fi
    
    # 检查 Go (可选，用于 SSH 模块)
    if ! command -v go &> /dev/null; then
        print_warning "Go is not installed. Go SSH module will be unavailable."
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_info "Please install the missing dependencies and try again."
        exit 1
    fi
    
    print_success "All required dependencies are installed."
}

# 清理构建目录
clean_build() {
    print_info "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned."
    else
        print_info "Build directory does not exist, skipping clean."
    fi
}

# 配置 CMake
configure_cmake() {
    print_info "Configuring CMake..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # 构建 CMake 参数
    local cmake_args=(-DCMAKE_BUILD_TYPE=Release)
    
    # 添加用户指定的 CMake 选项
    if [ "$BUILD_IMAGE_PREVIEW" = "ON" ]; then
        cmake_args+=(-DBUILD_IMAGE_PREVIEW=ON)
        print_info "  - Image preview support: ENABLED"
    fi
    
    if [ "$BUILD_TREE_SITTER" = "ON" ]; then
        cmake_args+=(-DBUILD_TREE_SITTER=ON)
        print_info "  - Tree-sitter syntax highlighting: ENABLED"
    fi
    
    if [ "$BUILD_LUA" = "ON" ]; then
        cmake_args+=(-DBUILD_LUA=ON)
        print_info "  - Lua plugin system: ENABLED"
    fi
    
    if [ "$BUILD_AI_CLIENT" = "ON" ]; then
        cmake_args+=(-DBUILD_AI_CLIENT=ON)
        print_info "  - AI client support: ENABLED"
    fi

    if [ "$BUILD_LIBVTERM" = "ON" ]; then
        cmake_args+=(-DBUILD_LIBVTERM=ON)
        print_info "  - libvterm terminal emulation: ENABLED"
    fi
    
    # SSH 模式处理（交互式确认）
    if [ "$BUILD_SSH_MODE" != "" ]; then
        cmake_args+=(-DBUILD_SSH_MODE="${BUILD_SSH_MODE}")
        print_info "  - SSH mode: ${BUILD_SSH_MODE}"
    fi
    
    cmake .. "${cmake_args[@]}"
    
    if [ $? -eq 0 ]; then
        print_success "CMake configuration completed."
    else
        print_error "CMake configuration failed."
        exit 1
    fi
}

# 编译项目
build_project() {
    print_info "Building project..."
    cd "$BUILD_DIR"
    
    # 获取 CPU 核心数
    local cores=$(nproc 2>/dev/null || echo 4)
    print_info "Using $cores parallel jobs..."
    
    make -j"$cores"
    
    if [ $? -eq 0 ]; then
        print_success "Build completed successfully!"
    else
        print_error "Build failed."
        exit 1
    fi
}

# 安装（可选）
install_project() {
    if [ "$1" == "--install" ]; then
        print_info "Installing project..."
        cd "$BUILD_DIR"
        sudo make install
        
        if [ $? -eq 0 ]; then
            print_success "Installation completed."
        else
            print_error "Installation failed."
            exit 1
        fi
    fi
}

# 显示帮助信息
show_help() {
    echo "PNANA Build Script"
    echo ""
    echo "Usage: $0 [OPTIONS] [CMAKE_OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --clean          Clean build directory before building"
    echo "  --all            Enable all CMake build options (sets all BUILD_* to ON)"
    echo "  --install        Install the project after building"
    echo "  --help           Show this help message"
    echo ""
    echo "CMake Options (enable features):"
    echo "  BUILD_IMAGE_PREVIEW=ON    Enable image preview support (requires chafa)"
    echo "  BUILD_TREE_SITTER=ON      Enable Tree-sitter syntax highlighting"
    echo "  BUILD_LUA=ON              Enable Lua plugin system"
    echo "  BUILD_SSH_MODE=GO|CPP|NONE  Select SSH implementation (GO/Cpp/NONE, default: NONE)"
    echo "  BUILD_AI_CLIENT=ON       Enable AI client support (requires libcurl)"
    echo "  BUILD_LIBVTERM=ON        Enable libvterm terminal emulation"
    echo ""
    echo "Examples:"
    echo "  \$0                                    # Build the project"
    echo "  \$0 --clean                            # Clean and build"
    echo "  \$0 --install                          # Build and install"
    echo "  \$0 BUILD_IMAGE_PREVIEW=ON             # Build with image preview"
    echo "  \$0 BUILD_LUA=ON BUILD_SSH_MODE=CPP    # Build with Lua and C++ SSH"
    echo "  \$0 BUILD_SSH_MODE=GO                  # Build with Go SSH module"
    echo "  \$0 --clean BUILD_TREE_SITTER=ON       # Clean and build with Tree-sitter"
    echo "  \$0 --clean --install BUILD_AI_CLIENT=ON  # Clean, build, install with AI client"
    echo "  \$0 BUILD_LIBVTERM=ON                    # Build with libvterm support"
    echo "  \$0 --all                                # Build with all optional features enabled (interactive SSH mode selection)"
}

# 交互式选择 SSH 模式
interactive_ssh_mode_selection() {
    echo ""
    print_info "=== SSH Implementation Selection ==="
    echo "Please choose SSH implementation mode:"
    echo "  1) GO    - Use Go SSH module (requires Go compiler)"
    echo "  2) CPP   - Use C++ native SSH (requires libssh2)"
    echo "  3) NONE  - No SSH support (default)"
    echo ""
    
    while true; do
        read -p "Enter your choice (1/2/3 or GO/CPP/NONE): " ssh_choice
        
        case $ssh_choice in
            1|GO|go)
                BUILD_SSH_MODE="GO"
                print_info "Selected: Go SSH module"
                break
                ;;
            2|CPP|cpp)
                BUILD_SSH_MODE="CPP"
                print_info "Selected: C++ native SSH"
                break
                ;;
            3|NONE|none)
                BUILD_SSH_MODE="NONE"
                print_info "Selected: No SSH support"
                break
                ;;
            *)
                print_warning "Invalid choice. Please enter 1, 2, 3, GO, CPP, or NONE."
                ;;
        esac
    done
    
    echo ""
}

# 主函数
main() {
    local clean_flag=false
    local install_flag=false
    local build_all=false
    
    # 初始化 CMake 选项变量（空表示未由用户显式设置）
    BUILD_IMAGE_PREVIEW=""
    BUILD_TREE_SITTER=""
    BUILD_LUA=""
    BUILD_SSH_MODE=""
    BUILD_AI_CLIENT=""
    BUILD_LIBVTERM=""
    # 标记每个选项是否由用户显式设置（用于 --all 后允许显式覆盖）
    BUILD_IMAGE_PREVIEW_SET=false
    BUILD_TREE_SITTER_SET=false
    BUILD_LUA_SET=false
    BUILD_SSH_MODE_SET=false
    BUILD_AI_CLIENT_SET=false
    BUILD_LIBVTERM_SET=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                clean_flag=true
                shift
                ;;
            --install)
                install_flag=true
                shift
                ;;
            --all)
                build_all=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            BUILD_IMAGE_PREVIEW=*)
                BUILD_IMAGE_PREVIEW="${1#*=}"
                BUILD_IMAGE_PREVIEW_SET=true
                shift
                ;;
            BUILD_TREE_SITTER=*)
                BUILD_TREE_SITTER="${1#*=}"
                BUILD_TREE_SITTER_SET=true
                shift
                ;;
            BUILD_LUA=*)
                BUILD_LUA="${1#*=}"
                BUILD_LUA_SET=true
                shift
                ;;
            BUILD_SSH_MODE=*)
                BUILD_SSH_MODE="${1#*=}"
                BUILD_SSH_MODE_SET=true
                shift
                ;;
            BUILD_AI_CLIENT=*)
                BUILD_AI_CLIENT="${1#*=}"
                BUILD_AI_CLIENT_SET=true
                shift
                ;;
            BUILD_LIBVTERM=*)
                BUILD_LIBVTERM="${1#*=}"
                BUILD_LIBVTERM_SET=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done

    # 如果启用了 --all，则将未显式设置的 BUILD_* 选项置为 ON（允许用户在命令行中显式覆盖）
    if [ "$build_all" = true ]; then
        print_info "--all: enabling all optional CMake features (unless explicitly set)."
        if [ "$BUILD_IMAGE_PREVIEW_SET" = false ]; then BUILD_IMAGE_PREVIEW="ON"; fi
        if [ "$BUILD_TREE_SITTER_SET" = false ]; then BUILD_TREE_SITTER="ON"; fi
        if [ "$BUILD_LUA_SET" = false ]; then BUILD_LUA="ON"; fi
        if [ "$BUILD_AI_CLIENT_SET" = false ]; then BUILD_AI_CLIENT="ON"; fi
        if [ "$BUILD_LIBVTERM_SET" = false ]; then BUILD_LIBVTERM="ON"; fi
        
        # 交互式选择 SSH 模式（如果用户未显式指定）
        if [ "$BUILD_SSH_MODE_SET" = false ]; then
            interactive_ssh_mode_selection
        else
            print_info "SSH mode specified by user: ${BUILD_SSH_MODE}"
        fi
    fi
    
    print_info "Starting build process..."
    print_info "Project root: $PROJECT_ROOT"
    print_info "Build directory: $BUILD_DIR"
    echo ""
    
    # 检查依赖
    check_dependencies
    echo ""
    
    # 清理（如果需要）
    if [ "$clean_flag" = true ]; then
        clean_build
        echo ""
    fi
    
    # 配置 CMake
    configure_cmake
    echo ""
    
    # 编译
    build_project
    echo ""
    
    # 安装（如果需要）
    if [ "$install_flag" = true ]; then
        install_project --install
        echo ""
    fi
    
    print_success "Build process completed!"
    print_info "Executable location: $BUILD_DIR/pnana"
}

# 运行主函数
main "$@"
