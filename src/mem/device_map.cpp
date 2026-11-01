//
// Created by anish on 8/16/2026.
//

#include <algorithm>
#include <cassert>
#include <ranges>
#include <stdexcept>

#include "mem/bus_device.h"
#include "mem/device_map.h"

using namespace riscv_emu::mem_io;

void DeviceMap::add_mapping(const uint64_t base_addr, BusDevice& device)
{
    if (is_finalized) {
        throw std::runtime_error("Attempted to add a device to a finalized device map");
    }

    base_addrs.push_back(base_addr);
    devices.push_back(&device);
}

void DeviceMap::finalize()
{
    std::ranges::sort(std::views::zip(base_addrs, devices), {}, [](const auto& entry) { return std::get<0>(entry); });

    for (size_t i = 0; i < base_addrs.size(); ++i) {
        const uint64_t size = devices[i]->get_size();
        if (size == 0) {
            throw std::runtime_error("Device map contains device with zero size");
        }
        if (i + 1 < base_addrs.size() && size > base_addrs[i + 1] - base_addrs[i]) {
            throw std::runtime_error("Device map contains overlapping device mappings");
        }
    }

    devices.shrink_to_fit();
    base_addrs.shrink_to_fit();

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
    const uint64_t dev_size = devices[idx]->get_size();

    // The access must start inside the device and end inside it too.
    // Checking offset first guarantees dev_size - offset can't underflow.
    if (offset >= dev_size || access_size > dev_size - offset) {
        return std::nullopt;
    }

    return LookupResult{.device = devices[idx], .offset = offset};
}
