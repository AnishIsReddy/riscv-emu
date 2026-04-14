//
// Created by anish on 4/12/2026.
//

#include "bus.h"
#include "ram.h"

using namespace riscv_emu;

bus::bus(std::unique_ptr<ram> mem_ptr)
{
    memory = std::move(mem_ptr);
}
