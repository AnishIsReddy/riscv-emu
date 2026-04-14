//
// Created by anish on 3/29/2026.
//

#ifndef RISCV_EMU_DEFS_H
#define RISCV_EMU_DEFS_H

namespace riscv_emu
{
    constexpr uint64_t ADDR_WIDTH = 64; // 64-bit system
    constexpr uint64_t MEM_SIZE = 128 * 1024 * 1024; // 128 MiB RAM
    constexpr uint64_t REG_COUNT = 32;

    enum class opcode
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
        MISC_MEM = 0b0001111
    };

    enum class instr_type
    {
        // RV32i instructions
        INVALID, LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU, LB, LH, LW, LBU,
        LHU, SB, SH, SW, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, ADD, SUB,
        SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND, FENCE, FENCE_TSO, PAUSE, ECALL, EBREAK,

        // RV64i instructions
        LWU, LD, SD, ADDIW, SLLIW, SRLIW, SRAIW, ADDW, SUBW, SLLW, SRLW, SRAW,

        // RV32M instructions
        MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU,

        // RV64M instructions
        MULW, DIVW, DIVUW, REMW, REMUW
    };

    struct instr_info
    {
        int64_t imm = 0;
        instr_type itype = instr_type::INVALID;
        uint8_t rd = 0;
        uint8_t rs1 = 0;
        uint8_t rs2 = 0;
    };

    enum class exec_result_type
    {
        NO_UPDATE,
        UPDATE_RD_FROM_VAL,
        UPDATE_RD_FROM_MEM,
        UPDATE_MEM_FROM_VAL,
    };

    struct exec_result
    {
        exec_result_type type;

        uint8_t rd;
        uint64_t val;
        uint64_t mem_addr;
        uint8_t mem_size;
        bool zero_extend_val;

        uint64_t new_pc;
    };


    //--------------------------------------------
    // Helper Funcs
    //--------------------------------------------

    inline bool is_shift_imm_instr(const instr_type itype)
    {
        return itype == instr_type::SLLI || itype == instr_type::SRLI || itype == instr_type::SRAI
            || itype == instr_type::SLLIW || itype == instr_type::SRLIW || itype == instr_type::SRAIW;
    }

    // Param "bits" is the true size of "value". Anything below it will not be affected.
    // Think of "bits" like .size() if value was a std::vector.
    inline int64_t sign_extend(const uint64_t value, const uint8_t bits)
    {
        return static_cast<int64_t>(value) << (64 - bits) >> (64 - bits);
    }

    inline uint64_t arith_shift_right(const uint64_t value, const uint8_t bits)
    {
        return static_cast<int64_t>(value) >> bits;
    }

    inline uint64_t logical_shift_right(const uint64_t value, const uint8_t bits)
    {
        return static_cast<uint64_t>(value) >> bits;
    }

}
#endif //RISCV_EMU_DEFS_H
