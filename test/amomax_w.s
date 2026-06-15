# amomax_w.s
    la        x5, scratch
    li        x6, 1
    amomax.w  x7, x6, (x5)       # signed max(mem, 1) as int32
    lw        x28, 0(x5)
    ebreak

    .align 2
    .data
scratch:
    .word 0x80000000