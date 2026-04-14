//
// Created by anish on 4/12/2026.
//

#include "bus.h"
#include "ram.h"

using namespace riscv_emu;

bus::bus(ram* mem_ptr) : main_memory(mem_ptr) {}

uint64_t bus::read_memory(const uint64_t addr, const uint8_t size) const
{
    return main_memory->read_addr(addr, size);
}

void bus::write_memory(const uint64_t addr, const uint64_t data, const uint8_t size) const
{
    main_memory->write_addr(addr, data, size);
}
