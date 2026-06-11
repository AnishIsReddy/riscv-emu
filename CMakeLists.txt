cmake_minimum_required(VERSION 3.30)
project(riscv_emu)

set(CMAKE_CXX_STANDARD 23)


add_executable(riscv_emu src/main.cpp
        src/hart.cpp
        src/hart.h
        src/decode.cpp
        src/decode.h
        src/defs.h
        src/ram.cpp
        src/ram.h
        src/bus.cpp
        src/bus.h
        src/execute.cpp
        src/execute.h
        src/machine.cpp
        src/machine.h
)

target_compile_options(riscv_emu PRIVATE -Wall -Wextra -Werror=return-type -Werror=switch)
#target_compile_options(riscv_emu PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
#target_link_options(riscv_emu PRIVATE $<$<CONFIG:Debug>:-fsanitize=address,undefined>)
