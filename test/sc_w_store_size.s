    .text
    .globl _start
_start:
    la      x5, scratch
    lr.w    x6, (x5)                    # low word of the dword
    li      x7, 0xCAFEBABEDEADBEEF      # dirty upper 32 — the active ingredient
    sc.w    x28, x7, (x5)               # must store only the low word
    ld      x29, 0(x5)                  # full-width read-back, no extension logic
    wfi

    .data
    .align 3
scratch:
    .dword 0x55AA55AA12345678
