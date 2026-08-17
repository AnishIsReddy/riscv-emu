//
// Created by anish on 6/15/2026.
//

#include <utility>

#include "csr_file.h"

namespace riscv_emu {
std::expected<uint64_t, TrapCause> CsrFile::read(const uint16_t addr, const PrivilegeLevel priv) const
{
    if (!is_correct_privilege(addr, priv)) {
        return std::unexpected(ExceptionType::ILLEGAL_INSTRUCTION);
    }

    // handle HPM addrs with hardcoded 0
    if (is_hpm_addr(addr)) {
        return 0;
    }

    switch (static_cast<CsrRegister>(addr)) {
        using enum CsrRegister;

    case MISA:
        return 0x8000000000001101; // 0x8000000000141101 when S and U modes added
    case MVENDORID:
    case MARCHID:
        return 0;
    case MHARTID:
        return mhartid;
    case MSTATUS:
        return mstatus.read();
    case MTVEC:
        return mtvec;
    case MEDELEG:
        return medeleg;
    case MIDELEG:
        return mideleg;
    case MIP:
        return mip;
    case MIE:
        return mie;
    case MCYCLE:
        return mcycle;
    case MINSTRET:
        return minstret;
    case MCOUNTEREN:
        return mcounteren;
    case MCOUNTINHIBIT:
        return 0;
    case MSCRATCH:
        return mscratch;
    case MEPC:
        return mepc;
    case MCAUSE:
        return mcause;
    case MTVAL:
        return mtval;
    case MCONFIGPTR:
        return 0;
    case MENVCFG:
        return menvcfg;
    case MSECCFG:
        return mseccfg;
    }

    // invalid register addr was passed
    return std::unexpected(ExceptionType::ILLEGAL_INSTRUCTION);
}

std::optional<TrapCause> CsrFile::write(const uint16_t addr, const uint64_t value, const PrivilegeLevel priv)
{
    if (!is_writeable_addr(addr)) {
        return ExceptionType::ILLEGAL_INSTRUCTION;
    }

    if (!is_correct_privilege(addr, priv)) {
        return ExceptionType::ILLEGAL_INSTRUCTION;
    }

    if (is_hpm_addr(addr)) {
        return std::nullopt;
    }

    switch (static_cast<CsrRegister>(addr)) {
        using enum CsrRegister;

    case MISA:
        return std::nullopt;
    case MVENDORID:
    case MARCHID:
    case MHARTID:
        std::unreachable();
    case MSTATUS:
        mstatus.write(value);
        return std::nullopt;
    case MTVEC:
        mtvec = (mtvec & ~MTVEC_WRITE_MASK) | (value & MTVEC_WRITE_MASK);
        return std::nullopt;
    case MEDELEG:
        medeleg = (medeleg & ~MEDELEG_WRITE_MASK) | (value & MEDELEG_WRITE_MASK);
        return std::nullopt;
    case MIDELEG:
        mideleg = (mideleg & ~MIDELEG_WRITE_MASK) | (value & MIDELEG_WRITE_MASK);
        return std::nullopt;
    case MIP:
        mip = (mip & ~MIP_WRITE_MASK) | (value & MIP_WRITE_MASK);
        return std::nullopt;
    case MIE:
        mie = (mie & ~MIE_WRITE_MASK) | (value & MIE_WRITE_MASK);
        return std::nullopt;
    case MCYCLE:
        mcycle = value;
        return std::nullopt;
    case MINSTRET:
        wrote_minstret = true;
        minstret = value;
        return std::nullopt;
    case MCOUNTEREN:
        mcounteren = value;
        return std::nullopt;
    case MCOUNTINHIBIT:
        return std::nullopt;
    case MSCRATCH:
        mscratch = value;
        return std::nullopt;
    case MEPC:
        mepc = value & ~0b11; // prevent any misaligned addr writes
        return std::nullopt;
    case MCAUSE:
        mcause = value;
        return std::nullopt;
    case MTVAL:
        mtval = value;
        return std::nullopt;
    case MCONFIGPTR:
        std::unreachable();
    case MENVCFG:
        menvcfg = value;
        return std::nullopt;
    case MSECCFG:
        mseccfg = value;
        return std::nullopt;
    }

    return std::nullopt;
}

inline bool CsrFile::is_correct_privilege(const uint16_t addr, const PrivilegeLevel access_priv)
{
    // bits [9:8] of addr represent privilege
    const auto reg_priv = static_cast<PrivilegeLevel>(addr >> 8 & 0b11);
    return access_priv >= reg_priv;
}

inline bool CsrFile::is_writeable_addr(const uint16_t addr)
{
    // bits [11:10] represent writable (0b11 is read-only)
    return ((addr >> 10) & 0b11) != 0b11;
}

uint64_t CsrFile::enter_trap_m(const TrapCause cause, const uint64_t tval, const uint64_t pc,
                                PrivilegeLevel old_priv)
{
    mstatus.set_mpp(static_cast<uint8_t>(old_priv));
    mstatus.set_mpie(mstatus.mie());
    mstatus.set_mie(false);

    mcause = cause.code;
    if (cause.is_interrupt) {
        mcause |= (uint64_t{1} << 63);
    }

    mtval = tval;
    mepc = pc;

    const auto mtvec_base = mtvec & ~0b11;
    const bool is_mtvec_mode_vectored = mtvec & 0b1;
    auto new_pc = mtvec_base;
    if (is_mtvec_mode_vectored && cause.is_interrupt) {
        new_pc += cause.code * 4;
    }

    return new_pc;
}

CsrFile::trap_ret_info CsrFile::return_trap_m()
{
    uint64_t pc = mepc;
    auto new_priv = static_cast<PrivilegeLevel>(mstatus.mpp());

    mstatus.set_mie(mstatus.mpie());
    mstatus.set_mpie(true);
    mstatus.set_mpp(std::to_underlying(PrivilegeLevel::M)); // Change this to U after user space impl

    if (new_priv != PrivilegeLevel::M) {
        mstatus.set_mprv(false);
    }

    return std::make_pair(pc, new_priv);
}

bool CsrFile::is_hpm_addr(const uint16_t addr)
{
    constexpr uint64_t ctr_base = 0xB03;
    constexpr uint64_t event_base = 0x323;
    constexpr size_t size = 29; // regs [3:31] inclusive

    return (addr >= ctr_base && addr < ctr_base + size) || (addr >= event_base && addr < event_base + size);
}

void CsrFile::increment_cycle_count()
{
    mcycle++;
}

void CsrFile::increment_retired_instructions()
{
    if (!wrote_minstret) {
        minstret++;
    }

    wrote_minstret = false;
}

bool CsrFile::is_wfi_valid(const PrivilegeLevel priv) const
{
    if (priv == PrivilegeLevel::M) {
        return true;
    }

    if (priv == PrivilegeLevel::S) {
        return mstatus.tsr() == 0;
    }

    return false;
}
} // namespace riscv_emu
