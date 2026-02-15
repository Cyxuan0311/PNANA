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

# AI客户端支持（手动启用）
option(BUILD_AI_CLIENT "Enable AI client support" OFF)

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

# 检查 nlohmann/json (用于 AI 配置和 LSP 支持)
set(NLOHMANN_JSON_HEADER "${CMAKE_SOURCE_DIR}/third-party/nlohmann/json.hpp")

# 检查 jsonrpccxx (本地第三方库)
set(JSONRPCCXX_DIR "${CMAKE_SOURCE_DIR}/third-party/JSON-RPC-CXX")
set(JSONRPCCXX_INCLUDE_DIR "${JSONRPCCXX_DIR}")

# 检查 nlohmann/json 是否存在
if(EXISTS "${NLOHMANN_JSON_HEADER}")
    set(NLOHMANN_JSON_FOUND ON)
    message(STATUS "nlohmann/json found at: ${CMAKE_SOURCE_DIR}/third-party/nlohmann/json.hpp")
else()
    set(NLOHMANN_JSON_FOUND OFF)
    message(WARNING "nlohmann/json not found at ${NLOHMANN_JSON_HEADER}")
    message(STATUS "  AI configuration features will be limited")
endif()

# AI客户端支持（手动启用）
if(BUILD_AI_CLIENT)
    message(STATUS "AI client support enabled - checking for libcurl...")

    find_package(CURL QUIET)
    if(CURL_FOUND)
        set(BUILD_AI_CLIENT_SUPPORT ON)
        message(STATUS "✓ libcurl found - AI client support enabled")
        message(STATUS "  Version: ${CURL_VERSION_STRING}")
    else()
        set(BUILD_AI_CLIENT_SUPPORT OFF)
        message(FATAL_ERROR "✗ libcurl not found but BUILD_AI_CLIENT is enabled")
        message(STATUS "  Install with: sudo apt install libcurl4-openssl-dev")
        message(STATUS "  Then run: rm -rf build && mkdir build && cd build && cmake -DBUILD_AI_CLIENT=ON ..")
    endif()
else()
    set(BUILD_AI_CLIENT_SUPPORT OFF)
    message(STATUS "AI client support disabled (use -DBUILD_AI_CLIENT=ON to enable)")
    message(STATUS "  Requires: sudo apt install libcurl4-openssl-dev")
endif()

# 如果找到本地的 nlohmann/json 和 jsonrpccxx，启用 LSP 支持
if(NLOHMANN_JSON_FOUND AND EXISTS "${JSONRPCCXX_INCLUDE_DIR}/jsonrpccxx/client.hpp")
    set(BUILD_LSP_SUPPORT ON)
    message(STATUS "jsonrpccxx found at: ${JSONRPCCXX_INCLUDE_DIR}")
    message(STATUS "LSP support enabled (using local third-party libraries)")
else()
    set(BUILD_LSP_SUPPORT OFF)
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

# 配置 iconv 库支持（编码转换）
# iconv 是可选的，如果找不到会使用内置的简化实现
# 注意：在 GNU/Linux 系统上，iconv 通常是 glibc 的一部分，可能不需要单独链接库

# 首先查找头文件
find_path(ICONV_INCLUDE_DIR
    NAMES iconv.h
    PATHS 
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
    NO_DEFAULT_PATH
)

# 如果没找到，尝试默认路径
if(NOT ICONV_INCLUDE_DIR)
    find_path(ICONV_INCLUDE_DIR NAMES iconv.h)
endif()

# 查找库文件（在某些系统上可能需要单独链接）
# 在 GNU/Linux 上，iconv 通常在 libc 中，不需要单独链接
# 但在 macOS 或其他系统上可能需要
find_library(ICONV_LIBRARY
    NAMES 
        iconv
        libiconv
    PATHS 
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib64
        /usr/local/lib
        /opt/local/lib
        /sw/lib
    NO_DEFAULT_PATH
)

# 如果没找到，尝试默认路径
if(NOT ICONV_LIBRARY)
    find_library(ICONV_LIBRARY NAMES iconv libiconv)
endif()

