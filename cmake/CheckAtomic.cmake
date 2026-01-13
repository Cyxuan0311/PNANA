# CheckAtomic.cmake - 检查原子操作支持并配置 libatomic 链接
#
# 此模块用于检测目标架构是否需要 libatomic 库来支持原子操作
# 主要针对 RISC-V 64/32 位、ARM 32 位、PowerPC 等缺乏硬件原子操作支持的架构
# 特别针对 PowerPC 架构优化 64 位原子操作检测

include(CheckCXXSourceCompiles)

# 测试代码：检查原子操作是否可用（包括 64 位）
set(ATOMIC_TEST_CODE "
#include <atomic>
#include <cstdint>
int main() {
    std::atomic<uint8_t> a{0};
    std::atomic<uint16_t> b{0};
    std::atomic<uint32_t> c{0};
    std::atomic<uint64_t> d{0};
    a.store(1);
    b.store(1);
    c.store(1);
    d.store(1);
    return a.load() + b.load() + c.load() + d.load();
}
")

# 检查是否需要 libatomic
function(check_atomic_support)
    # 获取目标架构信息
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "powerpc|ppc")
        set(IS_POWERPC TRUE)
    else()
        set(IS_POWERPC FALSE)
    endif()

    # 首先尝试编译测试代码
    check_cxx_source_compiles("${ATOMIC_TEST_CODE}" ATOMIC_WORKS_WITHOUT_LIBATOMIC)

    if(NOT ATOMIC_WORKS_WITHOUT_LIBATOMIC)
        # 原子操作测试失败，尝试查找 libatomic 库
        find_library(ATOMIC_LIBRARY atomic)

        if(ATOMIC_LIBRARY)
            set(NEED_LIBATOMIC TRUE CACHE INTERNAL "Whether libatomic is needed")
            set(ATOMIC_LIBRARY_PATH ${ATOMIC_LIBRARY} CACHE INTERNAL "Path to libatomic library")
            if(IS_POWERPC)
                message(STATUS "Found libatomic: ${ATOMIC_LIBRARY} (required for 64-bit atomic operations on PowerPC)")
            else()
                message(STATUS "Found libatomic: ${ATOMIC_LIBRARY} (required for atomic operations on this architecture)")
            endif()
        else()
            if(IS_POWERPC)
                message(WARNING "libatomic not found. 64-bit atomic operations may not work correctly on PowerPC.")
            else()
                message(WARNING "libatomic not found. Atomic operations may not work correctly on this architecture.")
            endif()
            message(STATUS "  Consider installing libatomic (usually part of gcc runtime libraries)")
            message(STATUS "  On Ubuntu/Debian: sudo apt install libatomic1")
            message(STATUS "  On CentOS/RHEL: sudo yum install libatomic")
            message(STATUS "  On Arch Linux: sudo pacman -S gcc-libs")
            set(NEED_LIBATOMIC FALSE CACHE INTERNAL "Whether libatomic is needed")
        endif()
    else()
        # 即使原生支持，也要在 PowerPC 上检查 64 位原子操作的特殊情况
        if(IS_POWERPC)
            # PowerPC 32 位可能需要 libatomic 来支持 64 位原子操作
            # 即使小尺寸原子操作原生支持，我们仍需检查 64 位的情况
            set(ATOMIC_64_TEST_CODE "
#include <atomic>
#include <cstdint>
int main() {
    std::atomic<uint64_t> d{0};
    d.store(1ULL);
    return d.load() == 1ULL ? 0 : 1;
}
")
            check_cxx_source_compiles("${ATOMIC_64_TEST_CODE}" ATOMIC_64_WORKS_WITHOUT_LIBATOMIC)

            if(NOT ATOMIC_64_WORKS_WITHOUT_LIBATOMIC)
                find_library(ATOMIC_LIBRARY atomic)
                if(ATOMIC_LIBRARY)
                    set(NEED_LIBATOMIC TRUE CACHE INTERNAL "Whether libatomic is needed")
                    set(ATOMIC_LIBRARY_PATH ${ATOMIC_LIBRARY} CACHE INTERNAL "Path to libatomic library")
                    message(STATUS "Found libatomic: ${ATOMIC_LIBRARY} (required for 64-bit atomic operations on PowerPC)")
                else()
                    message(WARNING "libatomic not found. 64-bit atomic operations may not work correctly on PowerPC.")
                    set(NEED_LIBATOMIC FALSE CACHE INTERNAL "Whether libatomic is needed")
                endif()
            else()
                message(STATUS "Native atomic operations available (no libatomic needed)")
                set(NEED_LIBATOMIC FALSE CACHE INTERNAL "Whether libatomic is needed")
            endif()
        else()
            message(STATUS "Native atomic operations available (no libatomic needed)")
            set(NEED_LIBATOMIC FALSE CACHE INTERNAL "Whether libatomic is needed")
        endif()
    endif()
endfunction()

# 配置目标的原子操作链接
function(configure_atomic_linking target_name)
    if(NEED_LIBATOMIC)
        target_link_libraries(${target_name} PRIVATE ${ATOMIC_LIBRARY_PATH})
        message(STATUS "Linking ${target_name} with libatomic for atomic operation support")
    endif()
endfunction()

# 自动检查并配置（便捷函数）
function(check_and_configure_atomic target_name)
    check_atomic_support()
    configure_atomic_linking(${target_name})
endfunction()
