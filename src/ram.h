//
// Created by anish on 4/12/2026.
//

#ifndef RISCV_EMU_MEMORY_H
#define RISCV_EMU_MEMORY_H

#include <memory>
#include "defs.h"

namespace riscv_emu::mem_io {
class Ram : public BusDevice
{
  public:
    explicit Ram(uint64_t size);

    void load(const uint8_t* data, std::size_t size) const;

    [[nodiscard]]
    uint64_t read(uint64_t offset, uint8_t size) const override;
    void write(uint64_t offset, uint64_t data, uint8_t size) override;

    void dump(std::ostream& os) const;

  private:
    std::unique_ptr<uint8_t[]> memory;
};
} // namespace riscv_emu::mem_io

#endif // RISCV_EMU_MEMORY_H
