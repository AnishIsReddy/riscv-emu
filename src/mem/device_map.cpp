//
// Created by anish on 8/16/2026.
//

#include <algorithm>
#include <cassert>
#include <ranges>
#include <stdexcept>

#include "mem/device_base.h"
#include "mem/device_map.h"

using namespace riscv_emu::mem;

void DeviceMap::add_mapping(const uint64_t base_addr, DeviceBase& device, std::optional<uint64_t> addr_space_size)
{
    if (is_finalized) {
        throw std::runtime_error("Attempted to add a device to a finalized device map");
    }

    base_addrs.push_back(base_addr);
    addr_space_sizes.push_back(addr_space_size.value_or(device.get_size()));
    devices.push_back(&device);
}

void DeviceMap::finalize()
{
    std::ranges::sort(std::views::zip(base_addrs, addr_space_sizes, devices), {}, [](const auto& entry) {
        return std::get<0>(entry);
    });

    for (size_t i = 0; i < base_addrs.size(); ++i) {
        const uint64_t addr_space = addr_space_sizes[i];

        if (addr_space == 0) {
            throw std::runtime_error("Device map contains mapping with zero address space size");
        }
        if (devices[i]->get_size() == 0) {
            throw std::runtime_error("Device map contains device with zero size");
        }
        if (addr_space < devices[i]->get_size()) {
            throw std::runtime_error("Device map contains an address space smaller than the device's implemented size");
        }
        if (i + 1 < base_addrs.size() && addr_space > base_addrs[i + 1] - base_addrs[i]) {
            throw std::runtime_error("Device map contains overlapping device mappings");
        }
    }

    devices.shrink_to_fit();
    base_addrs.shrink_to_fit();
    addr_space_sizes.shrink_to_fit();

    is_finalized = true;
}

std::optional<DeviceMap::LookupResult> DeviceMap::try_lookup(const uint64_t addr, const size_t access_size) const
{
    assert(is_finalized);

    // Find the first base address greater than addr
    auto it = std::ranges::upper_bound(base_addrs, addr);

    // If addr is before the first device, no match
    if (it == base_addrs.begin()) {
        return std::nullopt;
    }

    // The device we want is the one before the upper_bound result
    --it;
    const size_t idx = std::distance(base_addrs.begin(), it);
    const uint64_t offset = addr - base_addrs[idx];
    const uint64_t addr_space = addr_space_sizes[idx];

    // The access must start inside the address space and end inside it too.
    // Checking offset first guarantees addr_space - offset can't underflow.
    if (offset >= addr_space || access_size > addr_space - offset) {
        return std::nullopt;
    }

    return LookupResult{.device = devices[idx], .offset = offset};
}
