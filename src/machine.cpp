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

void machine::load(const uint8_t* data, const std::size_t size) const
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

void machine::dump(std::ostream& os) const
{
    for (const auto& h : m_harts) {
        h.dump_regs(os);
    }
}
