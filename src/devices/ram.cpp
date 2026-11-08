//
// Created by anish on 4/12/2026.
//

#include <cstring>
#include <iomanip>
#include <stdexcept>

#include "common/utils.h"
#include "devices/ram.h"

using namespace riscv_emu::mem_io;

Ram::Ram(const uint64_t size) : BusDevice(size)
{
    memory = std::make_unique<uint8_t[]>(size);
}

void Ram::load(const uint8_t* data, const std::size_t size) const
{
    if (size > get_size()) {
        throw std::runtime_error("Tried to load a program larger than Ram size");
    }

    memcpy(memory.get(), data, size);
}

std::optional<uint64_t> Ram::read_internal(const uint64_t offset, const uint8_t size) const
{
    uint64_t out = 0;
    memcpy(&out, memory.get() + offset, size);
    return out;
}

// ReSharper disable once CppMemberFunctionMayBeConst [modifies member memory]
bool Ram::write_internal(const uint64_t offset, const uint64_t data, const uint8_t size)
{
    memcpy(memory.get() + offset, &data, size);
    return true;
}

void Ram::dump(std::ostream& os) const
{
    for (std::size_t i = 0; i < get_size(); i += 16) {
        bool all_zero = true;

        for (std::size_t j = i; j < i + 16 && j < get_size(); j++) {
            if (memory[j] != 0) {
                all_zero = false;
                break;
            }
        }

        if (all_zero) {
            continue;
        }

        os << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << i << ": ";

        for (std::size_t j = i; j < i + 16 && j < get_size(); j++) {
            os << std::uppercase << std::setw(2) << u32(memory[j]) << " ";
        }

        os << "\n";
    }
}
