//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>
#include <variant>

#include "mem/bus_device.h"
#include "cpu/trap.h"

namespace riscv_emu::cpu {

//--------------------------------------------
// Atomic Memory Ops
//--------------------------------------------
enum class AmoType
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

enum class CsrOpType
{
    RW,
    RS,
    RC
};

//--------------------------------------------
// Instruction Effects
//--------------------------------------------
struct InstrEffect
{
    struct NoEffect
    {
    };

    struct UpdateRd
    {
        uint8_t rd;
        uint64_t value;
    };

    // ReSharper disable once CppTemplateParameterNeverUsed
    template <mem::LoadFamily T>
    struct LoadMem
    {
        uint8_t rd;
        uint64_t addr;
    };

    template <mem::UintFamily T>
    struct StoreMem
    {
        uint64_t addr;
        T value;
    };

    // ReSharper disable once CppTemplateParameterNeverUsed
    template <mem::LoadFamily T>
    struct LoadReserved
    {
        uint8_t rd;
        uint64_t addr;
    };

    template <mem::UintFamily T>
    struct StoreConditional
    {
        uint8_t rd;
        uint64_t addr;
        T value;
    };

    template <mem::UintFamily T>
    struct AmoRmw
    {
        uint8_t rd;
        AmoType type;
        uint64_t addr;
        T value;
    };

    struct CsrRmw
    {
        uint8_t rd;
        CsrOpType type;
        uint16_t addr;
        uint64_t value;
        bool skip_read;
        bool skip_write;
    };

    struct RaiseTrap
    {
        TrapCause cause;
        uint64_t tval;
    };

    struct TrapReturn
    {
        PrivilegeLevel return_priv;
    };

    struct HandleWfi
    {
    };

    using effect_type =
        std::variant<NoEffect, UpdateRd, LoadMem<uint8_t>, LoadMem<uint16_t>, LoadMem<uint32_t>, LoadMem<uint64_t>,
                     LoadMem<int8_t>, LoadMem<int16_t>, LoadMem<int32_t>, StoreMem<uint8_t>, StoreMem<uint16_t>,
                     StoreMem<uint32_t>, StoreMem<uint64_t>, LoadReserved<uint64_t>, LoadReserved<int32_t>,
                     StoreConditional<uint32_t>, StoreConditional<uint64_t>, AmoRmw<uint32_t>, AmoRmw<uint64_t>, CsrRmw,
                     RaiseTrap, TrapReturn, HandleWfi>;

    effect_type effect;
    uint64_t new_pc;
};
} // namespace riscv_emu::cpu
