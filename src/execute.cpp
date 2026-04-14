//
// Created by anish on 4/13/2026.
//

#include <cstdint>
#include "execute.h"


#define MASK_32 0xFFFFFFFF

using namespace riscv_emu;

exec_result riscv_emu::execute(instr_info instr, const uint64_t reg_file[REG_COUNT], uint64_t pc)
{
    exec_result out = {
        .type = exec_result_type::NO_UPDATE,

        .rd = instr.rd,
        .val = 0,
        .mem_addr = 0,
        .mem_size = 0,
        .zero_extend_val = false,

        .new_pc = pc + 4
    };

    const uint8_t shift_amt_imm = instr.imm & 0x3F;
    const uint8_t shift_amt_imm_32 = instr.imm & 0x1F;
    const uint8_t shift_amt_rs2 = reg_file[instr.rs2] & 0x3F;
    const uint8_t shift_amt_rs2_32 = reg_file[instr.rs2] & 0x1F;

    switch (instr.itype) {
    case instr_type::LUI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = instr.imm;
        return out;
    }

    case instr_type::AUIPC: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = pc + instr.imm;
        return out;
    }

    case instr_type::JAL: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = pc + 4;
        out.new_pc = pc + instr.imm;
        return out;
    }

    case instr_type::JALR: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = pc + 4;
        out.new_pc = (instr.imm + reg_file[instr.rs1]) & ~0x1;
        return out;
    }

    case instr_type::BEQ: {
        out.type = exec_result_type::NO_UPDATE;
        if (reg_file[instr.rs1] == reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case instr_type::BNE: {
        out.type = exec_result_type::NO_UPDATE;
        if (reg_file[instr.rs1] != reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case instr_type::BLT: {
        out.type = exec_result_type::NO_UPDATE;
        if (static_cast<int64_t>(reg_file[instr.rs1]) < static_cast<int64_t>(reg_file[instr.rs2])) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case instr_type::BGE: {
        out.type = exec_result_type::NO_UPDATE;
        if (static_cast<int64_t>(reg_file[instr.rs1]) >= static_cast<int64_t>(reg_file[instr.rs2])) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case instr_type::BLTU: {
        out.type = exec_result_type::NO_UPDATE;
        if (reg_file[instr.rs1] < reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case instr_type::BGEU: {
        out.type = exec_result_type::NO_UPDATE;
        if (reg_file[instr.rs1] >= reg_file[instr.rs2]) {
            out.new_pc = pc + instr.imm;
        }
        return out;
    }

    case instr_type::LB: {
        out.type = exec_result_type::UPDATE_RD_FROM_MEM;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 1;
        return out;
    }

    case instr_type::LH: {
        out.type = exec_result_type::UPDATE_RD_FROM_MEM;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 2;
        return out;
    }

    case instr_type::LW: {
        out.type = exec_result_type::UPDATE_RD_FROM_MEM;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 4;
        return out;
    }

    case instr_type::LBU: {
        out.type = exec_result_type::UPDATE_RD_FROM_MEM;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 1;
        out.zero_extend_val = true;
        return out;
    }

    case instr_type::LHU: {
        out.type = exec_result_type::UPDATE_RD_FROM_MEM;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 2;
        out.zero_extend_val = true;
        return out;
    }

    case instr_type::SB: {
        out.type = exec_result_type::UPDATE_MEM_FROM_VAL;
        out.val = reg_file[instr.rs2] & 0xFF;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 1;
        return out;
    }

    case instr_type::SH: {
        out.type = exec_result_type::UPDATE_MEM_FROM_VAL;
        out.val = reg_file[instr.rs2] & 0xFFFF;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 2;
        return out;
    }

    case instr_type::SW: {
        out.type = exec_result_type::UPDATE_MEM_FROM_VAL;
        out.val = reg_file[instr.rs2] & MASK_32;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 4;
        return out;
    }

    case instr_type::ADDI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] + instr.imm;
        return out;
    }

    case instr_type::SLTI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = static_cast<int64_t>(reg_file[instr.rs1]) < instr.imm;
        return out;
    }

    case instr_type::SLTIU: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] < static_cast<uint64_t>(instr.imm);
        return out;
    }

    case instr_type::XORI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] ^ instr.imm;
        return out;
    }

    case instr_type::ORI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] | instr.imm;
        return out;
    }

    case instr_type::ANDI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] & instr.imm;
        return out;
    }

    case instr_type::SLLI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] << shift_amt_imm;
        return out;
    }

    case instr_type::SRLI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = logical_shift_right(reg_file[instr.rs1], shift_amt_imm);
        return out;
    }

    case instr_type::SRAI: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = arith_shift_right(reg_file[instr.rs1], shift_amt_imm);
        return out;
    }

    case instr_type::ADD: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] + reg_file[instr.rs2];
        return out;
    }

    case instr_type::SUB: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] - reg_file[instr.rs2];
        return out;
    }

    case instr_type::SLL: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] << shift_amt_rs2;
        return out;
    }

    case instr_type::SLT: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = static_cast<int64_t>(reg_file[instr.rs1]) < static_cast<int64_t>(reg_file[instr.rs2]);
        return out;
    }

    case instr_type::SLTU: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] < reg_file[instr.rs2];
        return out;
    }

    case instr_type::XOR: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] ^ reg_file[instr.rs2];
        return out;
    }

    case instr_type::SRL: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = logical_shift_right(reg_file[instr.rs1], shift_amt_rs2);
        return out;
    }

    case instr_type::SRA: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = arith_shift_right(reg_file[instr.rs1], shift_amt_rs2);
        return out;
    }

    case instr_type::OR: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] | reg_file[instr.rs2];
        return out;
    }

    case instr_type::AND: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = reg_file[instr.rs1] & reg_file[instr.rs2];
        return out;
    }
    case instr_type::FENCE:
    case instr_type::FENCE_TSO:
    case instr_type::PAUSE:
    case instr_type::ECALL:
    case instr_type::EBREAK:
        return out;


    case instr_type::LWU: {
        out.type = exec_result_type::UPDATE_RD_FROM_MEM;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 4;
        out.zero_extend_val = true;
        return out;
    }

    case instr_type::LD: {
        out.type = exec_result_type::UPDATE_RD_FROM_MEM;
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 8;
        return out;
    }

    case instr_type::SD: {
        out.type = exec_result_type::UPDATE_MEM_FROM_VAL;
        out.val = reg_file[instr.rs2];
        out.mem_addr = reg_file[instr.rs1] + instr.imm;
        out.mem_size = 8;
        return out;
    }

    case instr_type::ADDIW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = sign_extend((reg_file[instr.rs1] & MASK_32) + (instr.imm & MASK_32), 32);
        return out;
    }

    case instr_type::SLLIW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = sign_extend((reg_file[instr.rs1] << shift_amt_imm_32) & MASK_32 , 32);
        return out;
    }

    case instr_type::SRLIW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = logical_shift_right(reg_file[instr.rs1] & MASK_32, shift_amt_imm_32);
        out.val = sign_extend(out.val,32);
        return out;
    }

    case instr_type::SRAIW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = sign_extend(reg_file[instr.rs1] & MASK_32, 32);
        out.val = arith_shift_right(out.val, shift_amt_imm_32);
        return out;
    }

    case instr_type::ADDW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = sign_extend((reg_file[instr.rs1] & MASK_32) + (reg_file[instr.rs2] & MASK_32), 32);
        return out;
    }

    case instr_type::SUBW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = sign_extend((reg_file[instr.rs1] & MASK_32) - (reg_file[instr.rs2] & MASK_32), 32);
        return out;
    }

    case instr_type::SLLW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = sign_extend((reg_file[instr.rs1] << shift_amt_rs2_32) & MASK_32, 32);
        return out;
    }

    case instr_type::SRLW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = logical_shift_right(reg_file[instr.rs1] & MASK_32, shift_amt_rs2_32);
        out.val = sign_extend(out.val, 32);
        return out;
    }

    case instr_type::SRAW: {
        out.type = exec_result_type::UPDATE_RD_FROM_VAL;
        out.val = sign_extend(reg_file[instr.rs1] & MASK_32, 32);
        out.val = arith_shift_right(out.val, shift_amt_rs2_32);
        return out;
    }

    default:
        return out;
    }
}
