//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>

namespace riscv_emu {
constexpr uint64_t ADDR_WIDTH = 64; // 64-bit system
constexpr uint64_t MEM_SIZE = 128 * 1024 * 1024; // 128 MiB RAM
constexpr uint64_t REG_COUNT = 32;
} // namespace riscv_emu
