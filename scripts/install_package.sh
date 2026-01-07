#!/bin/bash

# PNANA Installation Script
# Used to install PNANA packages in generic Linux (.tar.gz) format and create related configuration files

set -e

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default configuration
INSTALL_DIR="/usr/local"
CONFIG_DIR="$HOME/.config/pnana"
PLUGINS_DIR="$CONFIG_DIR/plugins"
THEMES_DIR="$CONFIG_DIR/themes"
DESKTOP_FILE_DIR="$HOME/.local/share/applications"
DESKTOP_FILE="pnana.desktop"
UNINSTALL_SCRIPT="$INSTALL_DIR/bin/pnana-uninstall"

# Show help information
show_help() {
    echo -e "${BLUE}PNANA Installation Script${NC}"
    echo ""
    echo "Usage: $0 [OPTIONS] <tar.gz file path>"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -d, --dir DIR           Specify installation directory (default: $INSTALL_DIR)"
    echo "  -c, --config DIR        Specify configuration directory (default: $CONFIG_DIR)"
    echo "  --no-desktop            Do not create desktop shortcut"
    echo "  --no-plugins            Do not install default plugins"
    echo "  --no-themes             Do not install default themes"
    echo ""
    echo "Examples:"
    echo "  $0 pnana-1.0.0-linux-x86_64.tar.gz"
    echo "  $0 -d /opt pnana-1.0.0-linux-x86_64.tar.gz"
    echo "  $0 --no-desktop pnana-1.0.0-linux-x86_64.tar.gz"
}

# Parse command line arguments
parse_args() {
    CREATE_DESKTOP=true
    INSTALL_PLUGINS=true
    INSTALL_THEMES=true
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -d|--dir)
                INSTALL_DIR="$2"
                shift 2
                ;;
            -c|--config)
                CONFIG_DIR="$2"
                PLUGINS_DIR="$CONFIG_DIR/plugins"
                THEMES_DIR="$CONFIG_DIR/themes"
                shift 2
                ;;
            --no-desktop)
                CREATE_DESKTOP=false
                shift
                ;;
            --no-plugins)
                INSTALL_PLUGINS=false
                shift
                ;;
            --no-themes)
                INSTALL_THEMES=false
                shift
                ;;
            -*)
                echo -e "${RED}Error: Unknown option $1${NC}" >&2
                show_help
                exit 1
                ;;
            *)
                PACKAGE_FILE="$1"
                shift
                ;;
        esac
    done
    
    if [[ -z "$PACKAGE_FILE" ]]; then
        echo -e "${RED}Error: Please specify the tar.gz file to install${NC}" >&2
        show_help
        exit 1
    fi
    
    if [[ ! -f "$PACKAGE_FILE" ]]; then
        echo -e "${RED}Error: File $PACKAGE_FILE does not exist${NC}" >&2
        exit 1
    fi
}

# Check dependencies
check_dependencies() {
    echo -e "${BLUE}Checking dependencies...${NC}"

    local missing_deps=()

    # Check basic commands
    for cmd in tar mkdir; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=($cmd)
        fi
    done
    
    # Check optional dependencies
    if [[ "$CREATE_DESKTOP" == true ]]; then
        if ! command -v desktop-file-validate &> /dev/null; then
            echo -e "${YELLOW}Warning: desktop-file-validate not found, skipping desktop file validation${NC}"
        fi
    fi

    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        echo -e "${RED}Error: Missing dependencies: ${missing_deps[*]}${NC}" >&2
        exit 1
    fi

    echo -e "${GREEN}Dependency check completed${NC}"
}

