//
// Created by anish on 4/12/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>

#include "mem/device_base.h"

namespace riscv_emu::mem::devices {
class Ram : public DeviceBase
{
  public:
    explicit Ram(uint64_t size);

    void load(const uint8_t* data, std::size_t size) const;

    void dump(std::ostream& os) const;

  protected:
    [[nodiscard]]
    std::optional<uint64_t> read_internal(uint64_t offset, uint8_t size) const override;
    bool write_internal(uint64_t offset, uint64_t data, uint8_t size) override;


  private:
    std::unique_ptr<uint8_t[]> memory;
};
} // namespace riscv_emu::mem::devices
