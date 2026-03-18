# Configure libvterm detection and flags

if(BUILD_LIBVTERM)
    message(STATUS "libvterm enabled - checking for vterm...")

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        message(STATUS "Checking for libvterm with pkg-config...")
        pkg_check_modules(LIBVTERM vterm QUIET)
    endif()

    # 如果 pkg-config 没找到，尝试 find_library / find_path
    if(NOT LIBVTERM_FOUND)
        message(STATUS "pkg-config didn't find libvterm, trying find_library...")
        find_library(LIBVTERM_LIBRARY
            NAMES vterm
            PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
        )
        find_path(LIBVTERM_INCLUDE_DIR
            NAMES vterm.h
            PATHS /usr/include /usr/local/include
        )
        if(LIBVTERM_LIBRARY AND LIBVTERM_INCLUDE_DIR)
            set(LIBVTERM_FOUND TRUE)
            set(LIBVTERM_LIBRARIES ${LIBVTERM_LIBRARY})
            set(LIBVTERM_INCLUDE_DIRS ${LIBVTERM_INCLUDE_DIR})
            message(STATUS "  Found libvterm library: ${LIBVTERM_LIBRARY}")
            message(STATUS "  Found libvterm includes: ${LIBVTERM_INCLUDE_DIR}")
        else()
            if(NOT LIBVTERM_LIBRARY)
                message(STATUS "  libvterm library not found")
            endif()
            if(NOT LIBVTERM_INCLUDE_DIR)
                message(STATUS "  libvterm include directory not found")
            endif()
        endif()
    endif()

    if(LIBVTERM_FOUND)
        set(BUILD_LIBVTERM_SUPPORT ON)
        message(STATUS "✓ libvterm found - full terminal emulation enabled")
        message(STATUS "  Libraries: ${LIBVTERM_LIBRARIES}")
        message(STATUS "  Include dirs: ${LIBVTERM_INCLUDE_DIRS}")
    else()
        set(BUILD_LIBVTERM_SUPPORT OFF)
        message(FATAL_ERROR "✗ libvterm not found but BUILD_LIBVTERM is enabled")
        message(STATUS "  Please install libvterm-dev using your system package manager")
        message(STATUS "  After installing, reconfigure: rm -rf build && mkdir build && cd build && cmake -DBUILD_LIBVTERM=ON ..")
    endif()
else()
    set(BUILD_LIBVTERM_SUPPORT OFF)
    message(STATUS "libvterm disabled (use -DBUILD_LIBVTERM=ON to enable)")
    message(STATUS "  Requires: libvterm dev package — install via your package manager")
endif()