# 检查 iconv 是否可用（通过测试编译）
if(ICONV_INCLUDE_DIR)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
    if(ICONV_LIBRARY)
        set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
    endif()
    
    check_cxx_source_compiles("
        #include <iconv.h>
        int main() {
            iconv_t cd = iconv_open(\"UTF-8\", \"UTF-8\");
            if (cd != (iconv_t)(-1)) {
                iconv_close(cd);
                return 0;
            }
            return 1;
        }
    " ICONV_COMPILES)
    
    unset(CMAKE_REQUIRED_INCLUDES)
    unset(CMAKE_REQUIRED_LIBRARIES)
    
    if(ICONV_COMPILES)
        set(ICONV_FOUND TRUE)
        set(BUILD_ICONV_SUPPORT ON)
        message(STATUS "✓ iconv found - enhanced encoding conversion enabled")
        if(ICONV_LIBRARY)
            message(STATUS "  Library: ${ICONV_LIBRARY}")
        else()
            message(STATUS "  Library: (built into libc)")
        endif()
        message(STATUS "  Include dir: ${ICONV_INCLUDE_DIR}")
    else()
        set(ICONV_FOUND FALSE)
        set(BUILD_ICONV_SUPPORT OFF)
        message(STATUS "iconv header found but compilation test failed")
        message(STATUS "  Using built-in encoding conversion")
    endif()
else()
    set(ICONV_FOUND FALSE)
    set(BUILD_ICONV_SUPPORT OFF)
    message(STATUS "iconv not found - using built-in encoding conversion")
    message(STATUS "  For better encoding support, install: sudo apt install libc6-dev")
    message(STATUS "  (This is optional - built-in conversion will still work)")
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
    src/core/editor_commands.cpp
    src/core/editor_file_ops.cpp
    src/core/editor_cursor.cpp
    src/core/editor_edit.cpp
    src/core/editor_ui.cpp
    src/core/editor_word_highlight.cpp
    src/core/overlay_manager.cpp
    src/core/editor_input.cpp
    src/core/region_manager.cpp
    config/config_manager.cpp
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
    src/ui/ssh_dialog.cpp
    src/ui/ssh_transfer_dialog.cpp
    src/ui/ai_config_dialog.cpp
    src/ui/ai_assistant_panel.cpp
    src/ui/completion_popup.cpp
    src/ui/command_palette_ui.cpp
    src/ui/diagnostics_popup.cpp
    src/ui/symbol_navigation_popup.cpp
    src/ui/format_dialog.cpp
    src/ui/git_panel.cpp
    src/ui/todo_panel.cpp
    src/ui/package_manager_panel.cpp
    src/ui/package_detail_dialog.cpp
    src/ui/package_install_dialog.cpp
    src/ui/move_file_dialog.cpp
    src/ui/extract_dialog.cpp
    src/ui/extract_path_dialog.cpp
    src/ui/extract_progress_dialog.cpp
    # 工具模块
    src/utils/logger.cpp
    src/utils/text_analyzer.cpp
    src/utils/text_utils.cpp
    src/utils/clipboard.cpp
    src/utils/file_type_detector.cpp
    src/utils/file_type_icon_mapper.cpp
    src/utils/file_type_color_mapper.cpp
    src/utils/archive_validator.cpp
    src/utils/version_detector.cpp
    src/features/recent_files_manager.cpp
    src/features/tui_config_manager.cpp
    src/ui/recent_files_popup.cpp
    src/ui/tui_config_popup.cpp
    src/utils/assembly_analyzer.cpp
    # Markdown 解析库
    third-party/md4c/md4c.c
    # 功能模块
    src/features/search.cpp
    src/features/file_browser.cpp
    src/features/extract.cpp
    src/features/SyntaxHighlighter/syntax_highlighter.cpp
    src/features/command_palette.cpp
    src/features/todo/todo_manager.cpp
    src/features/vgit/git_manager.cpp
    src/features/package_manager/package_manager_registry.cpp
    src/features/package_manager/pip_manager.cpp
    src/features/package_manager/apt_manager.cpp
    src/features/package_manager/cargo_manager.cpp
    src/features/package_manager/npm_manager.cpp
    src/features/package_manager/yarn_manager.cpp
    src/features/package_manager/conda_manager.cpp
    src/features/package_manager/pacman_manager.cpp
    src/features/package_manager/yum_manager.cpp
    src/features/package_manager/brew_manager.cpp
    src/features/terminal/terminal.cpp
    src/features/terminal/terminal_color.cpp
    src/features/terminal/terminal_parser.cpp
    src/features/terminal/terminal_builtin.cpp
    src/features/terminal/terminal_shell.cpp
    src/features/terminal/terminal_utils.cpp
    src/features/terminal/terminal_completion.cpp
    src/features/split_view/split_view.cpp
    src/features/md_render/markdown_parser.cpp
    src/features/md_render/markdown_renderer.cpp
    src/features/encoding_converter.cpp
    src/features/cursor/cursor_renderer.cpp
    src/features/ai_config/ai_config.cpp
    $<$<BOOL:${BUILD_AI_CLIENT_SUPPORT}>:src/features/ai_client/ai_client.cpp>
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
    # 使用通配符自动添加所有 snippets 文件
    file(GLOB SNIPPET_FILES "${CMAKE_SOURCE_DIR}/src/features/lsp/snippets/*_snippets.cpp")
    list(APPEND SOURCES ${SNIPPET_FILES})
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
        src/plugins/path_validator.cpp
    )
    list(APPEND HEADERS
        include/pnana/ui/plugin_manager_dialog.h
        include/pnana/plugins/path_validator.h
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
    include/pnana/ui/ai_assistant_panel.h
    include/pnana/ui/ai_config_dialog.h
    include/pnana/ui/icons.h
    include/pnana/ui/completion_popup.h
    include/pnana/ui/command_palette_ui.h
    include/pnana/ui/symbol_navigation_popup.h
    include/pnana/ui/format_dialog.h
    include/pnana/ui/todo_panel.h
    include/pnana/ui/move_file_dialog.h
    # 功能模块头文件
    include/pnana/features/search.h
    include/pnana/features/file_browser.h
    include/pnana/features/SyntaxHighlighter/syntax_highlighter.h
    include/pnana/features/SyntaxHighlighter/makefile_syntax_constants.h
    include/pnana/features/command_palette.h
    include/pnana/features/todo/todo_manager.h
    include/pnana/features/vgit/git_manager.h
    include/pnana/ui/git_panel.h
    include/pnana/features/terminal.h
    include/pnana/features/split_view/split_view.h
    include/pnana/features/markdown_parser.h
    include/pnana/features/md_render/markdown_renderer.h
    include/pnana/features/encoding_converter.h
    include/pnana/features/cursor/cursor_renderer.h
    include/pnana/features/ai_config/ai_config.h
    include/pnana/features/ssh/ssh_client.h
    include/pnana/features/ssh/ssh_client_cgo.h
    # 插件系统头文件
    include/pnana/plugins/lua_engine.h
    include/pnana/plugins/plugin_manager.h
    include/pnana/plugins/lua_api.h
    # 工具模块头文件
    include/pnana/utils/logger.h
    include/pnana/utils/text_analyzer.h
    include/pnana/utils/text_utils.h
    include/pnana/utils/clipboard.h
    include/pnana/utils/file_type_detector.h
    include/pnana/utils/file_type_color_mapper.h
    include/pnana/utils/archive_validator.h
    include/pnana/utils/version_detector.h
    include/pnana/utils/assembly_analyzer.h
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
    
    # Go SSH 模块源文件列表（重组后的结构）
    set(GO_SSH_SOURCES
        ${GO_MODULE_DIR}/ssh/types.go
        ${GO_MODULE_DIR}/ssh/pool.go
        ${GO_MODULE_DIR}/ssh/auth.go
        ${GO_MODULE_DIR}/ssh/utils.go
        ${GO_MODULE_DIR}/ssh/operations.go
        ${GO_MODULE_DIR}/ssh/client.go
    )
    
    # 构建 Go 模块为静态库
    # 注意：go.mod 在项目根目录，所有 Go 文件在 go/ssh/ 子目录
    add_custom_command(
        OUTPUT ${GO_MODULE_OUTPUT}
        COMMAND ${GO_EXECUTABLE} mod tidy
        COMMAND ${GO_EXECUTABLE} mod download
        COMMAND ${GO_EXECUTABLE} build -buildmode=c-archive -o ${GO_MODULE_OUTPUT} ./go/ssh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building Go SSH module"
        DEPENDS ${GO_SSH_SOURCES}
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

# AI客户端模块（如果启用）
if(BUILD_AI_CLIENT_SUPPORT)
    target_link_libraries(pnana PRIVATE CURL::libcurl)
    target_compile_definitions(pnana PRIVATE BUILD_AI_CLIENT_SUPPORT)
    message(STATUS "AI client support configured with libcurl")
endif()

# iconv 库（如果找到）
if(BUILD_ICONV_SUPPORT)
    # 添加头文件目录
    target_include_directories(pnana PRIVATE ${ICONV_INCLUDE_DIR})
    
    # 只有在需要单独链接库时才链接（GNU/Linux 上通常不需要）
    if(ICONV_LIBRARY)
        target_link_libraries(pnana PRIVATE ${ICONV_LIBRARY})
        message(STATUS "iconv library configured for enhanced encoding conversion")
        message(STATUS "  Linked library: ${ICONV_LIBRARY}")
    else()
        message(STATUS "iconv configured for enhanced encoding conversion")
        message(STATUS "  Using iconv from libc (no separate library needed)")
    endif()
    
    target_compile_definitions(pnana PRIVATE BUILD_ICONV_SUPPORT)
endif()

# 包含第三方库
target_include_directories(pnana PRIVATE ${CMAKE_SOURCE_DIR}/third-party)

# 包含 jsonrpccxx 和 nlohmann/json (如果启用 LSP 支持)
if(BUILD_LSP_SUPPORT)
    target_include_directories(pnana PRIVATE ${JSONRPCCXX_INCLUDE_DIR})
    target_compile_definitions(pnana PRIVATE BUILD_LSP_SUPPORT)
    message(STATUS "LSP support configured with local third-party libraries")
    message(STATUS "  jsonrpccxx: ${JSONRPCCXX_INCLUDE_DIR}")
endif()

if(NLOHMANN_JSON_FOUND)
    message(STATUS "nlohmann/json available for AI configuration")
else()
    message(WARNING "nlohmann/json not available - AI configuration will have limited functionality")
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
