# CheckAtomic.cmake - 修复 RISC-V 原子操作检测
include(CheckCXXSourceCompiles)

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

function(check_atomic_support)
    unset(NEED_LIBATOMIC CACHE)
    unset(ATOMIC_LIBRARY_PATH CACHE)

    # ==============================================
    # 🔥 强制 RISC-V 必须使用 libatomic
    # ==============================================
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64" OR ARCHITECTURE STREQUAL "riscv64")
        find_library(ATOMIC_LIBRARY NAMES atomic libatomic)
        if(ATOMIC_LIBRARY)
            set(NEED_LIBATOMIC TRUE CACHE INTERNAL "RISC-V 必须使用 libatomic")
            set(ATOMIC_LIBRARY_PATH ${ATOMIC_LIBRARY} CACHE INTERNAL "Path to libatomic")
            message(STATUS "Atomic: RISC-V 强制启用 libatomic")
            return()
        endif()
    endif()

    # 其他架构正常检测
    check_cxx_source_compiles("${ATOMIC_TEST_CODE}" ATOMIC_WORKS_WITHOUT_LIBATOMIC)
    if(ATOMIC_WORKS_WITHOUT_LIBATOMIC)
        set(NEED_LIBATOMIC FALSE CACHE INTERNAL "No libatomic needed")
        message(STATUS "Atomic: Native supported (no libatomic)")
        return()
    endif()

    find_library(ATOMIC_LIBRARY NAMES atomic libatomic)
    if(ATOMIC_LIBRARY)
        set(NEED_LIBATOMIC TRUE CACHE INTERNAL "libatomic required")
        set(ATOMIC_LIBRARY_PATH ${ATOMIC_LIBRARY} CACHE INTERNAL "Path to libatomic")
        message(STATUS "Atomic: using ${ATOMIC_LIBRARY}")
    else()
        set(NEED_LIBATOMIC FALSE CACHE INTERNAL "libatomic not found")
        message(WARNING "Atomic: libatomic not found!")
    endif()
endfunction()

function(configure_atomic_linking target_name)
    if(NEED_LIBATOMIC)
        target_link_libraries(${target_name} PRIVATE ${ATOMIC_LIBRARY_PATH})
        message(STATUS "Atomic: linked ${target_name} with libatomic")
    endif()
endfunction()

function(check_and_configure_atomic target_name)
    check_atomic_support()
    configure_atomic_linking(${target_name})
endfunction()
