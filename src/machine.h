//
// Created by anish on 4/14/2026.
//

#ifndef RISCV_EMU_MACHINE_H
#define RISCV_EMU_MACHINE_H

#include <vector>
#include "bus.h"
#include "ram.h"
#include "hart.h"

namespace riscv_emu
{
    class machine
    {
    public:
        machine();
        void run();
        void load(const uint8_t* data, std::size_t size) const;

        ram m_ram;
        bus m_bus;
        std::vector<hart> m_harts;
    };
} // riscv_emu

#endif //RISCV_EMU_MACHINE_H