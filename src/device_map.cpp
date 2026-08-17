//
// Created by anish on 8/16/2026.
//

#include "device_map.h"

using namespace riscv_emu::mem_io;

void DeviceMap::add_mapping(const uint64_t base_addr, BusDevice& device)
{
    entries.push_back(Entry{.device = &device, .base_addr = base_addr});
}

std::optional<uint64_t> DeviceMap::try_load(const uint64_t addr, const uint8_t size) const
{
    for (const auto& [device, base_addr] : entries) {
        const uint64_t offset = addr - base_addr;
        const uint64_t max = device->max_offset();
        if (offset < max && offset + size <= max) {
            return device->read(offset, size);
        }
    }
    return std::nullopt;
}

bool DeviceMap::try_store(const uint64_t addr, const uint64_t value, const uint8_t size) const
{
    for (const auto& [device, base_addr] : entries) {
        const uint64_t offset = addr - base_addr;
        const uint64_t max = device->max_offset();
        if (offset < max && offset + size <= max) {
            device->write(offset, value, size);
            return true;
        }
    }
    return false;
}
