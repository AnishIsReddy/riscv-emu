//
// Created by anish on 4/13/2026.
//

#pragma once

#include "common/config.h"
#include "cpu/effect.h"
#include "cpu/instr.h"
#include "cpu/trap.h"

namespace riscv_emu::cpu {
InstrEffect execute(DecodedInstr instr, const uint64_t reg_file[REG_COUNT], uint64_t pc, PrivilegeLevel priv);
} // namespace riscv_emu::cpu
