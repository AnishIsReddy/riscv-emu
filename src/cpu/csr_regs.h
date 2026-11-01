//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>

namespace riscv_emu::cpu {
enum class CsrRegister : uint16_t
{
    // Machine info registers
    MVENDORID = 0xF11,
    MARCHID = 0xF12,
    // MIMPID = 0xF13,
    MHARTID = 0xF14,
    MCONFIGPTR = 0xF15,

    // Machine trap setup
    MSTATUS = 0x300,
    MISA = 0x301,
    MEDELEG = 0x302,
    MIDELEG = 0x303,
    MIE = 0x304,
    MTVEC = 0x305,
    MCOUNTEREN = 0x306,
    // MSTATUSH = 0x310,
    // MDELEGH = 0x312,

    // Machine trap handling
    MSCRATCH = 0x340,
    MEPC = 0x341,
    MCAUSE = 0x342,
    MTVAL = 0x343,
    MIP = 0x344,
    // MTINST = 0x34A,
    // MTVAL2 = 0x34B,

    // Machine indirect
    // MISELECT = 0x350,
    // MIREG_BASE = 0x351,
    // MIREG_SIZE = 6

    // Machine configuration
    MENVCFG = 0x30A,
    MSECCFG = 0x747,

    // Machine counters/timers
    MCYCLE = 0xB00,
    MINSTRET = 0xB02,

    // Machine counter setup
    MCOUNTINHIBIT = 0x320,
};
} // namespace riscv_emu::cpu
