//
// Created by anish on 3/29/2026.
//

#pragma once

#include <cstdint>

#include "cpu/instr.h"

namespace riscv_emu::cpu {
DecodedInstr decode(uint32_t raw);
} // namespace riscv_emu::cpu
