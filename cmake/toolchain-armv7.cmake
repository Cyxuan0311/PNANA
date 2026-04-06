# CMake toolchain file for ARMv7 cross-compilation
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 指定交叉编译器
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# 🔥 修复：必须把 /usr/local 加进来！FTXUI 安装在这里！
set(CMAKE_FIND_ROOT_PATH 
    /usr/local 
    /usr/arm-linux-gnueabihf 
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 清除系统 x86 头文件干扰
unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)

# 架构设置
set(ARCHITECTURE "armv7" CACHE STRING "Target architecture" FORCE)
set(ARCH_CFLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "Architecture flags" FORCE)

# 线程库修复
set(THREADS_PTHREAD_ARG "2" CACHE INTERNAL "pthread works" FORCE)

# 编译选项
set(CMAKE_C_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "" FORCE)
