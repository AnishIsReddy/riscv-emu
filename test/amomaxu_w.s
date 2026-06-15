# amomaxu_w.s
    la      x5, scratch
    li      x6, 1
    amomaxu.w x7, x6, (x5)      # unsigned max → 0x80000000 wins
    lw      x28, 0(x5)
    ebreak
.align 2
.data
scratch: .word 0x80000000