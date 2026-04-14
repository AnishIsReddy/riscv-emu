//
// Created by anish on 3/29/2026.
//

#include "hart.h"

#include "bus.h"
#include "decode.h"
#include "execute.h"

using namespace riscv_emu;

hart::hart(bus* bus_ptr)
{
    mem_bus = bus_ptr;
}


bool hart::step()
{
    const auto raw_instr = mem_bus->read_memory(pc, 32);
    const auto instr = decode(raw_instr);
    if (instr.itype == instr_type::INVALID) {
        return false;
    }
    const auto exec_result = execute(instr, reg_file, pc);

    switch (exec_result.type) {
    case exec_result_type::UPDATE_RD_FROM_VAL: {
        reg_file[exec_result.rd] = exec_result.val;
        break;
    }
    case exec_result_type::UPDATE_RD_FROM_MEM: {
        uint64_t val = mem_bus->read_memory(exec_result.mem_addr, exec_result.mem_size);
        if (!exec_result.zero_extend_val) {
            val = sign_extend(val, exec_result.mem_size * 8);
        }
        reg_file[exec_result.rd] = val;
        break;
    }
    case exec_result_type::UPDATE_MEM_FROM_VAL: {
        mem_bus->write_memory(exec_result.mem_addr, exec_result.val, exec_result.mem_size);
        break;
    }
    default:
        break;
    }

    // Pin 0 to r0. Do this here to ensure it's decoupled from the rest of the logic
    reg_file[0] = 0;

    return true;
}