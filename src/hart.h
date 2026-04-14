//
// Created by anish on 3/29/2026.
//

#ifndef RISCV_EMU_HART_H
#define RISCV_EMU_HART_H

#include "defs.h"

namespace riscv_emu {
    class bus;

    class hart
    {
    public:
        explicit hart(bus* bus_ptr);
        bool step();

    private:
        uint64_t pc = 0;
        uint64_t reg_file[REG_COUNT] = {};
        bus* mem_bus;
    };

} // riscv_emu
#endif //RISCV_EMU_HART_H