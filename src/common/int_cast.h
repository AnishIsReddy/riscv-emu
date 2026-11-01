//
// Created by anish on 11/1/2026.
//

#pragma once

#include <concepts>
#include <cstdint>

namespace riscv_emu {
constexpr uint8_t u8(const std::integral auto x)
{
    return static_cast<uint8_t>(x);
}
constexpr uint16_t u16(const std::integral auto x)
{
    return static_cast<uint16_t>(x);
}
constexpr uint32_t u32(const std::integral auto x)
{
    return static_cast<uint32_t>(x);
}
constexpr uint64_t u64(const std::integral auto x)
{
    return static_cast<uint64_t>(x);
}
constexpr int8_t i8(const std::integral auto x)
{
    return static_cast<int8_t>(x);
}
constexpr int16_t i16(const std::integral auto x)
{
    return static_cast<int16_t>(x);
}
constexpr int32_t i32(const std::integral auto x)
{
    return static_cast<int32_t>(x);
}
constexpr int64_t i64(const std::integral auto x)
{
    return static_cast<int64_t>(x);
}
} // namespace riscv_emu
