//
// Created by anish on 3/29/2026.
//

#include "decode.h"
#include "defs.h"

using namespace riscv_emu;

inline instr_type resolve_instr_type_OP(const uint8_t func3, const uint8_t func7)
{
    using enum instr_type;
    const uint16_t func = func7 << 3 | func3;

    switch (func) {
    case 0b000:
        return ADD;
    case 0b0100000000:
        return SUB;
    case 0b001:
        return SLL;
    case 0b010:
        return SLT;
    case 0b011:
        return SLTU;
    case 0b100:
        return XOR;
    case 0b101:
        return SRL;
    case 0b0100000101:
        return SRA;
    case 0b110:
        return OR;
    case 0b111:
        return AND;
    case 0b1000:
        return MUL;
    case 0b1001:
        return MULH;
    case 0b1010:
        return MULHSU;
    case 0b1011:
        return MULHU;
    case 0b1100:
        return DIV;
    case 0b1101:
        return DIVU;
    case 0b1110:
        return REM;
    case 0b1111:
        return REMU;
    default:
        return INVALID;
    }
}

inline instr_type resolve_instr_type_OP_32(const uint8_t func3, const uint8_t func7)
{
    using enum instr_type;
    const uint16_t func = func7 << 3 | func3;

    switch (func) {
    case 0b000:
        return ADDW;
    case 0b0100000000:
        return SUBW;
    case 0b001:
        return SLLW;
    case 0b101:
        return SRLW;
    case 0b0100000101:
        return SRAW;
    case 0b1000:
        return MULW;
    case 0b1100:
        return DIVW;
    case 0b1101:
        return DIVUW;
    case 0b1110:
        return REMW;
    case 0b1111:
        return REMUW;
    default:
        return INVALID;
    }
}

inline instr_type resolve_instr_type_AMO(const uint8_t func3, const uint8_t func7, const uint8_t rs2)
{
    using enum instr_type;
    const uint8_t func5 = func7 >> 2;

    if (func3 != 0b010 && func3 != 0b011) {
        return INVALID;
    }

    const bool is_double_op = func3 == 0b011;

    auto itype_w = INVALID;
    auto itype_d = INVALID;

    switch (func5) {
    case 0b00010:
        if (rs2 != 0) {
            return INVALID;
        }
        itype_w = LR_W;
        itype_d = LR_D;
        break;
    case 0b00011:
        itype_w = SC_W;
        itype_d = SC_D;
        break;
    case 0b00001:
        itype_w = AMOSWAP_W;
        itype_d = AMOSWAP_D;
        break;
    case 0b00000:
        itype_w = AMOADD_W;
        itype_d = AMOADD_D;
        break;
    case 0b00100:
        itype_w = AMOXOR_W;
        itype_d = AMOXOR_D;
        break;
    case 0b01100:
        itype_w = AMOAND_W;
        itype_d = AMOAND_D;
        break;
    case 0b01000:
        itype_w = AMOOR_W;
        itype_d = AMOOR_D;
        break;
    case 0b10000:
        itype_w = AMOMIN_W;
        itype_d = AMOMIN_D;
        break;
    case 0b10100:
        itype_w = AMOMAX_W;
        itype_d = AMOMAX_D;
        break;
    case 0b11000:
        itype_w = AMOMINU_W;
        itype_d = AMOMINU_D;
        break;
    case 0b11100:
        itype_w = AMOMAXU_W;
        itype_d = AMOMAXU_D;
        break;
    default:
        return INVALID;
    }

    if (is_double_op) {
        return itype_d;
    }

    return itype_w;
}

