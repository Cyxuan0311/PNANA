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
        print_warning "Go is not installed. SSH module will use system commands instead."
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
    
    if [ "$BUILD_GO" = "ON" ]; then
        cmake_args+=(-DBUILD_GO=ON)
        print_info "  - Go SSH module: ENABLED"
    fi
    
    if [ "$BUILD_AI_CLIENT" = "ON" ]; then
        cmake_args+=(-DBUILD_AI_CLIENT=ON)
        print_info "  - AI client support: ENABLED"
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
    echo "  --install        Install the project after building"
    echo "  --help           Show this help message"
    echo ""
    echo "CMake Options (enable features):"
    echo "  BUILD_IMAGE_PREVIEW=ON    Enable image preview support (requires FFmpeg)"
    echo "  BUILD_TREE_SITTER=ON      Enable Tree-sitter syntax highlighting"
    echo "  BUILD_LUA=ON              Enable Lua plugin system"
    echo "  BUILD_GO=ON              Enable Go SSH module"
    echo "  BUILD_AI_CLIENT=ON       Enable AI client support (requires libcurl)"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Build the project"
    echo "  $0 --clean                            # Clean and build"
    echo "  $0 --install                          # Build and install"
    echo "  $0 BUILD_IMAGE_PREVIEW=ON             # Build with image preview"
    echo "  $0 BUILD_LUA=ON BUILD_GO=ON           # Build with Lua and Go support"
    echo "  $0 --clean BUILD_TREE_SITTER=ON       # Clean and build with Tree-sitter"
    echo "  $0 --clean --install BUILD_AI_CLIENT=ON  # Clean, build, install with AI client"
}

# 主函数
main() {
    local clean_flag=false
    local install_flag=false
    
    # 初始化 CMake 选项变量
    BUILD_IMAGE_PREVIEW=""
    BUILD_TREE_SITTER=""
    BUILD_LUA=""
    BUILD_GO=""
    BUILD_AI_CLIENT=""
    
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
            --help|-h)
                show_help
                exit 0
                ;;
            BUILD_IMAGE_PREVIEW=*)
                BUILD_IMAGE_PREVIEW="${1#*=}"
                shift
                ;;
            BUILD_TREE_SITTER=*)
                BUILD_TREE_SITTER="${1#*=}"
                shift
                ;;
            BUILD_LUA=*)
                BUILD_LUA="${1#*=}"
                shift
                ;;
            BUILD_GO=*)
                BUILD_GO="${1#*=}"
                shift
                ;;
            BUILD_AI_CLIENT=*)
                BUILD_AI_CLIENT="${1#*=}"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
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
