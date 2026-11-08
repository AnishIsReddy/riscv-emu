//
// Created by anish on 11/1/2026.
//

#pragma once

#include <concepts>
#include <cstdint>
#include <optional>

#include "common/utils.h"

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

class BusDevice
{
  public:
    explicit BusDevice(const uint64_t size) : size(size) {}

    virtual ~BusDevice() = default;

    [[nodiscard]]
    std::optional<uint64_t> read(const uint64_t offset, const uint8_t access_size) const
    {
        if (!is_access_within_device(offset, access_size)) {
            return u64(0);
        }

        return read_internal(offset, access_size);
    }

    bool write(const uint64_t offset, const uint64_t data, const uint8_t access_size)
    {
        if (!is_access_within_device(offset, access_size)) {
            return true;
        }

        return write_internal(offset, data, access_size);
    }

    [[nodiscard]]
    uint64_t get_size() const
    {
        return size;
    }

  protected:
    [[nodiscard]]
    virtual std::optional<uint64_t> read_internal(uint64_t offset, uint8_t access_size) const = 0;

    virtual bool write_internal(uint64_t offset, uint64_t data, uint8_t access_size) = 0;

  private:
    [[nodiscard]]
    bool is_access_within_device(const uint64_t offset, const uint8_t access_size) const
    {
        return offset < size && access_size <= size - offset;
    }

    const uint64_t size;
};
} // namespace riscv_emu::mem
