//
// Created by anish on 4/14/2026.
//

#ifndef RISCV_EMU_MACHINE_H
#define RISCV_EMU_MACHINE_H

#include <vector>
#include "bus.h"

namespace riscv_emu
{
    class bus;
    class hart;

    class machine
    {
    public:
        machine();
        void run();
        bus m_bus;
        std::vector<hart> m_harts;
    };
} // riscv_emu

#endif //RISCV_EMU_MACHINE_H