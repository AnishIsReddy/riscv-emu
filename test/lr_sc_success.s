    .text
    .globl _start
_start:
    la      x5, scratch
    li      x6, 0xDEADBEEF
    lr.w    x7, (x5)            # reserve; also tests LR's load
    sc.w    x28, x6, (x5)       # must succeed
    lwu     x29, 0(x5)          # launder memory back into a reg
    wfi

    .data
scratch:
    .word 0x12345678