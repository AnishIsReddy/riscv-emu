//
// Created by anish on 8/16/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace riscv_emu::mem_io {

class BusDevice;

class DeviceMap
{
  public:
    void add_mapping(uint64_t base_addr, BusDevice& device);

    void finalize();

    struct LookupResult
    {
        BusDevice* device;
        uint64_t offset;
    };

    [[nodiscard]]
    std::optional<LookupResult> try_lookup(uint64_t addr, size_t access_size) const;

  private:
    std::vector<uint64_t> base_addrs;
    std::vector<BusDevice*> devices;

    bool is_finalized = false;
};

} // namespace riscv_emu::mem_io
