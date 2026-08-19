//
// Created by anish on 4/14/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace riscv_emu {
namespace mem_io {
class Bus;
class Ram;
} // namespace mem_io
class Hart;

class Machine
{
  public:
    Machine();
    ~Machine();
    void run();
    void load(const uint8_t* data, std::size_t size) const;
    void dump(std::ostream& os) const;

    Machine(Machine&&) = delete;
    Machine& operator=(Machine&&) = delete;

  private:
    std::unique_ptr<mem_io::Ram> dram;

    std::unique_ptr<mem_io::Bus> bus;
    std::unique_ptr<std::vector<Hart>> harts;
};
} // namespace riscv_emu
