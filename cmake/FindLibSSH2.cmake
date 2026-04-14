# FindLibSSH2.cmake
# 查找 libssh2 库的 CMake 配置脚本
# 
# 用法:
#   include(cmake/FindLibSSH2.cmake)
#
# 输出变量:
#   LIBSSH2_FOUND        - 是否找到 libssh2
#   LIBSSH2_LIBRARIES    - libssh2 库文件
#   LIBSSH2_INCLUDE_DIRS - libssh2 头文件目录
#   LIBSSH2_LIBRARY_DIRS - libssh2 库目录
#   LIBSSH2_VERSION      - libssh2 版本

include(FindPackageHandleStandardArgs)

# 尝试使用 pkg-config 查找
if(NOT LIBSSH2_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PKG_LIBSSH2 QUIET libssh2)
        if(PKG_LIBSSH2_FOUND)
            set(LIBSSH2_FOUND TRUE)
            set(LIBSSH2_LIBRARIES ${PKG_LIBSSH2_LIBRARIES})
            set(LIBSSH2_INCLUDE_DIRS ${PKG_LIBSSH2_INCLUDE_DIRS})
            set(LIBSSH2_LIBRARY_DIRS ${PKG_LIBSSH2_LIBRARY_DIRS})
            set(LIBSSH2_VERSION ${PKG_LIBSSH2_VERSION})
        endif()
    endif()
endif()

# 如果 pkg-config 未找到，尝试 find_package
if(NOT LIBSSH2_FOUND)
    find_package(LibSSH2 QUIET)
    if(LibSSH2_FOUND)
        set(LIBSSH2_FOUND TRUE)
        set(LIBSSH2_LIBRARIES ${LIBSSH2_LIBRARIES})
        set(LIBSSH2_INCLUDE_DIRS ${LIBSSH2_INCLUDE_DIRS})
        set(LIBSSH2_LIBRARY_DIRS ${LIBSSH2_LIBRARY_DIRS})
    endif()
endif()

# 如果还是未找到，尝试手动查找
if(NOT LIBSSH2_FOUND)
    # 查找头文件
    find_path(LIBSSH2_INCLUDE_DIR
        NAMES libssh2.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/local/include
            /sw/include
            ENV LIBSSH2_INCLUDE_DIR
        PATH_SUFFIXES libssh2
    )
    
    # 查找库文件
    find_library(LIBSSH2_LIBRARY
        NAMES ssh2 libssh2
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /sw/lib
            ENV LIBSSH2_LIBRARY_DIR
        PATH_SUFFIXES lib64 lib
    )
    
    if(LIBSSH2_INCLUDE_DIR AND LIBSSH2_LIBRARY)
        set(LIBSSH2_FOUND TRUE)
        set(LIBSSH2_LIBRARIES ${LIBSSH2_LIBRARY})
        set(LIBSSH2_INCLUDE_DIRS ${LIBSSH2_INCLUDE_DIR})
    endif()
endif()

# 处理结果
if(LIBSSH2_FOUND)
    # 确保库文件是列表形式
    if(NOT DEFINED LIBSSH2_LIBRARIES OR "${LIBSSH2_LIBRARIES}" STREQUAL "")
        # 库文件为空，不做处理
    elseif(NOT "${LIBSSH2_LIBRARIES}" MATCHES ";")
        # 如果不是列表（不包含分号分隔符），转换为列表
        set(LIBSSH2_LIBRARIES "${LIBSSH2_LIBRARIES}")
    endif()
endif()

# 使用标准宏处理 FOUND 变量
find_package_handle_standard_args(LibSSH2
    REQUIRED_VARS LIBSSH2_LIBRARIES
    VERSION_VAR LIBSSH2_VERSION
)

# 设置导入目标（如果尚未设置）
if(LIBSSH2_FOUND AND NOT TARGET LibSSH2::LibSSH2)
    add_library(LibSSH2::LibSSH2 UNKNOWN IMPORTED)
    set_target_properties(LibSSH2::LibSSH2 PROPERTIES
        IMPORTED_LOCATION "${LIBSSH2_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSSH2_INCLUDE_DIRS}"
    )
endif()
