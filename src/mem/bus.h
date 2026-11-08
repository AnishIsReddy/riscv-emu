//
// Created by anish on 4/12/2026.
//

#pragma once

#include <concepts>
#include <optional>
#include <vector>

#include "cpu/effect.h"
#include "mem/device_map.h"

namespace riscv_emu::mem {

template <typename T>
concept UintFamily =
    std::same_as<T, uint8_t> || std::same_as<T, uint16_t> || std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

template <typename T>
concept LoadFamily = UintFamily<T> || std::same_as<T, int8_t> || std::same_as<T, int16_t> || std::same_as<T, int32_t>;

template <typename T>
concept LoadResFamily = std::same_as<T, uint64_t> || std::same_as<T, int32_t>;

template <typename T>
concept AtomicFamily = std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

class Bus
{
  public:
    Bus(DeviceMap&& map, size_t hart_count);

    template <LoadFamily T>
    [[nodiscard]]
    std::optional<T> load(uint64_t addr) const;

    template <UintFamily T>
    bool store(uint64_t addr, T data);

    template <LoadResFamily T>
    [[nodiscard]]
    std::optional<T> load_reserved(uint64_t addr, size_t hart_id);

    template <AtomicFamily T>
    std::optional<bool> store_conditional(uint64_t addr, T data, size_t hart_id);

    template <AtomicFamily T>
    std::optional<T> handle_amo(cpu::AmoType type, uint64_t addr, T data);

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

        void invalidate()
        {
            size = 0;
        }
    };

    DeviceMap device_map;
    std::vector<HartReservation> hart_reservations;
};
} // namespace riscv_emu::mem
