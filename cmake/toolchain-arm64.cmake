# CMake toolchain file for ARM64 cross-compilation

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 指定交叉编译器
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# 设置架构变量（使用 CACHE FORCE 确保覆盖）
set(ARCHITECTURE "arm64" CACHE STRING "Target architecture" FORCE)
set(ARCH_CFLAGS "-march=armv8-a+crc" CACHE STRING "Architecture flags" FORCE)

# 设置 pthread 参数，避免检测失败
set(THREADS_PTHREAD_ARG "2" CACHE INTERNAL "pthread works" FORCE)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Additional flags for ARM64
set(CMAKE_C_FLAGS "-march=armv8-a+crc" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-march=armv8-a+crc" CACHE STRING "" FORCE)
