//
// Created by anish on 4/12/2026.
//

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
