//
// Created by anish on 6/15/2026.
//

#pragma once

#include <cstdint>
#include <expected>
#include <optional>
#include <utility>

#include "cpu/csr/mstatus.h"

#include "cpu/trap.h"

namespace riscv_emu::cpu {
class CsrFile
{
  public:
    explicit CsrFile(const uint64_t hart_id) : mhartid(hart_id) {}

    [[nodiscard]]
    std::expected<uint64_t, TrapCause> read(uint16_t addr, PrivilegeLevel priv) const;

    std::optional<TrapCause> write(uint16_t addr, uint64_t value, PrivilegeLevel priv);

    uint64_t enter_trap_m(TrapCause cause, uint64_t tval, uint64_t pc, PrivilegeLevel old_priv);

    using trap_ret_info = std::pair<uint64_t, PrivilegeLevel>;
    trap_ret_info return_trap_m();

    [[nodiscard]]
    bool is_wfi_valid(PrivilegeLevel priv) const;

    void increment_cycle_count();
    void increment_retired_instructions();

  private:
    static constexpr uint64_t MEDELEG_WRITE_MASK = 0xB3FF;
    // delegatable exceptions, ECALL_M(11) & reserved(10,14) excluded
    static constexpr uint64_t MIDELEG_WRITE_MASK = 0x222; // S-mode interrupts (1,5,9); M-interrupts excluded
    static constexpr uint64_t MTVEC_WRITE_MASK = ~0x2; // everything except bit 1 should be writable

    // M-Mode and S-Mode interrupts
    static constexpr uint64_t MIE_WRITE_MASK = 1 << InterruptType::MACHINE_SOFTWARE |
                                               1 << InterruptType::MACHINE_TIMER |
                                               1 << InterruptType::MACHINE_EXTERNAL |
                                               1 << InterruptType::SUPERVISOR_SOFTWARE |
                                               1 << InterruptType::SUPERVISOR_TIMER |
                                               1 << InterruptType::SUPERVISOR_EXTERNAL;

    // Software interrupts should be writable
    static constexpr uint64_t MIP_WRITE_MASK =
        1 << InterruptType::MACHINE_SOFTWARE | 1 << InterruptType::SUPERVISOR_SOFTWARE;

    static bool is_correct_privilege(uint16_t addr, PrivilegeLevel access_priv);
    static bool is_writeable_addr(uint16_t addr);
    static bool is_hpm_addr(uint16_t addr);

    uint64_t mhartid;
    MStatusReg mstatus;
    uint64_t mtvec = 0;
    uint64_t medeleg = 0;
    uint64_t mideleg = 0;
    uint64_t mip = 0;
    uint64_t mie = 0;

    uint64_t mcycle = 0;
    uint64_t minstret = 0;
    bool wrote_minstret = false;

    uint64_t mcounteren = 0;

    uint64_t mscratch = 0;
    uint64_t mepc = 0;
    uint64_t mcause = 0;
    uint64_t mtval = 0;

    uint64_t menvcfg = 0;
    uint64_t mseccfg = 0;
};
} // namespace riscv_emu::cpu
