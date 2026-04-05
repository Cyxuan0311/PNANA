# CMake toolchain file for ARMv7 cross-compilation

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

# 指定交叉编译器
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# 设置架构变量（使用 CACHE FORCE 确保覆盖）
set(ARCHITECTURE "armv7" CACHE STRING "Target architecture" FORCE)
set(ARCH_CFLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "Architecture flags" FORCE)

# 设置 pthread 参数，避免检测失败
set(THREADS_PTHREAD_ARG "2" CACHE INTERNAL "pthread works" FORCE)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Additional flags for ARMv7 with NEON
set(CMAKE_C_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "" FORCE)
