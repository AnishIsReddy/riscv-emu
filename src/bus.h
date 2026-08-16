//
// Created by anish on 4/12/2026.
//

#ifndef RISCV_EMU_BUS_H
#define RISCV_EMU_BUS_H

#include <vector>
#include "defs.h"

namespace riscv_emu::mem_io {
class ram;

class bus
{
  public:
    explicit bus(ram* mem_ptr);


    template <UintFamily T>
    [[nodiscard]]
    T load(uint64_t addr) const;

    template <UintFamily T>
    void store(uint64_t addr, T data);

    template <UintFamily T>
    [[nodiscard]]
    T load_reserved(uint64_t addr, size_t hart_id);

    template <UintFamily T>
    bool store_conditional(uint64_t addr, T data, size_t hart_id);

    template <UintFamily T>
    T handle_amo(amo_type type, uint64_t addr, T data);

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
        bool valid() const
        {
            return size != 0;
        }

        void invalidate() { size = 0; }
    };

    ram* dram;
    std::vector<hart_res_entry> hart_reservations;
};
} // namespace riscv_emu::mem_io

#endif // RISCV_EMU_BUS_H
