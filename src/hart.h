//
// Created by anish on 3/29/2026.
//

#ifndef RISCV_EMU_HART_H
#define RISCV_EMU_HART_H

#include "defs.h"
#include <cstdint>
#include <iostream>

namespace riscv_emu {
    class bus;

    class hart
    {
    public:
        explicit hart(bus* bus_ptr);
        bool step();
        void dump_regs(std::ostream& os) const;

    private:
        uint64_t pc = 0;
        uint64_t reg_file[REG_COUNT] = {};
        bus* mem_bus;
    };

} // riscv_emu
#endif //RISCV_EMU_HART_H