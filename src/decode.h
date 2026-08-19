//
// Created by anish on 3/29/2026.
//

#pragma once
#include <cstdint>
#include "defs.h"

namespace riscv_emu {
InstrInfo decode(uint32_t raw);
} // namespace riscv_emu
