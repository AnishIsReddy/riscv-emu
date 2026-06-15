//
// Created by anish on 4/12/2026.
//

#ifndef RISCV_EMU_BUS_H
#define RISCV_EMU_BUS_H

#include <vector>
#include "defs.h"

namespace riscv_emu
{
    class ram;

    class bus
    {
    public:
        explicit bus(ram* mem_ptr);

        [[nodiscard]]
        uint64_t load(uint64_t addr, uint8_t size) const;
        void store(uint64_t addr, uint64_t data, uint8_t size);

        [[nodiscard]]
        uint64_t load_reserved(uint64_t addr, uint8_t size, size_t hart_id);
        bool store_conditional(uint64_t addr, uint64_t data, uint8_t size, size_t hart_id);

        uint64_t handle_amo(amo_type type, uint64_t addr, uint64_t data, uint8_t size);

    private:
        void clear_addr_reservations(uint64_t addr, uint8_t size);
        void reserve_addr(uint64_t addr, uint8_t size, size_t hart_id);

        [[nodiscard]]
        bool holds_reservation(uint64_t addr, uint8_t size, size_t hart_id) const;

        struct hart_res_entry
        {
            uint64_t addr = 0;
            uint8_t size = 0;

            [[nodiscard]]
            bool valid() const {return size != 0; }
            void invalidate() { size = 0; }
        };

        ram* main_memory;
        std::vector<hart_res_entry> hart_reservations;
    };
} // riscv_emu

#endif //RISCV_EMU_BUS_H