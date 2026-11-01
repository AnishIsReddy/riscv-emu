//
// Created by anish on 11/1/2026.
//

#pragma once

#include <concepts>
#include <cstdint>
#include <optional>

namespace riscv_emu::mem_io {
template <typename T>
concept UintFamily =
    std::same_as<T, uint8_t> || std::same_as<T, uint16_t> || std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

template <typename T>
concept LoadFamily = UintFamily<T> || std::same_as<T, int8_t> || std::same_as<T, int16_t> || std::same_as<T, int32_t>;

template <typename T>
concept LoadResFamily = std::same_as<T, uint64_t> || std::same_as<T, int32_t>;

template <typename T>
concept AtomicFamily = std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

class BusDevice
{
  public:
    explicit BusDevice(const uint64_t size) : size(size) {}
    virtual ~BusDevice() = default;

    [[nodiscard]]
    virtual std::optional<uint64_t> read(uint64_t offset, uint8_t size) const = 0;

    virtual bool write(uint64_t offset, uint64_t data, uint8_t size) = 0;

    [[nodiscard]]
    uint64_t get_size() const
    {
        return size;
    }

  private:
    const uint64_t size;
};
} // namespace riscv_emu::mem_io
