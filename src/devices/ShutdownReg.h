//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>

#include "bus_device.h"

namespace riscv_emu::mem_io {

// Equivalent to OpenSBI test finisher (sifive_test, qemu hw/misc/sifive_test.c):
// a guest write of 0x5555 = clean exit, 0x3333 = failure exit;
// 0x7777 requests a VM reset which this emulation does not yet support.
class ShutdownReg : public BusDevice
{
  public:
    static constexpr uint64_t ADDR_SPACE_WIDTH = 0x1000;

    static constexpr uint32_t CLEAN_SHUTDOWN = 0x5555;
    static constexpr uint32_t PANIC_SHUTDOWN = 0x3333;

    explicit ShutdownReg() : BusDevice(ADDR_SPACE_WIDTH) {}

    [[nodiscard]]
    std::optional<uint64_t> read(uint64_t offset, uint8_t size) const override;
    bool write(uint64_t offset, uint64_t data, uint8_t size) override;

    [[nodiscard]] bool is_shutdown_pending() const;
    [[nodiscard]] bool is_clean_shutdown_pending() const;
    [[nodiscard]] bool is_panic_shutdown_pending() const;
    [[nodiscard]] uint32_t get_status_code() const;

  private:
    uint32_t reg = 0;
};

} // namespace riscv_emu::mem_io
