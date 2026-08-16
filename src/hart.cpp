//
// Created by anish on 3/29/2026.
//

#include "hart.h"

#include <iomanip>
#include <variant>

#include "bus.h"
#include "csr_file.h"
#include "decode.h"
#include "execute.h"

using namespace riscv_emu;

hart::hart(mem_io::bus* bus_ptr, const size_t id) : hart_id(id), mem_bus(bus_ptr), csrs(id)
{
}

std::optional<uint64_t> hart::fetch_next_instr(next_state& ns)
{
    if (pc & 0x3) {
        raise(ns, exception_type::INSTR_ADDR_MISALIGNED, pc);
        return std::nullopt;
    }

    return mem_bus->load<uint64_t>(pc);
}

void hart::raise(next_state& ns, const trap_cause cause, const uint64_t tval)
{
    const auto new_pc = csrs.enter_trap_m(cause, tval, pc, priv);
    ns.set_pc(new_pc);
    ns.set_priv(privilege_level::M);
    ns.mark_trapped();
}

void hart::apply_instr_effect(next_state& ns, const instr_effect::effect_type& effect, const uint64_t new_pc)
{
    ns.set_pc(new_pc);
    auto visitor = overloaded{[&](const instr_effect::no_effect&) {},
                              [&](const instr_effect::update_rd& e) { regs[e.rd] = e.value; },
                              [&]<mem_io::UintFamily T>(const instr_effect::load_rd_from_mem<T>& e)
                              {
                                  regs[e.rd] = static_cast<uint64_t>(mem_bus->load<T>(e.addr));
                                  if (e.sign_ext) {
                                      regs[e.rd] = sign_extend(regs[e.rd], sizeof(T) * 8);
                                  }
                              },
                              [&]<mem_io::UintFamily T>(const instr_effect::store_mem<T>& e)
                              { mem_bus->store<T>(e.addr, e.value); },
                              [&]<mem_io::UintFamily T>(const instr_effect::load_reserved<T>& e)
                              {
                                  regs[e.rd] = mem_bus->load_reserved<T>(e.addr, hart_id);
                                  if (e.sign_ext) {
                                      regs[e.rd] = sign_extend(regs[e.rd], sizeof(T) * 8);
                                  }
                              },
                              [&]<mem_io::UintFamily T>(const instr_effect::store_conditional<T>& e)
                              {
                                  // SC writes 0 to rd on success / nonzero on failure (RISC-V spec);
                                  // store_conditional() returns true on success, so negate to bridge the conventions
                                  regs[e.rd] = !mem_bus->store_conditional<T>(e.addr, e.value, hart_id);
                              },
                              [&]<mem_io::UintFamily T>(const instr_effect::amo_rmw<T>& e)
                              {
                                  regs[e.rd] = mem_bus->handle_amo<T>(e.type, e.addr, e.value);
                                  if (e.sign_ext) {
                                      regs[e.rd] = sign_extend(regs[e.rd], sizeof(T) * 8);
                                  }
                              },
                              [&](const instr_effect::csr_rmw& e)
                              {
                                  uint64_t to_csr_value = 0;
                                  uint64_t to_reg_value = 0;
                                  if (!e.skip_read) {
                                      const auto read_result = csrs.read(e.addr, priv);
                                      if (!read_result.has_value()) {
                                          raise(ns, read_result.error(), 0);
                                          return;
                                      }
                                      to_reg_value = read_result.value();
                                  }

                                  switch (e.type) {
                                      using enum csr_op_type;
                                  case RW: {
                                      to_csr_value = e.value;
                                      break;
                                  }
                                  case RS: {
                                      to_csr_value = to_reg_value | e.value;
                                      break;
                                  }
                                  case RC: {
                                      to_csr_value = to_reg_value & ~e.value;
                                      break;
                                  }
                                  }

                                  if (!e.skip_write) {
                                      const auto write_result = csrs.write(e.addr, to_csr_value, priv);
                                      if (write_result.has_value()) {
                                          raise(ns, write_result.value(), 0);
                                          return;
                                      }
                                  }

                                  regs[e.rd] = to_reg_value;
                              },
                              [&](const instr_effect::raise_trap& e) { raise(ns, e.cause, e.tval); },
                              [&](const instr_effect::trap_return& e)
                              {
                                  if (e.return_priv == privilege_level::M) {
                                      const auto [trap_pc, trap_priv] = csrs.return_trap_m();
                                      ns.set_pc(trap_pc);
                                      ns.set_priv(trap_priv);
                                  }
                                  // TBI add S-mode return here later
                              },
                              [&](const instr_effect::handle_wfi&)
                              {
                                  if (!csrs.is_wfi_valid(priv)) {
                                      raise(ns, exception_type::ILLEGAL_INSTRUCTION, 0);
                                  }
                                  // TBI WFI implementation
                              }};
    effect.visit(visitor);
    regs[0] = 0;
}

void hart::step()
{
    // Increment csr performance counter (mcycles)
    next_state ns(&pc, &priv);
    csrs.increment_cycle_count();

    // Fetch
    const auto raw_instr = fetch_next_instr(ns);
    if (!raw_instr.has_value()) {
        return;
    }

    // Decode
    const auto instr = decode(raw_instr.value());
    if (instr.op_type == instr_type::INVALID) {
        raise(ns, exception_type::ILLEGAL_INSTRUCTION, 0);
        return;
    }

    // Execute & Writeback
    const auto [effect, new_pc] = execute(instr, regs, pc, priv);
    apply_instr_effect(ns, effect, new_pc);
    if (ns.is_trapped()) {
        return;
    }

    csrs.increment_retired_instructions();
}

void hart::dump_regs(std::ostream& os) const
{
    os << "PC: " << std::hex << std::setfill('0') << std::setw(16) << pc << "\n";
    for (size_t i = 0; i < REG_COUNT; i++) {
        os << "x" << std::dec << std::setw(2) << i << ": 0x" << std::hex << std::uppercase << std::setfill('0')
           << std::setw(16) << regs[i] << "\n";
    }
}
