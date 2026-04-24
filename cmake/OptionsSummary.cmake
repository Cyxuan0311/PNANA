# OptionsSummary.cmake - 生成启用的 CMake 选项的图表显示

function(generate_options_summary)
    # 收集所有选项及其状态
    set(OPTIONS_LIST
        "BUILD_LIBVTERM"
        "BUILD_IMAGE_PREVIEW"
        "BUILD_AI_CLIENT"
        "BUILD_LSP_SUPPORT"
        "BUILD_LUA"
        "BUILD_ICONV_SUPPORT"
        "BUILD_TREE_SITTER"
        "BUILD_SSH_MODE"
        "NLOHMANN_JSON_FOUND"
    )
    
    # 选项的友好名称
    set(OPTIONS_NAMES
        "libvterm"
        "Image Preview"
        "AI Client"
        "LSP Support"
        "Lua Plugins"
        "Iconv Support"
        "Tree-sitter"
        "SSH Mode"
        "nlohmann/json"
    )
    
    # 生成图表
    message(STATUS "")
    message(STATUS "CMake Option        | Feature        | Status    ")
    message(STATUS "--------------------|----------------|----------")
    
    # 遍历所有选项
    list(LENGTH OPTIONS_LIST OPTIONS_COUNT)
    math(EXPR OPTIONS_COUNT "${OPTIONS_COUNT} - 1")
    
    foreach(INDEX RANGE ${OPTIONS_COUNT})
        list(GET OPTIONS_LIST ${INDEX} OPTION)
        list(GET OPTIONS_NAMES ${INDEX} NAME)
        
        # 检查选项状态
        if(OPTION STREQUAL "BUILD_SSH_MODE")
            if(BUILD_SSH_MODE STREQUAL "NONE" OR NOT BUILD_SSH_MODE)
                set(STATUS "✗ DISABLED")
            else()
                set(STATUS "✓ ENABLED (${BUILD_SSH_MODE})")
            endif()
        elseif(${OPTION})
            set(STATUS "✓ ENABLED")
        else()
            set(STATUS "✗ DISABLED")
        endif()
        
        # 格式化输出 - 使用固定宽度确保对齐
        # 精确计算每个列的宽度
        set(OPTION_WIDTH 19)
        set(FEATURE_WIDTH 14)
        set(STATUS_WIDTH 10)
        
        # 填充 CMake Option 列
        string(LENGTH "${OPTION}" OPTION_LENGTH)
        set(OPTION_PADDED "${OPTION}")
        if(OPTION_LENGTH LESS OPTION_WIDTH)
            math(EXPR PAD_LENGTH "${OPTION_WIDTH} - ${OPTION_LENGTH}")
            string(REPEAT " " ${PAD_LENGTH} PAD_SPACES)
            set(OPTION_PADDED "${OPTION}${PAD_SPACES}")
        endif()
        
        # 填充 Feature 列
        string(LENGTH "${NAME}" NAME_LENGTH)
        set(NAME_PADDED "${NAME}")
        if(NAME_LENGTH LESS FEATURE_WIDTH)
            math(EXPR PAD_LENGTH "${FEATURE_WIDTH} - ${NAME_LENGTH}")
            string(REPEAT " " ${PAD_LENGTH} PAD_SPACES)
            set(NAME_PADDED "${NAME}${PAD_SPACES}")
        endif()
        
        # 填充 Status 列
        string(LENGTH "${STATUS}" STATUS_LENGTH)
        set(STATUS_PADDED "${STATUS}")
        if(STATUS_LENGTH LESS STATUS_WIDTH)
            math(EXPR PAD_LENGTH "${STATUS_WIDTH} - ${STATUS_LENGTH}")
            string(REPEAT " " ${PAD_LENGTH} PAD_SPACES)
            set(STATUS_PADDED "${STATUS}${PAD_SPACES}")
        endif()
        
        message(STATUS "${OPTION_PADDED} | ${NAME_PADDED} | ${STATUS_PADDED}")
    endforeach()
    
    message(STATUS "")
    
    # 显示构建类型
    if(NOT CMAKE_BUILD_TYPE)
        set(BUILD_TYPE "None")
    else()
        set(BUILD_TYPE "${CMAKE_BUILD_TYPE}")
    endif()
    message(STATUS "Build Type: ${BUILD_TYPE}")
    message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "")
endfunction()
