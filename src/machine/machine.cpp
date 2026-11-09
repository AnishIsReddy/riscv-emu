//
// Created by anish on 4/14/2026.
//

#include "machine/machine.h"

#include <cassert>
#include <iostream>

#include "common/config.h"
#include "cpu/hart.h"
#include "mem/bus.h"
#include "mem/device_map.h"
#include "mem/devices/clint.h"
#include "mem/devices/ram.h"

using namespace riscv_emu;

Machine::Machine()
{
    // init devices
    auto m_ram = std::make_unique<mem::devices::Ram>(MEM_SIZE);
    auto m_clint = std::make_unique<mem::devices::Clint>(1);
    auto m_shutdown_reg = std::make_unique<mem::devices::ShutdownReg>();

    dram = m_ram.get();
    clint = m_clint.get();
    shutdown_reg = m_shutdown_reg.get();

    // init bus
    mem::DeviceMap map;
    map.add_mapping(0x0200'0000, *m_clint);
    map.add_mapping(0x8000'0000, *m_ram);
    map.add_mapping(0x0010'0000, *m_shutdown_reg, 0x1000);
    map.finalize();
    bus = std::make_unique<mem::Bus>(std::move(map), 1);

    // transfer device ownership to the machine
    devices.push_back(std::move(m_ram));
    devices.push_back(std::move(m_clint));
    devices.push_back(std::move(m_shutdown_reg));

    // create harts
    harts.emplace_back(bus.get(), 0, m_clint.get());
}

Machine::~Machine() = default;

uint32_t Machine::run()
{
    assert(shutdown_reg);

    while (true) {
        for (auto& hart : harts) {
            hart.step();

            if (shutdown_reg->is_shutdown_pending()) {
                return shutdown_reg->is_panic_shutdown_pending() ? 1 : 0;
            }
        }
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
