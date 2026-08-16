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
class bus;
}

class hart
{
  public:
    explicit hart(mem_io::bus* bus_ptr, size_t id);

    void step();
    void dump_regs(std::ostream& os) const;

    const size_t hart_id;

  private:
    class next_state
    {
      public:
        next_state(uint64_t* pc_ptr, privilege_level* priv_ptr) : hart_pc(pc_ptr), hart_priv(priv_ptr)
        {
            next_pc = *hart_pc;
            next_priv = *hart_priv;
        }

        ~next_state()
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

        void set_priv(const privilege_level value)
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
        privilege_level next_priv;
        uint64_t* hart_pc;
        privilege_level* hart_priv;
        bool trapped = false;
    };

    void apply_instr_effect(next_state& ns, const instr_effect::effect_type& effect, uint64_t new_pc);
    std::optional<uint64_t> fetch_next_instr(next_state& ns);
    void raise(next_state& ns, trap_cause cause, uint64_t tval);

    uint64_t pc = 0;
    privilege_level priv = privilege_level::M;

    uint64_t regs[REG_COUNT] = {};
    mem_io::bus* mem_bus;
    csr_file csrs;

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
};
} // namespace riscv_emu
#endif // RISCV_EMU_HART_H
