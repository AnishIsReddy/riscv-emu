//
// Created by anish on 4/12/2026.
//

#pragma once

#include <vector>
#include "defs.h"
#include "device_map.h"

namespace riscv_emu::mem_io {

class Bus
{
  public:
    explicit Bus(DeviceMap&& map);

    template <UintFamily T>
    [[nodiscard]]
    std::optional<T> load(uint64_t addr) const;

    template <UintFamily T>
    bool store(uint64_t addr, T data);

    template <UintFamily T>
    [[nodiscard]]
    std::optional<T> load_reserved(uint64_t addr, size_t hart_id);

    template <UintFamily T>
    std::optional<bool> store_conditional(uint64_t addr, T data, size_t hart_id);

    template <UintFamily T>
    std::optional<T> handle_amo(AmoType type, uint64_t addr, T data);

  private:
    void clear_addr_reservations(uint64_t addr, uint8_t size);
    void reserve_addr(uint64_t addr, uint8_t size, size_t hart_id);

    [[nodiscard]]
    bool holds_reservation(uint64_t addr, uint8_t size, size_t hart_id) const;

    struct HartReservation
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

    DeviceMap device_map;
    std::vector<HartReservation> hart_reservations;
};
} // namespace riscv_emu::mem_io
