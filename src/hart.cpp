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

Hart::Hart(mem_io::Bus* bus_ptr, const size_t id) : hart_id(id), memory_bus(bus_ptr), csr_file(id)
{
}

std::optional<uint64_t> Hart::fetch_next_instr(NextState& ns)
{
    if (pc & 0x3) {
        raise(ns, ExceptionType::INSTR_ADDR_MISALIGNED, pc);
        return std::nullopt;
    }

    return memory_bus->load<uint64_t>(pc);
}

void Hart::raise(NextState& ns, const TrapCause cause, const uint64_t tval)
{
    const auto new_pc = csr_file.enter_trap_m(cause, tval, pc, priv);
    ns.set_pc(new_pc);
    ns.set_priv(PrivilegeLevel::M);
    ns.mark_trapped();
}

void Hart::apply_instr_effect(NextState& ns, const InstrEffect::effect_type& effect, const uint64_t new_pc)
{
    ns.set_pc(new_pc);
    auto visitor = overloaded{[&](const InstrEffect::NoEffect&) {},
                              [&](const InstrEffect::UpdateRd& e) { regs[e.rd] = e.value; },
                              [&]<mem_io::UintFamily T>(const InstrEffect::LoadRdFromMem<T>& e)
                              {
                                  auto load_result = memory_bus->load<T>(e.addr);
                                  if (!load_result.has_value()) {
                                      raise(ns, ExceptionType::LOAD_ACCESS_FAULT, e.addr);
                                      return;
                                  }
                                  regs[e.rd] = static_cast<uint64_t>(load_result.value());
                                  if (e.sign_ext) {
                                      regs[e.rd] = sign_extend(regs[e.rd], sizeof(T) * 8);
                                  }
                              },
                              [&]<mem_io::UintFamily T>(const InstrEffect::StoreMem<T>& e)
                              {
                                  auto store_ok = memory_bus->store<T>(e.addr, e.value);
                                  if (!store_ok) {
                                      raise(ns, ExceptionType::STORE_AMO_ACCESS_FAULT, e.addr);
                                  }
                              },
                              [&]<mem_io::UintFamily T>(const InstrEffect::LoadReserved<T>& e)
                              {
                                  auto load_result = memory_bus->load_reserved<T>(e.addr, hart_id);
                                  if (!load_result.has_value()) {
                                      raise(ns, ExceptionType::LOAD_ACCESS_FAULT, e.addr);
                                      return;
                                  }
                                  regs[e.rd] = load_result.value();
                                  {
                                      regs[e.rd] = sign_extend(regs[e.rd], sizeof(T) * 8);
                                  }
                              },
                              [&]<mem_io::UintFamily T>(const InstrEffect::StoreConditional<T>& e)
                              {
                                  // SC writes 0 to rd on success / nonzero on failure (RISC-V spec);
                                  // store_conditional() returns true on success, so negate to bridge the conventions
                                  auto store_result = memory_bus->store_conditional<T>(e.addr, e.value, hart_id);
                                  if (!store_result.has_value()) {
                                      raise(ns, ExceptionType::STORE_AMO_ACCESS_FAULT, e.addr);
                                      return;
                                  }

                                  if (store_result.value()) {
                                      regs[e.rd] = 0;
                                  }
                                  else {
                                      regs[e.rd] = 1;
                                  }
                              },
                              [&]<mem_io::UintFamily T>(const InstrEffect::AmoRmw<T>& e)
                              {
                                  auto amo_result = memory_bus->handle_amo<T>(e.type, e.addr, e.value);
                                  if (!amo_result.has_value()) {
                                      raise(ns, ExceptionType::STORE_AMO_ACCESS_FAULT, e.addr);
                                      return;
                                  }
                                  regs[e.rd] = amo_result.value();
                                  if (e.sign_ext) {
                                      regs[e.rd] = sign_extend(regs[e.rd], sizeof(T) * 8);
                                  }
                              },
                              [&](const InstrEffect::CsrRmw& e)
                              {
                                  uint64_t to_csr_value = 0;
                                  uint64_t to_reg_value = 0;
                                  if (!e.skip_read) {
                                      const auto read_result = csr_file.read(e.addr, priv);
                                      if (!read_result.has_value()) {
                                          raise(ns, read_result.error(), 0);
                                          return;
                                      }
                                      to_reg_value = read_result.value();
                                  }

                                  switch (e.type) {
                                      using enum CsrOpType;
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
                                      const auto write_result = csr_file.write(e.addr, to_csr_value, priv);
                                      if (write_result.has_value()) {
                                          raise(ns, write_result.value(), 0);
                                          return;
                                      }
                                  }

                                  regs[e.rd] = to_reg_value;
                              },
                              [&](const InstrEffect::RaiseTrap& e) { raise(ns, e.cause, e.tval); },
                              [&](const InstrEffect::TrapReturn& e)
                              {
                                  if (e.return_priv == PrivilegeLevel::M) {
                                      const auto [trap_pc, trap_priv] = csr_file.return_trap_m();
                                      ns.set_pc(trap_pc);
                                      ns.set_priv(trap_priv);
                                  }
                                  // TBI add S-mode return here later
                              },
                              [&](const InstrEffect::HandleWfi&)
                              {
                                  if (!csr_file.is_wfi_valid(priv)) {
                                      raise(ns, ExceptionType::ILLEGAL_INSTRUCTION, 0);
                                  }
                                  // TBI WFI implementation
                              }};
    effect.visit(visitor);
    regs[0] = 0;
}

void Hart::step()
{
    // Increment csr performance counter (mcycles)
    NextState ns(&pc, &priv);
    csr_file.increment_cycle_count();

    // Fetch
    const auto raw_instr = fetch_next_instr(ns);
    if (!raw_instr.has_value()) {
        return;
    }

    // Decode
    const auto instr = decode(raw_instr.value());
    if (instr.op_type == InstrType::INVALID) {
        raise(ns, ExceptionType::ILLEGAL_INSTRUCTION, 0);
        return;
    }

    // Execute & Writeback
    const auto [effect, new_pc] = execute(instr, regs, pc, priv);
    apply_instr_effect(ns, effect, new_pc);
    if (ns.is_trapped()) {
        return;
    }

    csr_file.increment_retired_instructions();
}

void Hart::dump_regs(std::ostream& os) const
{
    os << "PC: " << std::hex << std::setfill('0') << std::setw(16) << pc << "\n";
    for (size_t i = 0; i < REG_COUNT; i++) {
        os << "x" << std::dec << std::setw(2) << i << ": 0x" << std::hex << std::uppercase << std::setfill('0')
           << std::setw(16) << regs[i] << "\n";
    }
}
