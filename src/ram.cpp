//
// Created by anish on 4/12/2026.
//

#include <cstring>
#include "ram.h"

using namespace riscv_emu;

void ram::load(const uint8_t* data, const std::size_t size) const
{
    memcpy(memory.get(), data, size);
}

uint64_t ram::read_addr(const uint64_t addr, const uint8_t size) const
{
    uint64_t out = 0;
    memcpy(&out, memory.get() + addr, size);
    return out;
}

void ram::write_addr(const uint64_t addr, const uint64_t data, const uint8_t size) const
{
    memcpy(memory.get() + addr, &data, size);
}
