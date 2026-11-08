//
// Created by anish on 11/8/2026.
//

#pragma once

#include <cstdint>

namespace riscv_emu::cpu {
class MStatusReg
{
    static constexpr int MIE = 3;
    static constexpr int MPIE = 7;
    static constexpr int MPP = 11; // 2-bit field [12:11]
    static constexpr int SIE = 1;
    static constexpr int SPIE = 5;
    static constexpr int SPP = 8; // 1-bit field
    static constexpr int MPRV = 17;
    static constexpr int TW = 21;
    static constexpr int TSR = 22;

    // TBI after S & U Modes:
    static constexpr uint64_t RESET = (uint64_t{0b11} << MPP);
    //                                 | (uint64_t{0} << 34)   // SXL = 2
    //                                 | (uint64_t{0} << 32);  // UXL = 2

    static constexpr uint64_t MSTATUS_WRITE_MASK = (uint64_t{1} << MIE) |
                                                   (uint64_t{1} << MPIE) |
                                                   (uint64_t{0b11} << MPP) |
                                                   (uint64_t{1} << MPRV) |
                                                   (uint64_t{1} << TW) |
                                                   (uint64_t{1} << TSR);

  public:
    // single-bit fields: get/set
    [[nodiscard]]
    bool mie() const
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
        set_bit(MIE, v);
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
        raw = (raw & ~(0b11ul << MPP)) | (uint64_t{mode & 0b11u} << MPP);
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
    void set_bit(const int pos, const bool v)
    {
        if (v) {
            raw |= (uint64_t{1} << pos);
        }
        else {
            raw &= ~(uint64_t{1} << pos);
        }
    }

    uint64_t raw = RESET;
};
} // namespace riscv_emu::cpu
