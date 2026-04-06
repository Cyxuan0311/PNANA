# CMake toolchain file for RISC-V 64 cross-compilation

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR riscv64)

# 指定交叉编译器
set(CMAKE_C_COMPILER riscv64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER riscv64-linux-gnu-g++)

# 🔥 核心修复：强制只使用交叉编译头文件，禁止搜索 x86 路径
set(CMAKE_FIND_ROOT_PATH /usr/riscv64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 🔥 强制清除系统默认的 x86 包含路径
unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
unset(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)

# 设置架构变量
set(ARCHITECTURE "riscv64" CACHE STRING "Target architecture" FORCE)
set(ARCH_CFLAGS "-march=rv64gc -mabi=lp64d" CACHE STRING "Architecture flags" FORCE)

# 设置 pthread 参数，避免检测失败
set(THREADS_PTHREAD_ARG "2" CACHE INTERNAL "pthread works" FORCE)

# Additional flags for RISC-V 64
set(CMAKE_C_FLAGS "-march=rv64gc -mabi=lp64d" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-march=rv64gc -mabi=lp64d" CACHE STRING "" FORCE)
