//
// Created by anish on 3/29/2026.
//

#ifndef RISCV_EMU_DEFS_H
#define RISCV_EMU_DEFS_H

#include <cstdint>
#include <iostream>
#include <utility>
#include <variant>

namespace riscv_emu {
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
    MISC_MEM = 0b0001111,
    AMO = 0b0101111
};

enum class instr_type
{
    // RV32i instructions
    INVALID,
    LUI,
    AUIPC,
    JAL,
    JALR,
    BEQ,
    BNE,
    BLT,
    BGE,
    BLTU,
    BGEU,
    LB,
    LH,
    LW,
    LBU,
    LHU,
    SB,
    SH,
    SW,
    ADDI,
    SLTI,
    SLTIU,
    XORI,
    ORI,
    ANDI,
    SLLI,
    SRLI,
    SRAI,
    ADD,
    SUB,
    SLL,
    SLT,
    SLTU,
    XOR,
    SRL,
    SRA,
    OR,
    AND,
    FENCE,
    FENCE_TSO,
    PAUSE,
    ECALL,
    EBREAK,

    // rv32 Zifencei
    FENCE_I,

    // RV64i instructions
    LWU,
    LD,
    SD,
    ADDIW,
    SLLIW,
    SRLIW,
    SRAIW,
    ADDW,
    SUBW,
    SLLW,
    SRLW,
    SRAW,

    // RV32m instructions
    MUL,
    MULH,
    MULHSU,
    MULHU,
    DIV,
    DIVU,
    REM,
    REMU,

    // RV64m instructions
    MULW,
    DIVW,
    DIVUW,
    REMW,
    REMUW,

    // RV32a instructions
    LR_W,
    SC_W,
    AMOSWAP_W,
    AMOADD_W,
    AMOXOR_W,
    AMOAND_W,
    AMOOR_W,
    AMOMIN_W,
    AMOMAX_W,
    AMOMINU_W,
    AMOMAXU_W,

    // RV64a instructions
    LR_D,
    SC_D,
    AMOSWAP_D,
    AMOADD_D,
    AMOXOR_D,
    AMOAND_D,
    AMOOR_D,
    AMOMIN_D,
    AMOMAX_D,
    AMOMINU_D,
    AMOMAXU_D,

    // CSR instructions
    CSRRW,
    CSRRS,
    CSRRC,
    CSRRWI,
    CSRRSI,
    CSRRCI,

    // Trap Return
    SRET,
    MRET,

    // Interrupts
    WFI
};

struct instr_info
{
    int64_t imm = 0;
    instr_type op_type = instr_type::INVALID;
    uint8_t rd = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
};

//--------------------------------------------
// Atomic Memory Ops
//--------------------------------------------
enum class amo_type
{
    SWAP,
    ADD,
    XOR,
    AND,
    OR,
    MIN,
    MAX,
    MINU,
    MAXU
};

inline amo_type to_amo_type(const instr_type instr)
{
    switch (instr) {
        using enum instr_type;

    case AMOSWAP_W:
    case AMOSWAP_D:
        return amo_type::SWAP;

    case AMOADD_W:
    case AMOADD_D:
        return amo_type::ADD;

    case AMOXOR_W:
    case AMOXOR_D:
        return amo_type::XOR;

    case AMOAND_W:
    case AMOAND_D:
        return amo_type::AND;

    case AMOOR_W:
    case AMOOR_D:
        return amo_type::OR;

    case AMOMIN_W:
    case AMOMIN_D:
        return amo_type::MIN;

    case AMOMAX_W:
    case AMOMAX_D:
        return amo_type::MAX;

    case AMOMINU_W:
    case AMOMINU_D:
        return amo_type::MINU;

    case AMOMAXU_W:
    case AMOMAXU_D:
        return amo_type::MAXU;

    default:
        std::cerr << "to_amo_type: not an AMO instruction\n";
        std::abort();
    }
}

enum class csr_op_type
{
    RW,
    RS,
    RC
};

inline csr_op_type to_csr_op_type(const instr_type instr)
{
    switch (instr) {
        using enum instr_type;
    case CSRRW:
    case CSRRWI:
        return csr_op_type::RW;
    case CSRRS:
    case CSRRSI:
        return csr_op_type::RS;
    case CSRRC:
    case CSRRCI:
        return csr_op_type::RC;
    default:
        std::cerr << "to_csr_op_type: not an CSRRW instruction\n";
        std::abort();
    }
}