# Extract package information
extract_package_info() {
    echo -e "${BLUE}Extracting package information...${NC}"

    # Get package name (without extension)
    PACKAGE_NAME=$(basename "$PACKAGE_FILE" .tar.gz)

    # Create temporary directory
    TEMP_DIR=$(mktemp -d)

    # Extract package to temporary directory
    echo -e "${BLUE}Extracting package...${NC}"
    tar -xzf "$PACKAGE_FILE" -C "$TEMP_DIR"
    
    # Find extracted directory
    EXTRACTED_DIR=$(find "$TEMP_DIR" -maxdepth 1 -type d | head -n 2 | tail -n 1)

    if [[ ! -d "$EXTRACTED_DIR" ]]; then
        echo -e "${RED}Error: Cannot find extracted directory${NC}" >&2
        rm -rf "$TEMP_DIR"
        exit 1
    fi

    # Find binary file
    BINARY_PATH=$(find "$EXTRACTED_DIR" -name "pnana" -type f -executable | head -n 1)

    if [[ -z "$BINARY_PATH" ]]; then
        echo -e "${RED}Error: Cannot find pnana binary file in package${NC}" >&2
        rm -rf "$TEMP_DIR"
        exit 1
    fi

    # Check binary architecture
    BINARY_ARCH=$(file "$BINARY_PATH" | grep -o -E 'x86-64|x86_64|i386|i686|arm64|aarch64|arm' | head -n 1)

    if [[ -z "$BINARY_ARCH" ]]; then
        BINARY_ARCH="unknown"
    fi

    # Get version information
    VERSION_INFO=""
    if [[ -x "$BINARY_PATH" ]]; then
        VERSION_INFO=$("$BINARY_PATH" --version 2>&1 | head -n 1 || echo "")
    fi

    echo -e "${GREEN}Package information extraction completed${NC}"
    echo "  Package name: $PACKAGE_NAME"
    echo "  Version: $VERSION_INFO"
    echo "  Architecture: $BINARY_ARCH"
}

# Install binary file
install_binary() {
    echo -e "${BLUE}Installing binary file...${NC}"

    # Create installation directory
    sudo mkdir -p "$INSTALL_DIR/bin"

    # Copy binary file
    sudo cp "$BINARY_PATH" "$INSTALL_DIR/bin/pnana"

    # Set executable permissions
    sudo chmod +x "$INSTALL_DIR/bin/pnana"

    echo -e "${GREEN}Binary file installed to $INSTALL_DIR/bin/pnana${NC}"
}

# Create configuration directories
create_config_dirs() {
    echo -e "${BLUE}Creating configuration directories...${NC}"

    # Create main configuration directory
    mkdir -p "$CONFIG_DIR"

    # Create plugins directory
    if [[ "$INSTALL_PLUGINS" == true ]]; then
        mkdir -p "$PLUGINS_DIR"
    fi

    # Create themes directory
    if [[ "$INSTALL_THEMES" == true ]]; then
        mkdir -p "$THEMES_DIR"
    fi

    echo -e "${GREEN}Configuration directories created${NC}"
}

# Install configuration file
install_config() {
    echo -e "${BLUE}Installing configuration file...${NC}"

    # Find configuration file in package
    CONFIG_SOURCE=$(find "$EXTRACTED_DIR" -name "*.json" -path "*/config/*" | head -n 1)

    if [[ -n "$CONFIG_SOURCE" ]]; then
        # If configuration file is found, copy to configuration directory
        cp "$CONFIG_SOURCE" "$CONFIG_DIR/config.json"
        echo -e "${GREEN}Default configuration file installed to $CONFIG_DIR/config.json${NC}"
    else
        # If no configuration file is found, create a basic configuration file
        cat > "$CONFIG_DIR/config.json" << EOF
{
  "editor": {
    "theme": "monokai",
    "font_size": 12,
    "tab_size": 4,
    "insert_spaces": true,
    "word_wrap": false,
    "auto_indent": true
  },
  "display": {
    "show_line_numbers": true,
    "relative_line_numbers": false,
    "highlight_current_line": true,
    "show_whitespace": false
  },
  "files": {
    "encoding": "UTF-8",
    "line_ending": "LF",
    "trim_trailing_whitespace": true,
    "insert_final_newline": true,
    "auto_save": false,
    "auto_save_interval": 60
  },
  "search": {
    "case_sensitive": false,
    "whole_word": false,
    "regex": false,
    "wrap_around": true
  },
  "themes": {
    "current": "monokai"
  }
}
EOF
        echo -e "${GREEN}Basic configuration file created at $CONFIG_DIR/config.json${NC}"
    fi
}

