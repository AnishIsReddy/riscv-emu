cmake_minimum_required(VERSION 4.2)
project(riscv_emu)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    message(FATAL_ERROR
            "GCC >= 16 required for C++26 reflection (found ${CMAKE_CXX_COMPILER_VERSION})")
endif ()

# Everything except main.cpp, built once and shared by both executables.
add_library(riscv_emu_core STATIC
        src/machine/machine.cpp
        src/cpu/hart.cpp src/cpu/csr/csr_file.cpp src/cpu/decode.cpp src/cpu/execute.cpp
        src/mem/bus.cpp src/mem/device_map.cpp
        src/mem/devices/ram.cpp src/mem/devices/clint.cpp
        src/mem/devices/shutdown_reg.cpp
        src/mem/devices/shutdown_reg.h)
target_include_directories(riscv_emu_core PUBLIC src)
target_compile_options(riscv_emu_core PRIVATE -Wall -Wextra -Werror=return-type -Werror=switch)

add_executable(riscv_emu src/main.cpp)
target_link_libraries(riscv_emu PRIVATE riscv_emu_core)
target_compile_options(riscv_emu PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
target_link_options(riscv_emu PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)

enable_testing()

add_executable(test_execute test/isa/test_execute.cpp)
target_link_libraries(test_execute PRIVATE riscv_emu_core)
target_compile_options(test_execute PRIVATE -freflection)

# --- CTest discovery: ask the built binary for its tests -------------------
# The harness self-registers tests and reports them via --list; after every
# build we regenerate a file of add_test() calls, which CTest sources at test
# time (TEST_INCLUDE_FILES). Adding a TEST(...) in C++ needs no CMake re-run.
set(test_execute_discovered ${CMAKE_CURRENT_BINARY_DIR}/test/isa/discover_tests.cmake)

# Must exist before the first build so ctest doesn't error on the include.
if (NOT EXISTS ${test_execute_discovered})
    file(WRITE ${test_execute_discovered} "")
endif ()

add_custom_command(TARGET test_execute POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        -D TEST_EXECUTABLE=$<TARGET_FILE:test_execute>
        -D OUT_FILE=${test_execute_discovered}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/test/isa/discover_tests.cmake
        COMMENT "Discovering tests in test_execute"
        VERBATIM)

set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${test_execute_discovered})
