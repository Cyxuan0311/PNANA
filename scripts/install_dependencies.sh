#!/bin/bash

# pnana Dependency Installation Script
# This script helps install all dependencies required for building pnana
# Supports: Ubuntu/Debian, Fedora/RHEL, macOS

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/debian_version ]; then
            OS="debian"
        elif [ -f /etc/redhat-release ]; then
            OS="fedora"
        else
            OS="linux"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
    else
        OS="unknown"
    fi
}

# Print colored message
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
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Install build tools
install_build_tools() {
    print_info "Installing build tools..."
    
    case $OS in
        debian)
            sudo apt update
            sudo apt install -y cmake build-essential g++
            ;;
        fedora)
            sudo dnf install -y cmake gcc-c++
            ;;
        macos)
            if ! command_exists cmake; then
                brew install cmake
            fi
            if ! command_exists xcode-select; then
                print_warning "Xcode Command Line Tools not found. Installing..."
                xcode-select --install || true
            fi
            ;;
        *)
            print_error "Unsupported OS for automatic installation"
            return 1
            ;;
    esac
    
    print_success "Build tools installed"
}

# Install FTXUI
install_ftxui() {
    print_info "Installing FTXUI..."
    
    case $OS in
        debian)
            sudo apt install -y libftxui-dev || {
                print_warning "FTXUI not found in package manager. You may need to install from source."
                return 1
            }
            ;;
        fedora)
            sudo dnf install -y ftxui-devel || {
                print_warning "FTXUI not found in package manager. You may need to install from source."
                return 1
            }
            ;;
        macos)
            brew install ftxui || {
                print_warning "FTXUI not found in Homebrew. You may need to install from source."
                return 1
            }
            ;;
    esac
    
    print_success "FTXUI installed"
}

# Install LSP dependencies
install_lsp_deps() {
    print_info "Installing LSP dependencies (nlohmann/json)..."
    
    case $OS in
        debian)
            sudo apt install -y nlohmann-json3-dev
            ;;
        fedora)
            sudo dnf install -y nlohmann_json-devel
            ;;
        macos)
            brew install nlohmann-json
            ;;
    esac
    
    print_success "LSP dependencies installed"
    print_info "Note: jsonrpccxx is included in the project as a submodule"
}

# Install Tree-sitter
install_treesitter() {
    print_info "Installing Tree-sitter..."
    
    case $OS in
        debian)
            # Try package manager first
            if sudo apt install -y libtree-sitter-dev 2>/dev/null; then
                print_success "Tree-sitter installed from package manager"
            else
                print_warning "Tree-sitter not found in package manager. Installing from source..."
                install_treesitter_from_source
            fi
            ;;
        fedora)
            # Try package manager first
            if sudo dnf install -y tree-sitter-devel 2>/dev/null; then
                print_success "Tree-sitter installed from package manager"
            else
                print_warning "Tree-sitter not found in package manager. Installing from source..."
                install_treesitter_from_source
            fi
            ;;
        macos)
            if brew install tree-sitter 2>/dev/null; then
                print_success "Tree-sitter installed from Homebrew"
            else
                print_warning "Tree-sitter not found in Homebrew. Installing from source..."
                install_treesitter_from_source
            fi
            ;;
    esac
}

# Install Tree-sitter from source
install_treesitter_from_source() {
    print_info "Installing Tree-sitter from source..."
    
    local temp_dir=$(mktemp -d)
    cd "$temp_dir"
    
    git clone https://github.com/tree-sitter/tree-sitter.git
    cd tree-sitter
    make
    sudo make install
    
    cd - > /dev/null
    rm -rf "$temp_dir"
    
    print_success "Tree-sitter installed from source"
}

# Install Tree-sitter language libraries
install_treesitter_languages() {
    print_info "Installing Tree-sitter language libraries..."
    
    local languages=(
        "tree-sitter-cpp"
        "tree-sitter-c"
        "tree-sitter-python"
        "tree-sitter-javascript"
        "tree-sitter-typescript"
        "tree-sitter-json"
        "tree-sitter-markdown"
        "tree-sitter-bash"
        "tree-sitter-rust"
        "tree-sitter-go"
        "tree-sitter-java"
    )
    
    local temp_dir=$(mktemp -d)
    cd "$temp_dir"
    
    for lang in "${languages[@]}"; do
        print_info "Installing $lang..."
        if git clone "https://github.com/tree-sitter/${lang}.git" 2>/dev/null; then
            cd "$lang"
            if [ -f Makefile ]; then
                make
                sudo make install
                print_success "$lang installed"
            else
                print_warning "$lang: No Makefile found, skipping"
            fi
            cd "$temp_dir"
        else
            print_warning "$lang: Failed to clone repository"
        fi
    done
    
    cd - > /dev/null
    rm -rf "$temp_dir"
    
    print_success "Tree-sitter language libraries installation completed"
}