# Install plugins
install_plugins() {
    if [[ "$INSTALL_PLUGINS" != true ]]; then
        return
    fi

    echo -e "${BLUE}Installing plugins...${NC}"

    # Find plugins directory in package
    PLUGINS_SOURCE=$(find "$EXTRACTED_DIR" -name "plugins" -type d | head -n 1)

    if [[ -n "$PLUGINS_SOURCE" ]]; then
        # If plugins directory is found, copy to configuration directory
        cp -r "$PLUGINS_SOURCE"/* "$PLUGINS_DIR/"
        echo -e "${GREEN}Plugins installed to $PLUGINS_DIR${NC}"
    else
        # If no plugins directory is found, create a basic plugin example
        mkdir -p "$PLUGINS_DIR/example"
        cat > "$PLUGINS_DIR/example/init.lua" << EOF
-- Example plugin
-- This is a simple PNANA plugin example

function pnana.init()
    -- Plugin initialization code
    pnana.log("Example plugin loaded")
end

function pnana.on_file_open(filename)
    -- Triggered when file is opened
    pnana.log("Opened file: " .. filename)
end

function pnana.on_key_pressed(key)
    -- Triggered when key is pressed
    -- Return true to indicate key is handled, false to continue processing
    return false
end

function pnana.cleanup()
    -- Plugin cleanup code
    pnana.log("Example plugin unloaded")
end
EOF
        echo -e "${GREEN}Example plugin created at $PLUGINS_DIR/example/init.lua${NC}"
    fi
}

# Install themes
install_themes() {
    if [[ "$INSTALL_THEMES" != true ]]; then
        return
    fi

    echo -e "${BLUE}Installing themes...${NC}"

    # Find themes directory in package
    THEMES_SOURCE=$(find "$EXTRACTED_DIR" -name "themes" -type d | head -n 1)

    if [[ -n "$THEMES_SOURCE" ]]; then
        # If themes directory is found, copy to configuration directory
        cp -r "$THEMES_SOURCE"/* "$THEMES_DIR/"
        echo -e "${GREEN}Themes installed to $THEMES_DIR${NC}"
    else
        # If no themes directory is found, create a basic theme example
        mkdir -p "$THEMES_DIR"
        cat > "$THEMES_DIR/custom.json" << EOF
{
  "name": "Custom Theme",
  "background": [30, 30, 30],
  "foreground": [255, 255, 255],
  "current_line": [50, 50, 50],
  "selection": [60, 60, 60],
  "line_number": [150, 150, 150],
  "line_number_current": [255, 255, 255],
  "statusbar_bg": [40, 40, 40],
  "statusbar_fg": [255, 255, 255],
  "menubar_bg": [25, 25, 25],
  "menubar_fg": [255, 255, 255],
  "helpbar_bg": [40, 40, 40],
  "helpbar_fg": [150, 150, 150],
  "helpbar_key": [100, 200, 100],
  "keyword": [255, 100, 100],
  "string": [100, 255, 100],
  "comment": [150, 150, 150],
  "number": [255, 200, 100],
  "function": [100, 150, 255],
  "type": [200, 100, 255],
  "operator_color": [255, 100, 100],
  "error": [255, 50, 50],
  "warning": [255, 200, 50],
  "info": [50, 150, 255],
  "success": [50, 255, 50]
}
EOF
        echo -e "${GREEN}Example theme created at $THEMES_DIR/custom.json${NC}"
    fi
}

# Create desktop shortcut
create_desktop_entry() {
    if [[ "$CREATE_DESKTOP" != true ]]; then
        return
    fi

    echo -e "${BLUE}Creating desktop shortcut...${NC}"

    # Create applications directory
    mkdir -p "$DESKTOP_FILE_DIR"

    # Create desktop file
    cat > "$DESKTOP_FILE_DIR/$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=pnana
Comment=Modern terminal text editor
Comment[zh_CN]=Modern terminal text editor
Exec=$INSTALL_DIR/bin/pnana %F
Icon=pnana
Terminal=true
Categories=TextEditor;Development;Utility;
MimeType=text/plain;text/x-chdr;text/x-csrc;text/x-c++hdr;text/x-c++src;text/x-java;text/x-perl;text/x-python;application/x-shellscript;text/x-c;text/x-c++;
StartupNotify=true
EOF

    # Validate desktop file
    if command -v desktop-file-validate &> /dev/null; then
        if desktop-file-validate "$DESKTOP_FILE_DIR/$DESKTOP_FILE"; then
            echo -e "${GREEN}Desktop shortcut created and validated${NC}"
        else
            echo -e "${YELLOW}Warning: Desktop file validation failed, but created${NC}"
        fi
    else
        echo -e "${GREEN}Desktop shortcut created${NC}"
    fi

    # Update desktop database
    if command -v update-desktop-database &> /dev/null; then
        update-desktop-database "$DESKTOP_FILE_DIR" 2>/dev/null || true
    fi
}

# Create uninstall script
create_uninstall_script() {
    echo -e "${BLUE}Creating uninstall script...${NC}"
    
    # Create uninstall script
    sudo tee "$UNINSTALL_SCRIPT" > /dev/null << EOF
#!/bin/bash

# PNANA uninstallation script
# This script is automatically generated by the PNANA installer

set -e

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Installation information
INSTALL_DIR="$INSTALL_DIR"
CONFIG_DIR="$CONFIG_DIR"
DESKTOP_FILE_DIR="$DESKTOP_FILE_DIR"
DESKTOP_FILE="$DESKTOP_FILE"

# Show help information
show_help() {
    echo -e "\${BLUE}PNANA Uninstallation Script\${NC}"
    echo ""
    echo "Usage: \$0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  --config-only           Remove only configuration files"
    echo "  --desktop-only          Remove only desktop shortcut"
    echo ""
}

# Parse command line arguments
CONFIG_ONLY=false
DESKTOP_ONLY=false

while [[ \$# -gt 0 ]]; do
    case \$1 in
        -h|--help)
            show_help
            exit 0
            ;;
        --config-only)
            CONFIG_ONLY=true
            shift
            ;;
        --desktop-only)
            DESKTOP_ONLY=true
            shift
            ;;
        -*)
            echo -e "\${RED}Error: Unknown option \$1\${NC}" >&2
            show_help
            exit 1
            ;;
        *)
            shift
            ;;
    esac
done

# Uninstall binary files
if [[ "\$DESKTOP_ONLY" != true ]]; then
    echo -e "\${BLUE}Uninstalling binary files...\${NC}"

    if [[ -f "\$INSTALL_DIR/bin/pnana" ]]; then
        sudo rm -f "\$INSTALL_DIR/bin/pnana"
        echo -e "\${GREEN}Binary files removed\${NC}"
    else
        echo -e "\${YELLOW}Warning: Binary files do not exist\${NC}"
    fi
fi

# Remove configuration files
if [[ "\$DESKTOP_ONLY" != true ]]; then
    echo -e "\${BLUE}Removing configuration files...\${NC}"

    if [[ -d "\$CONFIG_DIR" ]]; then
        read -p "Remove configuration directory \$CONFIG_DIR? [y/N] " -n 1 -r
        echo
        if [[ \$REPLY =~ ^[Yy]\$ ]]; then
            rm -rf "\$CONFIG_DIR"
            echo -e "\${GREEN}Configuration directory removed\${NC}"
        else
            echo -e "\${YELLOW}Configuration directory preserved\${NC}"
        fi
    else
        echo -e "\${YELLOW}Warning: Configuration directory does not exist\${NC}"
    fi
fi

# Remove desktop shortcut
if [[ "\$CONFIG_ONLY" != true ]]; then
    echo -e "\${BLUE}Removing desktop shortcut...\${NC}"

    if [[ -f "\$DESKTOP_FILE_DIR/\$DESKTOP_FILE" ]]; then
        rm -f "\$DESKTOP_FILE_DIR/\$DESKTOP_FILE"
        echo -e "\${GREEN}Desktop shortcut removed\${NC}"

        # Update desktop database
        if command -v update-desktop-database &> /dev/null; then
            update-desktop-database "\$DESKTOP_FILE_DIR" 2>/dev/null || true
        fi
    else
        echo -e "\${YELLOW}Warning: Desktop shortcut does not exist\${NC}"
    fi
fi

# Remove uninstall script itself
echo -e "\${BLUE}Removing uninstall script...\${NC}"
sudo rm -f "\$0"

echo -e "\${GREEN}PNANA uninstallation completed\${NC}"
EOF
    
    # Set executable permissions
    sudo chmod +x "$UNINSTALL_SCRIPT"
    
    echo -e "${GREEN}Uninstall script created at $UNINSTALL_SCRIPT${NC}"
}

# Clean up temporary files
cleanup() {
    if [[ -n "$TEMP_DIR" && -d "$TEMP_DIR" ]]; then
        rm -rf "$TEMP_DIR"
    fi
}

# Show installation completion information
show_completion_info() {
    echo ""
    echo -e "${GREEN}=====================================${NC}"
    echo -e "${GREEN}PNANA installation completed!${NC}"
    echo -e "${GREEN}=====================================${NC}"
    echo ""
    echo -e "${BLUE}Installation information:${NC}"
    echo "  Binary file: $INSTALL_DIR/bin/pnana"
    echo "  Configuration directory: $CONFIG_DIR"
    echo "  Plugins directory: $PLUGINS_DIR"
    echo "  Themes directory: $THEMES_DIR"
    echo ""

    if [[ "$CREATE_DESKTOP" == true ]]; then
        echo -e "${BLUE}Desktop shortcut:${NC}"
        echo "  File location: $DESKTOP_FILE_DIR/$DESKTOP_FILE"
        echo ""
    fi

    echo -e "${BLUE}Usage:${NC}"
    echo "  Start editor: $INSTALL_DIR/bin/pnana"
    echo "  Open file: $INSTALL_DIR/bin/pnana <filename>"
    echo ""

    echo -e "${BLUE}Uninstallation:${NC}"
    echo "  Run uninstall script: $UNINSTALL_SCRIPT"
    echo ""

    echo -e "${YELLOW}Tips:${NC}"
    echo "  - Ensure $INSTALL_DIR/bin is in your PATH environment variable"
    echo "  - Configuration file is located at $CONFIG_DIR/config.json"
    echo "  - You can add more plugins to $PLUGINS_DIR"
    echo "  - You can add more themes to $THEMES_DIR"
    echo ""
}

# Main function
main() {
    # Parse command line arguments
    parse_args "$@"
    
    # Set cleanup trap
    trap cleanup EXIT
    
    # Check dependencies
    check_dependencies
    
    # Extract package information
    extract_package_info
    
    # Install binary file
    install_binary
    
    # Create configuration directories
    create_config_dirs
    
    # Install configuration file
    install_config
    
    # Install plugins
    install_plugins
    
    # Install themes
    install_themes
    
    # Create desktop shortcut
    create_desktop_entry
    
    # Create uninstall script
    create_uninstall_script
    
    # Show installation completion information
    show_completion_info
}

# Execute main function
main "$@"