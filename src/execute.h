//
// Created by anish on 4/13/2026.
//

#pragma once

#include "defs.h"

namespace riscv_emu {
InstrEffect execute(InstrInfo instr, const uint64_t reg_file[REG_COUNT], uint64_t pc, PrivilegeLevel priv);
} // namespace riscv_emu
