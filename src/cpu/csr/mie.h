//
// Created by anish on 11/8/2026.
//

#pragma once

#include <cstdint>

#include "cpu/trap.h"

namespace riscv_emu::cpu {
class MIEReg
{
  public:
    // M-Mode and S-Mode interrupts
    // clang-format off
    static constexpr uint64_t MIE_WRITE_MASK = 1 << InterruptType::MACHINE_SOFTWARE |
                                               1 << InterruptType::MACHINE_TIMER |
                                               1 << InterruptType::MACHINE_EXTERNAL |
                                               1 << InterruptType::SUPERVISOR_SOFTWARE |
                                               1 << InterruptType::SUPERVISOR_TIMER |
                                               1 << InterruptType::SUPERVISOR_EXTERNAL;
    // clang-format on

    [[nodiscard]]
    uint64_t read() const
    {
        return raw;
    }

    void write(const uint64_t value)
    {
        raw = (raw & ~MIE_WRITE_MASK) | (value & MIE_WRITE_MASK);
    }

  private:
    uint64_t raw = 0;
};
} // namespace riscv_emu::cpu
