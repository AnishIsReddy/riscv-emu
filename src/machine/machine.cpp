//
// Created by anish on 4/14/2026.
//

#include "machine/machine.h"

#include <iostream>

#include "common/config.h"
#include "cpu/hart.h"
#include "devices/clint.h"
#include "devices/ram.h"
#include "mem/bus.h"
#include "mem/device_map.h"

using namespace riscv_emu;

Machine::Machine()
{
    // init devices
    auto ram = std::make_unique<mem_io::Ram>(MEM_SIZE);
    auto clint = std::make_unique<mem_io::Clint>(1);

    // save dram ref for dump
    dram = ram.get();

    // save clint for clock steps
    this->clint = clint.get();

    // init bus
    mem_io::DeviceMap map;
    map.add_mapping(0x0200'0000, *clint);
    map.add_mapping(0x8000'0000, *ram);
    map.finalize();
    bus = std::make_unique<mem_io::Bus>(std::move(map), 1);

    // transfer device ownership to the machine
    devices.push_back(std::move(ram));
    devices.push_back(std::move(clint));

    // create harts
    harts.emplace_back(bus.get(), 0);
}

Machine::~Machine() = default;

void Machine::run()
{
    // TBI check shutdown address and break
    for (size_t i = 0; i < 1000000; i++) {
        harts[0].step();
        clint->step();
    }
}

void Machine::load(const uint8_t* data, const std::size_t size) const
{
    dram->load(data, size);
}

void Machine::dump(std::ostream& os) const
{
    os << "[HARTS]" << std::endl;
    for (const auto& h : harts) {
        h.dump_regs(os);
    }

    os << std::endl;

    os << "[RAM]" << std::endl;
    dram->dump(os);
}
