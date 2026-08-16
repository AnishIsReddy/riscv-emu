//
// Created by anish on 4/12/2026.
//

#include "ram.h"
#include <cstring>

#include <iomanip>

using namespace riscv_emu::mem_io;

void ram::load(const uint8_t* data, const std::size_t size) const
{
    memcpy(memory.get(), data, size);
}

uint64_t ram::read(const uint64_t offset, const uint8_t size) const
{
    uint64_t out = 0;
    memcpy(&out, memory.get() + offset, size);
    return out;
}

// ReSharper disable once CppMemberFunctionMayBeConst [modifies member memory]
void ram::write(const uint64_t offset, const uint64_t data, const uint8_t size)
{
    memcpy(memory.get() + offset, &data, size);
}

void ram::dump(std::ostream& os) const
{
    for (std::size_t i = 0; i < MEM_SIZE; i += 16) {
        bool all_zero = true;

        for (std::size_t j = i; j < i + 16 && j < MEM_SIZE; j++) {
            if (memory[j] != 0) {
                all_zero = false;
                break;
            }
        }

        if (all_zero)
            continue;

        os << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << i << ": ";

        for (std::size_t j = i; j < i + 16 && j < MEM_SIZE; j++) {
            os << std::uppercase << std::setw(2) << static_cast<unsigned>(memory[j]) << " ";
        }

        os << "\n";
    }
}
