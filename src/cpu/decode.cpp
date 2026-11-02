//
// Created by anish on 3/29/2026.
//

#include "cpu/decode.h"

#include "common/utils.h"

namespace riscv_emu::cpu {
using namespace riscv_emu;

// clang-format off
static constexpr DecodedInstr invalid_instr()
{
    return {.op = Op::INVALID};
}

static constexpr uint8_t get_rd(const uint32_t raw)
{
    return u8(raw >> 7 & 0x1F);
}

static constexpr uint8_t get_rs1(const uint32_t raw)
{
    return u8(raw >> 15 & 0x1F);
}

static constexpr uint8_t get_rs2(const uint32_t raw)
{
    return u8(raw >> 20 & 0x1F);
}

static constexpr uint8_t get_funct3(const uint32_t raw)
{
    return u8(raw >> 12 & 0x7);
}

static constexpr uint8_t get_funct7(const uint32_t raw)
{
    return u8(raw >> 25 & 0x7F);
}

static constexpr uint8_t get_funct5(const uint32_t raw)
{
    return u8(raw >> 27 & 0x1F);
}

static constexpr uint8_t get_funct6(const uint32_t raw)
{
    return u8(raw >> 26 & 0x3F);
}

static constexpr Op resolve_op(const uint32_t raw)
{
    using enum Op;
    switch (get_funct7(raw) << 3 | get_funct3(raw)) {
    case 0b0000000'000: return ADD;
    case 0b0100000'000: return SUB;
    case 0b0000000'001: return SLL;
    case 0b0000000'010: return SLT;
    case 0b0000000'011: return SLTU;
    case 0b0000000'100: return XOR;
    case 0b0000000'101: return SRL;
    case 0b0100000'101: return SRA;
    case 0b0000000'110: return OR;
    case 0b0000000'111: return AND;
    case 0b0000001'000: return MUL;
    case 0b0000001'001: return MULH;
    case 0b0000001'010: return MULHSU;
    case 0b0000001'011: return MULHU;
    case 0b0000001'100: return DIV;
    case 0b0000001'101: return DIVU;
    case 0b0000001'110: return REM;
    case 0b0000001'111: return REMU;
    default:            return INVALID;
    }
}

static constexpr Op resolve_op_32(const uint32_t raw)
{
    using enum Op;
    switch (get_funct7(raw) << 3 | get_funct3(raw)) {
    case 0b0000000'000: return ADDW;
    case 0b0100000'000: return SUBW;
    case 0b0000000'001: return SLLW;
    case 0b0000000'101: return SRLW;
    case 0b0100000'101: return SRAW;
    case 0b0000001'000: return MULW;
    case 0b0000001'100: return DIVW;
    case 0b0000001'101: return DIVUW;
    case 0b0000001'110: return REMW;
    case 0b0000001'111: return REMUW;
    default:            return INVALID;
    }
}

static constexpr Op resolve_amo(const uint32_t raw)
{
    using enum Op;
    switch (get_funct5(raw) << 3 | get_funct3(raw)) {
    case 0b00010'010: return LR_W;
    case 0b00010'011: return LR_D;
    case 0b00011'010: return SC_W;
    case 0b00011'011: return SC_D;
    case 0b00001'010: return AMOSWAP_W;
    case 0b00001'011: return AMOSWAP_D;
    case 0b00000'010: return AMOADD_W;
    case 0b00000'011: return AMOADD_D;
    case 0b00100'010: return AMOXOR_W;
    case 0b00100'011: return AMOXOR_D;
    case 0b01100'010: return AMOAND_W;
    case 0b01100'011: return AMOAND_D;
    case 0b01000'010: return AMOOR_W;
    case 0b01000'011: return AMOOR_D;
    case 0b10000'010: return AMOMIN_W;
    case 0b10000'011: return AMOMIN_D;
    case 0b10100'010: return AMOMAX_W;
    case 0b10100'011: return AMOMAX_D;
    case 0b11000'010: return AMOMINU_W;
    case 0b11000'011: return AMOMINU_D;
    case 0b11100'010: return AMOMAXU_W;
    case 0b11100'011: return AMOMAXU_D;
    default:          return INVALID;
    }
}

static constexpr Op resolve_op_imm(const uint32_t raw)
{
    using enum Op;
    switch (get_funct3(raw)) {
    case 0b000: return ADDI;
    case 0b010: return SLTI;
    case 0b011: return SLTIU;
    case 0b100: return XORI;
    case 0b110: return ORI;
    case 0b111: return ANDI;
    default:    break; // shifts (001, 101) below
    }

    switch (get_funct6(raw) << 3 | get_funct3(raw)) {
    case 0b000000'001: return SLLI;
    case 0b000000'101: return SRLI;
    case 0b010000'101: return SRAI;
    default:           return INVALID;
    }
}

static constexpr Op resolve_op_imm_32(const uint32_t raw)
{
    using enum Op;
    if (get_funct3(raw) == 0b000) {
        return ADDIW;
    }

    switch (get_funct7(raw) << 3 | get_funct3(raw)) {
    case 0b0000000'001: return SLLIW;
    case 0b0000000'101: return SRLIW;
    case 0b0100000'101: return SRAIW;
    default:            return INVALID;
    }
}

static constexpr Op resolve_load(const uint32_t raw)
{
    using enum Op;
    switch (get_funct3(raw)) {
    case 0b000: return LB;
    case 0b001: return LH;
    case 0b010: return LW;
    case 0b011: return LD;
    case 0b100: return LBU;
    case 0b101: return LHU;
    case 0b110: return LWU;
    default:    return INVALID;
    }
}

static constexpr Op resolve_misc_mem(const uint32_t raw)
{
    using enum Op;
    switch (get_funct3(raw)) {
    case 0b000: return FENCE;
    case 0b001: return FENCE_I;
    default:    return INVALID;
    }
}

static constexpr Op resolve_csr(const uint32_t raw)
{
    using enum Op;
    switch (get_funct3(raw)) {
    case 0b001: return CSRRW;
    case 0b010: return CSRRS;
    case 0b011: return CSRRC;
    case 0b101: return CSRRWI;
    case 0b110: return CSRRSI;
    case 0b111: return CSRRCI;
    default:    return INVALID;
    }
}

static constexpr Op resolve_priv(const uint32_t raw)
{
    using enum Op;
    if (get_funct7(raw) == 0b0001001) {
        return SFENCE_VMA; // rs2 is an operand for this
    }

    switch (get_funct7(raw) << 5 | get_rs2(raw)) { // imm[11:0]
    case 0b0000000'00000: return ECALL;
    case 0b0000000'00001: return EBREAK;
    case 0b0001000'00010: return SRET;
    case 0b0011000'00010: return MRET;
    case 0b0001000'00101: return WFI;
    default:              return INVALID;
    }
}

static constexpr Op resolve_store(const uint32_t raw)
{
    using enum Op;
    switch (get_funct3(raw)) {
    case 0b000: return SB;
    case 0b001: return SH;
    case 0b010: return SW;
    case 0b011: return SD;
    default:    return INVALID;
    }
}

static constexpr Op resolve_branch(const uint32_t raw)
{
    using enum Op;
    switch (get_funct3(raw)) {
    case 0b000: return BEQ;
    case 0b001: return BNE;
    case 0b100: return BLT;
    case 0b101: return BGE;
    case 0b110: return BLTU;
    case 0b111: return BGEU;
    default:    return INVALID;
    }
}

static constexpr DecodedInstr build_rtype(const Op instr, const uint32_t raw)
{
    return {.op = instr, .rd = get_rd(raw), .rs1 = get_rs1(raw), .rs2 = get_rs2(raw)};
}

static constexpr DecodedInstr build_itype(const Op instr, const uint32_t raw)
{
    return {.op = instr, .rd = get_rd(raw), .rs1 = get_rs1(raw), .imm = i32(raw) >> 20};
}

static constexpr DecodedInstr build_stype(const Op instr, const uint32_t raw)
{
    const auto s = i32(raw);
    return {.op = instr,
            .rs1 = get_rs1(raw),
            .rs2 = get_rs2(raw),
            .imm = (s >> 25) << 5 | (s >> 7 & 0x1F)};   // imm[11:5] | imm[4:0]
}

static constexpr DecodedInstr build_btype(const Op instr, const uint32_t raw)
{
    const auto s = i32(raw);
    return {.op = instr,
            .rs1 = get_rs1(raw),
            .rs2 = get_rs2(raw),
            .imm = (s >> 31) << 12          // imm[12] (sign)
                 | (s >> 7 & 0x1) << 11     // imm[11]
                 | (s >> 25 & 0x3F) << 5    // imm[10:5]
                 | (s >> 8 & 0xF) << 1};    // imm[4:1]
}

static constexpr DecodedInstr build_utype(const Op instr, const uint32_t raw)
{
    const auto s = i32(raw);
    return {.op = instr,
            .rd = get_rd(raw),
            .imm = (s >> 12) << 12};        // imm[31:12], low 12 bits zero
}

static constexpr DecodedInstr build_jtype(const Op instr, const uint32_t raw)
{
    const auto s = i32(raw);
    return {.op = instr,
            .rd = get_rd(raw),
            .imm = (s >> 31) << 20              // imm[20] (sign)
                 | (s >> 12 & 0xFF) << 12       // imm[19:12]
                 | (s >> 20 & 0x1) << 11        // imm[11]
                 | (s >> 21 & 0x3FF) << 1};     // imm[10:1]
}

static constexpr DecodedInstr decode_op(const uint32_t raw)
{
    return build_rtype(resolve_op(raw), raw);
}

static constexpr DecodedInstr decode_op_32(const uint32_t raw)
{
    return build_rtype(resolve_op_32(raw), raw);
}

static constexpr DecodedInstr decode_amo(const uint32_t raw)
{
    auto instr = build_rtype(resolve_amo(raw), raw);

    if (instr.op == Op::LR_D || instr.op == Op::LR_W) {
        if (instr.rs2 != 0) {
            instr.op = Op::INVALID;
        }
    }

    return instr;
}

static constexpr DecodedInstr decode_op_imm(const uint32_t raw)
{
    auto out = build_itype(resolve_op_imm(raw), raw);
    const uint8_t funct3 = get_funct3(raw);

    if (funct3 == 0b001 || funct3 == 0b101) {
        out.imm &= 0x3F; // shamt[5:0]
    }
    return out;
}

static constexpr DecodedInstr decode_op_imm_32(const uint32_t raw)
{
    auto out = build_itype(resolve_op_imm_32(raw), raw);

    if (get_funct3(raw) != 0b000) {
        out.imm &= 0x1F; // shamt[4:0]
    }
    return out;
}

static constexpr DecodedInstr decode_load(const uint32_t raw)
{
    return build_itype(resolve_load(raw), raw);
}

static constexpr DecodedInstr decode_jalr(const uint32_t raw)
{
    if (get_funct3(raw) != 0b000) {
        return invalid_instr();
    }

    return build_itype(Op::JALR, raw);
}

static constexpr DecodedInstr decode_misc_mem(const uint32_t raw)
{
    return build_itype(resolve_misc_mem(raw), raw);
}

static constexpr DecodedInstr decode_system(const uint32_t raw)
{
    if (get_funct3(raw) != 0b000) {
        auto out = build_itype(resolve_csr(raw), raw);
        out.imm &= 0xFFF;                   // CSR address is unsigned
        return out;
    }

    const Op op = resolve_priv(raw);

    if (get_rd(raw) != 0) {
        return invalid_instr();
    }

    // rs1 = vaddr, rs2 = asid
    if (op == Op::SFENCE_VMA) {
        return build_rtype(op, raw);
    }

    if (get_rs1(raw) != 0) {
        return invalid_instr();
    }

    return {.op = op};
}

static constexpr DecodedInstr decode_store(const uint32_t raw)
{
    return build_stype(resolve_store(raw), raw);
}

static constexpr DecodedInstr decode_branch(const uint32_t raw)
{
    return build_btype(resolve_branch(raw), raw);
}

static constexpr DecodedInstr decode_lui(const uint32_t raw)
{
    return build_utype(Op::LUI, raw);
}

static constexpr DecodedInstr decode_auipc(const uint32_t raw)
{
    return build_utype(Op::AUIPC, raw);
}

static constexpr DecodedInstr decode_jal(const uint32_t raw)
{
    return build_jtype(Op::JAL, raw);
}

DecodedInstr decode(const uint32_t raw)
{
    const auto op = static_cast<Opcode>(raw & 0x7F);
    using enum Opcode;
    switch (op) {
    case OP:        return decode_op(raw);
    case OP_32:     return decode_op_32(raw);
    case AMO:       return decode_amo(raw);
    case OP_IMM:    return decode_op_imm(raw);
    case OP_IMM_32: return decode_op_imm_32(raw);
    case LOAD:      return decode_load(raw);
    case JALR:      return decode_jalr(raw);
    case MISC_MEM:  return decode_misc_mem(raw);
    case SYSTEM:    return decode_system(raw);
    case STORE:     return decode_store(raw);
    case BRANCH:    return decode_branch(raw);
    case LUI:       return decode_lui(raw);
    case AUIPC:     return decode_auipc(raw);
    case JAL:       return decode_jal(raw);
    default:        return invalid_instr();
    }
}
} // namespace riscv_emu::cpu
