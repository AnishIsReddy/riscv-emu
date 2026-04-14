//
// Created by anish on 4/12/2026.
//

#include "bus.h"
#include "ram.h"

using namespace riscv_emu;

bus::bus(ram* mem_ptr)
{
    memory = mem_ptr;
}

uint64_t bus::read_memory(uint64_t addr, uint8_t size)
{
    return 0;
}

void bus::write_memory(uint64_t addr, uint64_t data, uint8_t size)
{

}