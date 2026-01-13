cmake_minimum_required(VERSION 3.10)
project(pnana)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 查找FTXUI库
find_package(ftxui REQUIRED)

# 添加自定义 CMake 模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 检查原子操作支持（针对 RISC-V、ARM 等架构）
include(CheckAtomic)

# 图片预览支持（手动启用，使用FFmpeg）
option(BUILD_IMAGE_PREVIEW "Enable image preview support with FFmpeg" OFF)

# Tree-sitter语法高亮支持（手动启用）
option(BUILD_TREE_SITTER "Enable Tree-sitter syntax highlighting" OFF)

# Lua插件系统支持（手动启用）
option(BUILD_LUA "Enable Lua plugin system" OFF)

# Go SSH模块支持（手动启用）
option(BUILD_GO "Enable Go SSH module" OFF)

if(BUILD_IMAGE_PREVIEW)
    message(STATUS "Image preview support enabled - checking for FFmpeg...")

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBAV QUIET
            libavformat
            libavcodec
            libswscale
            libavutil
        )
    endif()

    if(LIBAV_FOUND)
        set(BUILD_IMAGE_PREVIEW_SUPPORT ON)
        message(STATUS "✓ FFmpeg found - image preview enabled")
        message(STATUS "  Libraries: ${LIBAV_LIBRARIES}")
        message(STATUS "  Include dirs: ${LIBAV_INCLUDE_DIRS}")
    else()
        set(BUILD_IMAGE_PREVIEW_SUPPORT OFF)
        message(FATAL_ERROR "✗ FFmpeg not found but BUILD_IMAGE_PREVIEW is enabled")
        message(STATUS "  Install with: sudo apt install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev")
        message(STATUS "  Then run: rm -rf build && mkdir build && cd build && cmake -DBUILD_IMAGE_PREVIEW=ON ..")
    endif()
else()
    set(BUILD_IMAGE_PREVIEW_SUPPORT OFF)
    message(STATUS "Image preview support disabled (use -DBUILD_IMAGE_PREVIEW=ON to enable)")
    message(STATUS "  Requires: sudo apt install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev")
endif()

# 检查 nlohmann/json (本地第三方库，用于 LSP 支持)
set(NLOHMANN_JSON_HEADER "${CMAKE_SOURCE_DIR}/third-party/nlohmann/json.hpp")

# 检查 jsonrpccxx (本地第三方库)
set(JSONRPCCXX_DIR "${CMAKE_SOURCE_DIR}/third-party/JSON-RPC-CXX")
set(JSONRPCCXX_INCLUDE_DIR "${JSONRPCCXX_DIR}")

# 如果找到本地的 nlohmann/json 和 jsonrpccxx，启用 LSP 支持
if(EXISTS "${NLOHMANN_JSON_HEADER}" AND EXISTS "${JSONRPCCXX_INCLUDE_DIR}/jsonrpccxx/client.hpp")
    set(BUILD_LSP_SUPPORT ON)
    set(NLOHMANN_JSON_FOUND ON)
    message(STATUS "nlohmann/json found at: ${CMAKE_SOURCE_DIR}/third-party/nlohmann/json.hpp")
    message(STATUS "jsonrpccxx found at: ${JSONRPCCXX_INCLUDE_DIR}")
    message(STATUS "LSP support enabled (using local third-party libraries)")
else()
    set(BUILD_LSP_SUPPORT OFF)
    set(NLOHMANN_JSON_FOUND OFF)
    if(NOT EXISTS "${NLOHMANN_JSON_HEADER}")
        message(WARNING "nlohmann/json not found at ${NLOHMANN_JSON_HEADER}, LSP support disabled")
        message(STATUS "  To enable LSP support, ensure nlohmann/json.hpp exists in third-party/nlohmann/")
    endif()
    if(NOT EXISTS "${JSONRPCCXX_INCLUDE_DIR}/jsonrpccxx/client.hpp")
        message(WARNING "jsonrpccxx not found at ${JSONRPCCXX_INCLUDE_DIR}, LSP support disabled")
    endif()
