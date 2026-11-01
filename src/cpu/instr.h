//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>

namespace riscv_emu::cpu {
enum class Opcode
{
    LUI = 0b0110111,
    AUIPC = 0b0010111,
    JAL = 0b1101111,
    JALR = 0b1100111,
    OP = 0b0110011,
    OP_32 = 0b0111011,
    OP_IMM = 0b0010011,
    OP_IMM_32 = 0b0011011,
    BRANCH = 0b1100011,
    LOAD = 0b0000011,
    STORE = 0b0100011,
    SYSTEM = 0b1110011,
    MISC_MEM = 0b0001111,
    AMO = 0b0101111
};

// clang-format off
enum class Op
{
    // RV32i instructions
    INVALID,
    LUI, AUIPC, JAL, JALR,
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    LB, LH, LW, LBU, LHU,
    SB, SH, SW,
    ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI,
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
    FENCE, FENCE_TSO, PAUSE, ECALL, EBREAK,

    // RV32 Zifencei
    FENCE_I,

    // RV64i instructions
    LWU, LD, SD,
    ADDIW, SLLIW, SRLIW, SRAIW,
    ADDW, SUBW, SLLW, SRLW, SRAW,

    // RV32m instructions
    MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU,

    // RV64m instructions
    MULW, DIVW, DIVUW, REMW, REMUW,

    // RV32a instructions
    LR_W, SC_W,
    AMOSWAP_W, AMOADD_W, AMOXOR_W, AMOAND_W, AMOOR_W, AMOMIN_W, AMOMAX_W, AMOMINU_W, AMOMAXU_W,

    // RV64a instructions
    LR_D, SC_D,
    AMOSWAP_D, AMOADD_D, AMOXOR_D, AMOAND_D, AMOOR_D, AMOMIN_D, AMOMAX_D, AMOMINU_D, AMOMAXU_D,

    // CSR instructions
    CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI,

    // Privileged Instructions
    WFI, MRET, SRET, SFENCE_VMA
};
// clang-format on

struct DecodedInstr
{
    Op op = Op::INVALID;
    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    int64_t imm = 0;
};
} // namespace riscv_emu::cpu
