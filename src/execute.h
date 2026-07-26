//
// Created by anish on 4/13/2026.
//

#ifndef RISCV_EMU_EXECUTE_H
#define RISCV_EMU_EXECUTE_H

#include "defs.h"

namespace riscv_emu
{
    instr_effect execute(
        instr_info instr,
        const uint64_t reg_file[REG_COUNT],
        uint64_t pc,
        privilege_level priv
    );
} // riscv_emu

#endif //RISCV_EMU_EXECUTE_H