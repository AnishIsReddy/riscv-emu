//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include "common/int_cast.h"

namespace riscv_emu {
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
