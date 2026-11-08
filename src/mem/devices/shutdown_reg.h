//
// Created by anish on 11/1/2026.
//

#pragma once

#include <cstdint>

#include "mem/device_base.h"

namespace riscv_emu::mem::devices {

// Equivalent to OpenSBI test finisher (sifive_test, qemu hw/misc/sifive_test.c):
// a guest write of 0x5555 = clean exit, 0x3333 = failure exit;
// 0x7777 requests a VM reset which this emulation does not yet support.
class ShutdownReg : public DeviceBase
{
  public:
    static constexpr uint64_t SIZE = 0x8;

    static constexpr uint32_t CLEAN_SHUTDOWN = 0x5555;
    static constexpr uint32_t PANIC_SHUTDOWN = 0x3333;

    explicit ShutdownReg() : DeviceBase(SIZE) {}

    [[nodiscard]] bool is_shutdown_pending() const;
    [[nodiscard]] bool is_clean_shutdown_pending() const;
    [[nodiscard]] bool is_panic_shutdown_pending() const;
    [[nodiscard]] uint32_t get_status_code() const;

  protected:
    [[nodiscard]]
    std::optional<uint64_t> read_internal(uint64_t offset, uint8_t size) const override;
    bool write_internal(uint64_t offset, uint64_t data, uint8_t size) override;


  private:
    uint32_t reg = 0;
};

} // namespace riscv_emu::mem::devices
