//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>
#include <utility>

namespace riscv_emu::cpu {

//--------------------------------------------
// Exceptions and Traps
//--------------------------------------------
enum class ExceptionType : uint64_t
{
    INSTR_ADDR_MISALIGNED = 0,
    INSTR_ACCESS_FAULT = 1,
    ILLEGAL_INSTRUCTION = 2,
    BREAKPOINT = 3,
    LOAD_ADDR_MISALIGNED = 4,
    LOAD_ACCESS_FAULT = 5,
    STORE_AMO_ADDR_MISALIGNED = 6,
    STORE_AMO_ACCESS_FAULT = 7,
    ECALL_U = 8,
    ECALL_S = 9,
    ECALL_M = 11,
    INSTR_PAGE_FAULT = 12,
    LOAD_PAGE_FAULT = 13,
    STORE_PAGE_FAULT = 15
};

enum class InterruptType : uint64_t
{
    MACHINE_SOFTWARE = 3,
    MACHINE_TIMER = 7,
    MACHINE_EXTERNAL = 11,

    SUPERVISOR_SOFTWARE = 1,
    SUPERVISOR_TIMER = 5,
    SUPERVISOR_EXTERNAL = 9
};

constexpr uint64_t operator<<(const uint64_t lhs, const InterruptType rhs)
{
    return lhs << std::to_underlying(rhs);
}

// Simultaneous interrupt priority per the privileged spec: external >
// software > timer, M > S
inline constexpr InterruptType IRQ_PRIORITY[] = {
    InterruptType::MACHINE_EXTERNAL,
    InterruptType::SUPERVISOR_EXTERNAL,
    InterruptType::MACHINE_SOFTWARE,
    InterruptType::SUPERVISOR_SOFTWARE,
    InterruptType::MACHINE_TIMER,
    InterruptType::SUPERVISOR_TIMER,
};

struct TrapCause
{
    uint64_t code;
    bool is_interrupt;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    TrapCause(const ExceptionType e)
    {
        code = std::to_underlying(e);
        is_interrupt = false;
    }

    // ReSharper disable once CppNonExplicitConvertingConstructor
    TrapCause(const InterruptType i)
    {
        code = std::to_underlying(i);
        is_interrupt = true;
    }
};

enum class PrivilegeLevel
{
    U = 0,
    S = 1,
    M = 3
};
} // namespace riscv_emu::cpu
