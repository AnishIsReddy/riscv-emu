//
// Created by anish on 3/29/2026.
//

#include "hart.h"

#include <iomanip>
#include <variant>

#include "bus.h"
#include "decode.h"
#include "execute.h"

using namespace riscv_emu;

hart::hart(bus* bus_ptr, const size_t id)
    : hart_id(id), mem_bus(bus_ptr) {}


bool hart::step()
{
    const auto raw_instr = mem_bus->load(pc, 4);
    const auto instr = decode(raw_instr);

    // If the instruction was invalid, then return false.
    if (instr.itype == instr_type::INVALID) {
        return false;
    }

    const auto [effect, new_pc] = execute(instr, reg_file, pc);

    std::visit(overloaded {
        [&](const instr_effect::no_effect&){},
        [&](const instr_effect::update_rd & e)
        {
            reg_file[e.rd] = e.value;
        },
        [&](const instr_effect::load_rd_from_mem & e)
        {
            reg_file[e.rd] = mem_bus->load(e.addr, e.size);
            if (e.sign_ext) {
                reg_file[e.rd] = sign_extend(reg_file[e.rd], e.size * 8);
            }
        },
        [&](const instr_effect::store_mem & e)
        {
            mem_bus->store(e.addr, e.value, e.size);
        },
        [&](const instr_effect::load_reserved & e)
        {
            reg_file[e.rd] = mem_bus->load_reserved(e.addr, e.size, hart_id);
            if (e.sign_ext) {
                reg_file[e.rd] = sign_extend(reg_file[e.rd], e.size * 8);
            }
        },
        [&](const instr_effect::store_conditional & e)
        {
            uint64_t data = e.value;
            if (e.sign_ext) {
                data = sign_extend(data, e.size * 8);
            }
            // SC writes 0 to rd on success / nonzero on failure (RISC-V spec);
            // store_conditional() returns true on success, so negate to bridge the conventions
            reg_file[e.rd] = !mem_bus->store_conditional(e.addr, data, e.size, hart_id);
        }
    }, effect);

    // Pin 0 to r0. Do this here to ensure it's decoupled from the rest of the logic.
    reg_file[0] = 0;

    // Advance the pc.
    pc = new_pc;

    return true;
}

void hart::dump_regs(std::ostream& os) const {
    os << "PC: " << std::hex << std::setfill('0') << std::setw(16) << pc << "\n";
    for (size_t i = 0; i < REG_COUNT; i++) {
        os << "x" << std::dec << std::setw(2) << i << ": 0x"
           << std::hex << std::uppercase << std::setfill('0') << std::setw(16) << reg_file[i] << "\n";
    }
}