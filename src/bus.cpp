//
// Created by anish on 4/12/2026.
//

#include <cassert>

#include "bus.h"
#include "ram.h"

using namespace riscv_emu;

bus::bus(ram* mem_ptr) : main_memory(mem_ptr)
{
    hart_reservations.push_back(hart_res_entry{});
}

uint64_t bus::load(const uint64_t addr, const uint8_t size) const
{
    return main_memory->read(addr, size);
}

void bus::store(const uint64_t addr, const uint64_t data, const uint8_t size)
{
    clear_addr_reservations(addr, size);
    main_memory->write(addr, data, size);
}

uint64_t bus::load_reserved(const uint64_t addr, const uint8_t size, const size_t hart_id)
{
    reserve_addr(addr, size, hart_id);
    return load(addr, size);
}

void bus::reserve_addr(const uint64_t addr, const uint8_t size, const size_t hart_id)
{
    hart_reservations[hart_id] = {.addr = addr, .size = size};
}

bool bus::holds_reservation(const uint64_t addr, const uint8_t size, const size_t hart_id) const
{
    const auto & res = hart_reservations[hart_id];

    if (!res.valid()) {
        return false;
    }

    return res.addr == addr && res.size == size;
}

bool bus::store_conditional(const uint64_t addr, const uint64_t data, const uint8_t size, const size_t hart_id)
{
    const bool ok = holds_reservation(addr, size, hart_id);
    hart_reservations[hart_id].invalidate();
    if (ok) {
        store(addr, data, size);
    }
    return ok;
}

void bus::clear_addr_reservations(const uint64_t addr, const uint8_t size)
{
    for (auto & res : hart_reservations) {
        if (!res.valid()) {
            continue;
        }

        if (addr - res.addr < res.size || res.addr - addr < size) {
            res.invalidate();
        }
    }
}

uint64_t bus::handle_amo(const amo_type type, const uint64_t addr, const uint64_t data, const uint8_t size)
{
    assert(size == 4 || size == 8);

    const uint64_t old_value = load(addr, size);
    uint64_t new_value;

    if (size == 8) {
        switch (type) {
            using enum amo_type;

        case SWAP: {
            new_value = data;
            break;
        }

        case ADD: {
            new_value = data + old_value;
            break;
        }

        case XOR: {
            new_value = data ^ old_value;
            break;
        }

        case AND: {
            new_value = data & old_value;
            break;
        }

        case OR: {
            new_value = data | old_value;
            break;
        }

        case MIN: {
            new_value = static_cast<int64_t>(data) < static_cast<int64_t>(old_value) ? data : old_value;
            break;
        }

        case MAX: {
            new_value = static_cast<int64_t>(data) > static_cast<int64_t>(old_value) ? data : old_value;
            break;
        }

        case MINU: {
            new_value = data < old_value ? data : old_value;
            break;
        }

        case MAXU: {
            new_value = data > old_value ? data : old_value;
            break;
        }
        }
    }
    else {
        const auto old_value_32 = static_cast<uint32_t>(old_value);
        const auto data_32 = static_cast<uint32_t>(data);
        uint32_t new_value_32;

        switch (type) {
            using enum amo_type;

        case SWAP: {
            new_value_32 = data_32;
            break;
        }

        case ADD: {
            new_value_32 = data_32 + old_value_32;
            break;
        }

        case XOR: {
            new_value_32 = data_32 ^ old_value_32;
            break;
        }

        case AND: {
            new_value_32 = data_32 & old_value_32;
            break;
        }

        case OR: {
            new_value_32 = data_32 | old_value_32;
            break;
        }

        case MIN: {
            new_value_32 = static_cast<int32_t>(data_32) < static_cast<int32_t>(old_value_32) ? data_32 : old_value_32;
            break;
        }

        case MAX: {
            new_value_32 = static_cast<int32_t>(data_32) > static_cast<int32_t>(old_value_32) ? data_32 : old_value_32;
            break;
        }

        case MINU: {
            new_value_32 = data_32 < old_value_32 ? data_32 : old_value_32;
            break;
        }

        case MAXU: {
            new_value_32 = data_32 > old_value_32 ? data_32 : old_value_32;
            break;
        }
        }

        new_value = new_value_32;
    }
    
    store(addr, new_value, size);
    return old_value;
}
