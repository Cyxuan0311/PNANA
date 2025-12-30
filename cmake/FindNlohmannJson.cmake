# FindNlohmannJson.cmake
# 查找 nlohmann/json 库的 CMake 模块
#
# nlohmann/json 是一个 header-only 库，只需要查找头文件
#
# 使用方式:
#   find_package(NlohmannJson [REQUIRED] [QUIET])
#
# 定义变量:
#   NlohmannJson_FOUND          - 如果找到库则为 TRUE
#   NlohmannJson_INCLUDE_DIRS  - 包含目录
#   NlohmannJson_VERSION       - 版本号（如果可用）

include(FindPackageHandleStandardArgs)

# 首先尝试使用 pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_NLOHMANN_JSON QUIET nlohmann_json3)
    if(PC_NLOHMANN_JSON_FOUND)
        set(NlohmannJson_INCLUDE_DIRS ${PC_NLOHMANN_JSON_INCLUDE_DIRS})
        set(NlohmannJson_VERSION ${PC_NLOHMANN_JSON_VERSION})
    endif()
endif()

# 如果 pkg-config 找不到，尝试手动查找
if(NOT PC_NLOHMANN_JSON_FOUND)
    # 查找头文件目录
    # nlohmann/json 的头文件在 nlohmann/json.hpp
    find_path(NlohmannJson_INCLUDE_DIR
        NAMES nlohmann/json.hpp
        PATHS
            /usr/include
            /usr/local/include
            /opt/local/include
            ${CMAKE_SOURCE_DIR}/third-party
            ${CMAKE_SOURCE_DIR}/third-party/nlohmann
            ~/.local/include
            ${CMAKE_INSTALL_PREFIX}/include
        PATH_SUFFIXES
            include
    )
    
    if(NlohmannJson_INCLUDE_DIR)
        set(NlohmannJson_INCLUDE_DIRS ${NlohmannJson_INCLUDE_DIR})
        
        # 尝试从头文件中提取版本号
        if(EXISTS "${NlohmannJson_INCLUDE_DIR}/nlohmann/json.hpp")
            file(READ "${NlohmannJson_INCLUDE_DIR}/nlohmann/json.hpp" JSON_HPP_CONTENT)
            string(REGEX MATCH "#define NLOHMANN_JSON_VERSION_MAJOR[ ]+([0-9]+)" _ "${JSON_HPP_CONTENT}")
            if(CMAKE_MATCH_1)
                set(NLOHMANN_JSON_VERSION_MAJOR ${CMAKE_MATCH_1})
            endif()
            string(REGEX MATCH "#define NLOHMANN_JSON_VERSION_MINOR[ ]+([0-9]+)" _ "${JSON_HPP_CONTENT}")
            if(CMAKE_MATCH_1)
                set(NLOHMANN_JSON_VERSION_MINOR ${CMAKE_MATCH_1})
            endif()
            string(REGEX MATCH "#define NLOHMANN_JSON_VERSION_PATCH[ ]+([0-9]+)" _ "${JSON_HPP_CONTENT}")
            if(CMAKE_MATCH_1)
                set(NLOHMANN_JSON_VERSION_PATCH ${CMAKE_MATCH_1})
            endif()
            if(NLOHMANN_JSON_VERSION_MAJOR AND NLOHMANN_JSON_VERSION_MINOR AND NLOHMANN_JSON_VERSION_PATCH)
                set(NlohmannJson_VERSION "${NLOHMANN_JSON_VERSION_MAJOR}.${NLOHMANN_JSON_VERSION_MINOR}.${NLOHMANN_JSON_VERSION_PATCH}")
            endif()
        endif()
    endif()
endif()

# 验证必需的组件是否找到
find_package_handle_standard_args(NlohmannJson
    FOUND_VAR NlohmannJson_FOUND
    REQUIRED_VARS
        NlohmannJson_INCLUDE_DIRS
    VERSION_VAR NlohmannJson_VERSION
)

# 如果找到，创建导入目标（如果 CMake 版本支持）
if(NlohmannJson_FOUND AND NOT TARGET NlohmannJson::json)
    # 创建接口库（header-only 库）
    add_library(NlohmannJson::json INTERFACE IMPORTED)
    
    set_target_properties(NlohmannJson::json PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${NlohmannJson_INCLUDE_DIRS}"
    )
    
    # 打印找到的信息
    if(NOT NlohmannJson_FIND_QUIETLY)
        message(STATUS "Found nlohmann/json:")
        message(STATUS "  Include dirs: ${NlohmannJson_INCLUDE_DIRS}")
        if(NlohmannJson_VERSION)
            message(STATUS "  Version: ${NlohmannJson_VERSION}")
        endif()
    endif()
endif()

# 标记为高级变量（在 CMake GUI 中隐藏）
mark_as_advanced(
    NlohmannJson_INCLUDE_DIR
    NlohmannJson_INCLUDE_DIRS
)

