//
// Created by anish on 4/14/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <vector>

#include "mem/devices/shutdown_reg.h"

namespace riscv_emu {

namespace mem {
class Bus;
class DeviceBase;
namespace devices {
class Ram;
class Clint;
} // namespace mem::devices
} // namespace mem

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
    std::vector<std::unique_ptr<mem::DeviceBase>> devices; // owns all devices
    std::unique_ptr<mem::Bus> bus;

    mem::devices::Ram* dram = nullptr;
    mem::devices::Clint* clint = nullptr;
    mem::devices::ShutdownReg* shutdown_reg = nullptr;

    std::vector<cpu::Hart> harts;
};
} // namespace riscv_emu
