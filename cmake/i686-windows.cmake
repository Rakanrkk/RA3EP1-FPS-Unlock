# Toolchain for the i686 Windows payloads and loader. Works with LLVM-MinGW
# or MinGW-w64 GCC on Linux, macOS, or Windows hosts.
#
#   cmake -S . -B build/i686 -G Ninja \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/i686-windows.cmake \
#       [-DRA3EP1FPSUNLOCK_CROSS_PREFIX=i686-w64-mingw32-] \
#       [-DRA3EP1FPSUNLOCK_C_COMPILER=/path/to/i686-w64-mingw32-clang]
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(RA3EP1FPSUNLOCK_CROSS_PREFIX "i686-w64-mingw32-" CACHE STRING
    "Prefix of the i686 Windows toolchain commands")
set(RA3EP1FPSUNLOCK_C_COMPILER "" CACHE FILEPATH
    "Explicit i686 Windows C compiler path (overrides the prefix search)")

if(RA3EP1FPSUNLOCK_C_COMPILER)
    set(CMAKE_C_COMPILER "${RA3EP1FPSUNLOCK_C_COMPILER}")
else()
    find_program(RA3EP1FPSUNLOCK_COMPILER_PATH
        NAMES "${RA3EP1FPSUNLOCK_CROSS_PREFIX}clang" "${RA3EP1FPSUNLOCK_CROSS_PREFIX}gcc"
        DOC "i686 Windows C compiler")
    set(CMAKE_C_COMPILER "${RA3EP1FPSUNLOCK_COMPILER_PATH}")
endif()

get_filename_component(_ra3ep1fpsunlock_bindir "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(CMAKE_AR
    NAMES "${RA3EP1FPSUNLOCK_CROSS_PREFIX}ar" "${RA3EP1FPSUNLOCK_CROSS_PREFIX}llvm-ar"
          "${RA3EP1FPSUNLOCK_CROSS_PREFIX}gcc-ar" llvm-ar
    HINTS "${_ra3ep1fpsunlock_bindir}" NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB
    NAMES "${RA3EP1FPSUNLOCK_CROSS_PREFIX}ranlib" "${RA3EP1FPSUNLOCK_CROSS_PREFIX}llvm-ranlib"
          llvm-ranlib ranlib
    HINTS "${_ra3ep1fpsunlock_bindir}" NO_DEFAULT_PATH)
unset(_ra3ep1fpsunlock_bindir)
