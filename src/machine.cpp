//
// Created by anish on 4/14/2026.
//

#include "machine.h"

#include "bus.h"
#include "hart.h"

using namespace riscv_emu;

machine::machine() : m_bus(&m_ram)
{
    m_harts.emplace_back(&m_bus);
}

void machine::load(uint8_t* data, std::size_t size)
{
    m_ram.load(data, size);
}

void machine::run()
{
    bool step_ok = true;
    while (step_ok) {
        step_ok = m_harts[0].step();
    }
}
