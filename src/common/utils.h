//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

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

[[noreturn]] constexpr void unreachable([[maybe_unused]] const std::string& msg)
{
#if DEBUG
    std::cout << msg << std::endl;
    std::abort();
#else
    std::unreachable();
#endif
}

// Param "bits" is the true size of "value". Anything below it will not be affected.
// Think of "bits" like .size() if value was a std::vector.
inline int64_t sign_extend(const uint64_t value, const uint8_t bits)
{
    return i64(value) << (64 - bits) >> (64 - bits);
}

inline uint64_t arith_shift_right(const uint64_t value, const uint8_t bits)
{
    return i64(value) >> bits;
}

inline uint64_t logical_shift_right(const uint64_t value, const uint8_t bits)
{
    return u64(value) >> bits;
}
} // namespace riscv_emu
