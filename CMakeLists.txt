cmake_minimum_required(VERSION 3.15)

# RA3EP1 FPS Unlock - portable fixed-rate timing payloads (stable profile 60/120)
# and their Windows loader for Command & Conquer Red Alert 3: Uprising (EP1 1.0).
# One source tree, one payload source; the rate is a compile-time definition.
project(RA3EP1FPSUnlock VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Mirror the accepted research build: -O2 without NDEBUG (the tests rely on
# assert). Release is the default single-config build type.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_FLAGS_RELEASE "-O2")

set(RA3EP1FPSUNLOCK_WARNING_FLAGS -Wall -Wextra -Werror)

# Stable releases build 60 and 120 only. 30 and 90 remain research-only
# compile-time switches of the same single source tree.
set(RA3EP1FPSUNLOCK_RATES "60;120" CACHE STRING
    "Payload rates to build (subset of 30;60;90;120; stable profile is 60;120)")
foreach(rate IN LISTS RA3EP1FPSUNLOCK_RATES)
    if(NOT rate MATCHES "^(30|60|90|120)$")
        message(FATAL_ERROR
            "RA3EP1FPSUNLOCK_RATES entries must be 30, 60, 90 or 120; got '${rate}'")
    endif()
endforeach()

include(CTest)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Host builds run the pure contract tests only. Configure with
    # -DCMAKE_TOOLCHAIN_FILE=cmake/i686-windows.cmake for the payload DLLs
    # and loader.
    if(BUILD_TESTING)
        foreach(rate IN ITEMS 30 60 90 120)
            foreach(test_name IN ITEMS particle_clock schedule120 sway_compat)
                set(target host_${test_name}_${rate})
                add_executable(${target} tests/test_${test_name}.c)
                target_compile_definitions(${target} PRIVATE RA3_TARGET_FPS=${rate})
                target_compile_options(${target} PRIVATE ${RA3EP1FPSUNLOCK_WARNING_FLAGS})
                target_include_directories(${target} PRIVATE src)
                add_test(NAME ${target} COMMAND ${target})
            endforeach()
        endforeach()
        find_package(Python3 COMPONENTS Interpreter)
        if(Python3_FOUND)
            add_test(NAME contract_check
                     COMMAND ${CMAKE_COMMAND} -E env
                             CC=${CMAKE_C_COMPILER}
                             ${Python3_EXECUTABLE}
                             ${CMAKE_CURRENT_SOURCE_DIR}/tests/contract_check.py
                             --output build/ctest-contract-check)
        endif()
    endif()
    return()
endif()

# Windows i686 targets (native MinGW or cross).
# PE binaries pin a zero link timestamp so equal inputs give equal bytes.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(RA3EP1FPSUNLOCK_TIMESTAMP_FLAGS -Wl,--no-insert-timestamp)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(RA3EP1FPSUNLOCK_TIMESTAMP_FLAGS "-Wl,/timestamp:0")
else()
    set(RA3EP1FPSUNLOCK_TIMESTAMP_FLAGS "")
endif()

foreach(rate IN LISTS RA3EP1FPSUNLOCK_RATES)
    add_library(fps${rate} SHARED src/payload.c)
    target_compile_definitions(fps${rate} PRIVATE RA3_TARGET_FPS=${rate})
    target_compile_options(fps${rate} PRIVATE ${RA3EP1FPSUNLOCK_WARNING_FLAGS})
    target_include_directories(fps${rate} PRIVATE src)
    set_target_properties(fps${rate} PROPERTIES PREFIX "")
    target_link_options(fps${rate} PRIVATE -static-libgcc ${RA3EP1FPSUNLOCK_TIMESTAMP_FLAGS})
endforeach()

add_executable(RA3EP1FPSUnlock src/loader.c)
target_compile_options(RA3EP1FPSUnlock PRIVATE ${RA3EP1FPSUNLOCK_WARNING_FLAGS})
target_link_options(RA3EP1FPSUnlock PRIVATE -municode -static-libgcc ${RA3EP1FPSUNLOCK_TIMESTAMP_FLAGS})
target_link_libraries(RA3EP1FPSUnlock PRIVATE advapi32)

# Actual i686 suspension-bridge exercise (research-verified, exit 42, at
# 60/120). Builds wherever an i686 Windows toolchain is active; runs in-place
# only when the build itself targets this machine (no Wine in CI).
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_executable(test_sway_runtime tests/test_sway_runtime.c)
    target_compile_options(test_sway_runtime PRIVATE ${RA3EP1FPSUNLOCK_WARNING_FLAGS})
    target_include_directories(test_sway_runtime PRIVATE src)
    target_link_options(test_sway_runtime PRIVATE ${RA3EP1FPSUNLOCK_TIMESTAMP_FLAGS})
    if(NOT CMAKE_CROSSCOMPILING AND BUILD_TESTING)
        # The research fixture returns 42 on success; gate on its PASS banner.
        add_test(NAME test_sway_runtime COMMAND test_sway_runtime)
        set_tests_properties(test_sway_runtime PROPERTIES
            PASS_REGULAR_EXPRESSION "PASS: actual i686 suspension")
    endif()
endif()