instr_info decode_rtype(const uint32_t raw)
{
    const auto op = static_cast<opcode>(raw & 0x7F);
    const uint8_t func3 = raw >> 12 & 0x7;
    const uint8_t func7 = raw >> 25 & 0x7F;

    const auto rd = static_cast<uint8_t>(raw >> 7 & 0x1F);
    const auto rs1 = static_cast<uint8_t>(raw >> 15 & 0x1F);
    const auto rs2 = static_cast<uint8_t>(raw >> 20 & 0x1F);

    instr_type itype;

    switch (op) {
    case opcode::OP:
        itype = resolve_instr_type_OP(func3, func7);
        break;
    case opcode::OP_32:
        itype = resolve_instr_type_OP_32(func3, func7);
        break;
    case opcode::AMO:
        itype = resolve_instr_type_AMO(func3, func7, rs2);
        break;
    default:
        itype = instr_type::INVALID;
    }

    return instr_info{
        .itype = itype,
        .rd = rd,
        .rs1 = rs1,
        .rs2 = rs2
    };
}

inline instr_type resolve_instr_type_OP_IMM(const uint8_t func3, const bool use_alt_instr)
{
    using enum instr_type;
    switch (func3) {
    case 0b000:
        return ADDI;
    case 0b001:
        return SLLI;
    case 0b010:
        return SLTI;
    case 0b011:
        return SLTIU;
    case 0b100:
         return XORI;
    case 0b101:
        return use_alt_instr ? SRAI : SRLI;
    case 0b110:
         return ORI;
    case 0b111:
        return ANDI;
    default:
        return INVALID;
    }
}

inline instr_type resolve_instr_type_OP_IMM_32(const uint8_t func3, const bool use_alt_instr)
{
    using enum instr_type;
    switch (func3) {
    case 0b000:
        return ADDIW;
    case 0b001:
        return SLLIW;
    case 0b101:
        return use_alt_instr ? SRAIW : SRLIW;
    default:
        return INVALID;
    }
}

inline instr_type resolve_instr_type_LOAD(const uint8_t func3)
{
    using enum instr_type;
    switch (func3) {
    case 0b000:
        return LB;
    case 0b001:
        return LH;
    case 0b010:
        return LW;
    case 0b011:
        return LD;
    case 0b100:
        return LBU;
    case 0b101:
        return LHU;
    case 0b110:
        return LWU;
    default:
        return INVALID;
    }
}

inline instr_type resolve_instr_type_MISC_MEM(const uint8_t func3)
{
    using enum instr_type;
    switch (func3) {
    case 0b000:
        return FENCE;
    case 0b001:
        return FENCE_I;
    default:
        return INVALID;
    }
}

inline instr_type resolve_instr_type_SYSTEM(const uint64_t imm)
{
    using enum instr_type;
    switch (imm) {
    case 0b0:
        return ECALL;
    case 0b1:
        return EBREAK;
    default:
        return INVALID;
    }
}

instr_info decode_itype(const uint32_t raw)
{
    const auto op = static_cast<opcode>(raw & 0x7F);
    const uint8_t func3 = raw >> 12 & 0x7;
    const bool use_alt_instr = raw >> 30 & 0b1;
    const uint64_t raw_imm = raw >> 20 & 0xFFF;

    instr_type itype;

    switch (op) {
        using enum opcode;

    case OP_IMM:
        itype = resolve_instr_type_OP_IMM(func3, use_alt_instr);
        break;
    case LOAD:
        itype = resolve_instr_type_LOAD(func3);
        break;
    case OP_IMM_32:
        itype = resolve_instr_type_OP_IMM_32(func3, use_alt_instr);
        break;
    case JALR:
        itype = instr_type::JALR;
        break;
    case MISC_MEM:
        itype = resolve_instr_type_MISC_MEM(func3);
        break;
    case SYSTEM:
        itype = resolve_instr_type_SYSTEM(raw_imm);
        break;
    default:
        itype = instr_type::INVALID;
    }

    // calculate imm value
    auto imm = static_cast<int64_t>(raw_imm);
    if (!is_shift_imm_instr(itype)) {
        imm = sign_extend(raw_imm, 12);
    }

    const auto out = instr_info{
        .imm = imm,
        .itype = itype,
        .rd = static_cast<uint8_t>(raw >> 7 & 0x1F),
        .rs1 = static_cast<uint8_t>(raw >> 15 & 0x1F)
    };

    return out;
}

