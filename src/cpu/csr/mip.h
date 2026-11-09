//
// Created by anish on 11/8/2026.
//

#pragma once

#include <cstdint>

#include "cpu/trap.h"
#include "mem/devices/clint.h"

namespace riscv_emu::cpu {
class MIPReg
{
  public:
    explicit MIPReg(mem::devices::Clint* clint, const size_t hart_id) : hart_id(hart_id), clint_ptr(clint) {}

    // Only software interrupts should be writable
    // clang-format off
    static constexpr uint64_t MIP_WRITE_MASK = 1 << InterruptType::MACHINE_SOFTWARE
                                             | 1 << InterruptType::SUPERVISOR_SOFTWARE;
    // clang-format on

    [[nodiscard]]
    uint64_t read() const
    {
        return raw | pull_interrupts_from_clint();
    }

    void write(const uint64_t value)
    {
        raw = (raw & ~MIP_WRITE_MASK) | (value & MIP_WRITE_MASK);
    }

  private:
    [[nodiscard]]
    uint64_t pull_interrupts_from_clint() const
    {
        uint64_t reg = 0;

        if (clint_ptr) {
            reg |= clint_ptr->is_timer_interrupt_pending(hart_id) << InterruptType::MACHINE_TIMER;
            reg |= clint_ptr->is_software_interrupt_pending(hart_id) << InterruptType::MACHINE_SOFTWARE;
        }

        return reg;
    }

    uint64_t raw = 0;
    const size_t hart_id = 0;
    mem::devices::Clint* clint_ptr;
};
} // namespace riscv_emu::cpu
