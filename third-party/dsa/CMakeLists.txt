# dsa - 终端图片查看器
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
set(TARGET_NAME dsa)

# 添加可执行文件
add_executable(${TARGET_NAME} dsa.c)

# 链接数学库
target_link_libraries(${TARGET_NAME} m)

# 设置编译选项
target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -std=c99)

# 安装目标
install(TARGETS ${TARGET_NAME} DESTINATION bin)
