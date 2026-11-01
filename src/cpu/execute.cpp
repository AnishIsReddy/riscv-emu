//
// Created by anish on 4/13/2026.
//

#include <cassert>
#include "cpu/execute.h"

#include <cstdint>
#include <format>

#include "common/int_cast.h"
#include "common/util.h"

namespace riscv_emu::cpu {
using namespace riscv_emu;

// Truncate to 32 bits and sign-extend to 64, as every *W instruction requires.
static constexpr uint64_t sext32(const uint64_t x)
{
    return i64(i32(x));
}

InstrEffect execute(const DecodedInstr instr, const uint64_t reg_file[REG_COUNT], const uint64_t pc,
                               const PrivilegeLevel priv)
{
    const uint64_t rs1 = reg_file[instr.rs1];
    const uint64_t rs2 = reg_file[instr.rs2];
    const uint64_t mem_addr = rs1 + instr.imm;
    const uint64_t next_pc = pc + 4;

    // Building block for the helpers below; call sites use the named helpers.
    // Only branch_if and jump_and_link choose a PC other than next_pc.
    auto emit = [&](auto effect)
    {
        return InstrEffect{.effect = effect, .new_pc = next_pc};
    };

    auto no_effect = [&]
    {
        return emit(InstrEffect::NoEffect{});
    };

    auto set_rd = [&](const uint64_t value)
    {
        return emit(InstrEffect::UpdateRd{.rd = instr.rd, .value = value});
    };

    auto branch_if = [&](const bool taken)
    {
        return InstrEffect{.effect = InstrEffect::NoEffect{}, .new_pc = taken ? pc + instr.imm : next_pc};
    };

    auto jump_and_link = [&](const uint64_t target)
    {
        return InstrEffect{.effect = InstrEffect::UpdateRd{.rd = instr.rd, .value = next_pc}, .new_pc = target};
    };

    auto load = [&]<mem_io::LoadFamily T>(T)
    {
        return emit(InstrEffect::LoadMem<T>{.rd = instr.rd, .addr = mem_addr});
    };

    auto store = [&](auto value)
    {
        return emit(InstrEffect::StoreMem{.addr = mem_addr, .value = value});
    };

    auto load_reserved = [&]<mem_io::LoadResFamily T>(T)
    {
        return emit(InstrEffect::LoadReserved<T>{.rd = instr.rd, .addr = rs1});
    };

    auto store_conditional = [&](auto value)
    {
        return emit(InstrEffect::StoreConditional{.rd = instr.rd, .addr = rs1, .value = value});
    };

    auto amo = [&]<mem_io::AtomicFamily T>(const T value, const AmoType amo_type)
    {
        return emit(InstrEffect::AmoRmw<T>{.rd = instr.rd, .type = amo_type, .addr = rs1, .value = value});
    };

    auto csr = [&](const CsrOpType csr_type, const uint64_t value, const bool skip_read, const bool skip_write)
    {
        return emit(InstrEffect::CsrRmw{.rd = instr.rd,
                                        .type = csr_type,
                                        .addr = u16(instr.imm),
                                        .value = value,
                                        .skip_read = skip_read,
                                        .skip_write = skip_write});
    };

    auto trap = [&](const ExceptionType cause)
    {
        return emit(InstrEffect::RaiseTrap{.cause = cause, .tval = 0});
    };

    auto trap_return = [&](const PrivilegeLevel return_priv)
    {
        return emit(InstrEffect::TrapReturn{.return_priv = return_priv});
    };

    auto wait_for_interrupt = [&]
    {
        return emit(InstrEffect::HandleWfi{});
    };
    // End Helpers

    switch (instr.op) {
        using enum Op;
    case LUI:       return set_rd(u64(instr.imm));
    case AUIPC:     return set_rd(pc + instr.imm);
    case JAL:       return jump_and_link(pc + instr.imm);
    case JALR:      return jump_and_link((instr.imm + rs1) & ~0x1);
    case BEQ:       return branch_if(rs1 == rs2);
    case BNE:       return branch_if(rs1 != rs2);
    case BLT:       return branch_if(i64(rs1) < i64(rs2));
    case BGE:       return branch_if(i64(rs1) >= i64(rs2));
    case BLTU:      return branch_if(rs1 < rs2);
    case BGEU:      return branch_if(rs1 >= rs2);
    case LB:        return load(int8_t{});
    case LH:        return load(int16_t{});
    case LW:        return load(int32_t{});
    case LBU:       return load(uint8_t{});
    case LHU:       return load(uint16_t{});
    case SB:        return store(u8(rs2));
    case SH:        return store(u16(rs2));
    case SW:        return store(u32(rs2));
    case ADDI:      return set_rd(rs1 + instr.imm);
    case SLTI:      return set_rd(i64(rs1) < instr.imm);
    case SLTIU:     return set_rd(rs1 < u64(instr.imm));
    case XORI:      return set_rd(rs1 ^ instr.imm);
    case ORI:       return set_rd(rs1 | instr.imm);
    case ANDI:      return set_rd(rs1 & instr.imm);
    case SLLI:      return set_rd(rs1 << instr.imm); // decode masks shamt to 6 bits
    case SRLI:      return set_rd(rs1 >> instr.imm);
    case SRAI:      return set_rd(i64(rs1) >> instr.imm);
    case ADD:       return set_rd(rs1 + rs2);
    case SUB:       return set_rd(rs1 - rs2);
    case SLL:       return set_rd(rs1 << (rs2 & 0x3F));
    case SLT:       return set_rd(i64(rs1) < i64(rs2));
    case SLTU:      return set_rd(rs1 < rs2);
    case XOR:       return set_rd(rs1 ^ rs2);
    case SRL:       return set_rd(rs1 >> (rs2 & 0x3F));
    case SRA:       return set_rd(i64(rs1) >> (rs2 & 0x3F));
    case OR:        return set_rd(rs1 | rs2);
    case AND:       return set_rd(rs1 & rs2);
    case FENCE:
    case FENCE_TSO:
    case FENCE_I:
    case PAUSE:     return no_effect();
    case ECALL:     {
        switch (priv) {
            using enum PrivilegeLevel;
        case M: return trap(ExceptionType::ECALL_M);
        case S: return trap(ExceptionType::ECALL_S);
        case U: return trap(ExceptionType::ECALL_U);
        }

        unreachable(std::format("FATAL: unhandled PrivilegeLevel {} at pc={:#x}", static_cast<int>(priv), pc));
    }
    case EBREAK: return trap(ExceptionType::BREAKPOINT);
    case LWU:    return load(uint32_t{});
    case LD:     return load(uint64_t{});
    case SD:     return store(rs2);
    case ADDIW:  return set_rd(sext32(rs1 + instr.imm));
    case SLLIW:  return set_rd(sext32(rs1 << instr.imm)); // decode masks shamt to 5 bits
    case SRLIW:  return set_rd(sext32(u32(rs1) >> instr.imm));
    case SRAIW:  return set_rd(sext32(i32(rs1) >> instr.imm));
    case ADDW:   return set_rd(sext32(rs1 + rs2));
    case SUBW:   return set_rd(sext32(rs1 - rs2));
    case SLLW:   return set_rd(sext32(rs1 << (rs2 & 0x1F)));
    case SRLW:   return set_rd(sext32(u32(rs1) >> (rs2 & 0x1F)));
    case SRAW:   return set_rd(sext32(i32(rs1) >> (rs2 & 0x1F)));
    case MUL:    return set_rd(rs1 * rs2);
    case MULH:   {
        const auto op1 = static_cast<__int128_t>(i64(rs1));
        const auto op2 = static_cast<__int128_t>(i64(rs2));
        const uint64_t val = op1 * op2 >> 64;
        return set_rd(val);
    }
    case MULHSU: {
        const auto op1 = static_cast<__int128_t>(i64(rs1));
        const auto op2 = static_cast<__uint128_t>(rs2);
        const uint64_t val = static_cast<__int128_t>(op1 * op2) >> 64;
        return set_rd(val);
    }
    case MULHU: {
        const auto op1 = static_cast<__uint128_t>(rs1);
        const auto op2 = static_cast<__uint128_t>(rs2);
        const uint64_t val = op1 * op2 >> 64;
        return set_rd(val);
    }
    case DIV: {
        uint64_t val;
        if (rs2 == 0) {
            val = ~0;
        }
        else if (i64(rs1) == INT64_MIN && i64(rs2) == -1) {
            val = INT64_MIN;
        }
        else {
            val = i64(rs1) / i64(rs2);
        }
        return set_rd(val);
    }
    case DIVU: {
        uint64_t val;
        if (rs2 == 0) {
            val = ~0;
        }
        else {
            val = rs1 / rs2;
        }
        return set_rd(val);
    }
    case REM: {
        uint64_t val;
        if (rs2 == 0) {
            val = rs1;
        }
        else if (i64(rs1) == INT64_MIN && i64(rs2) == -1) {
            val = 0;
        }
        else {
            val = i64(rs1) % i64(rs2);
        }
        return set_rd(val);
    }
    case REMU: {
        uint64_t val;
        if (rs2 == 0) {
            val = rs1;
        }
        else {
            val = rs1 % rs2;
        }
        return set_rd(val);
    }
    case MULW: return set_rd(sext32(rs1 * rs2));
    case DIVW: {
        const auto op1 = i32(rs1);
        const auto op2 = i32(rs2);

        uint64_t val;
        if (op2 == 0) {
            val = ~0;
        }
        else if (op1 == INT32_MIN && op2 == -1) {
            val = sext32(INT32_MIN);
        }
        else {
            val = sext32(op1 / op2);
        }
        return set_rd(val);
    }
    case DIVUW: {
        const auto op1 = u32(rs1);
        const auto op2 = u32(rs2);

        uint64_t val;
        if (op2 == 0) {
            val = ~0;
        }
        else {
            val = sext32(op1 / op2);
        }
        return set_rd(val);
    }
    case REMW: {
        const auto op1 = i32(rs1);
        const auto op2 = i32(rs2);

        uint64_t val;
        if (op2 == 0) {
            val = sext32(op1);
        }
        else if (op1 == INT32_MIN && op2 == -1) {
            val = 0;
        }
        else {
            val = sext32(op1 % op2);
        }
        return set_rd(val);
    }
    case REMUW: {
        const auto op1 = u32(rs1);
        const auto op2 = u32(rs2);

        uint64_t val;
        if (op2 == 0) {
            val = sext32(op1);
        }
        else {
            val = sext32(op1 % op2);
        }
        return set_rd(val);
    }
    case LR_W:      return load_reserved(int32_t{});
    case SC_W:      return store_conditional(u32(rs2));
    case AMOSWAP_W: return amo(u32(rs2), AmoType::SWAP);
    case AMOADD_W:  return amo(u32(rs2), AmoType::ADD);
    case AMOXOR_W:  return amo(u32(rs2), AmoType::XOR);
    case AMOAND_W:  return amo(u32(rs2), AmoType::AND);
    case AMOOR_W:   return amo(u32(rs2), AmoType::OR);
    case AMOMIN_W:  return amo(u32(rs2), AmoType::MIN);
    case AMOMAX_W:  return amo(u32(rs2), AmoType::MAX);
    case AMOMINU_W: return amo(u32(rs2), AmoType::MINU);
    case AMOMAXU_W: return amo(u32(rs2), AmoType::MAXU);
    case LR_D:      return load_reserved(uint64_t{});
    case SC_D:      return store_conditional(rs2);
    case AMOSWAP_D: return amo(u64(rs2), AmoType::SWAP);
    case AMOADD_D:  return amo(u64(rs2), AmoType::ADD);
    case AMOXOR_D:  return amo(u64(rs2), AmoType::XOR);
    case AMOAND_D:  return amo(u64(rs2), AmoType::AND);
    case AMOOR_D:   return amo(u64(rs2), AmoType::OR);
    case AMOMIN_D:  return amo(u64(rs2), AmoType::MIN);
    case AMOMAX_D:  return amo(u64(rs2), AmoType::MAX);
    case AMOMINU_D: return amo(u64(rs2), AmoType::MINU);
    case AMOMAXU_D: return amo(u64(rs2), AmoType::MAXU);
    case CSRRW:     return csr(CsrOpType::RW, rs1, instr.rd == 0, false);
    case CSRRS:     return csr(CsrOpType::RS, rs1, false, instr.rs1 == 0);
    case CSRRC:     return csr(CsrOpType::RC, rs1, false, instr.rs1 == 0);
    case CSRRWI:    return csr(CsrOpType::RW, instr.rs1, instr.rd == 0, false); // rs1 field is uimm[4:0]
    case CSRRSI:    return csr(CsrOpType::RS, instr.rs1, false, instr.rs1 == 0);
    case CSRRCI:    return csr(CsrOpType::RC, instr.rs1, false, instr.rs1 == 0);
    case MRET:
        if (priv != PrivilegeLevel::M) {
            return trap(ExceptionType::ILLEGAL_INSTRUCTION);
        }
        return trap_return(PrivilegeLevel::M);
    case SRET:
        if (priv < PrivilegeLevel::S) {
            return trap(ExceptionType::ILLEGAL_INSTRUCTION);
        }
        return trap_return(PrivilegeLevel::S);
    case SFENCE_VMA:
    case WFI:        return wait_for_interrupt();
    case INVALID:    return trap(ExceptionType::ILLEGAL_INSTRUCTION);
    }

    unreachable(std::format("FATAL: unhandled instr_type {} at pc={:#x}", static_cast<int>(instr.op), pc));
}
} // namespace riscv_emu::cpu