endif()

# 配置 Lua 插件系统支持
if(BUILD_LUA)
    message(STATUS "Lua plugin system enabled - checking for Lua...")

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        message(STATUS "Checking for Lua with pkg-config...")
        pkg_check_modules(LUA lua5.4 QUIET)
        if(NOT LUA_FOUND)
            message(STATUS "  lua5.4 not found, trying lua5.3...")
            pkg_check_modules(LUA lua5.3 QUIET)
        endif()
        if(NOT LUA_FOUND)
            message(STATUS "  lua5.3 not found, trying lua...")
            pkg_check_modules(LUA lua QUIET)
        endif()
    endif()

    # 如果 pkg-config 没找到，尝试 find_library
    if(NOT LUA_FOUND)
        message(STATUS "pkg-config didn't find Lua, trying find_library...")
        find_library(LUA_LIBRARY
            NAMES lua5.4 lua54 lua5.3 lua53 lua
            PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
        )
        find_path(LUA_INCLUDE_DIR
            NAMES lua.h lualib.h lauxlib.h
            PATHS /usr/include /usr/local/include
            PATH_SUFFIXES lua5.4 lua54 lua5.3 lua53 lua
        )
        if(LUA_LIBRARY AND LUA_INCLUDE_DIR)
            set(LUA_FOUND TRUE)
            set(LUA_LIBRARIES ${LUA_LIBRARY})
            set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
            message(STATUS "  Found Lua library: ${LUA_LIBRARY}")
            message(STATUS "  Found Lua includes: ${LUA_INCLUDE_DIR}")
        else()
            if(NOT LUA_LIBRARY)
                message(STATUS "  Lua library not found")
            endif()
            if(NOT LUA_INCLUDE_DIR)
                message(STATUS "  Lua include directory not found")
            endif()
        endif()
    endif()

    if(LUA_FOUND)
        set(BUILD_LUA_SUPPORT ON)
        message(STATUS "✓ Lua found - plugin system enabled")
        message(STATUS "  Libraries: ${LUA_LIBRARIES}")
        message(STATUS "  Include dirs: ${LUA_INCLUDE_DIRS}")
    else()
        set(BUILD_LUA_SUPPORT OFF)
        message(FATAL_ERROR "✗ Lua not found but BUILD_LUA is enabled")
        message(STATUS "  Install with: sudo apt install liblua5.4-dev (or liblua5.3-dev)")
        message(STATUS "  Then run: rm -rf build && mkdir build && cd build && cmake -DBUILD_LUA=ON ..")
    endif()
else()
    set(BUILD_LUA_SUPPORT OFF)
    message(STATUS "Lua plugin system disabled (use -DBUILD_LUA=ON to enable)")
    message(STATUS "  Requires: sudo apt install liblua5.4-dev (or liblua5.3-dev)")
endif()

# 配置 Tree-sitter（语法高亮）
if(BUILD_TREE_SITTER)
    message(STATUS "Tree-sitter syntax highlighting enabled - checking dependencies...")
    include(ConfigureTreeSitter)
    if(NOT BUILD_TREE_SITTER_SUPPORT)
        message(FATAL_ERROR "✗ Tree-sitter not found but BUILD_TREE_SITTER is enabled")
        message(STATUS "  Install with: sudo apt install libtree-sitter-dev")
        message(STATUS "  Or build from source: https://github.com/tree-sitter/tree-sitter")
        message(STATUS "  Then run: rm -rf build && mkdir build && cd build && cmake -DBUILD_TREE_SITTER=ON ..")
    endif()
else()
    set(BUILD_TREE_SITTER_SUPPORT OFF)
    message(STATUS "Tree-sitter syntax highlighting disabled (use -DBUILD_TREE_SITTER=ON to enable)")
    message(STATUS "  Requires: sudo apt install libtree-sitter-dev")
