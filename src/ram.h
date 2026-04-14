//
// Created by anish on 4/12/2026.
//

#ifndef RISCV_EMU_MEMORY_H
#define RISCV_EMU_MEMORY_H
#include <memory>
#include "defs.h"

namespace riscv_emu
{
    class ram
    {
    public:

    private:
        std::unique_ptr<uint8_t[]> memory = std::make_unique<uint8_t[]>(MEM_SIZE);
    };
} // riscv_emu

#endif //RISCV_EMU_MEMORY_H