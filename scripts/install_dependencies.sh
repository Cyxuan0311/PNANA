#!/bin/bash

# PNANA Dependencies Installation Script
# Supports automatic download, compilation and installation of core dependencies: FTXUI, Tree-sitter, nlohmann/json

set -e  # Exit immediately on error

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build/dependencies"
INSTALL_PREFIX="/usr/local"

# Version configuration
FTXUI_VERSION="main"
TREE_SITTER_VERSION="v0.22.6"
NLOHMANN_JSON_VERSION="v3.11.3"

# Print colored messages
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

# Check if command exists
check_command() {
    if ! command -v "$1" &> /dev/null; then
        print_error "$1 is not installed or not in PATH"
        return 1
    fi
    return 0
}

# Detect operating system
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if command -v apt-get &> /dev/null; then
            OS="debian"
            PACKAGE_MANAGER="apt-get"
        elif command -v yum &> /dev/null; then
            OS="redhat"
            PACKAGE_MANAGER="yum"
        elif command -v pacman &> /dev/null; then
            OS="arch"
            PACKAGE_MANAGER="pacman"
        else
            OS="linux"
            PACKAGE_MANAGER=""
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
        PACKAGE_MANAGER="brew"
    else
        OS="unknown"
        PACKAGE_MANAGER=""
    fi

    print_info "Detected OS: $OS"
    if [[ -n "$PACKAGE_MANAGER" ]]; then
        print_info "Package manager: $PACKAGE_MANAGER"
    fi
}

# Install basic build tools
install_build_tools() {
    print_info "Installing build tools..."

    case $OS in
        debian)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                ninja-build \
                git \
                pkg-config \
                wget \
                curl
            ;;
        redhat)
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y \
                cmake \
                git \
                pkgconfig \
                wget \
                curl
            ;;
        arch)
            sudo pacman -Syu --noconfirm \
                base-devel \
                cmake \
                ninja \
                git \
                pkg-config \
                wget \
                curl
            ;;
        macos)
            if ! command -v brew &> /dev/null; then
                print_error "Homebrew is required on macOS. Please install it first."
                exit 1
            fi
            brew install \
                cmake \
                ninja \
                git \
                pkg-config \
                wget \
                curl
            ;;
        *)
            print_warning "Unknown OS. Please install build tools manually:"
            print_warning "  - C++ compiler (gcc/g++/clang)"
            print_warning "  - CMake"
            print_warning "  - Git"
            print_warning "  - pkg-config"
            print_warning "  - wget/curl"
            ;;
    esac

    # Verify installation
    if ! check_command cmake; then
        print_error "CMake installation failed"
        exit 1
    fi

    if ! check_command g++; then
        if ! check_command clang++; then
            print_error "No C++ compiler found (g++ or clang++)"
            exit 1
        fi
    fi

    print_success "Build tools installed successfully"
}

# Install optional dependencies
install_optional_deps() {
    print_info "Installing optional dependencies..."

    case $OS in
        debian)
            # Lua support
            if sudo apt-get install -y liblua5.4-dev 2>/dev/null; then
                print_success "Lua 5.4 installed"
            elif sudo apt-get install -y liblua5.3-dev 2>/dev/null; then
                print_success "Lua 5.3 installed"
            else
                print_warning "Lua not found. Plugin system will be disabled."
            fi

            # FFmpeg support (for image preview)
            if sudo apt-get install -y libavformat-dev libavcodec-dev libswscale-dev libavutil-dev 2>/dev/null; then
                print_success "FFmpeg installed"
            else
                print_warning "FFmpeg not found. Image preview will be disabled."
            fi

            # Go support (for SSH module)
            if sudo apt-get install -y golang-go 2>/dev/null; then
                print_success "Go installed"
            else
                print_warning "Go not found. SSH module will use fallback implementation."
            fi
            ;;
        redhat)
            # Similar package installation logic
            print_warning "Optional dependencies installation not fully implemented for Red Hat"
            ;;
        arch)
            # Similar package installation logic
            print_warning "Optional dependencies installation not fully implemented for Arch"
            ;;
        macos)
            # Similar package installation logic
            print_warning "Optional dependencies installation not fully implemented for macOS"
            ;;
    esac
}

# Build and install dependencies from source
build_and_install_dep() {
    local name="$1"
    local repo_url="$2"
    local version="$3"
    local build_commands="$4"
    local install_commands="$5"

    print_info "Building and installing $name..."

    # Create build directory
    local dep_build_dir="$BUILD_DIR/$name"
    mkdir -p "$dep_build_dir"
    cd "$dep_build_dir"

    # Clone repository
    if [[ ! -d ".git" ]]; then
        print_info "Cloning $name repository..."
        git clone "$repo_url" .
        if [[ -n "$version" && "$version" != "main" && "$version" != "master" ]]; then
            git checkout "$version"
        fi
    else
        print_info "Updating $name repository..."
        git pull
        if [[ -n "$version" && "$version" != "main" && "$version" != "master" ]]; then
            git checkout "$version"
        fi
    fi

    # Build
    print_info "Building $name..."
    eval "$build_commands"

    # Install
    print_info "Installing $name..."
    eval "$install_commands"

    print_success "$name installed successfully"

    # Return to project root directory
    cd "$PROJECT_ROOT"
}

# Install FTXUI
install_ftxui() {
    local build_cmd="mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DFTXUI_BUILD_EXAMPLES=OFF -DFTXUI_BUILD_DOCS=OFF -DFTXUI_ENABLE_INSTALL=ON && make -j\$(nproc)"
    local install_cmd="cd build && sudo make install"
    build_and_install_dep "FTXUI" "https://github.com/ArthurSonzogni/FTXUI.git" "$FTXUI_VERSION" "$build_cmd" "$install_cmd"
}