# Install Lua
install_lua() {
    print_info "Installing Lua..."
    
    case $OS in
        debian)
            sudo apt install -y liblua5.4-dev || sudo apt install -y liblua5.3-dev
            ;;
        fedora)
            sudo dnf install -y lua-devel || sudo dnf install -y lua5.4-devel
            ;;
        macos)
            brew install lua
            ;;
    esac
    
    print_success "Lua installed"
}

# Install Go
install_go() {
    print_info "Installing Go..."
    
    case $OS in
        debian)
            sudo apt install -y golang-go
            ;;
        fedora)
            sudo dnf install -y golang
            ;;
        macos)
            brew install go
            ;;
    esac
    
    print_success "Go installed"
}

# Main installation function
main() {
    print_info "pnana Dependency Installation Script"
    print_info "====================================="
    
    detect_os
    
    if [ "$OS" == "unknown" ]; then
        print_error "Unsupported operating system"
        exit 1
    fi
    
    print_info "Detected OS: $OS"
    echo
    
    # Parse command line arguments
    INSTALL_ALL=false
    INSTALL_BUILD_TOOLS=false
    INSTALL_FTXUI=false
    INSTALL_LSP=false
    INSTALL_TREESITTER=false
    INSTALL_TREESITTER_LANGS=false
    INSTALL_LUA=false
    INSTALL_GO=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --all)
                INSTALL_ALL=true
                shift
                ;;
            --build-tools)
                INSTALL_BUILD_TOOLS=true
                shift
                ;;
            --ftxui)
                INSTALL_FTXUI=true
                shift
                ;;
            --lsp)
                INSTALL_LSP=true
                shift
                ;;
            --treesitter)
                INSTALL_TREESITTER=true
                shift
                ;;
            --treesitter-langs)
                INSTALL_TREESITTER_LANGS=true
                shift
                ;;
            --lua)
                INSTALL_LUA=true
                shift
                ;;
            --go)
                INSTALL_GO=true
                shift
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo
                echo "Options:"
                echo "  --all              Install all dependencies"
                echo "  --build-tools      Install build tools (CMake, C++ compiler)"
                echo "  --ftxui            Install FTXUI library"
                echo "  --lsp              Install LSP dependencies (nlohmann/json)"
                echo "  --treesitter       Install Tree-sitter core library"
                echo "  --treesitter-langs Install Tree-sitter language libraries"
                echo "  --lua              Install Lua (for plugin system)"
                echo "  --go               Install Go (for SSH module)"
                echo "  --help             Show this help message"
                echo
                echo "Examples:"
                echo "  $0 --all                    # Install everything"
                echo "  $0 --build-tools --ftxui    # Install only build tools and FTXUI"
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    # If no options specified, show help
    if [ "$INSTALL_ALL" == false ] && \
       [ "$INSTALL_BUILD_TOOLS" == false ] && \
       [ "$INSTALL_FTXUI" == false ] && \
       [ "$INSTALL_LSP" == false ] && \
       [ "$INSTALL_TREESITTER" == false ] && \
       [ "$INSTALL_TREESITTER_LANGS" == false ] && \
       [ "$INSTALL_LUA" == false ] && \
       [ "$INSTALL_GO" == false ]; then
        print_info "No options specified. Showing help..."
        echo
        "$0" --help
        exit 0
    fi
    
    # Install based on flags
    if [ "$INSTALL_ALL" == true ]; then
        install_build_tools
        install_ftxui
        install_lsp_deps
        install_treesitter
        install_treesitter_languages
        install_lua
        install_go
    else
        [ "$INSTALL_BUILD_TOOLS" == true ] && install_build_tools
        [ "$INSTALL_FTXUI" == true ] && install_ftxui
        [ "$INSTALL_LSP" == true ] && install_lsp_deps
        [ "$INSTALL_TREESITTER" == true ] && install_treesitter
        [ "$INSTALL_TREESITTER_LANGS" == true ] && install_treesitter_languages
        [ "$INSTALL_LUA" == true ] && install_lua
        [ "$INSTALL_GO" == true ] && install_go
    fi
    
    echo
    print_success "Dependency installation completed!"
    print_info "You can now build pnana using: ./build.sh"
}

# Run main function
main "$@"

