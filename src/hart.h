//
// Created by anish on 3/29/2026.
//

#ifndef RISCV_EMU_HART_H
#define RISCV_EMU_HART_H

#include <cstdint>
#include <iostream>
#include "csr_file.h"
#include "defs.h"

namespace riscv_emu {
namespace mem_io {
class Bus;
}

class Hart
{
  public:
    explicit Hart(mem_io::Bus* bus_ptr, size_t id);

    void step();
    void dump_regs(std::ostream& os) const;

    const size_t hart_id;

  private:
    class NextState
    {
      public:
        NextState(uint64_t* pc_ptr, PrivilegeLevel* priv_ptr) : hart_pc(pc_ptr), hart_priv(priv_ptr)
        {
            next_pc = *hart_pc;
            next_priv = *hart_priv;
        }

        ~NextState()
        {
            *hart_pc = next_pc;
            *hart_priv = next_priv;
        }

        void set_pc(const uint64_t value)
        {
            if (trapped)
                return;
            next_pc = value;
        }

        void set_priv(const PrivilegeLevel value)
        {
            if (trapped)
                return;
            next_priv = value;
        }

        void mark_trapped() { trapped = true; }

        [[nodiscard]]
        bool is_trapped() const
        {
            return trapped;
        }

      private:
        uint64_t next_pc;
        PrivilegeLevel next_priv;
        uint64_t* hart_pc;
        PrivilegeLevel* hart_priv;
        bool trapped = false;
    };

    void apply_instr_effect(NextState& ns, const InstrEffect::effect_type& effect, uint64_t new_pc);
    std::optional<uint64_t> fetch_next_instr(NextState& ns);
    void raise(NextState& ns, TrapCause cause, uint64_t tval);

    uint64_t pc = 0;
    PrivilegeLevel priv = PrivilegeLevel::M;

    uint64_t regs[REG_COUNT] = {};
    mem_io::Bus* memory_bus;
    CsrFile csr_file;

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
};
} // namespace riscv_emu
#endif // RISCV_EMU_HART_H