endif()

# 配置 Go SSH 模块支持
if(BUILD_GO)
    message(STATUS "Go SSH module enabled - checking for Go compiler...")

    find_program(GO_EXECUTABLE go)
    if(NOT GO_EXECUTABLE)
        message(FATAL_ERROR "✗ Go compiler not found but BUILD_GO is enabled")
        message(STATUS "  Install with: sudo apt install golang-go")
        message(STATUS "  Then run: rm -rf build && mkdir build && cd build && cmake -DBUILD_GO=ON ..")
    else()
        set(BUILD_GO_MODULE ON)
        message(STATUS "✓ Found Go: ${GO_EXECUTABLE}")
    endif()
else()
    set(BUILD_GO_MODULE OFF)
    message(STATUS "Go SSH module disabled (use -DBUILD_GO=ON to enable)")
    message(STATUS "  Requires: sudo apt install golang-go")
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/pnana)

# 源文件
set(SOURCES
    src/main.cpp
    src/core/document.cpp
    src/core/document_manager.cpp
    src/core/editor.cpp
    src/core/editor_file_ops.cpp
    src/core/editor_cursor.cpp
    src/core/editor_edit.cpp
    src/core/editor_ui.cpp
    src/core/editor_input.cpp
    src/core/region_manager.cpp
    src/core/config_manager.cpp
    # 新的输入处理模块
    src/input/event_parser.cpp
    src/input/key_action.cpp
    src/input/key_binding_manager.cpp
    src/input/action_executor.cpp
    # UI模块
    src/ui/theme.cpp
    src/ui/statusbar.cpp
    src/ui/helpbar.cpp
    src/ui/tabbar.cpp
    src/ui/welcome_screen.cpp
    src/ui/split_welcome_screen.cpp
    src/ui/new_file_prompt.cpp
    src/ui/theme_menu.cpp
    src/ui/create_folder_dialog.cpp
    src/ui/cursor_config_dialog.cpp
    src/ui/save_as_dialog.cpp
    src/ui/binary_file_view.cpp
    src/ui/encoding_dialog.cpp
    src/ui/help.cpp
    src/ui/dialog.cpp
    src/ui/file_picker.cpp
    src/ui/file_browser_view.cpp
    src/ui/split_dialog.cpp
    src/ui/terminal_ui.cpp
    src/ui/search_dialog.cpp
    src/ui/ssh_dialog.cpp
    src/ui/ssh_transfer_dialog.cpp
    src/ui/file_type_color_mapper.cpp
    src/ui/completion_popup.cpp
    src/ui/diagnostics_popup.cpp
    src/ui/format_dialog.cpp
    src/ui/git_panel.cpp
    # 工具模块
    src/utils/logger.cpp
    src/utils/text_analyzer.cpp
    src/utils/clipboard.cpp
    src/utils/file_type_detector.cpp
    src/utils/file_type_icon_mapper.cpp
    # Markdown 解析库
    third-party/md4c/md4c.c
    # 功能模块
    src/features/search.cpp
    src/features/file_browser.cpp
    src/features/SyntaxHighlighter/syntax_highlighter.cpp
    src/features/command_palette.cpp
    src/features/vgit/git_manager.cpp
    src/features/terminal/terminal.cpp
    src/features/terminal/terminal_color.cpp
    src/features/terminal/terminal_parser.cpp
    src/features/terminal/terminal_builtin.cpp
    src/features/terminal/terminal_shell.cpp
    src/features/terminal/terminal_utils.cpp
    src/features/terminal/terminal_completion.cpp
    src/features/split_view.cpp
    src/features/md_render/markdown_parser.cpp
    src/features/md_render/markdown_renderer.cpp
    src/features/encoding_converter.cpp
    src/features/ssh/ssh_client.cpp
    src/core/editor_ssh.cpp
    src/core/editor_lsp.cpp
    # 新的输入处理模块（解耦优化）
    src/core/input/base_region_handler.cpp
    src/core/input/input_router.cpp
    src/core/input/region_handlers/terminal_handler.cpp
    src/core/input/region_handlers/file_browser_handler.cpp
    src/core/input/region_handlers/git_panel_handler.cpp
    # 新的UI渲染模块（解耦优化）
    src/core/ui/base_region_renderer.cpp
    src/core/ui/border_manager.cpp
    src/core/ui/ui_router.cpp
    # 插件系统
    src/plugins/lua_engine.cpp
    src/plugins/plugin_manager.cpp
    src/plugins/lua_api.cpp
)

