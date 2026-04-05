# CheckAtomic.cmake - 检查原子操作支持并配置 libatomic 链接
# 专为 RISC-V64、ARM、PowerPC 优化，自动链接 -latomic
include(CheckCXXSourceCompiles)

# 全覆盖测试：8/16/32/64 位原子操作
set(ATOMIC_TEST_CODE "
#include <atomic>
#include <cstdint>
int main() {
    std::atomic<uint8_t>  a{0};
    std::atomic<uint16_t> b{0};
    std::atomic<uint32_t> c{0};
    std::atomic<uint64_t> d{0};
    a = 1; b = 1; c = 1; d = 1;
    return 0;
}
")

# 检查是否需要 libatomic
function(check_atomic_support)
    unset(NEED_LIBATOMIC CACHE)
    unset(ATOMIC_LIBRARY_PATH CACHE)

    # 先测试无库情况
    check_cxx_source_compiles("${ATOMIC_TEST_CODE}" ATOMIC_WORKS_WITHOUT_LIBATOMIC)

    if(ATOMIC_WORKS_WITHOUT_LIBATOMIC)
        set(NEED_LIBATOMIC FALSE CACHE INTERNAL "No libatomic needed")
        message(STATUS "Atomic: Native supported (no libatomic)")
        return()
    endif()

    # 失败 → 尝试链接 libatomic
    find_library(ATOMIC_LIBRARY NAMES atomic libatomic)
    if(ATOMIC_LIBRARY)
        set(NEED_LIBATOMIC TRUE CACHE INTERNAL "libatomic required")
        set(ATOMIC_LIBRARY_PATH ${ATOMIC_LIBRARY} CACHE INTERNAL "Path to libatomic")
        message(STATUS "Atomic: using ${ATOMIC_LIBRARY} (required by RISC-V/ARM)")
    else()
        set(NEED_LIBATOMIC FALSE CACHE INTERNAL "libatomic not found")
        message(WARNING "Atomic: libatomic not found! May fail on RISC-V/ARM.")
    endif()
endfunction()

# 给目标链接 libatomic
function(configure_atomic_linking target_name)
    if(NEED_LIBATOMIC)
        target_link_libraries(${target_name} PRIVATE ${ATOMIC_LIBRARY_PATH})
        message(STATUS "Atomic: linked ${target_name} with libatomic")
    endif()
endfunction()

# 一键调用
function(check_and_configure_atomic target_name)
    check_atomic_support()
    configure_atomic_linking(${target_name})
endfunction()
