# PNANA 打包配置
# 此文件包含所有CPack和安装相关的配置

# ============================================================================
# 通用打包配置
# ============================================================================

set(CPACK_PACKAGE_NAME "pnana")
set(CPACK_PACKAGE_VERSION "0.0.4")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern Terminal Text Editor")
set(CPACK_PACKAGE_DESCRIPTION "PNANA is a modern terminal-based text editor built with C++ and FTXUI, featuring syntax highlighting, LSP support, and a plugin system.")
set(CPACK_PACKAGE_VENDOR "PNANA Project")
set(CPACK_PACKAGE_CONTACT "https://github.com/Cyxuan0311/PNANA.git")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Cyxuan0311/PNANA.git")

# 设置包的元信息
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")

# 设置安装组件
set(CPACK_COMPONENTS_ALL pnana config plugins)
set(CPACK_COMPONENT_PNANA_DISPLAY_NAME "PNANA Editor")
set(CPACK_COMPONENT_PNANA_DESCRIPTION "The main pnana executable")
set(CPACK_COMPONENT_PNANA_REQUIRED ON)
set(CPACK_COMPONENT_CONFIG_DISPLAY_NAME "Configuration")
set(CPACK_COMPONENT_CONFIG_DESCRIPTION "Default configuration files")
set(CPACK_COMPONENT_PLUGINS_DISPLAY_NAME "Plugins")
set(CPACK_COMPONENT_PLUGINS_DESCRIPTION "Plugin files")

# ============================================================================
# 平台特定的打包配置
# ============================================================================

# 启用多个生成器
if(WIN32)
    # Windows - 已移除
    set(CPACK_GENERATOR "ZIP")
    message(STATUS "Windows packaging: ZIP only (NSIS removed)")
elseif(APPLE)
    # macOS
    set(CPACK_GENERATOR "ZIP;DragNDrop")
elseif(UNIX)
    # Linux/Unix - 扩展支持多种包格式
    set(CPACK_GENERATOR "DEB;RPM;TGZ;ZIP;TXZ;TBZ2")

    # DEB 包配置 (Debian/Ubuntu/Mint/Pop!_OS 等)
    set(CPACK_DEBIAN_PACKAGE_NAME "pnana")
    set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_DEBIAN_PACKAGE_LICENSE "MIT")

    # RPM 包配置 (Red Hat/Fedora/CentOS/RHEL/SUSE 等)
    set(CPACK_RPM_PACKAGE_NAME "pnana")
    set(CPACK_RPM_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    set(CPACK_RPM_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Editors")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")

    # Arch Linux 包配置 (通过 TXZ)
    set(CPACK_ARCHIVE_TXZ_COMPRESSION "XZ")

    # 通用归档配置 - 生成单个完整包而不是按组件分割
    set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)

    # 安装后脚本
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/debian/postinst")
    set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/rpm/postinstall")

else()
    # 默认配置
    set(CPACK_GENERATOR "TGZ;ZIP")
endif()

