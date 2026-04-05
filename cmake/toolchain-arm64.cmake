# CMake toolchain file for ARM64 cross-compilation
# 必须在 project() 之前设置 CMAKE_SYSTEM_PROCESSOR

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 提前设置架构变量，这样 CMakeLists.txt 中的架构检测可以正确工作
set(ARCHITECTURE "arm64")
set(ARCH_CFLAGS "-march=armv8-a+crc")

# Specify the cross compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# 不设置 sysroot，让编译器使用默认路径
# Ubuntu 的交叉编译包会自动安装在正确的位置

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Additional flags for ARM64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crc" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+crc" CACHE STRING "" FORCE)
