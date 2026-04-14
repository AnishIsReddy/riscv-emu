cmake_minimum_required(VERSION 4.0)
project(riscv_emu)

set(CMAKE_CXX_STANDARD 20)

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
