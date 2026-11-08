//
// Created by anish on 4/14/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <vector>

#include "devices/ShutdownReg.h"

namespace riscv_emu {

namespace mem_io {
class Bus;
class BusDevice;
class Ram;
class Clint;
} // namespace mem_io

namespace cpu {
class Hart;
}

class Machine
{
  public:
    Machine();
    ~Machine();
    uint32_t run();
    void load(const uint8_t* data, std::size_t size) const;
    void dump(std::ostream& os) const;

    Machine(Machine&&) = delete;
    Machine& operator=(Machine&&) = delete;

  private:
    std::vector<std::unique_ptr<mem_io::BusDevice>> devices; // owns all devices
    std::unique_ptr<mem_io::Bus> bus;

    mem_io::Ram* dram = nullptr;
    mem_io::Clint* clint = nullptr;
    mem_io::ShutdownReg* shutdown_reg = nullptr;

    std::vector<cpu::Hart> harts;
};
} // namespace riscv_emu