//--------------------------------------------
// Exceptions and Traps
//--------------------------------------------
enum class exception_type : uint64_t
{
    INSTR_ADDR_MISALIGNED = 0,
    INSTR_ACCESS_FAULT = 1,
    ILLEGAL_INSTRUCTION = 2,
    BREAKPOINT = 3,
    LOAD_ADDR_MISALIGNED = 4,
    LOAD_ACCESS_FAULT = 5,
    STORE_ADDR_MISALIGNED = 6,
    STORE_ACCESS_FAULT = 7,
    ECALL_U = 8,
    ECALL_S = 9,
    ECALL_M = 11,
    INSTR_PAGE_FAULT = 12,
    LOAD_PAGE_FAULT = 13,
    STORE_PAGE_FAULT = 15
};

enum class interrupt_type : uint64_t
{
    MACHINE_SOFTWARE = 3,
    MACHINE_TIMER = 7,
    MACHINE_EXTERNAL = 11,

    SUPERVISOR_SOFTWARE = 1,
    SUPERVISOR_TIMER = 5,
    SUPERVISOR_EXTERNAL = 9
};

constexpr uint64_t operator<<(const uint64_t lhs, const interrupt_type rhs)
{
    return lhs << std::to_underlying(rhs);
}

struct trap_cause
{
    uint64_t code;
    bool is_interrupt;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    trap_cause(const exception_type e)
    {
        code = std::to_underlying(e);
        is_interrupt = false;
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    trap_cause(const interrupt_type i)
    {
        code = std::to_underlying(i);
        is_interrupt = true;
    }
};

//--------------------------------------------
// Memory IO
//--------------------------------------------
namespace mem_io {
template <typename T>
concept UintFamily =
    std::same_as<T, uint8_t> || std::same_as<T, uint16_t> || std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

class device
{
  public:
    virtual ~device() = default;
    [[nodiscard]]
    virtual uint64_t read(uint64_t offset, uint8_t size) const = 0;
    virtual void write(uint64_t offset, uint64_t data, uint8_t size) = 0;
};
} // namespace mem_io

//--------------------------------------------
// Instruction Effects
//--------------------------------------------

enum class privilege_level
{
    U = 0,
    S = 1,
    M = 3
};

enum class csr_register : uint16_t
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

struct instr_effect
{
    struct no_effect
    {
    };

    struct update_rd
    {
        uint8_t rd;
        uint64_t value;
    };

    // ReSharper disable once CppTemplateParameterNeverUsed
    template <mem_io::UintFamily T>
    struct load_rd_from_mem
    {
        uint8_t rd;
        uint64_t addr;
        bool sign_ext;
    };

    template <mem_io::UintFamily T>
    struct store_mem
    {
        uint64_t addr;
        T value;
    };

    // ReSharper disable once CppTemplateParameterNeverUsed
    template <mem_io::UintFamily T>
    struct load_reserved
    {
        uint8_t rd;
        uint64_t addr;
        bool sign_ext;
    };

    template <mem_io::UintFamily T>
    struct store_conditional
    {
        uint8_t rd;
        uint64_t addr;
        T value;
    };

    template <mem_io::UintFamily T>
    struct amo_rmw
    {
        uint8_t rd;
        amo_type type;
        uint64_t addr;
        T value;
        bool sign_ext;
    };

    struct csr_rmw
    {
        uint8_t rd;
        csr_op_type type;
        uint16_t addr;
        uint64_t value;
        bool skip_read;
        bool skip_write;
    };

    struct raise_trap
    {
        trap_cause cause;
        uint64_t tval;
    };

    struct trap_return
    {
        privilege_level return_priv;
    };

    struct handle_wfi
    {
    };

    using effect_type =
        std::variant<no_effect, update_rd, load_rd_from_mem<uint8_t>, load_rd_from_mem<uint16_t>,
                     load_rd_from_mem<uint32_t>, load_rd_from_mem<uint64_t>, store_mem<uint8_t>, store_mem<uint16_t>,
                     store_mem<uint32_t>, store_mem<uint64_t>, load_reserved<uint32_t>, load_reserved<uint64_t>,
                     store_conditional<uint32_t>, store_conditional<uint64_t>, amo_rmw<uint32_t>, amo_rmw<uint64_t>,
                     csr_rmw, raise_trap, trap_return, handle_wfi>;

    effect_type effect;
    uint64_t new_pc;
};

//--------------------------------------------
// Helper Funcs
//--------------------------------------------

inline bool is_shift_imm_instr(const instr_type itype)
{
    return itype == instr_type::SLLI ||
           itype == instr_type::SRLI ||
           itype == instr_type::SRAI ||
           itype == instr_type::SLLIW ||
           itype == instr_type::SRLIW ||
           itype == instr_type::SRAIW;
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
} // namespace riscv_emu
#endif // RISCV_EMU_DEFS_H
