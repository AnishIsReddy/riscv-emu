//
// Created by anish on 4/12/2026.
//

#include "bus.h"
#include "device_map.h"
#include "ram.h"

using namespace riscv_emu::mem_io;

Bus::Bus(DeviceMap&& map) : device_map(std::move(map))
{
}

template <UintFamily T>
std::optional<T> Bus::load(const uint64_t addr) const
{
    return device_map.try_load(addr, sizeof(T));
}

template <UintFamily T>
bool Bus::store(const uint64_t addr, const T data)
{
    clear_addr_reservations(addr, sizeof(T));
    return device_map.try_store(addr, data, sizeof(T));
}

template <UintFamily T>
std::optional<T> Bus::load_reserved(const uint64_t addr, const size_t hart_id)
{
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "LR is only supported for 32-bit and 64-bit widths");

    auto load_result = load<T>(addr);
    if (load_result.has_value()) {
        reserve_addr(addr, sizeof(T), hart_id);
    }
    return load_result;
}

template <UintFamily T>
std::optional<bool> Bus::store_conditional(const uint64_t addr, const T data, const size_t hart_id)
{
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "SC is only supported for 32-bit and 64-bit widths");
    const bool ok = holds_reservation(addr, sizeof(T), hart_id);
    hart_reservations[hart_id].invalidate();
    if (ok) {
        const bool store_ok = store<T>(addr, data);
        if (!store_ok) {
            return std::nullopt;
        }
    }
    return ok;
}

void Bus::reserve_addr(const uint64_t addr, const uint8_t size, const size_t hart_id)
{
    hart_reservations[hart_id] = {.addr = addr, .size = size};
}

bool Bus::holds_reservation(const uint64_t addr, const uint8_t size, const size_t hart_id) const
{
    const auto& res = hart_reservations[hart_id];

    if (!res.valid()) {
        return false;
    }

    return res.addr == addr && res.size == size;
}

void Bus::clear_addr_reservations(const uint64_t addr, const uint8_t size)
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

template <UintFamily T>
std::optional<T> Bus::handle_amo(const AmoType type, const uint64_t addr, const T data)
{
    static_assert(sizeof(T) == 4 || sizeof(T) == 8, "AMO op must be 32 or 64 bit");

    auto load_result = load<T>(addr);
    if (!load_result.has_value()) {
        return std::nullopt;
    }
    const T old_value = load_result.value();
    T new_value;

    switch (type) {
        using S = std::make_signed_t<T>;
        using enum AmoType;
    case SWAP:
        new_value = data;
        break;
    case ADD:
        new_value = static_cast<T>(data + old_value);
        break;
    case XOR:
        new_value = data ^ old_value;
        break;
    case AND:
        new_value = data & old_value;
        break;
    case OR:
        new_value = data | old_value;
        break;
    case MIN:
        new_value = std::min(static_cast<S>(data), static_cast<S>(old_value));
        break;
    case MAX:
        new_value = std::max(static_cast<S>(data), static_cast<S>(old_value));
        break;
    case MINU:
        new_value = std::min(data, old_value);
        break;
    case MAXU:
        new_value = std::max(data, old_value);
        break;
    }

    const bool store_ok = store<T>(addr, new_value);
    if (!store_ok) {
        return std::nullopt;
    }

    return old_value;
}

// Standard memory access (8, 16, 32, 64-bit)
#define INST_BUS_MEM(T)                                                                                                \
    template std::optional<T> Bus::load<T>(uint64_t) const;                                                            \
    template bool Bus::store<T>(uint64_t, T);

INST_BUS_MEM(uint8_t)
INST_BUS_MEM(uint16_t)
INST_BUS_MEM(uint32_t)
INST_BUS_MEM(uint64_t)

#undef INST_BUS_MEM

// Atomic & LR/SC operations (only 32, 64-bit)
#define INST_BUS_ATOMIC(T)                                                                                             \
    template std::optional<T> Bus::load_reserved<T>(uint64_t, size_t);                                                 \
    template std::optional<bool> Bus::store_conditional<T>(uint64_t, T, size_t);                                       \
    template std::optional<T> Bus::handle_amo<T>(AmoType, uint64_t, T);

INST_BUS_ATOMIC(uint32_t)
INST_BUS_ATOMIC(uint64_t)

#undef INST_BUS_ATOMIC
