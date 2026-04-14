//
// Created by anish on 4/12/2026.
//

#ifndef RISCV_EMU_BUS_H
#define RISCV_EMU_BUS_H

#include <memory>

namespace riscv_emu
{
    class ram;

    class bus
    {
    public:
        explicit bus(ram* mem_ptr);

        [[nodiscard]] uint64_t read_memory(uint64_t addr, uint8_t size) const;
        void write_memory(uint64_t addr, uint64_t data, uint8_t size) const;

    private:
        ram* main_memory;

    };
} // riscv_emu

#endif //RISCV_EMU_BUS_H