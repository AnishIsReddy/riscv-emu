//
// Created by anish on 8/18/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "mem/device_base.h"

namespace riscv_emu::mem::devices {

class Clint : public DeviceBase
{
  public:
    explicit Clint(size_t hart_count);

    static constexpr uint64_t CLINT_ADDR_WIDTH = 0xc000;

    void step();

    [[nodiscard]]
    bool is_timer_interrupt_pending(size_t hart_id) const;

    [[nodiscard]]
    bool is_software_interrupt_pending(size_t hart_id) const;

  protected:
    [[nodiscard]]
    std::optional<uint64_t> read_internal(uint64_t offset, uint8_t size) const override;

    bool write_internal(uint64_t offset, uint64_t data, uint8_t size) override;

  private:
    const size_t hart_count;
    std::unique_ptr<uint32_t[]> msip = std::make_unique<uint32_t[]>(hart_count);
    std::unique_ptr<uint64_t[]> mtimecmp = std::make_unique<uint64_t[]>(hart_count);
    uint64_t mtime = 0;

    static constexpr uint64_t msip_base = 0;
    const uint64_t msip_size = hart_count * 4;
    static constexpr uint64_t mtimecmp_base = 0x4000;
    const uint64_t mtimecmp_size = hart_count * 8;
    static constexpr uint64_t mtime_base = 0xbff8;
    static constexpr uint64_t mtime_size = 8;
};

} // namespace riscv_emu::mem::devices