# Tree-sitter 模块（如果启用）
if(BUILD_TREE_SITTER_SUPPORT)
    list(APPEND SOURCES
        src/features/SyntaxHighlighter/syntax_highlighter_tree_sitter.cpp
    )
endif()

# LSP 模块（如果启用）
if(BUILD_LSP_SUPPORT)
    list(APPEND SOURCES
        src/features/lsp/lsp_stdio_connector.cpp
        src/features/lsp/lsp_client.cpp
        src/features/lsp/lsp_server_config.cpp
        src/features/lsp/lsp_server_manager.cpp
        src/features/lsp/lsp_request_manager.cpp
        src/features/lsp/lsp_worker_pool.cpp
        src/features/lsp/document_change_tracker.cpp
        src/features/lsp/document_change_tracker.cpp
        src/features/lsp/lsp_async_manager.cpp
        src/features/lsp/folding_manager.cpp
        src/features/lsp/lsp_completion_cache.cpp
        src/features/lsp/lsp_formatter.cpp
        src/features/lsp/snippet_manager.cpp
    )
endif()

# 图片预览模块（如果启用）
if(BUILD_IMAGE_PREVIEW_SUPPORT)
    list(APPEND SOURCES
        src/features/image_preview.cpp
    )
    list(APPEND HEADERS
        include/pnana/features/image_preview.h
    )
    message(STATUS "Image preview enabled with FFmpeg support")
endif()

# 插件系统模块（如果启用）
if(BUILD_LUA_SUPPORT)
    list(APPEND SOURCES
        src/ui/plugin_manager_dialog.cpp
    )
    list(APPEND HEADERS
        include/pnana/ui/plugin_manager_dialog.h
    )
    message(STATUS "Plugin system enabled with Lua support")
endif()

