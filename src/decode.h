//
// Created by anish on 3/29/2026.
//

#ifndef RISCV_EMU_DECODER_H
#define RISCV_EMU_DECODER_H
#include <cstdint>
#include "defs.h"

namespace riscv_emu
{
     instr_info decode(uint32_t raw);
} // riscv_emu

#endif //RISCV_EMU_DECODER_H