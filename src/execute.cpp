//
// Created by anish on 4/13/2026.
//

#include <cstdint>
#include "execute.h"


#define MASK_32 0xFFFFFFFF

using namespace riscv_emu;

instr_effect riscv_emu::execute(instr_info instr, const uint64_t reg_file[REG_COUNT], uint64_t pc)
{
     instr_effect out = {
        .effect = instr_effect::no_effect{},
        .new_pc = pc + 4
    };

    const uint8_t shift_amt_imm = instr.imm & 0x3F;
    const uint8_t shift_amt_imm_32 = instr.imm & 0x1F;
    const uint8_t shift_amt_rs2 = reg_file[instr.rs2] & 0x3F;
    const uint8_t shift_amt_rs2_32 = reg_file[instr.rs2] & 0x1F;

    switch (instr.itype) {
        using enum instr_type;

    case LUI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = static_cast<uint64_t>(instr.imm)
        };
        return out;
    }

    case AUIPC: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = pc + instr.imm
        };
        return out;
    }

    case JAL: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = pc + 4
        };
        out.new_pc = pc + instr.imm;
        return out;
    }

    case JALR: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = pc + 4
        };
        out.new_pc = (instr.imm + reg_file[instr.rs1]) & ~0x1;
        return out;
    }

    case BEQ: {
        if (reg_file[instr.rs1] == reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case BNE: {
        if (reg_file[instr.rs1] != reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case BLT: {
        if (static_cast<int64_t>(reg_file[instr.rs1]) < static_cast<int64_t>(reg_file[instr.rs2])) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case BGE: {
        if (static_cast<int64_t>(reg_file[instr.rs1]) >= static_cast<int64_t>(reg_file[instr.rs2])) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case BLTU: {
        if (reg_file[instr.rs1] < reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case BGEU: {
        if (reg_file[instr.rs1] >= reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case LB: {
        out.effect = instr_effect::load_rd_from_mem
        {
            .rd = instr.rd,
            .addr = reg_file[instr.rs1] + instr.imm,
            .size = 1,
            .sign_ext = true
        };
        return out;
    }

    case LH: {
        out.effect = instr_effect::load_rd_from_mem
        {
            .rd = instr.rd,
            .addr = reg_file[instr.rs1] + instr.imm,
            .size = 2,
            .sign_ext = true
        };
        return out;
    }

    case LW: {
        out.effect = instr_effect::load_rd_from_mem
        {
            .rd = instr.rd,
            .addr = reg_file[instr.rs1] + instr.imm,
            .size = 4,
            .sign_ext = true
        };
        return out;
    }

    case LBU: {
        out.effect = instr_effect::load_rd_from_mem
        {
            .rd = instr.rd,
            .addr = reg_file[instr.rs1] + instr.imm,
            .size = 4,
            .sign_ext = false
        };
        return out;
    }

    case LHU: {
        out.effect = instr_effect::load_rd_from_mem
        {
            .rd = instr.rd,
            .addr = reg_file[instr.rs1] + instr.imm,
            .size = 2,
            .sign_ext = false
        };
        return out;
    }

    case SB: {
        out.effect = instr_effect::store_mem
        {
            .addr = reg_file[instr.rs1] + instr.imm,
            .value = reg_file[instr.rs2] & 0xFF,
            .size = 1
        };
        return out;
    }

    case SH: {
        out.effect = instr_effect::store_mem
        {
            .addr = reg_file[instr.rs1] + instr.imm,
            .value = reg_file[instr.rs2] & 0xFFFF,
            .size = 2
        };
        return out;
    }

    case SW: {
        out.effect = instr_effect::store_mem
        {
            .addr = reg_file[instr.rs1] + instr.imm,
            .value = reg_file[instr.rs2] & MASK_32,
            .size = 4
        };
        return out;
    }

    case ADDI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] + instr.imm
        };
        return out;
    }

    case SLTI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = static_cast<int64_t>(reg_file[instr.rs1]) < instr.imm
        };
        return out;
    }

    case SLTIU: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] < static_cast<uint64_t>(instr.imm)
        };
        return out;
    }

    case XORI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] ^ instr.imm
        };
        return out;
    }

    case ORI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] | instr.imm
        };
        return out;
    }

    case ANDI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] & instr.imm
        };
        return out;
    }

    case SLLI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] << shift_amt_imm
        };
        return out;
    }

    case SRLI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = logical_shift_right(reg_file[instr.rs1], shift_amt_imm)
        };
        return out;
    }

    case SRAI: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = arith_shift_right(reg_file[instr.rs1], shift_amt_imm)
        };
        return out;
    }

    case ADD: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] + reg_file[instr.rs2]
        };
        return out;
    }

    case SUB: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] - reg_file[instr.rs2]
        };
        return out;
    }

    case SLL: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] << reg_file[instr.rs2]
        };
        return out;
    }

    case SLT: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = static_cast<int64_t>(reg_file[instr.rs1]) < static_cast<int64_t>(reg_file[instr.rs2])
        };
        return out;
    }

    case SLTU: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] < reg_file[instr.rs2]
        };
        return out;
    }

    case XOR: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] ^ reg_file[instr.rs2]
        };
        return out;
    }

    case SRL: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = logical_shift_right(reg_file[instr.rs1], shift_amt_rs2)
        };
        return out;
    }

    case SRA: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = arith_shift_right(reg_file[instr.rs1], shift_amt_rs2)
        };
        return out;
    }

    case OR: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] | reg_file[instr.rs2]
        };
        return out;
    }

    case AND: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] & reg_file[instr.rs2]
        };
        return out;
    }

    case FENCE:
    case FENCE_TSO:
    case PAUSE:
    case ECALL:
    case EBREAK:
        return out;

    case LWU: {
        out.effect = instr_effect::load_rd_from_mem
        {
            .rd = instr.rd,
            .addr = reg_file[instr.rs1] + instr.imm,
            .size = 4,
            .sign_ext = false
        };
        return out;
    }

    case LD: {
        out.effect = instr_effect::load_rd_from_mem
        {
            .rd = instr.rd,
            .addr = reg_file[instr.rs1] + instr.imm,
            .size = 8,
            .sign_ext = true
        };
        return out;
    }

    case SD: {
        out.effect = instr_effect::store_mem
        {
            .addr = reg_file[instr.rs1] + instr.imm,
            .value = reg_file[instr.rs2],
            .size = 8
        };
        return out;
    }

    case ADDIW: {
        uint64_t val = sign_extend((reg_file[instr.rs1] & MASK_32) + (instr.imm & MASK_32), 32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case SLLIW: {
        uint64_t val = sign_extend((reg_file[instr.rs1] << shift_amt_imm_32) & MASK_32 , 32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case SRLIW: {
        uint64_t val = logical_shift_right(reg_file[instr.rs1] & MASK_32, shift_amt_imm_32);
        val = sign_extend(val,32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case SRAIW: {
        uint64_t val = sign_extend(reg_file[instr.rs1] & MASK_32, 32);
        val = arith_shift_right(val, shift_amt_imm_32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case ADDW: {
        uint64_t val = sign_extend((reg_file[instr.rs1] & MASK_32) + (reg_file[instr.rs2] & MASK_32), 32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case SUBW: {
        uint64_t val = sign_extend((reg_file[instr.rs1] & MASK_32) - (reg_file[instr.rs2] & MASK_32), 32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case SLLW: {
        uint64_t val = sign_extend((reg_file[instr.rs1] << shift_amt_rs2_32) & MASK_32, 32);
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };
        return out;
    }

    case SRLW: {
        uint64_t val = logical_shift_right(reg_file[instr.rs1] & MASK_32, shift_amt_rs2_32);
        val = sign_extend(val, 32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case SRAW: {
        uint64_t val = sign_extend(reg_file[instr.rs1] & MASK_32, 32);
        val = arith_shift_right(val, shift_amt_rs2_32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };
        return out;
    }

    case MUL: {
        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = reg_file[instr.rs1] * reg_file[instr.rs2]
        };
        return out;
    }

    case MULH: {
        auto op1 = static_cast<__int128_t>(static_cast<int64_t>(reg_file[instr.rs1]));
        auto op2 = static_cast<__int128_t>(static_cast<int64_t>(reg_file[instr.rs2]));
        uint64_t val = op1 * op2 >> 64;

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case MULHSU: {
        auto op1 = static_cast<__int128_t>(static_cast<int64_t>(reg_file[instr.rs1]));
        auto op2 = static_cast<__uint128_t>(reg_file[instr.rs2]);
        uint64_t val = static_cast<__int128_t>(op1 * op2) >> 64;

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case MULHU: {
        auto op1 = static_cast<__uint128_t>(reg_file[instr.rs1]);
        auto op2 = static_cast<__uint128_t>(reg_file[instr.rs2]);
        uint64_t val = op1 * op2 >> 64;

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case DIV: {
        uint64_t val;
        if (reg_file[instr.rs2] == 0) {
            val = ~0;
        }
        else if (static_cast<int64_t>(reg_file[instr.rs1]) == INT64_MIN && static_cast<int64_t>(reg_file[instr.rs2]) == -1) {
            val = INT64_MIN;
        }
        else {
            val = static_cast<int64_t>(reg_file[instr.rs1]) / static_cast<int64_t>(reg_file[instr.rs2]);
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case DIVU : {
        uint64_t val;
        if (reg_file[instr.rs2] == 0) {
            val = ~0;
        }
        else {
            val = reg_file[instr.rs1] / reg_file[instr.rs2];
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case REM: {
        uint64_t val;
        if (reg_file[instr.rs2] == 0) {
            val = reg_file[instr.rs1];
        }
        else if (static_cast<int64_t>(reg_file[instr.rs1]) == INT64_MIN && static_cast<int64_t>(reg_file[instr.rs2]) == -1) {
            val = 0;
        }
        else {
            val = static_cast<int64_t>(reg_file[instr.rs1]) % static_cast<int64_t>(reg_file[instr.rs2]);
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case REMU: {
        uint64_t val;
        if (reg_file[instr.rs2] == 0) {
            val = reg_file[instr.rs1];
        }
        else {
            val = reg_file[instr.rs1] % reg_file[instr.rs2];
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case MULW: {
        uint64_t val = sign_extend((reg_file[instr.rs1] & MASK_32) * (reg_file[instr.rs2] & MASK_32), 32);

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case DIVW: {
        auto op1 = static_cast<int32_t>(reg_file[instr.rs1]);
        auto op2 = static_cast<int32_t>(reg_file[instr.rs2]);

        uint64_t val;
        if (op2 == 0) {
            val = ~0;
        }
        else if (op1 == INT32_MIN && op2 == -1) {
            val = sign_extend(INT32_MIN, 32);
        }
        else {
            val = sign_extend(static_cast<uint64_t>(op1 / op2) & MASK_32, 32);
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case DIVUW: {
        auto op1 = reg_file[instr.rs1] & MASK_32;
        auto op2 = reg_file[instr.rs2] & MASK_32;

        uint64_t val;
        if (op2 == 0) {
            val = ~0;
        }
        else {
            val = sign_extend(op1 / op2, 32);
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case REMW: {
        auto op1 = static_cast<int32_t>(reg_file[instr.rs1]);
        auto op2 = static_cast<int32_t>(reg_file[instr.rs2]);

        uint64_t val;
        if (op2 == 0) {
            val = op1;
        }
        else if (op1 == INT32_MIN && op2 == -1) {
            val = 0;
        }
        else {
            val = sign_extend(op1 % op2, 32);
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    case REMUW: {
        auto op1 = reg_file[instr.rs1] & MASK_32;
        auto op2 = reg_file[instr.rs2] & MASK_32;

        uint64_t val;
        if (op2 == 0) {
            val = op1;
        }
        else {
            val = sign_extend(op1 % op2, 32);
        }

        out.effect = instr_effect::update_rd
        {
            .rd = instr.rd,
            .value = val
        };

        return out;
    }

    default:
        return out;
    }
}