# 头文件
set(HEADERS
    include/pnana/core/document.h
    include/pnana/core/document_manager.h
    include/pnana/core/editor.h
    include/pnana/core/region_manager.h
    include/pnana/core/config_manager.h
    # 新的输入处理模块头文件
    include/pnana/input/event_parser.h
    include/pnana/input/key_action.h
    include/pnana/input/key_binding_manager.h
    include/pnana/input/action_executor.h
    # 新的输入处理模块（解耦优化）
    include/pnana/core/input/base_region_handler.h
    include/pnana/core/input/base_mode_handler.h
    include/pnana/core/input/input_router.h
    include/pnana/core/input/region_handlers/terminal_handler.h
    include/pnana/core/input/region_handlers/file_browser_handler.h
    include/pnana/core/input/region_handlers/git_panel_handler.h
    # 新的UI渲染模块（解耦优化）
    include/pnana/core/ui/base_region_renderer.h
    include/pnana/core/ui/border_manager.h
    include/pnana/core/ui/ui_router.h
    # UI模块头文件
    include/pnana/ui/theme.h
    include/pnana/ui/statusbar.h
    include/pnana/ui/helpbar.h
    include/pnana/ui/tabbar.h
    include/pnana/ui/help.h
    include/pnana/ui/welcome_screen.h
    include/pnana/ui/new_file_prompt.h
    include/pnana/ui/theme_menu.h
    include/pnana/ui/create_folder_dialog.h
    include/pnana/ui/save_as_dialog.h
    include/pnana/ui/cursor_config_dialog.h
    include/pnana/ui/binary_file_view.h
    include/pnana/ui/encoding_dialog.h
    include/pnana/ui/dialog.h
    include/pnana/ui/file_picker.h
    include/pnana/ui/file_browser_view.h
    include/pnana/ui/split_dialog.h
    include/pnana/ui/terminal_ui.h
    include/pnana/ui/ssh_dialog.h
    include/pnana/ui/icons.h
    include/pnana/ui/completion_popup.h
    include/pnana/ui/format_dialog.h
    # 功能模块头文件
    include/pnana/features/search.h
    include/pnana/features/file_browser.h
    include/pnana/features/SyntaxHighlighter/syntax_highlighter.h
    include/pnana/features/command_palette.h
    include/pnana/features/vgit/git_manager.h
    include/pnana/ui/git_panel.h
    include/pnana/features/terminal.h
    include/pnana/features/split_view.h
    include/pnana/features/markdown_parser.h
    include/pnana/features/md_render/markdown_renderer.h
    include/pnana/features/encoding_converter.h
    include/pnana/features/ssh/ssh_client.h
    include/pnana/features/ssh/ssh_client_cgo.h
    # 插件系统头文件
    include/pnana/plugins/lua_engine.h
    include/pnana/plugins/plugin_manager.h
    include/pnana/plugins/lua_api.h
    # 工具模块头文件
    include/pnana/utils/logger.h
    include/pnana/utils/text_analyzer.h
    include/pnana/utils/clipboard.h
    include/pnana/utils/file_type_detector.h
)

# Tree-sitter 模块头文件（如果启用）
if(BUILD_TREE_SITTER_SUPPORT)
    list(APPEND HEADERS
        include/pnana/features/SyntaxHighlighter/syntax_highlighter_tree_sitter.h
    )
endif()

# LSP 模块头文件（如果启用）
if(BUILD_LSP_SUPPORT)
    list(APPEND HEADERS
        include/pnana/features/lsp/lsp_stdio_connector.h
        include/pnana/features/lsp/lsp_client.h
        include/pnana/features/lsp/lsp_formatter.h
        include/pnana/features/lsp/snippet_manager.h
    )
endif()

# 创建可执行文件
add_executable(pnana ${SOURCES})

# 设置包含目录
target_include_directories(pnana PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pnana
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party
)

# jsonrpccxx 是现代的 C++ 库，完全兼容 C++17，无需特殊处理

# 构建 Go SSH 模块（如果 Go 可用）
if(BUILD_GO_MODULE)
    set(GO_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/go)
    set(GO_MODULE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libssh_client.a)
    
    # 构建 Go 模块为静态库
    # 注意：go.mod 在项目根目录，ssh_client.go 在 go/ 目录
    add_custom_command(
        OUTPUT ${GO_MODULE_OUTPUT}
        COMMAND ${GO_EXECUTABLE} mod tidy
        COMMAND ${GO_EXECUTABLE} mod download
        COMMAND ${GO_EXECUTABLE} build -buildmode=c-archive -o ${GO_MODULE_OUTPUT} ./go/ssh_client.go
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Go SSH module"
    )
    
    add_custom_target(go_ssh_module DEPENDS ${GO_MODULE_OUTPUT})
    add_dependencies(pnana go_ssh_module)
    
    # 链接 Go 模块
    target_link_libraries(pnana
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/libssh_client.a
        PRIVATE pthread
        PRIVATE dl
    )
    
    # 添加 Go 模块的头文件路径
    target_include_directories(pnana PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
    )
    
    # 添加编译定义，表示使用 Go 模块
    target_compile_definitions(pnana PRIVATE BUILD_GO_SSH_MODULE)
    
    message(STATUS "Go SSH module will be built and linked")
