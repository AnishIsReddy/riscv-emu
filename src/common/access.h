//
// Created by anish on 11/8/2026.
//

#pragma once

#include <concepts>

namespace riscv_emu {

template <typename T>
concept UintFamily =
    std::same_as<T, uint8_t> || std::same_as<T, uint16_t> || std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

template <typename T>
concept LoadFamily = UintFamily<T> || std::same_as<T, int8_t> || std::same_as<T, int16_t> || std::same_as<T, int32_t>;

template <typename T>
concept LoadResFamily = std::same_as<T, uint64_t> || std::same_as<T, int32_t>;

template <typename T>
concept AtomicFamily = std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

} // namespace riscv_emu