instr_info decode_stype(const uint32_t raw)
{
    const uint8_t func3 = raw >> 12 & 0x7;

    instr_type itype;
    switch (func3) {
    case 0b000:
        itype = instr_type::SB;
        break;
    case 0b001:
        itype = instr_type::SH;
        break;
    case 0b010:
        itype = instr_type::SW;
        break;
    case 0b011:
        itype = instr_type::SD;
        break;
    default:
        return instr_info{.itype = instr_type::INVALID};
    }

    int64_t imm = ((raw >> 25 & 0x7F) << 5) | (raw >> 7 & 0x1F);
    imm = sign_extend(imm, 12);

    const auto out = instr_info{
        .imm = imm,
        .itype = itype,
        .rs1 = static_cast<uint8_t>(raw >> 15 & 0x1F),
        .rs2 = static_cast<uint8_t>(raw >> 20 & 0x1F)
    };

    return out;
}

instr_info decode_btype(const uint32_t raw)
{
    const uint8_t func3 = raw >> 12 & 0x7;

    instr_type itype;
    switch (func3) {
    case 0b000:
        itype = instr_type::BEQ;
        break;
    case 0b001:
        itype = instr_type::BNE;
        break;
    case 0b100:
        itype = instr_type::BLT;
        break;
    case 0b101:
        itype = instr_type::BGE;
        break;
    case 0b110:
        itype = instr_type::BLTU;
        break;
    case 0b111:
        itype = instr_type::BGEU;
        break;
    default:
        return instr_info{.itype = instr_type::INVALID};
    }

    int64_t imm = ((raw >> 31) & 1) << 12
        | ((raw >> 7) & 1) << 11
        | ((raw >> 25) & 0x3F) << 5
        | ((raw >> 8) & 0xF) << 1;
    if (raw >> 31 & 1) {
        imm |= ~static_cast<int64_t>(0x1FFF);
    }

    const auto out = instr_info{
        .imm = imm,
        .itype = itype,
        .rs1 = static_cast<uint8_t>(raw >> 15 & 0x1F),
        .rs2 = static_cast<uint8_t>(raw >> 20 & 0x1F)
    };

    return out;
}

instr_info decode_utype(const uint32_t raw)
{
    instr_type itype;
    const auto op = static_cast<opcode>(raw & 0x7F);

    if (op == opcode::LUI) {
        itype = instr_type::LUI;
    }
    else if (op == opcode::AUIPC) {
        itype = instr_type::AUIPC;
    }
    else {
        return instr_info{.itype = instr_type::INVALID};
    }

    const auto imm = sign_extend(raw & 0xFFFFF000, 32);

    const auto out = instr_info{
        .imm = imm,
        .itype = itype,
        .rd = static_cast<uint8_t>(raw >> 7 & 0x1F)
    };

    return out;
}

instr_info decode_jtype(const uint32_t raw)
{
    int64_t imm = ((raw >> 31) & 1) << 20
        | ((raw >> 12) & 0xFF) << 12
        | ((raw >> 20) & 1) << 11
        | ((raw >> 21) & 0x3FF) << 1;
    imm = sign_extend(imm, 21);

    const auto out = instr_info{
        .imm = imm,
        .itype = instr_type::JAL,
        .rd = static_cast<uint8_t>(raw >> 7 & 0x1F)
    };

    return out;
}

instr_info riscv_emu::decode(const uint32_t raw)
{
    const auto op = static_cast<opcode>(raw & 0x7F);

    switch (op) {
        using enum opcode;
    case OP:
    case OP_32:
    case AMO:
        return decode_rtype(raw);
    case OP_IMM:
    case OP_IMM_32:
    case LOAD:
    case JALR:
    case MISC_MEM:
    case SYSTEM:
        return decode_itype(raw);
    case STORE:
        return decode_stype(raw);
    case BRANCH:
        return decode_btype(raw);
    case LUI:
    case AUIPC:
        return decode_utype(raw);
    case JAL:
        return decode_jtype(raw);
    default:
        return instr_info{.itype = instr_type::INVALID};
    }
}