else()
    message(STATUS "Go SSH module will not be built. SSH functionality may be limited.")
endif()

# 链接FTXUI库
target_link_libraries(pnana
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)

# 检查并配置原子操作支持
check_and_configure_atomic(pnana)

# 只在找到 FFmpeg 时链接
if(BUILD_IMAGE_PREVIEW_SUPPORT)
    target_link_libraries(pnana PRIVATE ${LIBAV_LIBRARIES})
    target_include_directories(pnana PRIVATE ${LIBAV_INCLUDE_DIRS})
    target_compile_options(pnana PRIVATE ${LIBAV_CFLAGS_OTHER})
    target_compile_definitions(pnana PRIVATE BUILD_IMAGE_PREVIEW_SUPPORT)
endif()

# 包含 jsonrpccxx 和 nlohmann/json (如果启用 LSP 支持)
if(BUILD_LSP_SUPPORT)
    # 添加 jsonrpccxx 和 nlohmann/json 的包含目录
    target_include_directories(pnana PRIVATE
        ${JSONRPCCXX_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/third-party
    )

    target_compile_definitions(pnana PRIVATE BUILD_LSP_SUPPORT)
    message(STATUS "LSP support configured with local third-party libraries")
    message(STATUS "  jsonrpccxx: ${JSONRPCCXX_INCLUDE_DIR}")
    message(STATUS "  nlohmann/json: ${CMAKE_SOURCE_DIR}/third-party/nlohmann/json.hpp")
endif()

# 链接 Lua 库（如果启用插件系统）
if(BUILD_LUA_SUPPORT)
    target_include_directories(pnana PRIVATE ${LUA_INCLUDE_DIRS})
    target_link_libraries(pnana PRIVATE ${LUA_LIBRARIES})
    target_compile_definitions(pnana PRIVATE BUILD_LUA_SUPPORT)

    # 添加Lua API组件源文件
    target_sources(pnana PRIVATE
        src/plugins/editor_api.cpp
        src/plugins/file_api.cpp
        src/plugins/theme_api.cpp
        src/plugins/system_api.cpp
    )

    message(STATUS "Plugin system configured with Lua")
endif()

# 配置 Tree-sitter 库链接
include(ConfigureTreeSitterLinking)

# 安装配置在后面定义

# 设置版本信息
set_target_properties(pnana PROPERTIES
    SOVERSION 0.0.4
)

# 生成用户配置和插件安装脚本
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/install_user_config.sh.in"
    "${CMAKE_BINARY_DIR}/install_user_config.sh"
    @ONLY
)

# 设置脚本可执行权限
file(CHMOD "${CMAKE_BINARY_DIR}/install_user_config.sh"
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                 GROUP_READ GROUP_EXECUTE
                 WORLD_READ WORLD_EXECUTE
)

