//
// Created by anish on 4/14/2026.
//

#ifndef RISCV_EMU_MACHINE_H
#define RISCV_EMU_MACHINE_H

#include <vector>
#include "bus.h"
#include "hart.h"
#include "ram.h"

namespace riscv_emu {
class machine
{
  public:
    machine();
    void run();
    void load(const uint8_t* data, std::size_t size) const;
    void dump(std::ostream& os) const;

  private:
    mem_io::ram m_ram;
    mem_io::bus m_bus;
    std::vector<hart> m_harts;
};
} // namespace riscv_emu

#endif // RISCV_EMU_MACHINE_H
