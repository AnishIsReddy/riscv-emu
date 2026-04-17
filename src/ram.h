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
        void load(const uint8_t* data, std::size_t size) const;

        [[nodiscard]] uint64_t read_addr(uint64_t addr, uint8_t size) const;
        void write_addr(uint64_t addr, uint64_t data, uint8_t size) const;

        void dump(std::ostream& os) const;

    private:
        std::unique_ptr<uint8_t[]> memory = std::make_unique<uint8_t[]>(MEM_SIZE);
    };
} // riscv_emu

#endif //RISCV_EMU_MEMORY_H