//
// Created by anish on 4/14/2026.
//

#include "machine.h"

#include "bus.h"
#include "hart.h"
#include "ram.h"

using namespace riscv_emu;

machine::machine() : m_bus(std::move(std::make_unique<ram>()))
{
    m_harts.emplace_back(&m_bus);
}

void machine::run()
{
    bool step_ok = true;
    while (step_ok) {
        step_ok = m_harts[0].step();
    }
}
