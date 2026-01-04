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

# 图片预览支持（手动启用，使用FFmpeg）
option(BUILD_IMAGE_PREVIEW "Enable image preview support with FFmpeg" OFF)

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

# 查找 nlohmann/json (用于 LSP 支持)
find_package(NlohmannJson QUIET)

# 检查 jsonrpccxx (本地第三方库)
set(JSONRPCCXX_DIR "${CMAKE_SOURCE_DIR}/third-party/JSON-RPC-CXX")
set(JSONRPCCXX_INCLUDE_DIR "${JSONRPCCXX_DIR}")

# 如果找到 nlohmann/json 和 jsonrpccxx，启用 LSP 支持
if(NlohmannJson_FOUND AND EXISTS "${JSONRPCCXX_INCLUDE_DIR}/jsonrpccxx/client.hpp")
    set(BUILD_LSP_SUPPORT ON)
    message(STATUS "nlohmann/json found at: ${NlohmannJson_INCLUDE_DIRS}")
    if(NlohmannJson_VERSION)
        message(STATUS "  Version: ${NlohmannJson_VERSION}")
    endif()
    message(STATUS "jsonrpccxx found at: ${JSONRPCCXX_INCLUDE_DIR}")
    message(STATUS "LSP support enabled")
else()
    set(BUILD_LSP_SUPPORT OFF)
    if(NOT NlohmannJson_FOUND)
        message(WARNING "nlohmann/json not found, LSP support disabled")
        message(STATUS "  Install with: sudo apt install nlohmann-json3-dev")
    endif()
    if(NOT EXISTS "${JSONRPCCXX_INCLUDE_DIR}/jsonrpccxx/client.hpp")
        message(WARNING "jsonrpccxx not found at ${JSONRPCCXX_INCLUDE_DIR}, LSP support disabled")
    endif()
endif()

# 查找 Lua 库
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
    message(WARNING "✗ Lua not found, plugin system will be disabled")
    message(STATUS "  Install with: sudo apt install liblua5.4-dev (or liblua5.3-dev)")
    message(STATUS "  After installation, run: rm -rf build && mkdir build && cd build && cmake ..")
endif()

# 配置 Tree-sitter（语法高亮）
include(ConfigureTreeSitter)

# 查找 Go 编译器
find_program(GO_EXECUTABLE go)
if(NOT GO_EXECUTABLE)
    message(WARNING "Go compiler not found. SSH module will use fallback implementation.")
    set(BUILD_GO_MODULE OFF)
else()
    set(BUILD_GO_MODULE ON)
    message(STATUS "Found Go: ${GO_EXECUTABLE}")
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
    src/ui/completion_popup.cpp
    # 工具模块
    src/utils/logger.cpp
    src/utils/text_analyzer.cpp
    src/utils/clipboard.cpp
    src/utils/file_type_detector.cpp
    # 功能模块
    src/features/search.cpp
    src/features/file_browser.cpp
    src/features/SyntaxHighlighter/syntax_highlighter.cpp
    src/features/command_palette.cpp
    src/features/terminal/terminal.cpp
    src/features/terminal/terminal_parser.cpp
    src/features/terminal/terminal_builtin.cpp
    src/features/terminal/terminal_shell.cpp
    src/features/terminal/terminal_utils.cpp
    src/features/terminal/terminal_completion.cpp
    src/features/split_view.cpp
    src/features/encoding_converter.cpp
    src/features/ssh/ssh_client.cpp
    src/core/editor_ssh.cpp
    src/core/editor_lsp.cpp
    # 新的输入处理模块（解耦优化）
    src/core/input/base_region_handler.cpp
    src/core/input/input_router.cpp
    src/core/input/region_handlers/terminal_handler.cpp
    src/core/input/region_handlers/file_browser_handler.cpp
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
        src/features/lsp/document_change_tracker.cpp
        src/features/lsp/lsp_async_manager.cpp
        src/features/lsp/lsp_completion_cache.cpp
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
    # 功能模块头文件
    include/pnana/features/search.h
    include/pnana/features/file_browser.h
    include/pnana/features/SyntaxHighlighter/syntax_highlighter.h
    include/pnana/features/command_palette.h
    include/pnana/features/terminal.h
    include/pnana/features/split_view.h
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
    )
