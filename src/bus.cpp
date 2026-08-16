//
// Created by anish on 4/12/2026.
//

#include "bus.h"
#include "ram.h"

using namespace riscv_emu::mem_io;

bus::bus(ram* mem_ptr) : dram(mem_ptr)
{
    hart_reservations.push_back(hart_res_entry{});
}

template <UintFamily T>
T bus::load(const uint64_t addr) const
{
    return dram->read(addr, sizeof(T));
}

template <UintFamily T>
void bus::store(const uint64_t addr, const T data)
{
    clear_addr_reservations(addr, sizeof(T));
    dram->write(addr, data, sizeof(T));
}

template <UintFamily T>
T bus::load_reserved(const uint64_t addr, const size_t hart_id)
{
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "LR is only supported for 32-bit and 64-bit widths");
    reserve_addr(addr, sizeof(T), hart_id);
    return load<T>(addr);
}

template <UintFamily T>
bool bus::store_conditional(const uint64_t addr, const T data, const size_t hart_id)
{
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "SC is only supported for 32-bit and 64-bit widths");
    const bool ok = holds_reservation(addr, sizeof(T), hart_id);
    hart_reservations[hart_id].invalidate();
    if (ok) {
        store<T>(addr, data);
    }
    return ok;
}

void bus::reserve_addr(const uint64_t addr, const uint8_t size, const size_t hart_id)
{
    hart_reservations[hart_id] = {.addr = addr, .size = size};
}

bool bus::holds_reservation(const uint64_t addr, const uint8_t size, const size_t hart_id) const
{
    const auto& res = hart_reservations[hart_id];

    if (!res.valid()) {
        return false;
    }

    return res.addr == addr && res.size == size;
}

void bus::clear_addr_reservations(const uint64_t addr, const uint8_t size)
{
    for (auto& res : hart_reservations) {
        if (!res.valid()) {
            continue;
        }

        if (addr - res.addr < res.size || res.addr - addr < size) {
            res.invalidate();
        }
    }
}

namespace {

using namespace riscv_emu;

template <std::unsigned_integral U>
U amo_compute(const amo_type type, U data, U old_value)
{
    using S = std::make_signed_t<U>;
    using enum amo_type;

    switch (type) {
    case SWAP:
        return data;
    case ADD:
        return static_cast<U>(data + old_value);
    case XOR:
        return data ^ old_value;
    case AND:
        return data & old_value;
    case OR:
        return data | old_value;
    case MIN:
        return std::min(static_cast<S>(data), static_cast<S>(old_value));
    case MAX:
        return std::max(static_cast<S>(data), static_cast<S>(old_value));
    case MINU:
        return std::min(data, old_value);
    case MAXU:
        return std::max(data, old_value);
    }
    std::unreachable();
}
} // namespace

template <UintFamily T>
T bus::handle_amo(const amo_type type, const uint64_t addr, const T data)
{
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "AMO op must be 32 or 64 bit");

    const T old_value = load<T>(addr);
    auto new_value = amo_compute<T>(type, data, old_value);
    store<T>(addr, new_value);
    return old_value;
}

// Standard memory access (8, 16, 32, 64-bit)
#define INST_BUS_MEM(T)                                                                                                \
    template T bus::load<T>(uint64_t) const;                                                                           \
    template void bus::store<T>(uint64_t, T);

INST_BUS_MEM(uint8_t)
INST_BUS_MEM(uint16_t)
INST_BUS_MEM(uint32_t)
INST_BUS_MEM(uint64_t)

#undef INST_BUS_MEM

// Atomic & LR/SC operations (only 32, 64-bit)
#define INST_BUS_ATOMIC(T)                                                                                             \
    template T bus::load_reserved<T>(uint64_t, size_t);                                                                \
    template bool bus::store_conditional<T>(uint64_t, T, size_t);                                                      \
    template T bus::handle_amo<T>(amo_type, uint64_t, T);

INST_BUS_ATOMIC(uint32_t)
INST_BUS_ATOMIC(uint64_t)

#undef INST_BUS_ATOMIC