# 安装后运行脚本
install(CODE "
    message(STATUS \"Running user config and plugin installation script...\")
    execute_process(
        COMMAND bash \"${CMAKE_BINARY_DIR}/install_user_config.sh\"
        RESULT_VARIABLE INSTALL_RESULT
    )
    if(INSTALL_RESULT)
        message(WARNING \"User config/plugin installation script returned error: \${INSTALL_RESULT}\")
    else()
        message(STATUS \"User configuration and plugins installed successfully\")
    endif()
")

# 安装打包安装脚本
install(PROGRAMS ${CMAKE_BINARY_DIR}/package_install.sh
        DESTINATION share/pnana
        RENAME install.sh
        COMPONENT pnana)

# 插件现在通过统一的安装脚本处理

# 插件现在直接安装到用户目录，不需要安装后脚本

# ============================================================================
# 构建时文件复制和脚本生成配置
# ============================================================================

# 复制配置文件到构建目录
configure_file(
    ${CMAKE_SOURCE_DIR}/config/default_config.json
    ${CMAKE_BINARY_DIR}/default_config.json
    COPYONLY
)

# 复制插件文件到构建目录
file(GLOB_RECURSE PLUGIN_FILES "${CMAKE_SOURCE_DIR}/plugins/*")
foreach(PLUGIN_FILE ${PLUGIN_FILES})
    # 只复制文件，跳过目录
    if(NOT IS_DIRECTORY ${PLUGIN_FILE})
        get_filename_component(PLUGIN_FILENAME ${PLUGIN_FILE} NAME)
        # 获取相对路径
        file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/plugins" ${PLUGIN_FILE})
        get_filename_component(REL_DIR ${REL_PATH} DIRECTORY)
        if(REL_DIR)
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/${REL_DIR})
        endif()
        configure_file(
            ${PLUGIN_FILE}
            ${CMAKE_BINARY_DIR}/plugins/${REL_PATH}
            COPYONLY
        )
    endif()
endforeach()

# 生成安装脚本
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/install_script_template.sh.in
    ${CMAKE_BINARY_DIR}/install.sh
    @ONLY
)

# 为打包文件生成简化的安装脚本
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/package_install.sh.in
    ${CMAKE_BINARY_DIR}/package_install.sh
    @ONLY
)

# 设置安装脚本为可执行
file(CHMOD ${CMAKE_BINARY_DIR}/install.sh
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

# 设置打包安装脚本为可执行
file(CHMOD ${CMAKE_BINARY_DIR}/package_install.sh
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
)

# ============================================================================
# 安装配置
# ============================================================================

# 安装可执行文件到系统目录（仅在实际安装时）
install(TARGETS pnana
    RUNTIME DESTINATION bin
    COMPONENT pnana
)

# 安装默认配置文件到用户目录
install(FILES
    ${CMAKE_SOURCE_DIR}/config/default_config.json
    DESTINATION .config/pnana/
    COMPONENT config
    RENAME config.json
)

# 插件现在直接安装到用户目录

# 不再需要安装脚本

# 添加自定义安装目标
add_custom_target(install-user
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component pnana --prefix $ENV{HOME}/.local
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component config --prefix $ENV{HOME}/.config/pnana
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component plugins --prefix $ENV{HOME}/.config/pnana
    COMMENT "Installing pnana to user directories (~/.local and ~/.config/pnana)"
)

# Linux 包创建目标
if(UNIX AND NOT APPLE)
    add_custom_target(package-deb
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
        COMMAND ${CMAKE_COMMAND} -E echo "DEB package created: ${CMAKE_BINARY_DIR}/pnana-${CPACK_PACKAGE_VERSION}-amd64.deb"
        COMMENT "Creating DEB package for Debian/Ubuntu"
    )

    add_custom_target(package-rpm
        COMMAND ${CMAKE_COMMAND} -DCPACK_GENERATOR=RPM ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
        COMMAND ${CMAKE_COMMAND} -E echo "RPM package created: ${CMAKE_BINARY_DIR}/pnana-${CPACK_PACKAGE_VERSION}-x86_64.rpm"
        COMMENT "Creating RPM package for Red Hat/Fedora"
    )

    add_custom_target(package-arch
        COMMAND ${CMAKE_COMMAND} -DCPACK_GENERATOR=TXZ ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
        COMMAND ${CMAKE_COMMAND} -E echo "Arch Linux package created: ${CMAKE_BINARY_DIR}/pnana-${CPACK_PACKAGE_VERSION}-x86_64.tar.xz"
        COMMENT "Creating TXZ package for Arch Linux"
    )
endif()

# 构建完成提示已移除，避免CMake语法问题
# 用户可以通过运行 './install.sh --help' 查看安装选项

# ============================================================================
# 打包配置
# ============================================================================

# 包含打包配置文件
include(${CMAKE_SOURCE_DIR}/cmake/PackageConfig.cmake)