endif()

# 创建可执行文件
add_executable(pnana ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(pnana PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pnana
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
    message(WARNING "Go SSH module will not be built. SSH functionality may be limited.")
endif()

# 链接FTXUI库
target_link_libraries(pnana
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)

# 只在找到 FFmpeg 时链接
if(BUILD_IMAGE_PREVIEW_SUPPORT)
    target_link_libraries(pnana PRIVATE ${LIBAV_LIBRARIES})
    target_include_directories(pnana PRIVATE ${LIBAV_INCLUDE_DIRS})
    target_compile_options(pnana PRIVATE ${LIBAV_CFLAGS_OTHER})
    target_compile_definitions(pnana PRIVATE BUILD_IMAGE_PREVIEW_SUPPORT)
endif()

# 链接 jsonrpccxx 和 nlohmann/json (如果启用 LSP 支持)
if(BUILD_LSP_SUPPORT)
    # 添加 jsonrpccxx 的包含目录
    target_include_directories(pnana PRIVATE 
        ${JSONRPCCXX_INCLUDE_DIR}
    )
    
    # 链接 nlohmann/json（如果使用导入目标）
    if(TARGET NlohmannJson::json)
        target_link_libraries(pnana PRIVATE NlohmannJson::json)
    else()
        # 回退到直接包含目录
        target_include_directories(pnana PRIVATE ${NlohmannJson_INCLUDE_DIRS})
    endif()
    
    target_compile_definitions(pnana PRIVATE BUILD_LSP_SUPPORT)
    message(STATUS "LSP support configured with jsonrpccxx and nlohmann/json")
endif()

# 链接 Lua 库（如果启用插件系统）
if(BUILD_LUA_SUPPORT)
    target_include_directories(pnana PRIVATE ${LUA_INCLUDE_DIRS})
    target_link_libraries(pnana PRIVATE ${LUA_LIBRARIES})
    target_compile_definitions(pnana PRIVATE BUILD_LUA_SUPPORT)
    message(STATUS "Plugin system configured with Lua")
endif()

# 配置 Tree-sitter 库链接
include(ConfigureTreeSitterLinking)

# 安装
install(TARGETS pnana RUNTIME DESTINATION bin)

# 设置版本信息
set_target_properties(pnana PROPERTIES
    SOVERSION 0.0.3
)

# 可选：添加配置文件安装
install(FILES config/default_config.json
        DESTINATION share/pnana
        OPTIONAL)

# 安装插件到系统目录（作为模板）
if(EXISTS "${CMAKE_SOURCE_DIR}/plugins")
    install(DIRECTORY plugins/
            DESTINATION share/pnana/plugins
            PATTERN ".git" EXCLUDE
            PATTERN ".gitignore" EXCLUDE
            PATTERN "*.md" EXCLUDE
            PATTERN "CMakeLists.txt" EXCLUDE
    )
    message(STATUS "Plugins will be installed to: share/pnana/plugins")
endif()

# 创建安装后脚本，将插件复制到用户主目录
if(UNIX AND EXISTS "${CMAKE_SOURCE_DIR}/plugins")
    # 使用 install(CODE ...) 执行 CMake 代码来运行 shell 脚本
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/install_plugins.sh.in"
        "${CMAKE_BINARY_DIR}/install_plugins.sh"
        @ONLY
    )
    
    # 设置脚本可执行权限
    file(CHMOD "${CMAKE_BINARY_DIR}/install_plugins.sh"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE
    )
    
    # 使用 install(CODE ...) 执行 shell 脚本
    install(CODE "
        message(STATUS \"Running plugin installation script...\")
        execute_process(
            COMMAND bash \"${CMAKE_BINARY_DIR}/install_plugins.sh\"
            RESULT_VARIABLE INSTALL_RESULT
        )
        if(INSTALL_RESULT)
            message(WARNING \"Plugin installation script returned error: \${INSTALL_RESULT}\")
        else()
            message(STATUS \"Plugins installed to user directory successfully\")
        endif()
    ")
    message(STATUS "Plugin installation script configured")
    message(STATUS "  Plugins will be copied to: ~/.config/pnana/plugins")
endif()