# Install Tree-sitter
install_tree_sitter() {
    local build_cmd="mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX && make -j\$(nproc)"
    local install_cmd="cd build && sudo make install"
    build_and_install_dep "Tree-sitter" "https://github.com/tree-sitter/tree-sitter.git" "$TREE_SITTER_VERSION" "$build_cmd" "$install_cmd"
}

# Install nlohmann/json (header-only library)
install_nlohmann_json() {
    print_info "Installing nlohmann/json..."

    # Create installation directory
    local include_dir="$INSTALL_PREFIX/include/nlohmann"
    sudo mkdir -p "$include_dir"

    # Download and install
    local temp_dir="$BUILD_DIR/nlohmann_json"
    mkdir -p "$temp_dir"
    cd "$temp_dir"

    if [[ ! -d ".git" ]]; then
        git clone "https://github.com/nlohmann/json.git" .
        git checkout "$NLOHMANN_JSON_VERSION"
    fi

    # Copy header files
    sudo cp -r single_include/nlohmann/* "$include_dir/"

    print_success "nlohmann/json installed successfully"
    cd "$PROJECT_ROOT"
}

# Verify installation
verify_installation() {
    print_info "Verifying installations..."

    # Check FTXUI
    if pkg-config --exists ftxui || [[ -f "$INSTALL_PREFIX/lib/cmake/ftxui/ftxui-config.cmake" ]]; then
        print_success "FTXUI verification passed"
    else
        print_warning "FTXUI verification failed - may need manual configuration"
    fi

    # Check Tree-sitter
    if pkg-config --exists tree-sitter 2>/dev/null || [[ -f "$INSTALL_PREFIX/lib/libtree-sitter.a" ]]; then
        print_success "Tree-sitter verification passed"
    else
        print_warning "Tree-sitter verification failed - may need manual configuration"
    fi

    # Check nlohmann/json
    if [[ -f "$INSTALL_PREFIX/include/nlohmann/json.hpp" ]]; then
        print_success "nlohmann/json verification passed"
    else
        print_warning "nlohmann/json verification failed - may need manual configuration"
    fi
}

# Clean up build files
cleanup() {
    if [[ "$1" == "--cleanup" ]]; then
        print_info "Cleaning up build files..."
        rm -rf "$BUILD_DIR"
        print_success "Cleanup completed"
    fi
}

# Show help information
show_help() {
    echo "PNANA Dependencies Installation Script"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --help          Show this help message"
    echo "  --cleanup       Clean build files after installation"
    echo "  --skip-optional  Skip optional dependencies installation"
    echo "  --prefix PATH   Set installation prefix (default: /usr/local)"
    echo ""
    echo "Core dependencies (required):"
    echo "  - FTXUI: Terminal UI library"
    echo "  - Tree-sitter: Syntax parser"
    echo "  - nlohmann/json: JSON library"
    echo ""
    echo "Optional dependencies:"
    echo "  - Lua: Plugin system support"
    echo "  - FFmpeg: Image preview support"
    echo "  - Go: SSH module support"
    echo ""
    echo "Examples:"
    echo "  $0                           # Install all dependencies"
    echo "  $0 --cleanup                # Install and clean build files"
    echo "  $0 --skip-optional          # Install only core dependencies"
    echo "  $0 --prefix /opt/pnana      # Custom installation path"
}

# Main function
main() {
    local skip_optional=false
    local cleanup_after=false

    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help|-h)
                show_help
                exit 0
                ;;
            --cleanup)
                cleanup_after=true
                shift
                ;;
            --skip-optional)
                skip_optional=true
                shift
                ;;
            --prefix)
                if [[ -n "$2" && ! "$2" =~ ^- ]]; then
                    INSTALL_PREFIX="$2"
                    shift 2
                else
                    print_error "--prefix requires a path argument"
                    exit 1
                fi
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done

    print_info "=== PNANA Dependencies Installation Script ==="
    print_info "Installation prefix: $INSTALL_PREFIX"
    print_info "Build directory: $BUILD_DIR"
    echo ""

    # Detect operating system
    detect_os
    echo ""

    # Check if running as root
    if [[ $EUID -eq 0 ]]; then
        print_warning "Running as root. This may cause permission issues."
        print_warning "Consider running as regular user with sudo when needed."
    fi

    # Install basic tools
    install_build_tools
    echo ""

    # Install optional dependencies
    if [[ "$skip_optional" != true ]]; then
        install_optional_deps
        echo ""
    else
        print_info "Skipping optional dependencies installation"
    fi

    # Create build directory
    mkdir -p "$BUILD_DIR"

    # Install core dependencies
    print_info "Installing core dependencies..."
    echo ""

    install_ftxui
    echo ""

    install_tree_sitter
    echo ""

    install_nlohmann_json
    echo ""

    # Verify installation
    verify_installation
    echo ""

    # Cleanup
    if [[ "$cleanup_after" == true ]]; then
        cleanup --cleanup
        echo ""
    fi

    # Update library cache (Linux)
    if [[ "$OS" == "debian" || "$OS" == "redhat" ]]; then
        print_info "Updating library cache..."
        sudo ldconfig 2>/dev/null || true
    fi

    print_success "🎉 All dependencies installed successfully!"
    print_info "Now you can build PNANA with the following commands:"
    echo "  mkdir build && cd build"
    echo "  cmake .."
    echo "  make -j\$(nproc)"
    echo ""
    print_info "Or use the project's build script:"
    echo "  ./build.sh --clean"
}

# Run main function
main "$@"
