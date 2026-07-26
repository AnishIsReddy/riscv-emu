//
// Created by anish on 6/15/2026.
//

#ifndef RISCV_EMU_CSR_H
#define RISCV_EMU_CSR_H

#include <cstdint>
#include <expected>

#include "defs.h"

namespace riscv_emu {

class csr_file
{
public:
    explicit csr_file(const uint64_t hart_id) : mhartid(hart_id){}

    [[nodiscard]]
    std::expected<uint64_t, trap_cause> read(uint16_t addr, privilege_level priv) const;

    std::optional<trap_cause> write(uint16_t addr, uint64_t value, privilege_level priv);

    uint64_t enter_trap_m(trap_cause cause, uint64_t tval, uint64_t pc, privilege_level old_priv);

    using trap_ret_info = std::pair<uint64_t, privilege_level>;
    trap_ret_info return_trap_m();

    [[nodiscard]]
    bool is_wfi_valid(privilege_level priv) const;

    void increment_cycle_count();
    void increment_retired_instructions();

private:
    static constexpr uint64_t MEDELEG_WRITE_MASK = 0xB3FF;  // delegatable exceptions, ECALL_M(11) & reserved(10,14) excluded
    static constexpr uint64_t MIDELEG_WRITE_MASK = 0x222;   // S-mode interrupts (1,5,9); M-interrupts excluded
    static constexpr uint64_t MTVEC_WRITE_MASK = ~0x2;      // everything except bit 1 should be writable

    static constexpr uint64_t MIE_WRITE_MASK = 1 << interrupt_type::MACHINE_SOFTWARE
                                             | 1 << interrupt_type::MACHINE_TIMER
                                             | 1 << interrupt_type::MACHINE_EXTERNAL
                                             | 1 << interrupt_type::SUPERVISOR_SOFTWARE
                                             | 1 << interrupt_type::SUPERVISOR_TIMER
                                             | 1 << interrupt_type::SUPERVISOR_EXTERNAL;
                                             // M-Mode and S-Mode interrupts

    static constexpr uint64_t MIP_WRITE_MASK = 1 << interrupt_type::MACHINE_SOFTWARE
                                             | 1 << interrupt_type::SUPERVISOR_SOFTWARE;
                                             // Software interrupts should be writable


    class mstatus_value {
        static constexpr int MIE  = 3;
        static constexpr int MPIE = 7;
        static constexpr int MPP  = 11;   // 2-bit field [12:11]
        static constexpr int SIE  = 1;
        static constexpr int SPIE = 5;
        static constexpr int SPP  = 8;    // 1-bit field
        static constexpr int MPRV = 17;
        static constexpr int TW   = 21;
        static constexpr int TSR  = 22;

        // TODO after S & U Modes:
        static constexpr uint64_t RESET = (uint64_t{0b11} << MPP);
        //                                 | (uint64_t{0} << 34)   // SXL = 2
        //                                 | (uint64_t{0} << 32);  // UXL = 2

        static constexpr uint64_t MSTATUS_WRITE_MASK = (uint64_t{1} << MIE)
                                                     | (uint64_t{1} << MPIE)
                                                     | (uint64_t{0b11} << MPP)
                                                     | (uint64_t{1} << MPRV)
                                                     | (uint64_t{1} << TW)
                                                     | (uint64_t{1} << TSR);

    public:
        // single-bit fields: get/set
        [[nodiscard]]
        bool mie()  const
        {
            return raw >> MIE & 1;
        }

        [[nodiscard]]
        bool mpie() const
        {
            return raw >> MPIE & 1;
        }

        void set_mie(const bool v)
        {
            set_bit(MIE,  v);
        }

        void set_mpie(const bool v)
        {
            set_bit(MPIE, v);
        }

        // 2-bit MPP field [12:11]
        [[nodiscard]]
        uint8_t mpp() const
        {
            return raw >> MPP & 0b11ul;
        }

        void set_mpp(const uint8_t mode)
        {
            raw = (raw & ~(0b11ul << MPP)) | (uint64_t{mode & 0b11u } << MPP);
        }

        [[nodiscard]]
        uint8_t mprv() const
        {
            return raw >> MPRV & 1;
        }

        void set_mprv(const bool v)
        {
            set_bit(MPRV, v);
        }

        [[nodiscard]]
        uint8_t tw() const
        {
            return raw >> TW & 1;
        }

        [[nodiscard]]
        uint8_t tsr() const
        {
            return raw >> TSR & 1;
        }

        [[nodiscard]]
        uint64_t read() const
        {
            return raw;
        }

        void write(const uint64_t value)
        {
            raw = (value & MSTATUS_WRITE_MASK) | (raw & ~MSTATUS_WRITE_MASK);
        }

    private:
        void set_bit(const int pos, const bool v) {
            if (v) {
                raw |= (uint64_t{1} << pos);
            }
            else {
                raw &= ~(uint64_t{1} << pos);
            }
        }

        uint64_t raw = RESET;
    };

    static bool is_correct_privilege(uint16_t addr, privilege_level access_priv);
    static bool is_writeable_addr(uint16_t addr);
    static bool is_hpm_addr(uint16_t addr);

    uint64_t mhartid;
    mstatus_value mstatus;
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


} // riscv_emu

#endif //RISCV_EMU_CSR_H
