cmake_minimum_required(VERSION 3.10)
project(pnana VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 查找FTXUI库
find_package(ftxui REQUIRED)

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
    src/ui/help.cpp
    src/ui/dialog.cpp
    src/ui/file_picker.cpp
    # 功能模块
    src/features/search.cpp
    src/features/file_browser.cpp
    src/features/syntax_highlighter.cpp
    src/features/command_palette.cpp
    src/features/terminal.cpp
)

# 头文件
set(HEADERS
    include/pnana/core/document.h
    include/pnana/core/document_manager.h
    include/pnana/core/editor.h
    include/pnana/core/region_manager.h
    # 新的输入处理模块头文件
    include/pnana/input/event_parser.h
    include/pnana/input/key_action.h
    include/pnana/input/key_binding_manager.h
    include/pnana/input/action_executor.h
    # UI模块头文件
    include/pnana/ui/theme.h
    include/pnana/ui/statusbar.h
    include/pnana/ui/helpbar.h
    include/pnana/ui/tabbar.h
    include/pnana/ui/help.h
    include/pnana/ui/dialog.h
    include/pnana/ui/file_picker.h
    include/pnana/ui/icons.h
    # 功能模块头文件
    include/pnana/features/search.h
    include/pnana/features/file_browser.h
    include/pnana/features/syntax_highlighter.h
    include/pnana/features/command_palette.h
    include/pnana/features/terminal.h
)

# 创建可执行文件
add_executable(pnana ${SOURCES} ${HEADERS})

# 设置包含目录
target_include_directories(pnana PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pnana
)

# 链接FTXUI库
target_link_libraries(pnana
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component
)

# 安装
install(TARGETS pnana RUNTIME DESTINATION bin)

# 设置版本信息
set_target_properties(pnana PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# 可选：添加配置文件安装
install(FILES config/default_config.json
        DESTINATION share/pnana
        OPTIONAL)