# ============================================================================
# Linux 包格式扩展配置
# ============================================================================
if(UNIX AND NOT APPLE)
    # Pacman (Arch Linux) 包配置
    set(CPACK_PACMAN_PACKAGE_NAME "pnana")
    set(CPACK_PACMAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
    set(CPACK_PACMAN_PACKAGE_ARCHITECTURE "x86_64")
    set(CPACK_PACMAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_PACMAN_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
    set(CPACK_PACMAN_PACKAGE_LICENSE "MIT")
    set(CPACK_PACMAN_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")

    # Snap 包配置 (如果启用)
    option(BUILD_SNAP "Build Snap package for Ubuntu" OFF)
    if(BUILD_SNAP)
        set(CPACK_SNAP_PACKAGE_NAME "pnana")
        set(CPACK_SNAP_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}")
        set(CPACK_SNAP_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
        set(CPACK_SNAP_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
        set(CPACK_SNAP_PACKAGE_LICENSE "MIT")
        set(CPACK_SNAP_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
        set(CPACK_SNAP_PACKAGE_ARCHITECTURE "amd64")

        # Snap 特定配置
        set(CPACK_SNAP_SNAPCRAFT_FILE "${CMAKE_SOURCE_DIR}/snap/snapcraft.yaml")
    endif()

    # Flatpak 包配置 (如果启用)
    option(BUILD_FLATPAK "Build Flatpak package" OFF)
    if(BUILD_FLATPAK)
        set(CPACK_FLATPAK_MANIFEST_FILE "${CMAKE_SOURCE_DIR}/flatpak/org.pnana.Editor.yaml")
        set(CPACK_FLATPAK_BUILD_DIR "${CMAKE_BINARY_DIR}/flatpak-build")
    endif()

    # AppImage 配置 (如果启用)
    option(BUILD_APPIMAGE "Build AppImage package" OFF)
    if(BUILD_APPIMAGE)
        set(CPACK_APPIMAGE_BASEDIR "${CMAKE_BINARY_DIR}/AppDir")
        set(CPACK_APPIMAGE_DESKTOP_FILE "${CMAKE_SOURCE_DIR}/appimage/pnana.desktop")
        set(CPACK_APPIMAGE_ICON_FILE "${CMAKE_SOURCE_DIR}/appimage/pnana.png")
    endif()
endif()

# ============================================================================
# macOS 配置
# ============================================================================
if(APPLE)
    set(CPACK_BUNDLE_NAME "PNANA")
    set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/resources/pnana.icns")
    set(CPACK_BUNDLE_PLIST "${CMAKE_BINARY_DIR}/Info.plist")

    # 创建 Info.plist
    set(MACOSX_BUNDLE_BUNDLE_NAME "PNANA")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${CPACK_PACKAGE_VERSION}")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2024 PNANA Project")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.pnana.editor")

    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
        ${CMAKE_BINARY_DIR}/Info.plist
        @ONLY
    )
endif()

# ============================================================================
# Linux 安装后脚本
# ============================================================================
if(UNIX AND NOT APPLE)
    # 检测发行版
    execute_process(
        COMMAND sh -c "cat /etc/os-release | grep -E '^ID=' | cut -d'=' -f2 | tr -d '\"'"
        OUTPUT_VARIABLE DISTRO_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # 根据发行版设置不同的依赖和路径
    if(DISTRO_ID STREQUAL "ubuntu" OR DISTRO_ID STREQUAL "debian" OR DISTRO_ID STREQUAL "linuxmint" OR DISTRO_ID STREQUAL "pop")
        set(SYSTEM_DATA_DIR "/usr/share/pnana")
        set(DEPENDENCIES "libc6 (>= 2.27), libstdc++6 (>= 7.3.0), libncurses6 (>= 6.1), libtinfo6 (>= 6.1)")
    elseif(DISTRO_ID STREQUAL "fedora" OR DISTRO_ID STREQUAL "rhel" OR DISTRO_ID STREQUAL "centos")
        set(SYSTEM_DATA_DIR "/usr/share/pnana")
        set(DEPENDENCIES "glibc >= 2.27, libstdc++ >= 7.3.0, ncurses >= 6.1")
    elseif(DISTRO_ID STREQUAL "arch" OR DISTRO_ID STREQUAL "manjaro")
        set(SYSTEM_DATA_DIR "/usr/share/pnana")
        set(DEPENDENCIES "glibc>=2.27, gcc-libs>=7.3.0, ncurses>=6.1")
    else()
        set(SYSTEM_DATA_DIR "/usr/local/share/pnana")
        set(DEPENDENCIES "libc6 (>= 2.27), libstdc++6 (>= 7.3.0), libncurses6 (>= 6.1)")
    endif()

    # DEB 安装后脚本
    file(WRITE ${CMAKE_BINARY_DIR}/debian/postinst
        "#!/bin/bash\n"
        "set -e\n"
        "\n"
        "# PNANA 安装后配置脚本\n"
        "# 支持多个Linux发行版\n"
        "\n"
        "# 检测发行版\n"
        "if [ -f /etc/os-release ]; then\n"
        "    . /etc/os-release\n"
        "    DISTRO_ID=$ID\n"
        "else\n"
        "    DISTRO_ID=\"unknown\"\n"
        "fi\n"
        "\n"
        "# 根据发行版设置数据目录\n"
        "case $DISTRO_ID in\n"
        "    ubuntu|debian|linuxmint|pop)\n"
        "        SYSTEM_DATA_DIR=\"/usr/share/pnana\"\n"
        "        ;;\n"
        "    fedora|rhel|centos)\n"
        "        SYSTEM_DATA_DIR=\"/usr/share/pnana\"\n"
        "        ;;\n"
        "    arch|manjaro)\n"
        "        SYSTEM_DATA_DIR=\"/usr/share/pnana\"\n"
        "        ;;\n"
        "    *)\n"
        "        SYSTEM_DATA_DIR=\"/usr/local/share/pnana\"\n"
        "        ;;\n"
        "esac\n"
        "\n"
        "# 创建用户配置目录\n"
        "mkdir -p ~/.config/pnana 2>/dev/null || true\n"
        "mkdir -p ~/.config/pnana/plugins 2>/dev/null || true\n"
        "\n"
        "# 复制默认配置\n"
        "if [ -f \"$SYSTEM_DATA_DIR/default_config.json\" ]; then\n"
        "    cp -n \"$SYSTEM_DATA_DIR/default_config.json\" ~/.config/pnana/config.json 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "# 复制插件\n"
        "if [ -d \"$SYSTEM_DATA_DIR/plugins\" ]; then\n"
        "    cp -rn \"$SYSTEM_DATA_DIR/plugins/\"* ~/.config/pnana/plugins/ 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "# 设置正确的权限\n"
        "chmod -R u+rwX ~/.config/pnana 2>/dev/null || true\n"
        "\n"
        "echo \"PNANA has been installed successfully!\"\n"
        "echo \"Run 'pnana' to start the editor.\"\n"
        "\n"
        "exit 0\n"
    )

    # RPM 安装后脚本
    file(WRITE ${CMAKE_BINARY_DIR}/rpm/postinstall
        "#!/bin/bash\n"
        "set -e\n"
        "\n"
        "# PNANA RPM 安装后配置脚本\n"
        "\n"
        "# 检测发行版\n"
        "if [ -f /etc/os-release ]; then\n"
        "    . /etc/os-release\n"
        "    DISTRO_ID=$ID\n"
        "else\n"
        "    DISTRO_ID=\"unknown\"\n"
        "fi\n"
        "\n"
        "# 根据发行版设置数据目录\n"
        "case $DISTRO_ID in\n"
        "    fedora|rhel|centos|suse|opensuse)\n"
        "        SYSTEM_DATA_DIR=\"/usr/share/pnana\"\n"
        "        ;;\n"
        "    *)\n"
        "        SYSTEM_DATA_DIR=\"/usr/local/share/pnana\"\n"
        "        ;;\n"
        "esac\n"
        "\n"
        "# 创建用户配置目录\n"
        "mkdir -p ~/.config/pnana 2>/dev/null || true\n"
        "mkdir -p ~/.config/pnana/plugins 2>/dev/null || true\n"
        "\n"
        "# 复制默认配置\n"
        "if [ -f \"$SYSTEM_DATA_DIR/default_config.json\" ]; then\n"
        "    cp -n \"$SYSTEM_DATA_DIR/default_config.json\" ~/.config/pnana/config.json 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "# 复制插件\n"
        "if [ -d \"$SYSTEM_DATA_DIR/plugins\" ]; then\n"
        "    cp -rn \"$SYSTEM_DATA_DIR/plugins/\"* ~/.config/pnana/plugins/ 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "# 设置正确的权限\n"
        "chmod -R u+rwX ~/.config/pnana 2>/dev/null || true\n"
        "\n"
        "echo \"PNANA has been installed successfully!\"\n"
        "echo \"Run 'pnana' to start the editor.\"\n"
        "\n"
        "exit 0\n"
    )

    # Pacman 安装后脚本 (用于Arch Linux)
    file(WRITE ${CMAKE_BINARY_DIR}/pacman/postinstall
        "#!/bin/bash\n"
        "set -e\n"
        "\n"
        "# PNANA Pacman 安装后配置脚本\n"
        "\n"
        "# 创建用户配置目录\n"
        "mkdir -p ~/.config/pnana 2>/dev/null || true\n"
        "mkdir -p ~/.config/pnana/plugins 2>/dev/null || true\n"
        "\n"
        "# 复制默认配置\n"
        "if [ -f /usr/share/pnana/default_config.json ]; then\n"
        "    cp -n /usr/share/pnana/default_config.json ~/.config/pnana/config.json 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "# 复制插件\n"
        "if [ -d /usr/share/pnana/plugins ]; then\n"
        "    cp -rn /usr/share/pnana/plugins/* ~/.config/pnana/plugins/ 2>/dev/null || true\n"
        "fi\n"
        "\n"
        "# 设置正确的权限\n"
        "chmod -R u+rwX ~/.config/pnana 2>/dev/null || true\n"
        "\n"
        "echo \"PNANA has been installed successfully!\"\n"
        "echo \"Run 'pnana' to start the editor.\"\n"
        "\n"
        "exit 0\n"
    )

    # 设置脚本权限
    file(CHMOD ${CMAKE_BINARY_DIR}/debian/postinst
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
    )
    file(CHMOD ${CMAKE_BINARY_DIR}/rpm/postinstall
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
    )
    file(CHMOD ${CMAKE_BINARY_DIR}/pacman/postinstall
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
    )
endif()

# ============================================================================
# 打包目标已在主CMakeLists.txt中定义
# ============================================================================

# 显示支持的包格式
message(STATUS "Package generators: ${CPACK_GENERATOR}")
message(STATUS "Package name: ${CPACK_PACKAGE_FILE_NAME}")
message(STATUS "Package version: ${CPACK_PACKAGE_VERSION}")

include(CPack)
