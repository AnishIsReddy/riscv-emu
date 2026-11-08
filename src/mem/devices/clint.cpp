//
// Created by anish on 8/18/2026.
//

#include "common/utils.h"
#include "mem/devices/clint.h"

namespace riscv_emu::mem::devices {

constexpr uint64_t MSIP_MASK = 0x1;

Clint::Clint(const size_t hart_count) : BusDevice(CLINT_ADDR_WIDTH), hart_count(hart_count) {}

std::optional<uint64_t> Clint::read_internal(const uint64_t offset, const uint8_t size) const
{
    if (offset >= msip_base && offset + size <= msip_base + msip_size) {
        if (offset % 4 != 0 || size != 4) {
            return std::nullopt;
        }
        return msip[offset / 4];
    }

    if (offset >= mtimecmp_base && offset + size <= mtimecmp_base + mtimecmp_size) {
        if (offset % 8 != 0 || size != 8) {
            return std::nullopt;
        }
        const size_t mtimecmp_offset = offset - mtimecmp_base;
        return mtimecmp[mtimecmp_offset / 8];
    }

    if (offset == mtime_base) {
        if (size != 8) {
            return std::nullopt;
        }
        return mtime;
    }

    return std::nullopt;
}

bool Clint::write_internal(const uint64_t offset, const uint64_t data, const uint8_t size)
{
    if (offset >= msip_base && offset + size <= msip_base + msip_size) {
        if (offset % 4 != 0 || size != 4) {
            return false;
        }
        msip[offset / 4] = u32(data & MSIP_MASK);
        return true;
    }

    if (offset >= mtimecmp_base && offset + size <= mtimecmp_base + mtimecmp_size) {
        if (offset % 8 != 0 || size != 8) {
            return false;
        }

        const size_t mtimecmp_offset = offset - mtimecmp_base;
        mtimecmp[mtimecmp_offset / 8] = data;
        return true;
    }

    if (offset == mtime_base) {
        if (size != 8) {
            return false;
        }
        mtime = data;
        return true;
    }

    return false;
}

void Clint::step()
{
    mtime++;
}
} // namespace riscv_emu::mem::devices
