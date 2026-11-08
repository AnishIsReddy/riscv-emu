//
// Created by anish on 11/1/2026.
//

#include "ShutdownReg.h"

#include "common/utils.h"

namespace riscv_emu::mem_io {

std::optional<uint64_t> ShutdownReg::read(uint64_t, uint8_t) const
{
    return 0;
}

bool ShutdownReg::write(const uint64_t offset, const uint64_t data, const uint8_t size)
{
    // QEMU base: only offset 0 reacts, and access sizes 2-4 bytes are valid
    if (offset != 0 || size < 2 || size > 4) {
        return false;
    }

    reg = u32(data);
    return true;
}

bool ShutdownReg::is_shutdown_pending() const
{
    return is_panic_shutdown_pending() || is_clean_shutdown_pending();
}

bool ShutdownReg::is_clean_shutdown_pending() const
{
    return reg == CLEAN_SHUTDOWN;
}

bool ShutdownReg::is_panic_shutdown_pending() const
{
    return reg == PANIC_SHUTDOWN;
}

uint32_t ShutdownReg::get_status_code() const
{
    return reg >> 16;
}
} // namespace riscv_emu::mem_io
