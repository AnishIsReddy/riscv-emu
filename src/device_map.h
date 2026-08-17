//
// Created by anish on 8/16/2026.
//

#ifndef RISCV_EMU_DEVICE_MAP_H
#define RISCV_EMU_DEVICE_MAP_H

#pragma once

#include <vector>
#include "defs.h"

namespace riscv_emu::mem_io {

class BusDevice;

class DeviceMap
{
  public:
    void add_mapping(uint64_t base_addr, BusDevice& device);

    [[nodiscard]]
    std::optional<uint64_t> try_load(uint64_t addr, uint8_t size) const;

    [[nodiscard]]
    bool try_store(uint64_t addr, uint64_t value, uint8_t size) const;

  private:
    struct Entry
    {
        BusDevice* device;
        uint64_t base_addr;
    };
    std::vector<Entry> entries;
};

} // namespace riscv_emu::mem_io

#endif // RISCV_EMU_DEVICE_MAP_H
