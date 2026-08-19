//
// Created by anish on 4/14/2026.
//

#include "machine.h"

#include <iostream>
#include "bus.h"
#include "hart.h"
#include "ram.h"

using namespace riscv_emu;

Machine::Machine()
{
    dram = std::make_unique<mem_io::Ram>(MEM_SIZE);
    mem_io::DeviceMap bus_map;
    bus_map.add_mapping(0x80000000, *dram);
    bus = std::make_unique<mem_io::Bus>(std::move(bus_map));
    harts->emplace_back(bus.get(), 0);
}

Machine::~Machine() = default;

void Machine::run()
{
    for (size_t i = 0; i < 1000000; i++) {
        harts->at(0).step();
        // TBI check shutdown address and break
    }
}

void Machine::load(const uint8_t* data, const std::size_t size) const
{
    dram->load(data, size);
}

void Machine::dump(std::ostream& os) const
{
    os << "[HARTS]" << std::endl;
    for (const auto& h : *harts) {
        h.dump_regs(os);
    }

    os << std::endl;

    std::cout << "[RAM]" << std::endl;
    dram->dump(os);
}
