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
