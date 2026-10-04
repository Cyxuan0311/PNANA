# FindTreeSitter.cmake
# 查找 Tree-sitter 库的 CMake 模块
#
# Tree-sitter 是一个增量解析系统，用于语法高亮和代码分析
#
# 使用方式:
#   find_package(TreeSitter [REQUIRED] [QUIET])
#
# 定义变量:
#   TreeSitter_FOUND          - 如果找到库则为 TRUE
#   TreeSitter_LIBRARIES      - 库文件路径
#   TreeSitter_INCLUDE_DIRS   - 包含目录
#   TreeSitter_VERSION        - 版本号（如果可用）

include(FindPackageHandleStandardArgs)

# 首先尝试使用 pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_TREE_SITTER QUIET tree-sitter)
    if(PC_TREE_SITTER_FOUND)
        # pkg-config 返回的可能是库名，需要转换为完整路径
        # 先尝试使用 pkg-config 提供的库路径
        if(PC_TREE_SITTER_LIBRARY_DIRS)
            find_library(TreeSitter_LIBRARY
                NAMES tree-sitter libtree-sitter
                PATHS ${PC_TREE_SITTER_LIBRARY_DIRS}
                NO_DEFAULT_PATH
            )
        endif()
        # 如果没找到，尝试在标准路径查找
        if(NOT TreeSitter_LIBRARY)
            find_library(TreeSitter_LIBRARY
                NAMES tree-sitter libtree-sitter
                PATHS
                    /usr/lib
                    /usr/local/lib
                    /usr/lib/x86_64-linux-gnu
            )
        endif()
        # 如果找到了库文件，使用完整路径；否则使用 pkg-config 提供的库名
        if(TreeSitter_LIBRARY)
            set(TreeSitter_LIBRARIES ${TreeSitter_LIBRARY})
        else()
            set(TreeSitter_LIBRARIES ${PC_TREE_SITTER_LIBRARIES})
        endif()
        set(TreeSitter_INCLUDE_DIRS ${PC_TREE_SITTER_INCLUDE_DIRS})
        # pkg-config omits -I/usr/include by default, so the list can be empty
        # even though the headers are installed. Fall back to a real search.
        if(NOT TreeSitter_INCLUDE_DIRS)
            find_path(TreeSitter_INCLUDE_DIR NAMES tree_sitter/api.h)
            set(TreeSitter_INCLUDE_DIRS ${TreeSitter_INCLUDE_DIR})
        endif()
        set(TreeSitter_VERSION ${PC_TREE_SITTER_VERSION})
    endif()
endif()

# 如果 pkg-config 找不到，尝试手动查找
if(NOT PC_TREE_SITTER_FOUND)
    # 查找库文件
    find_library(TreeSitter_LIBRARY
        NAMES tree-sitter libtree-sitter
        PATHS
            /usr/lib
            /usr/local/lib
            /usr/lib/x86_64-linux-gnu
            /opt/local/lib
            ${CMAKE_SOURCE_DIR}/third-party
            ~/.local/lib
            ${CMAKE_INSTALL_PREFIX}/lib
        PATH_SUFFIXES
            lib
            lib64
    )
    
    # 查找头文件
    find_path(TreeSitter_INCLUDE_DIR
        NAMES tree_sitter/api.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/local/include
            ${CMAKE_SOURCE_DIR}/third-party
            ~/.local/include
            ${CMAKE_INSTALL_PREFIX}/include
        PATH_SUFFIXES
            include
    )
    
    if(TreeSitter_LIBRARY AND TreeSitter_INCLUDE_DIR)
        set(TreeSitter_LIBRARIES ${TreeSitter_LIBRARY})
        set(TreeSitter_INCLUDE_DIRS ${TreeSitter_INCLUDE_DIR})
    endif()
endif()

# 验证必需的组件是否找到
find_package_handle_standard_args(TreeSitter
    FOUND_VAR TreeSitter_FOUND
    REQUIRED_VARS
        TreeSitter_LIBRARIES
        TreeSitter_INCLUDE_DIRS
)

# 如果找到，尝试创建导入目标（需要完整路径）
if(TreeSitter_FOUND AND NOT TARGET TreeSitter::TreeSitter)
    # 如果 TreeSitter_LIBRARIES 是库名而不是完整路径，尝试查找完整路径
    if(NOT TreeSitter_LIBRARY)
        # 检查是否是完整路径
        if(NOT EXISTS "${TreeSitter_LIBRARIES}" AND NOT IS_ABSOLUTE "${TreeSitter_LIBRARIES}")
            # 是库名，尝试查找完整路径
            find_library(TreeSitter_LIBRARY
                NAMES tree-sitter libtree-sitter
                PATHS
                    /usr/lib
                    /usr/local/lib
                    /usr/lib/x86_64-linux-gnu
                    ${PC_TREE_SITTER_LIBRARY_DIRS}
            )
            if(TreeSitter_LIBRARY)
                set(TreeSitter_LIBRARIES ${TreeSitter_LIBRARY})
            endif()
        endif()
    else()
        set(TreeSitter_LIBRARIES ${TreeSitter_LIBRARY})
    endif()
    
    # 只有在有完整路径时才创建导入目标
    if(EXISTS "${TreeSitter_LIBRARIES}" OR IS_ABSOLUTE "${TreeSitter_LIBRARIES}")
        add_library(TreeSitter::TreeSitter UNKNOWN IMPORTED)
        set_target_properties(TreeSitter::TreeSitter PROPERTIES
            IMPORTED_LOCATION "${TreeSitter_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${TreeSitter_INCLUDE_DIRS}"
        )
    endif()
    
    # 打印找到的信息
    if(NOT TreeSitter_FIND_QUIETLY)
        message(STATUS "Found Tree-sitter:")
        message(STATUS "  Libraries: ${TreeSitter_LIBRARIES}")
        message(STATUS "  Include dirs: ${TreeSitter_INCLUDE_DIRS}")
        if(TreeSitter_VERSION)
            message(STATUS "  Version: ${TreeSitter_VERSION}")
        endif()
    endif()
endif()

# 标记为高级变量（在 CMake GUI 中隐藏）
mark_as_advanced(
    TreeSitter_LIBRARY
    TreeSitter_LIBRARIES
    TreeSitter_INCLUDE_DIR
    TreeSitter_INCLUDE_DIRS
)

